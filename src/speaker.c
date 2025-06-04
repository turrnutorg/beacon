/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * speaker.c
 */
#include <stdint.h>
#include "console.h"
#include "keyboard.h"
#include "port.h"
#include "time.h"
#include "string.h"
#include "command.h"
#include "speaker.h"
#include "screen.h"
#include "stdlib.h"
 
#define MELODY_QUEUE_SIZE 256
#define MAX_LINE_LEN 1024
#define MAX_TOKENS 32

typedef struct {
    int freq;
    int duration;
    int fg;
    int bg;
} MelodyNote;

MelodyNote melodyQueue[MELODY_QUEUE_SIZE];
int melodyQueueSize = 0;

int loopEnabled = 0;
int loopCount = 0;      // 0 means infinite loop
int loopStartNote = 0;
int loopEndNote = 0;


// beep_custom lets ye set the frequency (in Hz) and duration (in ms) for yer ear-assault
// if ye set frequency to 0, don't even bother, ye brainless twat
void beep(uint32_t frequency, uint32_t ms) {
    if (frequency == 0) {
        return; // don't be a clueless bastard
    }

    uint32_t div = 1193180 / frequency;  // calculate PIT divider

    // configure PIT channel 2 for square wave
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(div & 0xFF));        // low byte
    outb(0x42, (uint8_t)((div >> 8) & 0xFF)); // high byte

    // turn on the PC speaker (bits 0 and 1)
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 0x03);

    // ACTUALLY wait now using proper delay
    delay_ms(ms);

    // shut that shit off
    tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

 
 /* melody_read_line - reads a line from the keyboard using getch.
  * echoes characters using print() and handles backspace.
  */
 void melody_read_line(char *buffer, int max_len) {
     int i = 0;
     while (i < max_len - 1) {
         int ch = getch();
         if(ch == '\r' || ch == '\n')
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
         {
             char temp[2];
             temp[0] = (char)ch;
             temp[1] = '\0';
             print(temp);
         }
     }
     buffer[i] = '\0';
     println("");
 }
 
 /* tokenize_line - splits a line by spaces into tokens using strtok. */
 int tokenize_line(char *line, char *tokens[], int max_tokens) {
     int count = 0;
     char *token = strtok(line, " ");
     while(token != 0 && count < max_tokens) {
         tokens[count++] = token;
         token = strtok(0, " ");
     }
     return count;
 }
 
 /* print_help - displays available commands. */
 void print_help() {
     println("available commands:");
     println("  add <freq> <duration> [fg] [bg]   - add a note to the queue");
     println("  play                             - play the melody queue");
     println("  delete                           - delete the last note");
     println("  clear                            - clear the entire queue");
     println("  show [last]                      - show all notes or just the last note");
     println("  loop <count> [start note] [end note] - set loop points (0 count means infinite)");
     println("  random <count>                   - add <count> random notes");
     println("  edit <index> <freq> <duration> [fg] [bg] - edit a note at specified index");
     println("  help                             - display this help");
     println("  exit                             - exit the app");
 }
 
 /* play_melody - plays all notes in the melody queue, handling loops if enabled */
 void play_melody() {
     if(melodyQueueSize == 0) {
         println("queue's empty, ya muppet. nothing tae play.");
         return;
     }
     println("playing melody queue...");
     int i = 0;
     // play notes up to loop start if loop is enabled and loopStartNote > 0
     if(loopEnabled && loopStartNote > 0) {
         for(i = 0; i < loopStartNote; i++) {
             set_color(melodyQueue[i].fg, melodyQueue[i].bg);
             repaint_screen(melodyQueue[i].fg, melodyQueue[i].bg);
             {
                 char buf[128];
                 snprintf(buf, 128, "beep! %d Hz, %d ms. colors: fg %d, bg %d",
                          melodyQueue[i].freq, melodyQueue[i].duration,
                          melodyQueue[i].fg, melodyQueue[i].bg);
                 println(buf);
             }
             beep(melodyQueue[i].freq, melodyQueue[i].duration);
         }
     }
     // loop between loopStartNote and loopEndNote if enabled
     if(loopEnabled) {
         int currentLoop = 0;
         while(loopCount == 0 || currentLoop < loopCount) {
             for (int j = loopStartNote; j <= loopEndNote && j < melodyQueueSize; j++) {
                 set_color(melodyQueue[j].fg, melodyQueue[j].bg);
                 repaint_screen(melodyQueue[j].fg, melodyQueue[j].bg);
                 {
                     char buf[128];
                     snprintf(buf, 128, "beep! %d Hz, %d ms. colors: fg %d, bg %d",
                              melodyQueue[j].freq, melodyQueue[j].duration,
                              melodyQueue[j].fg, melodyQueue[j].bg);
                     println(buf);
                 }
                 beep(melodyQueue[j].freq, melodyQueue[j].duration);
             }
             currentLoop++;
         }
         i = loopEndNote + 1;
     }
     // play the remaining notes
     for(; i < melodyQueueSize; i++) {
         set_color(melodyQueue[i].fg, melodyQueue[i].bg);
         repaint_screen(melodyQueue[i].fg, melodyQueue[i].bg);
         {
             char buf[128];
             snprintf(buf, 128, "beep! %d Hz, %d ms. colors: fg %d, bg %d",
                      melodyQueue[i].freq, melodyQueue[i].duration,
                      melodyQueue[i].fg, melodyQueue[i].bg);
             println(buf);
         }
         beep(melodyQueue[i].freq, melodyQueue[i].duration);
     }
     // reset to default colors
     set_color(2, 15);
     repaint_screen(2, 15);
     println("finished playing melody. yer ears knackered yet?");
 }
 
 /* add_random_notes - adds random notes to the queue based on the provided count */
 void add_random_notes(char *count_str) {
     int count = atoi(count_str);
     if(count <= 0) {
         println("count must be greater than 0, ya muppet.");
         return;
     }
     if(melodyQueueSize + count > MELODY_QUEUE_SIZE) {
         println("queue cannae handle that many notes, ya greedy bastard.");
         return;
     }
     for (int i = 0; i < count; i++) {
         int freq = rand() % 14001 + 400; // 400 to 14400 Hz
         int duration = rand() % 4751 + 250;     // 250 to 5000 ms
         int fg = rand() % 16;                   // 0 to 15
         int bg = rand() % 16;                   // 0 to 15
         melodyQueue[melodyQueueSize].freq = freq;
         melodyQueue[melodyQueueSize].duration = duration;
         melodyQueue[melodyQueueSize].fg = fg;
         melodyQueue[melodyQueueSize].bg = bg;
         melodyQueueSize++;
     }
     {
         char buf[128];
         snprintf(buf, 128, "added %d random notes tae the queue. yer playlist's now a mess.", count);
         println(buf);
     }
 }
 
 /* delete_last_note - removes the last note from the queue */
 void delete_last_note() {
     if(melodyQueueSize == 0) {
         println("queue's already empty, ya tube.");
     } else {
         melodyQueueSize--;
         println("deleted last note from the queue.");
     }
 }
 
 /* clear_queue - clears all notes in the queue */
 void clear_queue() {
     melodyQueueSize = 0;
     println("cleared the entire queue. wiped cleaner than yer browsing history.");
 }
 
 /* show_notes - displays the melody queue or just the last note */
 void show_notes(int token_count, char *tokens[]) {
     if(token_count == 2 && strcmp(tokens[1], "last") == 0) {
         if(melodyQueueSize == 0) {
             println("queue's empty. no last note tae show, ya dafty.");
         } else {
             char buf[128];
             snprintf(buf, 128, "last note: %d Hz, %d ms, fg %d, bg %d",
                      melodyQueue[melodyQueueSize - 1].freq,
                      melodyQueue[melodyQueueSize - 1].duration,
                      melodyQueue[melodyQueueSize - 1].fg,
                      melodyQueue[melodyQueueSize - 1].bg);
             println(buf);
         }
     } else {
         if(melodyQueueSize == 0) {
             println("queue's empty. what the fuck are ye tryin' to look at?");
         } else {
             println("melody queue:");
             for (int i = 0; i < melodyQueueSize; i++) {
                 char buf[128];
                 snprintf(buf, 128, "%d: %d Hz, %d ms, fg %d, bg %d",
                          i, melodyQueue[i].freq, melodyQueue[i].duration,
                          melodyQueue[i].fg, melodyQueue[i].bg);
                 if(loopEnabled) {
                     if(i == loopStartNote)
                         strncat(buf, " [LOOP START]", 128 - strlen(buf) - 1);
                     if(i == loopEndNote)
                         strncat(buf, " [LOOP END]", 128 - strlen(buf) - 1);
                 }
                 println(buf);
             }
         }
     }
 }
 
 /* set_loop_parameters - configures the loop settings */
 void set_loop_parameters(int token_count, char *tokens[]) {
     if(token_count < 2) {
         println("usage: loop <count> [start note] [end note]");
         return;
     }
     loopCount = atoi(tokens[1]);
     loopStartNote = 0;
     loopEndNote = melodyQueueSize - 1;
     if(token_count >= 3)
         loopStartNote = atoi(tokens[2]);
     if(token_count >= 4)
         loopEndNote = atoi(tokens[3]);
     if(melodyQueueSize == 0) {
         println("queue's empty ya bawbag. nothin tae loop.");
         loopEnabled = 0;
     } else if(loopStartNote < 0 || loopStartNote >= melodyQueueSize || loopEndNote < 0 || loopEndNote >= melodyQueueSize) {
         println("start or end note is outta range, ya donkey.");
         loopEnabled = 0;
     } else if(loopStartNote > loopEndNote) {
         println("start note's after end note. did ye mash yer keyboard again?");
         loopEnabled = 0;
     } else {
         println("loop points set. play will loop now. don't say i didnae warn ye.");
         loopEnabled = 1;
     }
 }
 
 /* edit_note - edits an existing note in the queue */
 void edit_note(int token_count, char *tokens[]) {
     if(token_count < 4) {
         println("usage: edit <index> <freq> <duration> [fg] [bg]");
         return;
     }
     int index = atoi(tokens[1]);
     if(index < 0 || index >= melodyQueueSize) {
         println("index out of range, ya absolute melt.");
         return;
     }
     int freq = atoi(tokens[2]);
     int duration = atoi(tokens[3]);
     if(freq <= 0 || duration <= 0) {
         println("invalid freq or duration. this ain't rocket science ya cabbage.");
         return;
     }
     int fg = melodyQueue[index].fg;
     int bg = melodyQueue[index].bg;
     if(token_count >= 5)
         fg = atoi(tokens[4]);
     if(token_count >= 6)
         bg = atoi(tokens[5]);
     if(fg < 0 || fg > 15 || bg < 0 || bg > 15) {
         println("colour values must be between 0 and 15, ya speccy tube.");
         return;
     }
     melodyQueue[index].freq = freq;
     melodyQueue[index].duration = duration;
     melodyQueue[index].fg = fg;
     melodyQueue[index].bg = bg;
     {
         char buf[128];
         snprintf(buf, 128, "edited note #%d: %d Hz, %d ms, fg %d, bg %d",
                  index, freq, duration, fg, bg);
         println(buf);
     }
 }
 
 /* add_note_from_tokens - adds a note to the queue based on tokenized input */
 void add_note_from_tokens(int token_count, char *tokens[]) {
     if(token_count < 2) {
         println("usage: add <freq> <duration> [fg] [bg]");
         return;
     }
     int freq = atoi(tokens[0]);
     int duration = atoi(tokens[1]);
     int fg = 15;
     int bg = 0;
     if(token_count >= 3)
         fg = atoi(tokens[2]);
     if(token_count >= 4)
         bg = atoi(tokens[3]);
     if(freq <= 0 || duration <= 0) {
         println("invalid; discarded. ye tryin' tae crash this thing?");
         return;
     }
     if(fg < 0 || fg > 15 || bg < 0 || bg > 15) {
         println("colour values must be between 0 and 15, ya blind tosser.");
         return;
     }
     if(melodyQueueSize >= MELODY_QUEUE_SIZE) {
         println("queue's full, ya greedy bastard.");
         return;
     }
     melodyQueue[melodyQueueSize].freq = freq;
     melodyQueue[melodyQueueSize].duration = duration;
     melodyQueue[melodyQueueSize].fg = fg;
     melodyQueue[melodyQueueSize].bg = bg;
     melodyQueueSize++;
     println("added tae melody queue. it's like yer makin' yer own funeral music.");
 }
 
 /* melody_interactive - main interactive loop for the app */
 void melody_interactive() {
     char line[MAX_LINE_LEN];
     char *tokens[MAX_TOKENS];
     int token_count;
 
     clear_screen();
     println("Melody App - Interactive Melody Interpreter");
     println("type help if you need help");
     
     /* seed the random number generator with a fixed seed
      * (you can hook this up to an RTC later if ye fancy)
      */
     srand(123456);
     
     while(1) {
         print("melody> ");
         melody_read_line(line, MAX_LINE_LEN);
         if(strlen(line) == 0)
             continue;
         token_count = tokenize_line(line, tokens, MAX_TOKENS);
         if(token_count == 0)
             continue;
         
         if(strcmp(tokens[0], "exit") == 0) {
             println("exiting melody app... ye miserable twat");
             break;
         } else if(strcmp(tokens[0], "help") == 0) {
             print_help();
         } else if(strcmp(tokens[0], "play") == 0) {
             play_melody();
         } else if(strcmp(tokens[0], "random") == 0) {
             if(token_count < 2)
                 println("usage: random <count>. ye fuckin donut.");
             else
                 add_random_notes(tokens[1]);
         } else if(strcmp(tokens[0], "delete") == 0) {
             delete_last_note();
         } else if(strcmp(tokens[0], "clear") == 0) {
             clear_queue();
         } else if(strcmp(tokens[0], "show") == 0) {
             show_notes(token_count, tokens);
         } else if(strcmp(tokens[0], "loop") == 0) {
             set_loop_parameters(token_count, tokens);
         } else if(strcmp(tokens[0], "edit") == 0) {
             edit_note(token_count, tokens);
         } else if(token_count >= 2) {
             // default: if two or more tokens, treat it as an add command
             add_note_from_tokens(token_count, tokens);
         } else {
             println("invalid; discarded. ye want tae try speakin' English next time?");
         }
     }
     reset();
 }
 
 /* melody_main - entry point for the melody app */
 int melody_main() {
     gotoxy(0, 0);
     disable_cursor();
     clear_screen();
     melody_interactive();
     return 0;
 }
 
