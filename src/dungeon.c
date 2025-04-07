/**
 * dungeon.c - Ported dungeon game for Beacon with SFX and color effects
 * This game was originally written for DOS in Turbo C.
 * I made it just to see what I could personally do on just DOS :)
 * 
 * now it uses our os's functions (clear_screen, print, println, delay_ms, etc), with some ported from DOS / Turbo C (getch, gotoxy)
 * and our custom string/keyboard/screen libraries.
 * 
 * Written by Xander Gomez (tuvalutorture)
 * 
 * Copyright (c) 2025 Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * dungeon.c
 **/

 #include "console.h"
 #include "keyboard.h"
 #include "screen.h"
 #include "string.h"
 #include "command.h"
 #include "os.h"
 #include "speaker.h"
 #include "dungeon.h"
 #include <stdarg.h>
 #include <stdint.h>
 
 // max dimensions for text layout
 #define MAX_WIDTH 80
 #define MAX_HEIGHT 25
 
 // game variables
 int gold = 0;
 int healthPotions = 0;
 int staminaPotions = 0;
 int elixirs = 0;
 int arrows = 0;
 int escapeFailed = 0;
 int steps = 0;
 int enemiesDefeated = 0;
 int bossesDefeated = 0;
 int floor = 1;
 int textDisplayed = 0;
 int playerHealth = 100;
 int playerStamina = 50;
 int maxHealth = 100;    // computed as 100 + 20*(floor - 1)
 int maxStamina = 50;    // computed as 50 + 10*(floor - 1)
 
 char currentText[1024];
 char msg[1024];

