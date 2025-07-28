/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * games.c
 */

 #include "screen.h"
 #include "keyboard.h"
 #include "speaker.h"
 #include "time.h"
 #include "console.h"
 #include "string.h"
 #include "math.h"
 #include "command.h"
 #include "os.h"
 #include "stdlib.h"
 #include <stddef.h>
 #include <stdint.h>
 
 /*─────────────────────────────────────────────────────────────────────────────
   helper to print a positive integer (no leading zeros except for single-digit)
   and a helper to print a two-digit number with zero-padding
 ─────────────────────────────────────────────────────────────────────────────*/
 static void print_number(int n) {
     if (n < 0) {
         print("-");
         n = -n;
     }
     if (n >= 10) {
         print_number(n / 10);
     }
     char digit = '0' + (n % 10);
     char buf[2] = { digit, '\0' };
     print(buf);
 }
 
 static void print_padded_two_digits(int n) {
     if (n < 0) {
         print("-");
         n = -n;
     }
     if (n < 10) {
         print("0");
         print_number(n);
     } else {
         print_number(n);
     }
 }
 
 /*─────────────────────────────────────────────────────────────────────────────
   1) GLOBALS & FORWARD DECLARATIONS
 ─────────────────────────────────────────────────────────────────────────────*/
 // ───── shared helpers ───────────────────────────────────────────────────────
 static void read_line(char* buffer, size_t maxlen);
 static int  parse_int(const char* buf);
 static void clear_and_repaint(uint8_t fg, uint8_t bg);
 static void press_any_key_to_continue(void);
 
 // ───── menu & routing ────────────────────────────────────────────────────────
 void main_menu_loop(void);
 static void run_jeremy_simulator(void);
 static void run_math_quiz(void);
 static void run_guessing_game(void);
 
 /*─────────────────────────────────────────────────────────────────────────────
   2) SHARED I/O AND COLOR HELPERS
 ─────────────────────────────────────────────────────────────────────────────*/
 
 // reads up to (maxlen-1) characters into buffer, stopping on '\n'. always
 // null-terminates. echoes input and handles basic backspace.
 static void read_line(char* buffer, size_t maxlen) {
     size_t idx = 0;
     while (1) {
         int ch = getch();
         if (ch < 0) {
             // no key pressed; continue waiting
             continue;
         }
         if (ch == '\r' || ch == '\n') {
             print("\n");
             break;
         }
         if (ch == '\b' || ch == 0x7f) {
             if (idx > 0) {
                 idx--;
                 gotoxy(col - 1, row);
                 print(" ");
                 gotoxy(col - 1, row);
                 update_cursor();
             }
             continue;
         }
         if ((unsigned char)ch >= 0x20 && (unsigned char)ch <= 0x7e) {
             if (idx + 1 < maxlen) {
                 buffer[idx++] = (char)ch;
             }
         }
         // ignore other control characters
     }
     buffer[idx] = '\0';
 }
 
 // simple wrapper around atoi
 static int parse_int(const char* buf) {
     return atoi(buf);
 }
 
 // clear screen and repaint background to the given fg/bg.
 // for now, just clear_screen() suffices.
 static void clear_and_repaint(uint8_t fg, uint8_t bg) {
     clear_screen();
     repaint_screen(fg, bg);
 }
 
 // wait for one keypress before continuing
 static void press_any_key_to_continue(void) {
     print("\n press any key to return to menu...\n");
     getch();
 }
 
 /*─────────────────────────────────────────────────────────────────────────────
   3) MAIN MENU LOOP
 ─────────────────────────────────────────────────────────────────────────────*/
 
 void main_menu_loop(void) {
     clear_screen();
     char choice_buf[16];
 
     for (;;) {
         clear_and_repaint(15, 0);
        gotoxy(0, 0);
        set_color(2, 0); // light cyan on black
        print("    ____                                 ______                         \n");
        print("   / __ )___  ____ __________  ____     / ____/___ _____ ___  ___  _____\n");
        print("  / __  / _ \\/ __ `/ ___/ __ \\/ __ \\   / / __/ __ `/ __ `__ \\/ _ \\/ ___/\n");
        print(" / /_/ /  __/ /_/ / /__/ /_/ / / / /  / /_/ / /_/ / / / / / /  __(__  ) \n");
        print("/_____/\\___/\\__,_/\\___/\\____/_/ /_/   \\____/\\__,_/_/ /_/ /_/\\___/____/  \n");
        print("                                                                         \n");    
        print("\n");
        set_color(15, 0); // reset to white on black
        print(" welcome to the beacon game collection!\n\n");
         // print menu options
         set_color(12, 0);  print("  1");     // red “1”
         set_color(15, 0);  print(". jeremy simulator\n");
         set_color(12, 0);  print("  2");     // red “2”
         set_color(15, 0);  print(". math quiz\n");
         set_color(12, 0);  print("  3");     // red “3”
         set_color(15, 0);  print(". guessing game\n");
         set_color(12, 0);  print("  4");     // red “4”
         set_color(15, 0);  print(". quit to os\n\n");
         // move prompt to bottom row (assumes 25 rows, index 24)
         gotoxy(0, 24);
         set_color(15, 0);
         print("> ");
         gotoxy(2, 24);
         read_line(choice_buf, sizeof(choice_buf));
         int choice = parse_int(choice_buf);
 
         switch (choice) {
             case 1:
                 run_jeremy_simulator();
                 break;
             case 2:
                 run_math_quiz();
                 break;
             case 3:
                 run_guessing_game();
                 break;
             case 4:
                 clear_screen();
                 print(" thanks for playing—returning to os\n");
                 delay_ms(500);
                 reset();
                 return; // not reached
             default:
                 beep(400, 70);
                 print("\n invalid choice. press any key.\n");
                 while (getch() < 0) { }
                 break;
         }
     }
 }
 
 /*─────────────────────────────────────────────────────────────────────────────
   4) JEREMY SIMULATOR (mini-game #1)
 ─────────────────────────────────────────────────────────────────────────────*/
 
 // internal state for jeremy simulator:
 static int  jeremy_health             = 100;
 static int  jeremy_hunger             = 60;
 static int  jeremy_stamina            = 75;
 static int  jeremy_happy              = 50;
 static int  actions_since_last_play   = 0;
 static int  played                    = 0;
 static int  money                     = 5;
 static int  food                      = 3;
 static int  hour                      = 8;
 static int  minute                    = 30;
 static int  day                       = 5;   // 0=mon .. 6=sun
 static int  days_since_last_working   = 0;
 static int  worked                    = 0;
 static int  reprimands                = 0;
 static int  hit                       = 0;
 static int  print_msg                 = 1;
 static int  is_ill                    = 0;
 static int  fired                     = 0;
 static int  chosen_random             = 0;
 
 // we measure real time at start/end, but game uses its own in-game clock
 static uint8_t rtc_start_h, rtc_start_m, rtc_start_s, rtc_start_day, rtc_start_month, rtc_start_year;
 
 static const char* days_str[7] = {
     "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"
 };
 
 static const char* moods_str[9] = {
     "normal", "bored", "happy", "angry", "hungry", "tired", "silly", "skinless", "sickly"
 };
 
 static const char* randoms_arr[12] = {
     "jeremy poops on the floor.",
     "jeremy gets typhus. yum",
     "jeremy screams at the top of his lungs.",
     "jeremy bites your hand. ow.",
     "jeremy starts levitating and spinning at high velocity.\nfor a brief moment at least.",
     "jeremy just stares. blankly.",
     "jeremy starts running around and crawling on you.",
     "jeremy is nibbling a crumb.",
     "jeremy is chewing your headphone cable.",
     "jeremy is just looking out the window. at the peasants below.",
     "jeremy wishes you skin him. (put 'skin' in the terminal)",
     "jeremy is attempting to summon the soul of cinder"
 };
 
 static const char* actions_arr[12] = {
     "nothing", "feed", "pet", "play", "rest", "vet", "food",
     "work", "quit", "help", "skin", "wait"
 };
 
 static int  jeremy_mood = 0;
 
 // forward:
 static void js_set_mood(int mood);
 static void js_process_input(char check, int* hit_out);
 static void js_end_game(void);
 
 static void run_jeremy_simulator(void) {
     char input_buf[64];
     int  action = -1;
 
     clear_and_repaint(15, 0);
     gotoxy(0, 0);
 
     // reset all state:
     jeremy_health            = 100;
     jeremy_hunger            = 60;
     jeremy_stamina           = 75;
     jeremy_happy             = 50;
     actions_since_last_play  = 0;
     played                   = 0;
     money                    = 5;
     food                     = 3;
     hour                     = 8;
     minute                   = 30;
     day                      = 5;
     days_since_last_working  = 0;
     worked                   = 0;
     reprimands               = 0;
     hit                      = 0;
     print_msg                = 1;
     is_ill                   = 0;
     fired                    = 0;
     jeremy_mood              = 0;
 
     srand(extra_rand());
 
     while (1) {
         if (jeremy_health > 0 && jeremy_mood != 7) {
             // 1a) update mood based on stats:
             js_set_mood(0);
             hit   = 0;
             played = 0;
 
             // clamp stats:
             if (jeremy_health > 100) {
                 jeremy_health = 100;
                 jeremy_happy += 10;  // healthy => extra happy
             }
             if (jeremy_hunger < 0)   jeremy_hunger = 0;
             if (jeremy_happy < 0)    jeremy_happy  = 0;
             if (jeremy_stamina < 0)  jeremy_stamina = 0;
 
             // 1b) time advancement:
             if (minute >= 60) {
                 hour++;
                 minute -= 60;
                 if (hour < 8 || hour >= 20) {
                     jeremy_stamina -= 10;
                     print("jeremy is getting tired...\n");
                 }
                 if (hour > 16 && !worked && day < 5 && !fired) {
                     print("it seems you missed work today...\n");
                     days_since_last_working++;
                 }
             }
             if (hour >= 24) {
                 hour -= 24;
                 day++;
                 money -= 5;  // cost of living
                 worked = 0;
             }
             if (day >= 7) {
                 day = 0;
             }
 
             // 1c) show status if print_msg is set:
             if (print_msg) {
                 clear_and_repaint(15, 0);
 
                 // print old-style status:
                 gotoxy(0, 0);
                 print("jeremy is feeling ");
                 print(moods_str[jeremy_mood]);
                 print(". you have ");
                 print_number(money);
                 print(" pounds.\n");
                 print("it is ");
                 print_number(hour);
                 print(":");
                 print_number(minute);
                 print(" on ");
                 print(days_str[day]);
                 print(".\n");
                 print("what would you like to do (help for options)\n");
 
                 int tmp = rand() % 10;  // 0..9
                 if (tmp == 1) {
                     chosen_random = rand() % 12;
                     print(randoms_arr[chosen_random]);
                     print("\n");
                     if (chosen_random == 2) {
                         is_ill = 1;
                     }
                 }
 
                 jeremy_happy--;
                 jeremy_hunger--;
                 if (is_ill) {
                     jeremy_health -= 15;
                 }
                 if (hour >= 8 && day < 5 && hour < 16 && !fired) {
                     print("protip: go to work to earn money (you should do that)\n");
                 }
             }
             print_msg = 1;
 
             // 1d) prompt for action (bottom of screen)
             gotoxy(0, 24);
             print("> ");
             gotoxy(2, 24);
             read_line(input_buf, sizeof(input_buf));
 
             // 1e) parse input:
             action = -1;
             for (int i = 0; i < 13; i++) {
                 if (strstr(input_buf, actions_arr[i]) != NULL) {
                     action = i;
                     hit = 1;
                     break;
                 }
             }
 
             if (hit) {
                 clear_screen();
                 gotoxy(0, 0);
                 switch (action) {
                     case 0: // “nothing”
                         print("you do nothing for a whole hour. wow.\n");
                         jeremy_happy -= 5;
                         jeremy_stamina--;
                         hour++;
                         break;
 
                     case 1: // “feed”
                         if (food >= 1) {
                             print("you feed jeremy.\n");
                             food--;
                             if (jeremy_hunger <= 90 && jeremy_hunger > 30) {
                                 jeremy_hunger += 10;
                                 jeremy_happy   += 5;
                                 print("jeremy is happier now that he has been fed :d\n");
                             }
                             else if (jeremy_hunger <= 30) {
                                 jeremy_hunger += 20;
                                 jeremy_health += 10;
                                 jeremy_happy  += 15;
                                 print("jeremy is very happy that you saved him from starvation\nbut why did you starve him?\n");
                             }
                             else if (jeremy_hunger > 90) {
                                 jeremy_hunger += 10;
                                 jeremy_health -= 3;
                                 jeremy_happy  -= 3;
                                 print("jeremy is full already... stop feeding him please\n");
                             }
                             if (jeremy_hunger >= 150) {
                                 print("aaaand look at that. you killed jeremy.\nyou overfed him. he burst like a balloon. congrats\n");
                                 jeremy_health = 0;
                             }
                             minute += 10;
                         }
                         else {
                             print("you don't have food...\n");
                             print_msg = 0;
                         }
                         break;
 
                     case 2: // “pet”
                         print("you give jeremy the pets :)\nhe is content\n");
                         jeremy_happy += 5;
                         minute += 3;
                         break;
 
                     case 3: // “play”
                         if (jeremy_stamina < 30) {
                             print("jeremy is too tired to play... he should rest :p\n");
                         } else {
                             actions_since_last_play = 0;
                             jeremy_stamina -= 20;
                             jeremy_happy   += 10;
                             jeremy_hunger  -= 5;
                             print("jeremy had a good time playing with you :)\n");
                             played = 1;
                             hour++;
                         }
                         break;
 
                     case 4: // “rest”
                         print("you send jeremy to rest for a good while.\n");
                         jeremy_stamina += 40;
                         if (jeremy_stamina > 100) jeremy_stamina = 100;
                         hour += 8;
                         minute += 15;
                         break;
 
                     case 5: { // “vet”
                         int vet_cost = (rand() % 20) + 5; // 5..24
                         print("taking jeremy to the vet costs ");
                         print_number(vet_cost);
                         print(" pounds. y/n\n");
                         gotoxy(0, 24);
                         print("> ");
                         gotoxy(2, 24);
                         int yn_hit = 0;
                         js_process_input('y', &yn_hit);
                         gotoxy(0, 0);
                         if (yn_hit) {
                             if (money >= vet_cost + 5) {
                                 print("you take jeremy to the vet.\n");
                                 money -= vet_cost;
                                 print("jeremy is at ");
                                 print_number(jeremy_health);
                                 print(" health.\n");
                                 while (getch() < 0) { }
                                 int tmp = rand() % 20;
                                 if (tmp == 4 || is_ill) {
                                     print("jeremy was sick, but was cured for 5 quid extra :d\n");
                                     jeremy_health += 15;
                                     is_ill = 0;
                                 } else {
                                     print("jeremy is perfectly fine.\n");
                                 }
                                 while (getch() < 0) { }
                                 hour++;
                                 minute += 30;
                             }
                             else {
                                 print("you don't have the cash!\n");
                                 minute += 13;
                             }
                         }
                         else {
                             print("you decide not to.\n");
                         }
                         break;
                     }
 
                     case 6: // “food”
                         if (money >= 10) {
                             food++;
                             print("you buy food for jeremy at the store. you now have ");
                             print_number(food);
                             print(" food.\n");
                             money -= 10;
                             minute += (rand() % 25) + 10; // 10..34
                         }
                         else {
                             print("you don't have the money!\n");
                             print_msg = 0;
                         }
                         break;
 
                     case 7: // “work”
                         if (!fired) {
                             if (hour >= 17 || day > 4 || hour < 6) {
                                 print("the office is closed.\n");
                             }
                             else if (reprimands >= 3 || days_since_last_working >= 2) {
                                 print("oops, it looks like you were irresponsible and got fired :)\n");
                                 fired = 1;
                                 minute += 30;
                             }
                             else {
                                 minute += 15;
                                 if (hour >= 6 && hour < 10) {
                                     print("you work until 17 and return to jeremy.\n");
                                     hour = 17; minute = 15;
                                     worked = 1;
                                     money += 20;
                                 }
                                 else if (hour >= 10) {
                                     print("you arrive at work and are scolded for being late.\ndon't do it again\n");
                                     reprimands++;
                                     hour = 17; minute = 15;
                                     worked = 1;
                                     money += 15;
                                 }
                             }
                         }
                         else {
                             print("not like you have employment, bozo :)\n");
                         }
                         break;
 
                     case 8: // “quit”
                         print("aw man, you abandoned jeremy. he died without you :)\n");
                         js_end_game();
                         return;
 
                     case 9: // “help”
                         print("available actions: nothing, feed, pet, play, rest, vet, food, work, quit, help, save, wait\n");
                         print_msg = 0;
                         break;
 
                     case 10: // “skin”
                         clear_screen();
                         print("you... you skinned jeremy...\n...");
                         getch(); // wait for keypress
                         js_set_mood(7);
                         gotoxy(0, 0);
                         set_color(0, RED_COLOR);
                         clear_and_repaint(0, RED_COLOR);
                         break;
 
                     case 11: { // “wait”
                         print("for how long? (in hours up to 24)\n");
                         gotoxy(0, 24);
                         print("> ");
                         gotoxy(2, 24);
                         read_line(input_buf, sizeof(input_buf));
                         int tmp = parse_int(input_buf);
                         gotoxy(0, 0);
                         if (tmp > 0 && tmp <= 24) {
                             hour += tmp;
                             printf("you wait for %d hours.\n", tmp);
                         } else {
                             print("segmentation fault\n");
                         }
                         break;
                     }
 
                     default:
                         break;
                 }
 
                 print("\n");
                 gotoxy(2, 24);
                 getch(); // wait for keypress
                 if (print_msg && !played) {
                     actions_since_last_play++;
                     minute++;
                 }
             }
             else {
                 print("invalid action, silly. type help for an action list\n");
                 print_msg = 0;
             }
         }
         else if (jeremy_mood == 7 && jeremy_health > 0) {
             print("ALL HAIL THE FLESH RODENT\n");
             jeremy_health -= 5;
             while (getch() < 0) { }
         }
         else {
             // jeremy is gone: show final message & playtime:
             js_end_game();
             return;
         }
     }
 }
 
 // set jeremy’s mood based on current stats:
 static void js_set_mood(int forced_mood) {
     if (forced_mood == 0 && jeremy_mood != 7) {
         if (jeremy_happy >= 35 && jeremy_happy <= 60) {
             jeremy_mood = 0; // normal
         } else if (actions_since_last_play >= 5) {
             jeremy_mood = 1; // bored
         } else if (jeremy_happy <= 35) {
             jeremy_mood = 3; // angry
         } else if (actions_since_last_play <= 2 && played == 1) {
             jeremy_mood = 6; // silly
         } else if (jeremy_stamina <= 25) {
             jeremy_mood = 5; // tired
         } else {
             jeremy_mood = 2; // happy
         }
         if (jeremy_hunger <= 50) {
             jeremy_mood = 4; // hungry
         }
         if (jeremy_health <= 50 || is_ill) {
             jeremy_mood = 8; // sickly
             is_ill = 1;
         }
     } else {
         if (jeremy_mood == 7) {
             forced_mood = 7;
             jeremy_happy = 0;
             jeremy_stamina = 0;
         }
         jeremy_mood = forced_mood;
     }
 }
 
 // process input to see if the user typed the specific char (like 'y').
 static void js_process_input(char check, int* hit_out) {
     *hit_out = 0;
     while (1) {
         int c = getch();
         if (c < 0) {
             continue;
         }
         if ((char)c == check || (char)c == (check - 'a' + 'A')) {
             *hit_out = 1;
         }
         break;
     }
 }
 
 // when jeremy fades or you quit, show playtime:
 static void js_end_game(void) {
     clear_and_repaint(15, 0);
     print("\n jeremy is dead. how dare you.\n\n");
     press_any_key_to_continue();
 }
 
 /*─────────────────────────────────────────────────────────────────────────────
   5) MATH QUIZ (mini-game #2)
 ─────────────────────────────────────────────────────────────────────────────*/
 
 // difficulty, questions, counters:
 static int mq_difficulty = 1;
 static int mq_questions;
 static int mq_beginning_question_count;
 static int mq_num_correct;
 static int mq_op;
 static int mq_num1;
 static int mq_num2;
 static int mq_solution;
 static char mq_choice_buf[20];
 static char mq_ans_buf[15];
 static const char* mq_operand_str[4] = { "added", "subracted", "multiplied", "divided" };
 
 static void run_math_quiz(void) {
     clear_and_repaint(15, 0);
     gotoxy(0, 0);
 
     print("it is math quiz time.\n");
     print(" choose a difficulty\n");
     set_color(GREEN_COLOR, 0); print("  1 - easy (just addition)\n");
     set_color(LIGHT_BLUE_COLOR, 0); print("  2 - medium (subtraction.)\n");
     set_color(YELLOW_COLOR, 0); print("  3 - medium+ (multiplication oooh scary)\n");
     set_color(LIGHT_RED_COLOR, 0); print("  4 - normal human (aka division and larger num)\n");
     set_color(WHITE_COLOR, 0); print("\n");
     gotoxy(0, 24);
     print("> ");
     gotoxy(2, 24);
     read_line(mq_choice_buf, sizeof(mq_choice_buf));
     mq_difficulty = parse_int(mq_choice_buf);
     gotoxy(0, 0);
 
     if (mq_difficulty < 1 || mq_difficulty > 4) {
         print("\nassuming you're not smart enough to play. goodbye\n");
         press_any_key_to_continue();
         return;
     }
 
     gotoxy(0, 0);
     clear_and_repaint(15, 0);
     print("\nhow many questions? as many as you want\n");
     gotoxy(0, 24);
     print("> ");
     gotoxy(2, 24);
     read_line(mq_choice_buf, sizeof(mq_choice_buf));
     mq_questions = parse_int(mq_choice_buf);
     gotoxy(0, 0);
     if (mq_questions > 9999) {
         print("too many. try again next time\n");
         return;
     }
     if (mq_questions < 1) {
         print("thats zero questions. which means you win. because all have been answered.\n");
         return;
     }
 
     mq_beginning_question_count = mq_questions;
     srand(mq_difficulty * 385 - mq_questions);
     mq_num_correct = 0;
 
     while (mq_questions > 0) {
         mq_op = rand() % mq_difficulty; // 0..difficulty-1
         if (mq_difficulty < 4 || mq_op == 2) {
             mq_num1 = (rand() % 10) + 1;  // 1..10
             mq_num2 = (rand() % 10) + 1;
         } else {
             mq_num1 = (rand() % 100) + 1; // 1..100
             mq_num2 = (rand() % 100) + 1;
         }
         if (mq_op == 0) {
             mq_solution = mq_num1 + mq_num2;
         } else if (mq_op == 1) {
             mq_solution = mq_num1 - mq_num2;
         } else if (mq_op == 2) {
             mq_solution = mq_num1 * mq_num2;
         } else {
             // division: divide larger by smaller, round down
             if (mq_num2 > mq_num1) {
                 mq_solution = mq_num2 / mq_num1;
                 mq_num1 = mq_num2;
                 mq_num2 = (mq_num1 == 0) ? 1 : mq_num1;
             } else {
                 mq_solution = mq_num1 / mq_num2;
             }
         }
 
         // print question:
         gotoxy(0, 0);
         print("\n");
         print_number(mq_num1);
         print(" ");
         print(mq_operand_str[mq_op]);
         print(" by ");
         print_number(mq_num2);
         print(" = ");
         if (mq_op == 3) {
             print("(round down to the nearest whole num)");
         }
         gotoxy(0, 24);
         print("\n> ");
         gotoxy(2, 24);
 
         read_line(mq_ans_buf, sizeof(mq_ans_buf));
         int user_ans = parse_int(mq_ans_buf);
 
         gotoxy(0, 0);
 
         // check answer:
         print("your answer is... ");
         print_number(user_ans);
         print("\nwhich is...\n");
         if (user_ans == mq_solution) {
             print("correct!!!\n");
             mq_num_correct++;
             getch(); // wait for keypress
         } else {
             print("wrong...\nit should've been ");
             print_number(mq_solution);
             print("\n");
             getch(); // wait for keypress
         }
         clear_screen();
         mq_questions--;
     }
 
     // final score:
     gotoxy(0, 0);
     print("\n");
     print("you got ");
     print_number(mq_num_correct);
     print(" out of ");
     print_number(mq_beginning_question_count);
     print(" correct! adios\n");
     getch();
 }
 
 /*─────────────────────────────────────────────────────────────────────────────
   6) GUESSING GAME (mini-game #3)
 ─────────────────────────────────────────────────────────────────────────────*/
 
 // difficulty, solution, guesses:
 static int gf_difficulty = 1;
 static int gf_solution;
 static int gf_guesses;
 static char gf_choice_buf[32];
 static const char* roasts_arr[17] = {
     "holy hell, are you guessing with your elbows?",
     "your guess genuinely lowered my iq.",
     "wrong. again. you absolute binfire.",
     "if i had arms i'd slap you.",
     "aww, close! not really, but keep trying you precious little failure",
     "wrong, but i'm legally obligated to encourage you... somehow",
     "not quite, but hey, you're trying, and that's adorable",
     "maybe numbers just aren't your thing, sweetheart",
     "lmao no, did you even try?",
     "close, if 'close' meant 'nowhere near it'",
     "not it, champ. keep swinging like a blindfolded toddler",
     "my cat could do better and it's been dead for 2 years",
     "are you trying to lose on purpose you piss-gargler?",
     "if you had a neck i would strangle you",
     "jesus christ on a pogo stick that's horrifically off",
     "jesus christ just uninstall your brain",
     "don't go to vegas with those odds jesus christ"
 };
 
 static void run_guessing_game(void) {
     clear_and_repaint(2, 15);
     set_color(2, 15); // white on black
     gotoxy(0, 0);
     print("welcome to the beacon guessing game.\n");
     print(" choose a difficulty\n");
     print("  1 - easy\n");
     print("  2 - medium\n");
     print("  3 - hard\n");
     gotoxy(0, 24);
     print("> ");
     gotoxy(2, 24);
     read_line(gf_choice_buf, sizeof(gf_choice_buf));
     gf_difficulty = parse_int(gf_choice_buf);
     gotoxy(0, 0);
     clear_screen();
     if (gf_difficulty < 1 || gf_difficulty > 3) {
         print("\n invalid difficulty. goodbye\n");
         press_any_key_to_continue();
         return;
     }
 
     if (gf_difficulty == 1) {
         print("\nguess the number (1-10)\n");
         gf_solution = (rand() % 10) + 1;
     } else if (gf_difficulty == 2) {
         print("\nguess the number (1-25)\n");
         gf_solution = (rand() % 25) + 1;
     } else {
         print("\nguess the number (1-100)\n");
         gf_solution = (rand() % 100) + 1;
     }
 
     gotoxy(0, 24);
     print("> ");
     gotoxy(2, 24);
     gf_guesses = 0;
 
     while (1) {
         read_line(gf_choice_buf, sizeof(gf_choice_buf));
         int guess = parse_int(gf_choice_buf);
         gotoxy(0, 0);
         if (guess == 0) {
             print("you gave up. it was ");
             print_number(gf_solution);
             print(" you aborted\n");
             return;
         }
         gf_guesses++;
 
         int upper = (gf_difficulty == 1) ? 10 : (gf_difficulty == 2) ? 25 : 100;
         if (guess < 1 || guess > upper) {
             print("jesus christ you are stupid please give up already\n");
             return;
         }
         else if (guess == gf_solution) {
             print("correct!!!\nyou win. it took ");
             print_number(gf_guesses);
             print(" guesses. leave\n");
             return;
         }
         else {
             int roast_idx = rand() % 17;
             print(roasts_arr[roast_idx]);
             print("\nguess again\n");
             gotoxy(0, 24);
             print("> ");
             gotoxy(2, 24);
         }
     }
 }
 