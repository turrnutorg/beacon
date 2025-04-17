/**
 * simas.c - Fixed Extended SIMAS Interpreter for Beacon OS
 *
 * this interpreter now supports:
 *   - variable assignment and arithmetic (set, add, sub, mul, div)
 *   - printing (print, printc, println)
 *   - function definition and calls (fun, ret, call, end fun)
 *   - control flow with labels and jumps (label, jump, jumpv)
 *   - comparisons (eqc, eqv, neqc, neqv, gt, gte, st, ste)
 *   - logical operations (and, or, not)
 *   - type conversion (conv)
 *
 * it does NOT support: server, import, file handling.
 *
 * functions and labels are preprocessed into lookup tables. when a function is called,
 * arguments are stored into special global variables named "$0", "$1", etc.
 *
 * SIMAS created by: turrnut
 * 
 * ported to C and Beacon by: tuvalutorture
 * 
 * Copyright (c) Turrnut Open Source Organization
 * license: gpl v3
 * see copying for more info
 */

 #include "string.h"
 #include "screen.h"
 #include "console.h"
 #include "keyboard.h"
 #include "command.h"
 #include <stdarg.h>
 #include <stdint.h>
 
 #define MAX_SCRIPT_LENGTH 4096
 #define MAX_INSTRUCTIONS 256
 #define MAX_TOKENS 32
 #define MAX_VARIABLES 128
 #define MAX_LABELS 64
 #define MAX_FUNCTIONS 32
 #define MAX_CALL_STACK 16
 
 // -------------------- variable handling --------------------
 
 typedef enum { TYPE_NUM, TYPE_STR, TYPE_BOOL } ValueType;
 
 typedef struct {
     char name[32];
     ValueType type;
     union {
         double num;
         char str[256];
         int boolean;
     } value;
 } Variable;
 
 Variable variables[MAX_VARIABLES];
 int variable_count = 0;
 
 // get or create variable
 Variable* get_variable(const char* name, ValueType type_hint) {
     for (int i = 0; i < variable_count; i++) {
         if (strcmp(variables[i].name, name) == 0)
             return &variables[i];
     }
     if (variable_count < MAX_VARIABLES) {
         Variable *var = &variables[variable_count++];
         strncpy(var->name, name, sizeof(var->name)-1);
         var->name[sizeof(var->name)-1] = '\0';
         var->type = type_hint;
         if (type_hint == TYPE_NUM)
             var->value.num = 0;
         else if (type_hint == TYPE_STR)
             var->value.str[0] = '\0';
         else if (type_hint == TYPE_BOOL)
             var->value.boolean = 0;
         return var;
     } else {
         print("error: variable table full\n");
         return 0;
     }
 }
 
 // -------------------- helper string functions --------------------
 
 char to_lower_char(char c) {
     if (c >= 'A' && c <= 'Z')
         return c + 32;
     return c;
 }

 void trim(char *str) {
     // trim leading spaces
     int start = 0;
     while(str[start]==' ')
         start++;
     if(start > 0) {
         int i = 0;
         while(str[start + i] != '\0') {
             str[i] = str[start + i];
             i++;
         }
         str[i] = '\0';
     }
     // trim trailing spaces
     int len = strlen(str);
     while(len > 0 && str[len-1]==' ') {
         str[len-1] = '\0';
         len--;
     }
 }
 
 int tokenize(char *line, char delim, char* tokens[], int max_tokens) {
     int count = 0;
     char *start = line;
     while (*line && count < max_tokens) {
         if(*line == delim) {
             *line = '\0';
             tokens[count++] = start;
             start = line + 1;
         }
         line++;
     }
     if(count < max_tokens && start[0] != '\0')
         tokens[count++] = start;
     return count;
 }
 
 double parse_number(const char *str) {
     double result = 0;
     int i = 0, neg = 0;
     if(str[0] == '-') { neg = 1; i++; }
     for(; str[i] != '\0'; i++){
         if(str[i] >= '0' && str[i] <= '9')
             result = result * 10 + (str[i]-'0');
         else if(str[i] == '.')
             break;
     }
     if(neg)
         result = -result;
     return result;
 }
 
 int is_number(const char *str) {
     int i = 0;
     if(str[0]=='-' || str[0]=='+') i++;
     while(str[i] != '\0') {
         if(str[i] < '0' || str[i] > '9')
             return 0;
         i++;
     }
     return 1;
 }
 
 // -------------------- new commands: labels and functions --------------------
 
 typedef struct {
     char name[32];
     int ip;  // instruction pointer index
 } Label;
 
 Label label_table[MAX_LABELS];
 int label_count = 0;
 
 typedef struct {
     char name[32];
     int ip;       // starting ip of function body
     int num_args; // expected number of arguments
 } FunctionDef;
 
 FunctionDef function_table[MAX_FUNCTIONS];
 int function_count = 0;
 
 // call stack for function calls
 int call_stack[MAX_CALL_STACK];
 int call_stack_top = 0;
 
 // -------------------- sys input reading --------------------
 
 // read a line from input using getch(); handles backspace with move_cursor_left
 void simas_read_line(char *buffer, int max_len, const char *prompt) {
     int i = 0;
     while(i < max_len-1) {
         int ch = getch();
         if(ch == '\n' || ch == '\r')
             break;
         if(ch == 8 || ch == 127) {  // backspace
             if(i > 0) {
                 i--;
                 buffer[i] = '\0';
                 move_cursor_left();
                 print(" ");
                 move_cursor_left();
             }
             continue;
         }
         buffer[i++] = (char)ch;
         char temp[2] = { (char)ch, '\0' };
         print(temp);
     }
     buffer[i] = '\0';
     println("");
 }
 
 // -------------------- instruction execution --------------------
 
 // we split the script into instructions (each line ending with ';'),
 // then preprocess to build our label and function lookup tables.
 char *instructions[MAX_INSTRUCTIONS];
 int instruction_count = 0;
 
 void preprocess_instructions() {
     label_count = 0;
     function_count = 0;
     for (int i = 0; i < instruction_count; i++) {
         char line_copy[512];
         strncpy(line_copy, instructions[i], sizeof(line_copy)-1);
         line_copy[sizeof(line_copy)-1] = '\0';
         char *tokens[MAX_TOKENS];
         int count = tokenize(line_copy, ' ', tokens, MAX_TOKENS);
         if(count == 0)
             continue;
         if(stricmp(tokens[0], "label") == 0 && count >= 2) {
             if(label_count < MAX_LABELS) {
                 strncpy(label_table[label_count].name, tokens[1], sizeof(label_table[label_count].name)-1);
                 label_table[label_count].name[sizeof(label_table[label_count].name)-1] = '\0';
                 label_table[label_count].ip = i;
                 label_count++;
             }
         } else if(stricmp(tokens[0], "fun") == 0 && count >= 3) {
             if(function_count < MAX_FUNCTIONS) {
                 strncpy(function_table[function_count].name, tokens[1], sizeof(function_table[function_count].name)-1);
                 function_table[function_count].name[sizeof(function_table[function_count].name)-1] = '\0';
                 function_table[function_count].ip = i;
                 function_table[function_count].num_args = (int)parse_number(tokens[2]);
                 function_count++;
             }
         }
     }
 }
 
 int find_label(const char *name) {
     for (int i = 0; i < label_count; i++) {
         if(stricmp(label_table[i].name, name) == 0)
             return label_table[i].ip;
     }
     return -1;
 }
 
 int find_function(const char *name) {
     for (int i = 0; i < function_count; i++) {
         if(stricmp(function_table[i].name, name) == 0)
             return function_table[i].ip;
     }
     return -1;
 }
 
 void set_parameter(int index, const char *mode, const char *value) {
     char paramName[8];
     snprintf(paramName, sizeof(paramName), "$%d", index);
     if(stricmp(mode, "v") == 0) {
         Variable* src = get_variable(value, TYPE_STR);
         Variable* dest = get_variable(paramName, src->type);
         if(src->type == TYPE_NUM)
             dest->value.num = src->value.num;
         else if(src->type == TYPE_STR)
             strncpy(dest->value.str, src->value.str, sizeof(dest->value.str)-1);
         else if(src->type == TYPE_BOOL)
             dest->value.boolean = src->value.boolean;
     } else if(stricmp(mode, "c") == 0) {
         if(is_number(value)) {
             Variable* dest = get_variable(paramName, TYPE_NUM);
             dest->value.num = parse_number(value);
         } else {
             Variable* dest = get_variable(paramName, TYPE_STR);
             strncpy(dest->value.str, value, sizeof(dest->value.str)-1);
         }
     } else if(stricmp(mode, "b") == 0) {
         Variable* dest = get_variable(paramName, TYPE_BOOL);
         dest->value.boolean = (stricmp(value, "true") == 0) ? 1 : 0;
     }
 }
 
 void execute_instructions() {
     int ip = 0;
     while(ip < instruction_count) {
         // print debug info for each instruction
         // char debug[128];
         // snprintf(debug, sizeof(debug), "ip: %d, instr: %s\n", ip, instructions[ip]);
         // print(debug);
         
         char instr_copy[512];
         strncpy(instr_copy, instructions[ip], sizeof(instr_copy)-1);
         instr_copy[sizeof(instr_copy)-1] = '\0';
         char *tokens[MAX_TOKENS];
         int token_count = tokenize(instr_copy, ' ', tokens, MAX_TOKENS);
         if(token_count == 0) { ip++; continue; }
         
         // skip function definitions and labels at runtime
         if(stricmp(tokens[0], "fun") == 0) {
             ip++;
             while(ip < instruction_count) {
                 char temp[512];
                 strncpy(temp, instructions[ip], sizeof(temp)-1);
                 temp[sizeof(temp)-1] = '\0';
                 char *toks[MAX_TOKENS];
                 int cnt = tokenize(temp, ' ', toks, MAX_TOKENS);
                 if(cnt > 0 && stricmp(toks[0], "end") == 0 && cnt >= 2 && stricmp(toks[1], "fun") == 0)
                     break;
                 ip++;
             }
             ip++; // skip the "end fun"
             continue;
         }
         if(stricmp(tokens[0], "label") == 0) {
             ip++;
             continue;
         }
         
         // process each command
         if(stricmp(tokens[0], "set") == 0) {
             if(token_count < 4)
                 print("error: invalid set syntax\n");
             else {
                 char *type_str = tokens[1];
                 char *var_name = tokens[2];
                 char *value_str = tokens[3];
                 if(stricmp(type_str, "num") == 0) {
                     Variable* var = get_variable(var_name, TYPE_NUM);
                     var->value.num = parse_number(value_str);
                 } else if(stricmp(type_str, "str") == 0) {
                     Variable* var = get_variable(var_name, TYPE_STR);
                     strncpy(var->value.str, value_str, sizeof(var->value.str)-1);
                 } else if(stricmp(type_str, "bool") == 0) {
                     Variable* var = get_variable(var_name, TYPE_BOOL);
                     var->value.boolean = (stricmp(value_str, "true") == 0) ? 1 : 0;
                 } else {
                     print("error: unknown type in set\n");
                 }
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "add") == 0) {
            if(token_count < 4)
                print("error: invalid add syntax\n");
            else {
                char *dest = tokens[2];
                char *opnd = tokens[3];
                Variable* var = get_variable(dest, TYPE_NUM);
                double a = var->value.num;
                double b = 0;
        
                if (is_number(opnd)) {
                    b = parse_number(opnd);
                } else {
                    Variable* var2 = get_variable(opnd, TYPE_NUM);
                    b = var2->value.num;
                }
        
                var->value.num = a + b;
            }
            ip++;
            continue;
        }
        else if(stricmp(tokens[0], "sub") == 0) {
            if(token_count < 4)
                print("error: invalid sub syntax\n");
            else {
                char *dest = tokens[2];
                char *opnd = tokens[3];
                Variable* var = get_variable(dest, TYPE_NUM);
                double a = var->value.num;
                double b = 0;
        
                if (is_number(opnd)) {
                    b = parse_number(opnd);
                } else {
                    Variable* var2 = get_variable(opnd, TYPE_NUM);
                    b = var2->value.num;
                }
        
                var->value.num = a - b;
            }
            ip++;
            continue;
        }
        else if(stricmp(tokens[0], "mul") == 0) {
            if(token_count < 4)
                print("error: invalid mul syntax\n");
            else {
                char *dest = tokens[2];
                char *opnd = tokens[3];
                Variable* var = get_variable(dest, TYPE_NUM);
                double a = var->value.num;
                double b = 0;
        
                if (is_number(opnd)) {
                    b = parse_number(opnd);
                } else {
                    Variable* var2 = get_variable(opnd, TYPE_NUM);
                    b = var2->value.num;
                }
        
                var->value.num = a * b;
            }
            ip++;
            continue;
        }
        else if(stricmp(tokens[0], "div") == 0) {
            if(token_count < 4)
                print("error: invalid div syntax\n");
            else {
                char *dest = tokens[2];
                char *opnd = tokens[3];
                Variable* var = get_variable(dest, TYPE_NUM);
                double a = var->value.num;
                double b = 0;
        
                if (is_number(opnd)) {
                    b = parse_number(opnd);
                } else {
                    Variable* var2 = get_variable(opnd, TYPE_NUM);
                    b = var2->value.num;
                }
        
                if (b == 0)
                    print("error: division by zero\n");
                else
                    var->value.num = a / b;
            }
            ip++;
            continue;
        }
        
         else if(stricmp(tokens[0], "printc") == 0) {
             if(token_count < 2)
                 print("error: printc needs operands\n");
             else {
                 char fullText[256];
                 fullText[0] = '\0';
                 for(int j = 1; j < token_count; j++){
                     strcat(fullText, tokens[j]);
                     if(j < token_count-1)
                         strcat(fullText, " ");
                 }
                 print(fullText);
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "print") == 0) {
             if(token_count < 2)
                 print("error: print needs a variable name\n");
             else {
                 Variable* var = get_variable(tokens[1], TYPE_STR);
                 if(var->type == TYPE_NUM) {
                     char buf[64];
                     snprintf(buf, sizeof(buf), "%d", (int)var->value.num);
                     print(buf);
                 } else if(var->type == TYPE_STR)
                     print(var->value.str);
                 else if(var->type == TYPE_BOOL)
                     print(var->value.boolean ? "true" : "false");
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "println") == 0) {
             println("");
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "jump") == 0) {
             if(token_count < 2)
                 print("error: jump needs a label\n");
             else {
                 int target = find_label(tokens[1]);
                 if(target >= 0)
                     ip = target;
                 else {
                     print("error: label not found\n");
                     ip++;
                 }
             }
             continue;
         }
         else if(stricmp(tokens[0], "jumpv") == 0) {
             if(token_count < 3)
                 print("error: jumpv needs a label and a variable\n");
             else {
                 Variable* v = get_variable(tokens[2], TYPE_BOOL);
                 if(v->value.boolean) {
                     int target = find_label(tokens[1]);
                     if(target >= 0)
                         ip = target;
                     else {
                         print("error: label not found\n");
                         ip++;
                     }
                 } else {
                     ip++;
                 }
             }
             continue;
         }
         else if(stricmp(tokens[0], "eqc") == 0) {
             if(token_count < 4)
                 print("error: eqc syntax: eqc <dest> <var> <constant>\n");
             else {
                 Variable* var = get_variable(tokens[2], TYPE_STR);
                 Variable* dest = get_variable(tokens[1], TYPE_BOOL);
                 if(var->type == TYPE_NUM && is_number(tokens[3])) {
                     double numConst = parse_number(tokens[3]);
                     dest->value.boolean = (var->value.num == numConst);
                 } else {
                     dest->value.boolean = (strcmp(var->value.str, tokens[3]) == 0);
                 }
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "eqv") == 0) {
             if(token_count < 4)
                 print("error: eqv syntax: eqv <dest> <var1> <var2>\n");
             else {
                 Variable* v1 = get_variable(tokens[2], TYPE_STR);
                 Variable* v2 = get_variable(tokens[3], TYPE_STR);
                 Variable* dest = get_variable(tokens[1], TYPE_BOOL);
                 if(v1->type == TYPE_NUM && v2->type == TYPE_NUM)
                     dest->value.boolean = (v1->value.num == v2->value.num);
                 else
                     dest->value.boolean = (strcmp(v1->value.str, v2->value.str) == 0);
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "neqc") == 0) {
             if(token_count < 4)
                 print("error: neqc syntax: neqc <dest> <var> <constant>\n");
             else {
                 Variable* var = get_variable(tokens[2], TYPE_STR);
                 Variable* dest = get_variable(tokens[1], TYPE_BOOL);
                 if(var->type == TYPE_NUM && is_number(tokens[3]))
                     dest->value.boolean = (var->value.num != parse_number(tokens[3]));
                 else
                     dest->value.boolean = (strcmp(var->value.str, tokens[3]) != 0);
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "neqv") == 0) {
             if(token_count < 4)
                 print("error: neqv syntax: neqv <dest> <var1> <var2>\n");
             else {
                 Variable* v1 = get_variable(tokens[2], TYPE_STR);
                 Variable* v2 = get_variable(tokens[3], TYPE_STR);
                 Variable* dest = get_variable(tokens[1], TYPE_BOOL);
                 if(v1->type == TYPE_NUM && v2->type == TYPE_NUM)
                     dest->value.boolean = (v1->value.num != v2->value.num);
                 else
                     dest->value.boolean = (strcmp(v1->value.str, v2->value.str) != 0);
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "gt") == 0) {
             if(token_count < 4)
                 print("error: gt syntax: gt <dest> <var1> <var2>\n");
             else {
                 Variable* v1 = get_variable(tokens[2], TYPE_NUM);
                 Variable* v2 = get_variable(tokens[3], TYPE_NUM);
                 Variable* dest = get_variable(tokens[1], TYPE_BOOL);
                 dest->value.boolean = (v1->value.num > v2->value.num);
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "gte") == 0) {
             if(token_count < 4)
                 print("error: gte syntax: gte <dest> <var1> <var2>\n");
             else {
                 Variable* v1 = get_variable(tokens[2], TYPE_NUM);
                 Variable* v2 = get_variable(tokens[3], TYPE_NUM);
                 Variable* dest = get_variable(tokens[1], TYPE_BOOL);
                 dest->value.boolean = (v1->value.num >= v2->value.num);
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "st") == 0) {
             if(token_count < 4)
                 print("error: st syntax: st <dest> <var1> <var2>\n");
             else {
                 Variable* v1 = get_variable(tokens[2], TYPE_NUM);
                 Variable* v2 = get_variable(tokens[3], TYPE_NUM);
                 Variable* dest = get_variable(tokens[1], TYPE_BOOL);
                 dest->value.boolean = (v1->value.num < v2->value.num);
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "ste") == 0) {
             if(token_count < 4)
                 print("error: ste syntax: ste <dest> <var1> <var2>\n");
             else {
                 Variable* v1 = get_variable(tokens[2], TYPE_NUM);
                 Variable* v2 = get_variable(tokens[3], TYPE_NUM);
                 Variable* dest = get_variable(tokens[1], TYPE_BOOL);
                 dest->value.boolean = (v1->value.num <= v2->value.num);
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "and") == 0) {
             if(token_count < 4)
                 print("error: and syntax: and <dest> <var1> <var2>\n");
             else {
                 Variable* v1 = get_variable(tokens[2], TYPE_BOOL);
                 Variable* v2 = get_variable(tokens[3], TYPE_BOOL);
                 Variable* dest = get_variable(tokens[1], TYPE_BOOL);
                 dest->value.boolean = (v1->value.boolean && v2->value.boolean);
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "or") == 0) {
             if(token_count < 4)
                 print("error: or syntax: or <dest> <var1> <var2>\n");
             else {
                 Variable* v1 = get_variable(tokens[2], TYPE_BOOL);
                 Variable* v2 = get_variable(tokens[3], TYPE_BOOL);
                 Variable* dest = get_variable(tokens[1], TYPE_BOOL);
                 dest->value.boolean = (v1->value.boolean || v2->value.boolean);
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "not") == 0) {
             if(token_count < 2)
                 print("error: not syntax: not <var>\n");
             else {
                 Variable* v = get_variable(tokens[1], TYPE_BOOL);
                 v->value.boolean = !(v->value.boolean);
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "conv") == 0) {
             if(token_count < 3)
                 print("error: conv syntax: conv <var> <target_type>\n");
             else {
                 Variable* v = get_variable(tokens[1], TYPE_STR);
                 char *target = tokens[2];
                 if(stricmp(target, "num") == 0) {
                     Variable* nv = get_variable(tokens[1], TYPE_NUM);
                     nv->value.num = is_number(v->value.str) ? parse_number(v->value.str) : 0;
                     nv->type = TYPE_NUM;
                 } else if(stricmp(target, "str") == 0) {
                     v->type = TYPE_STR;
                 } else if(stricmp(target, "bool") == 0) {
                     Variable* bv = get_variable(tokens[1], TYPE_BOOL);
                     bv->value.boolean = (stricmp(v->value.str, "true") == 0) ? 1 : 0;
                     bv->type = TYPE_BOOL;
                 }
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "call") == 0) {
             if(token_count < 2)
                 print("error: call requires function name\n");
             else {
                 char *funcName = tokens[1];
                 int target = find_function(funcName);
                 if(target < 0) {
                     print("error: function not found\n");
                 } else {
                     int argIndex = 0;
                     for(int j = 2; j+1 < token_count; j += 2) {
                         set_parameter(argIndex, tokens[j], tokens[j+1]);
                         argIndex++;
                     }
                     if(call_stack_top < MAX_CALL_STACK)
                         call_stack[call_stack_top++] = ip + 1;
                     else
                         print("error: call stack overflow\n");
                     ip = target + 1;
                     continue;
                 }
             }
             ip++;
             continue;
         }
         else if(stricmp(tokens[0], "ret") == 0) {
             if(call_stack_top > 0)
                 ip = call_stack[--call_stack_top];
             else {
                 print("error: ret with empty call stack\n");
                 ip++;
             }
             continue;
         }
         else if(stricmp(tokens[0], "end") == 0) {
             if(token_count >= 2 && stricmp(tokens[1], "fun") == 0) {
                 if(call_stack_top > 0)
                     ip = call_stack[--call_stack_top];
                 else
                     ip++;
             } else {
                 ip++;
             }
             continue;
         }
         else if(stricmp(tokens[0], "exit") == 0) {
             print("exiting simas interpreter...\n");
             reset();
             return;
         }
         else {
             print("error: unknown command: ");
             print(tokens[0]);
             println("");
             ip++;
         }
     }
     println("");
 }
 
 // -------------------- script input / interactive mode --------------------
 
 char script_storage[MAX_SCRIPT_LENGTH];
 
 void simas_main() {
     gotoxy(0,0);
     disable_cursor();
     clear_screen();
     print("SIMAS for Beacon; version effective 2.0\n");
     int script_index = 0;
     script_storage[0] = '\0';
     const char *prompt = "simas> ";
     println("commands: new, write, run, help, exit");
     
     while(1) {
         print(prompt);
         char line_buffer[1024];
         simas_read_line(line_buffer, sizeof(line_buffer), prompt);
         trim(line_buffer);
         if(strlen(line_buffer) == 0)
             continue;
         if(stricmp(line_buffer, "exit") == 0)
             break;
         else if(stricmp(line_buffer, "help") == 0) {
             println("available commands:");
             println("  new   - clear current script buffer");
             println("  write - enter multi-line mode to write a program (end with a single dot '.')");
             println("  run   - run the current script buffer");
             println("  help  - show this message");
             println("  exit  - exit interpreter");
             println("");
             println("simas instructions supported:");
             println("  set, print, printc, println, add, sub, mul, div");
             println("  fun, ret, call, end fun");
             println("  label, jump, jumpv");
             println("  eqc, eqv, neqc, neqv, gt, gte, st, ste");
             println("  and, or, not, conv");
         }
         else if(stricmp(line_buffer, "new") == 0) {
             script_storage[0] = '\0';
             script_index = 0;
             print("script buffer cleared.\n");
         }
         else if(stricmp(line_buffer, "write") == 0) {
             println("enter your program. type a single dot (.) on a line to finish:");
             script_storage[0] = '\0';
             script_index = 0;
             while(1) {
                 print("  > ");
                 char temp_line[1024];
                 simas_read_line(temp_line, sizeof(temp_line), "  > ");
                 trim(temp_line);
                 if(strcmp(temp_line, ".") == 0)
                     break;
                 int len = strlen(temp_line);
                 if(script_index + len + 2 >= MAX_SCRIPT_LENGTH) {
                     print("script buffer full, cannot append further.\n");
                     break;
                 }
                 strcat(script_storage, temp_line);
                 strcat(script_storage, "; ");
                 script_index += len + 2;
             }
             print("multi-line input complete.\n");
         }
         else if(stricmp(line_buffer, "run") == 0) {
             if(strlen(script_storage) == 0)
                 print("script buffer is empty.\n");
             else {
                 char *instrs[MAX_INSTRUCTIONS];
                 instruction_count = tokenize(script_storage, ';', instrs, MAX_INSTRUCTIONS);
                 for (int i = 0; i < instruction_count; i++) {
                     instructions[i] = instrs[i];
                     trim(instructions[i]);
                 }
                 preprocess_instructions();
                 execute_instructions();
             }
         }
         else {
             print("unknown command. type 'help' for commands.\n");
         }
     }
     reset();
 }
 