void dungeon_reset() {
 // game variables
 gold = 0;
 healthPotions = 0;
 staminaPotions = 0;
 elixirs = 0;
 arrows = 0;
 escapeFailed = 0;
 steps = 0;
 enemiesDefeated = 0;
 bossesDefeated = 0;
 floor = 1;
 textDisplayed = 0;
 playerHealth = 100;
 playerStamina = 50;
 maxHealth = 100;    // computed as 100 + 20*(floor - 1)
 maxStamina = 50;    // computed as 50 + 10*(floor - 1)
}
 
 // implement gotoxy: set os print positions and update hardware cursor
 void gotoxy(size_t x, size_t y) {
    row = y;         // console module's current row
    col = x;         // console module's current col
    curs_row = y;    // vga cursor row
    curs_col = x;    // vga cursor col
    update_cursor();
}

 // new functions for sfx and color flashes
 
 void flash_damage() {
     set_color(4, 4); // red on black for damage flash
     repaint_screen(4, 4);
     beep(300, 200);  // low tone for damage
     delay_ms(500);
     set_color(7, 0); // reset to default white on black
     repaint_screen(7, 0);
 }
 
 void flash_heal() {
     set_color(2, 2); // green on black for healing flash
     repaint_screen(2, 2);
     beep(1000, 100); // high tone for heal
     delay_ms(500);
     set_color(7, 0); // reset to default
     repaint_screen(7, 0);
 }
 
 // clear only the text area (rows 15 to MAX_HEIGHT)
 void clear_text_area() {
     for (int i = 15; i < MAX_HEIGHT; i++) {
         gotoxy(0, i);
         for (int j = 0; j < MAX_WIDTH; j++) {
             print(" ");
         }
     }
     gotoxy(0, 15);
 }

 
 // updateText: format text into currentText buffer and mark as not displayed
 void updateText(const char *format, ...) {
     va_list args;
     va_start(args, format);
     vsnprintf(currentText, sizeof(currentText), format, args);
     va_end(args);
     textDisplayed = 0;
 }
 
 // displayText: show the currentText centered at row 15 with character delay
 void displayText() {
     clear_text_area();  // clear text area before printing new text
     static char lastDisplayedText[1024] = "";
     int yOffset = 15;
     int lineStart = 0;
     int len = strlen(currentText);
     char buffer[MAX_WIDTH + 1];
 
     // if text already displayed and unchanged, just reprint centered
     if (textDisplayed && strcmp(lastDisplayedText, currentText) == 0) {
         int xOffset = (MAX_WIDTH - len) / 2;
         gotoxy(xOffset, 15);
         print(currentText);
         return;
     }
 
     for (int i = 0; i <= len; i++) {
         if (currentText[i] == '\n' || i - lineStart >= MAX_WIDTH - 1 || i == len) {
             int lineEnd = (currentText[i] == '\n' || i == len) ? i : i - 1;
             int lineLength = lineEnd - lineStart;
             for (int j = 0; j < lineLength && j < MAX_WIDTH; j++) {
                 buffer[j] = currentText[lineStart + j];
             }
             buffer[lineLength] = '\0';
 
             int xOffset = (MAX_WIDTH - lineLength) / 2;
             gotoxy(xOffset, yOffset++);
             for (int j = 0; j < lineLength; j++) {
                 char ch[2];
                 ch[0] = buffer[j];
                 ch[1] = '\0';
                 print(ch);
                 delay_ms(50);
             }
             lineStart = i + 1;
             while (currentText[lineStart] == ' ' && currentText[lineStart] != '\0')
                 lineStart++;
         }
     }
     strcpy(lastDisplayedText, currentText);
     textDisplayed = 1;
 }
 
 // displayStatus: shows player health, stamina, gold, steps, enemies, and floor info at top-left
 void displayStatus() {
     char statusBuffer[128];
     int healthBars = playerHealth / 10;
     int staminaBars = playerStamina / 10;
 
     gotoxy(0, 0);
     snprintf(statusBuffer, sizeof(statusBuffer), "Health: [");
     print(statusBuffer);
     for (int i = 0; i < maxHealth / 10; i++) {
         print(i < healthBars ? "|" : " ");
     }
     print("]");
 
     gotoxy(0, 1);
     snprintf(statusBuffer, sizeof(statusBuffer), "Stamina: [");
     print(statusBuffer);
     for (int i = 0; i < maxStamina / 10; i++) {
         print(i < staminaBars ? "|" : " ");
     }
     print("]");
 
     gotoxy(0, 2);
     snprintf(statusBuffer, sizeof(statusBuffer), "Gold: %d", gold);
     print(statusBuffer);
 
     gotoxy(0, 3);
     snprintf(statusBuffer, sizeof(statusBuffer), "Steps: %d", steps);
     print(statusBuffer);
 
     gotoxy(0, 4);
     snprintf(statusBuffer, sizeof(statusBuffer), "Enemies defeated: %d", enemiesDefeated);
     print(statusBuffer);
 
     gotoxy(0, 5);
     snprintf(statusBuffer, sizeof(statusBuffer), "Floor: %d", floor);
     print(statusBuffer);
 }
 
 // displayInventory: shows items at top-right
 void displayInventory() {
     char invBuffer[128];
     gotoxy(60, 0);
     print("Inventory:");
     gotoxy(60, 1);
     snprintf(invBuffer, sizeof(invBuffer), "Health Potions: %d", healthPotions);
     print(invBuffer);
     gotoxy(60, 2);
     snprintf(invBuffer, sizeof(invBuffer), "Stamina Potions: %d", staminaPotions);
     print(invBuffer);
     gotoxy(60, 3);
     snprintf(invBuffer, sizeof(invBuffer), "Elixirs: %d", elixirs);
     print(invBuffer);
     gotoxy(60, 4);
     snprintf(invBuffer, sizeof(invBuffer), "Arrows: %d", arrows);
     print(invBuffer);
 }
 
 // navigateMenu: displays options and returns the selected index using arrow keys
 int navigateMenu(char **options, int numOptions) {
     int selection = 0;
     int key;
     int menuStartLine = 20;
     clear_screen();
     while (1) {
         displayStatus();
         displayInventory();
         displayText();
 
         gotoxy(0, menuStartLine - 1);
         for (int i = 0; i < MAX_WIDTH; i++)
             print("-");
         
         for (int i = 0; i < numOptions; i++) {
             int x = 5 + (i % 2) * 40;
             int y = menuStartLine + (i / 2);
             gotoxy(x, y);
             if (i == selection)
                 print("-> ");
             else
                 print("   ");
             print(options[i]);
             update_cursor(); // sync cursor with arrow
         }
 
         key = getch();
         if (key == UP_ARROW)
             selection = (selection - 2 + numOptions) % numOptions;
         if (key == DOWN_ARROW)
             selection = (selection + 2) % numOptions;
         if (key == LEFT_ARROW)
             selection = (selection - 1 + numOptions) % numOptions;
         if (key == RIGHT_ARROW)
             selection = (selection + 1) % numOptions;
         if (key == '\n') {
             clear_screen();
             return selection;
         }
     }
 }
 
 // regenerateStamina: restore 10 stamina up to max
 void regenerateStamina() {
     playerStamina += 10;
     if (playerStamina > maxStamina)
         playerStamina = maxStamina;
 }
 
 // runAway: attempt escape from combat
 void runAway() {
     int damage;
     if (playerStamina < 10) {
         escapeFailed = 1;
         snprintf(msg, sizeof(msg), "Not enough stamina to run away!");
     } else if (rand(10) < 2) {
         escapeFailed = 1;
         damage = rand(11) + 5;
         playerHealth -= damage;
         flash_damage(); // flash red for damage
         snprintf(msg, sizeof(msg), "Failed to escape! Took %d damage.", damage);
     } else {
         escapeFailed = 0;
         snprintf(msg, sizeof(msg), "Successfully escaped! Lost 10 stamina. Lost %d gold.", gold / 5);
         playerStamina -= 10;
         if (playerStamina < 0)
             playerStamina = 0;
         gold = gold - (gold / 5);
     }
     updateText(msg);
     displayText();
     clear_screen();
 }
 
 // encounterShop: let the player buy/sell items
 void encounterShop() {
     char *shopOptions[] = {"Buy Items", "Leave"};
     int healthPotionCost = rand(30) + 30;
     int staminaPotionCost = rand(10) + 20;
     int elixirCost = rand(50) + 170;
     int arrowCost = rand(20) + 20;
     int selection;
     int purchaseSelection;
 
     updateText("You encounter a shopkeeper!");
     displayText();
     while (1) {
         selection = navigateMenu(shopOptions, 2);
         if (selection == 0) {
             char *purchaseOptions[] = {"Buy Health Potion", "Buy Stamina Potion", "Buy Elixir", "Buy Arrows", "Cancel"};
             updateText("What would you like to buy?\nValues: Health Potion: %dg. Stamina Potion: %dg.\nElixir: %dg. Arrow Bundle: %dg.",
                        healthPotionCost, staminaPotionCost, elixirCost, arrowCost);
             displayText();
             purchaseSelection = navigateMenu(purchaseOptions, 5);
             if (purchaseSelection == 0 && gold >= healthPotionCost) {
                 healthPotions++;
                 gold -= healthPotionCost;
                 updateText("You bought a health potion.");
             } else if (purchaseSelection == 1 && gold >= staminaPotionCost) {
                 staminaPotions++;
                 gold -= staminaPotionCost;
                 updateText("You bought a stamina potion.");
             } else if (purchaseSelection == 2 && gold >= elixirCost) {
                 elixirs++;
                 gold -= elixirCost;
                 updateText("You bought an elixir.");
             } else if (purchaseSelection == 3 && gold >= arrowCost) {
                 arrows += 5;
                 gold -= arrowCost;
                 updateText("You bought a bundle of arrows.");
             } else if (purchaseSelection == 4) {
                 updateText("You chose not to buy anything.");
             } else {
                 updateText("You don't have enough gold!");
             }
         } else {
             updateText("You left the shop.");
             break;
         }
         displayText();
         clear_screen();
     }
 }
 
 // encounterEnemy: handle combat with enemy or boss (isBoss = 1 for boss)
 void encounterEnemy(int isBoss) {
     char *monsters[] = {"Goblin", "Troll", "Skeleton", "Wizard", "Zombie-sized Chicken", "Mimic", "Bandit", "Rabid Dog"};
     char *bosses[] = {"Ironclad King", "Ravager of Souls", "Stormbringer", "The Herald of Despair", "Voidcaller"};
     int baseHealth = rand(isBoss ? 120 : 60) + (isBoss ? 150 : 30);
     int monsterHealth = baseHealth;
     int retaliate = 1, defended = 0;
     int action;
     int monsterCount = isBoss ? 5 : 8;
 
     // set combat color scheme
     if (isBoss) {
         set_color(0, 4); // boss mode: black text on red background
         repaint_screen(0, 4);
     } else {
         set_color(15, 0); // normal combat: white on black
         repaint_screen(15, 0);
     }
 
     if (isBoss)
         snprintf(msg, sizeof(msg), "You encounter the %s!", bosses[rand(monsterCount)]);
     else
         snprintf(msg, sizeof(msg), "A wild %s appeared!", monsters[rand(monsterCount)]);
 
     updateText(msg);
     displayText();
     getch();
     clear_screen();
 
     while (monsterHealth > 0 && playerHealth > 0) {
         char *combatOptions[] = {"Attack Menu", "Item Menu", "Defend", "Run", "Scan"};
         displayStatus();
         displayInventory();
         displayText();
         action = navigateMenu(combatOptions, 5);
         retaliate = 1;
         if (action == 0) {
             defended = 0;
             attackMenu(&monsterHealth, &retaliate, isBoss, baseHealth);
         } else if (action == 1) {
             useItemMenu();
             retaliate = 0;
         } else if (action == 2) {
             defended = 1;
             playerStamina += 5;
             if (playerStamina > maxStamina)
                 playerStamina = maxStamina;
         } else if (action == 3) {
             if (isBoss) {
                 updateText("Can't run away from a boss!");
             } else {
                 runAway();
                 if (!escapeFailed)
                     return;
             }
             retaliate = 0;
         } else if (action == 4) {
             snprintf(msg, sizeof(msg), "Enemy has %d / %d health.", monsterHealth, baseHealth);
             retaliate = 0;
             updateText(msg);
         } else {
             updateText("You canceled your action.");
             retaliate = 0;
         }
         if (monsterHealth <= 0) {
             int loot = rand(20) + 10;
             gold += loot;
             if (isBoss) {
                 snprintf(msg, sizeof(msg), "Congrats! Health and stamina increased!");
                 floor++;
                 enemiesDefeated = 0;
                 playerHealth = maxHealth;
                 playerStamina = maxStamina;
             } else {
                 snprintf(msg, sizeof(msg), "You defeated the monster!\nGained %d gold.", loot);
             }
             enemiesDefeated++;
             updateText(msg);
             displayText();
             getch();
         }
         if (retaliate && monsterHealth > 0) {
             int baseDamage = rand(isBoss ? 15 : 8) + (isBoss ? 10 : 3);
             float damage = baseDamage;
             if (rand(10) == 0) {
                 damage *= 2;
                 snprintf(msg, sizeof(msg), "The monster scored a critical hit and dealt %d damage!", (int)damage);
             } else {
                 if (rand(10) == 0) {
                     snprintf(msg, sizeof(msg), "The monster completely missed!");
                     damage = 0;
                 } else if (defended == 1) {
                     snprintf(msg, sizeof(msg), "The monster attacks, but you block some of it! You take %d damage.", (int)damage / 2);
                 } else {
                     snprintf(msg, sizeof(msg), "The monster retaliates and deals %d damage!", (int)damage);
                 }
             }
             playerHealth -= (int)damage;
             flash_damage(); // flash red on damage
             updateText(msg);
             displayText();
             if (playerHealth <= 0)
                 endGame();
         }
     }
     // reset color scheme after combat
     set_color(2, 15); // reset to green on white for text
     repaint_screen(2, 15); // green text on white background
     clear_screen();
 }
 
 // useItemMenu: allow the player to use a potion or elixir
 void useItemMenu() {
     char *itemOptions[] = {"Use Health Potion", "Use Stamina Potion", "Use Elixir", "Cancel"};
     int selection = navigateMenu(itemOptions, 4);
     int heal, stamina;
     if (selection == 0) {
         if (healthPotions > 0) {
             healthPotions--;
             heal = rand(30) + 20;
             playerHealth += heal;
             if (playerHealth > maxHealth)
                 playerHealth = maxHealth;
             flash_heal(); // flash green on heal
             snprintf(msg, sizeof(msg), "Used health potion! Healed %d health.", heal);
             updateText(msg);
         } else {
             updateText("No health potions left!");
         }
     } else if (selection == 1) {
         if (staminaPotions > 0) {
             staminaPotions--;
             stamina = rand(20) + 15;
             playerStamina += stamina;
             if (playerStamina > maxStamina)
                 playerStamina = maxStamina;
             flash_heal(); // flash green on heal
             snprintf(msg, sizeof(msg), "Used stamina potion! Restored %d stamina.", stamina);
             updateText(msg);
         } else {
             updateText("No stamina potions left!");
         }
     } else if (selection == 2) {
         if (elixirs > 0) {
             elixirs--;
             playerHealth = maxHealth;
             playerStamina = maxStamina;
             flash_heal();
             updateText("Used elixir! Fully restored health and stamina.");
         } else {
             updateText("No elixirs left!");
         }
     } else {
         updateText("Canceled item menu.");
     }
     displayText();
     getch();
     clear_screen();
 }
 
 // attackMenu: let the player choose an attack type and calculate damage
 void attackMenu(int *monsterHealth, int *retaliate, int isBoss, int baseHealth) {
     char *attackOptions[] = {"Normal Attack (10 stamina)", "Heavy Attack (20 stamina)", "Light Attack (free)", "Fire Arrow (5 stamina)", "Cancel"};
     int selection = navigateMenu(attackOptions, 5);
     int damage = 0, crit = 0, miss = 0;
     if (selection == 4) {
         *retaliate = 0;
         updateText("Attack canceled.");
     } else {
         int cost[] = {10, 20, 0, 5};
         int baseDamage[] = {10, 20, 5, 15};
         int range[] = {6, 11, 4, 16};
         if ((selection == 3 && (arrows <= 0 || playerStamina < cost[selection])) || (playerStamina < cost[selection])) {
             updateText(selection == 3 ? "Not enough stamina or no arrows left!" : "Not enough stamina!");
             *retaliate = 0;
         } else {
             if (selection < 3 || (selection == 3 && arrows > 0)) {
                 playerStamina -= cost[selection];
                 if (selection == 3)
                     arrows--;
             }
             damage = rand(range[selection]) + baseDamage[selection];
             miss = (rand(10) == 0);
             crit = (rand(10) == 0);
             if (miss) {
                 updateText("You completely missed!");
                 damage = 0;
             } else if (crit) {
                 damage *= 2;
                 char temp[128];
                 snprintf(temp, sizeof(temp), "%s scored a critical hit and dealt %d damage!", attackOptions[selection], damage);
                 updateText(temp);
             } else {
                 char temp[128];
                 snprintf(temp, sizeof(temp), "%s dealt %d damage!", attackOptions[selection], damage);
                 updateText(temp);
             }
             *monsterHealth -= damage;
         }
     }
     displayText();
     getch();
     clear_screen();
 }
 
 // endGame: display game over and shutdown the system
 void endGame() {
     clear_screen();
     row = 0;
     col = 0;
     print("\nGame Over! Thanks for playing.\n\nMade by Xander Gomez (tuvalutorture)\n\n");
     print("Press any key to exit...\n");
     getch();
     reset(); // reset the system
}
 
 // startingItem: let player pick a starting item then begin the dungeon
 void startingItem() {
     char *startingOptions[] = {"Stamina potion", "Health potion", "Bundle of arrows", "Nothing"};
     int selection = navigateMenu(startingOptions, 4);
     if (selection == 0) {
         staminaPotions++;
     } else if (selection == 1) {
         healthPotions++;
     } else if (selection == 2) {
         arrows += 5;
     }
     updateText("Well then, let's begin.");
     displayText();
     exploreDungeon();
     displayStatus();
     displayInventory();
 }
 
 // exploreDungeon: main loop for dungeon exploration and encounters
 void exploreDungeon() {
     char *dungeonOptions[] = {"Delve further", "Rest", "Item", "Quit"};
     int action;
     while (1) {
        displayStatus();
        displayInventory();
        displayText();
        action = navigateMenu(dungeonOptions, 4);
        if (enemiesDefeated >= 5) {
            encounterEnemy(1);
        }
        if (floor == 6) {
            updateText("Congratulations! You've cleared all the floors!");
            displayText();
            getch();
            endGame();
        }
        if (action == 3) {
            endGame();
        }
        if (action == 2) {
            useItemMenu();
        }
        steps++;
        if (action == 1) {
            if (rand(4) == 0) {
                encounterEnemy(0);
            } else {
                regenerateStamina();
                playerHealth += 5;
                if (playerHealth > maxHealth)
                    playerHealth = maxHealth;
                if (playerStamina > maxStamina)
                    playerStamina = maxStamina;
                updateText("You rested, regaining 10 stamina and 5 health.");
            }
        } else {
            switch (rand(4)) {
                case 0:
                    encounterEnemy(0);
                    break;
                case 1:
                    encounterEnemy(0);
                    break;
                case 2:
                    encounterShop();
                    break;
                default:
                    updateText("Nothing happens.");
                    break;
            }
        }
        displayText();
        clear_screen();
    }
     reset(); // reset the system if we exit the loop
 }
 
 // main: entry point for the game
 int start_dungeon() {
     srand(123456789);
     clear_screen();
     updateText("Welcome to the dungeon...");
     displayText();
     getch();
     updateText("Pick a starting item.");
     displayText();
     startingItem();
     displayStatus();
     displayInventory();
     exploreDungeon();
     return 0;
 }
 