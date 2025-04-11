/**
 * dungeon.c - Ported dungeon game for Beacon with SFX and color effects
 *
 * This game was originally written for DOS in Turbo C.
 * Now it uses our os's functions (clear_screen, print, println, delay_ms, etc)
 * via our custom syscall table. All calls are now routed through sys-> and the
 * payload starts at the address provided by the CSA loader.
 *
 * Written by Xander Gomez (tuvalutortorture)
 * Copyright (c) 2025 Turrnut Open Source Organization
 * Licensed under the GPL v3 License
 */

 #include "dungeon.h"
 #include "syscall.h"
 #include <stdarg.h>
 #include <stdint.h>
 
 #define MAX_WIDTH 80
 #define MAX_HEIGHT 25
 
 #define UP_ARROW    -1
 #define DOWN_ARROW  -2
 #define LEFT_ARROW  -3
 #define RIGHT_ARROW -4
 
 /* 
  * extern symbols defined in the linker script for .bss boundaries 
  */
 extern char __bss_start;
 extern char __bss_end;
 
 /*
  * zero_bss:
  * wipe out the .bss section – necessary for proper runtime initialization
  */
 void zero_bss(void) {
     for (char* p = &__bss_start; p < &__bss_end; ++p)
         *p = 0;
 }
 
 
 /* 
  * GLOBAL GAME STATE
  * these globals are placed mostly in .bss so that they can be zeroed at startup
  */
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
 int maxHealth = 100;           // base max health
 int maxStamina = 50;           // base max stamina
 int damageBonus = 0;           // permanent bonus added to all attacks
 int permanentHealthBonus = 0;  // permanent boost from shrines
 int permanentStaminaBonus = 0; // permanent boost from shrines
 
 // shop rest mechanic: base 1 free rest per shop, can be increased with tickets
 int shopRestLimit = 1;
 
 char currentText[1024];
 char msg[1024];
 
 int current_fg_color = 2;  // default text color (green)
 int current_bg_color = 15; // default background (white)
 
 int is_playing = 0; // flag to check if in playing state
 
 // RTC start time globals for playtime tracking
 uint8_t rtc_start_hour = 0, rtc_start_minute = 0, rtc_start_second = 0;
 
 /*
  * global syscall pointer.
  * it gets set in main.c (after zero_bss) by reading from the fixed address.
  */
 syscall_table_t* sys;
 
 /*
  * change_color: sets the foreground and background colors and repaints the screen.
  */
 void change_color(uint8_t fg, uint8_t bg) {
     current_fg_color = fg;
     current_bg_color = bg;
     sys->set_color(fg, bg);
     sys->repaint_screen(fg, bg);
 }
 
 /*
  * dungeon_reset: resets all global game variables to their initial state.
  */
 void dungeon_reset() {
     is_playing = 0;
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
     maxHealth = 100;
     maxStamina = 50;
     damageBonus = 0;
     permanentHealthBonus = 0;
     permanentStaminaBonus = 0;
     shopRestLimit = 1;
 }
 
 /*
  * flash_damage: flashes the screen red to indicate damage.
  */
 void flash_damage() {
     sys->set_color(4, 4); // red on black for damage flash
     sys->repaint_screen(4, 4);
     sys->beep(300, 200);
     sys->delay_ms(500);
     sys->set_color(current_fg_color, current_bg_color);
     sys->repaint_screen(current_fg_color, current_bg_color);
 }
 
 /*
  * flash_heal: flashes the screen green to indicate healing.
  */
 void flash_heal() {
     sys->set_color(2, 2); // green on black for healing flash
     sys->repaint_screen(2, 2);
     sys->beep(1000, 100);
     sys->delay_ms(500);
     sys->set_color(current_fg_color, current_bg_color);
     sys->repaint_screen(current_fg_color, current_bg_color);
 }
 
 /*
  * updateText: formats text with a variable-argument list into currentText buffer.
  */
 void updateText(const char *format, ...) {
     va_list args;
     va_start(args, format);
     sys->vsnprintf(currentText, sizeof(currentText), format, args);
     va_end(args);
     textDisplayed = 0;
 }
 
 /*
  * displayText: clears a portion of the screen and displays the currentText with a delay.
  */
 void displayText() {
     for (int i = 15; i < MAX_HEIGHT; i++) {
         sys->gotoxy(0, i);
         for (int j = 0; j < MAX_WIDTH; j++) {
             sys->print(" ");
         }
     }
     sys->gotoxy(0, 15);
     static char lastDisplayedText[1024] = "";
     int yOffset = 15;
     int lineStart = 0;
     int len = sys->strlen(currentText);
     char buffer[MAX_WIDTH + 1];
 
     if (textDisplayed && sys->strcmp(lastDisplayedText, currentText) == 0) {
         int xOffset = (MAX_WIDTH - len) / 2;
         sys->gotoxy(xOffset, 15);
         sys->print(currentText);
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
             sys->gotoxy(xOffset, yOffset++);
             for (int j = 0; j < lineLength; j++) {
                 char ch[2] = { buffer[j], '\0' };
                 sys->print(ch);
                 sys->delay_ms(1500);
             }
             lineStart = i + 1;
             while (currentText[lineStart] == ' ' && currentText[lineStart] != '\0')
                 lineStart++;
         }
     }
     sys->strcpy(lastDisplayedText, currentText);
     textDisplayed = 1;
     sys->getch(); // wait for a key press
 }
 
 /*
  * showText: sets the currentText buffer with formatted input and calls displayText.
  */
 void showText(const char *format, ...) {
     va_list args;
     va_start(args, format);
     sys->vsnprintf(currentText, sizeof(currentText), format, args);
     va_end(args);
     textDisplayed = 0;
     displayText();
 }
 
 /*
  * displayStatus: shows player's current status (health, stamina, gold, steps, etc.) on the screen.
  */
 void displayStatus() {
     char statusBuffer[128];
     int healthBars = playerHealth / 10;
     int staminaBars = playerStamina / 10;
 
     sys->gotoxy(0, 0);
     sys->snprintf(statusBuffer, sizeof(statusBuffer), "Health: [");
     sys->print(statusBuffer);
     for (int i = 0; i < (maxHealth + permanentHealthBonus) / 10; i++) {
         sys->print(i < healthBars ? "|" : " ");
     }
     sys->print("]");
 
     sys->gotoxy(0, 1);
     sys->snprintf(statusBuffer, sizeof(statusBuffer), "Stamina: [");
     sys->print(statusBuffer);
     for (int i = 0; i < (maxStamina + permanentStaminaBonus) / 10; i++) {
         sys->print(i < staminaBars ? "|" : " ");
     }
     sys->print("]");
 
     sys->gotoxy(0, 2);
     sys->snprintf(statusBuffer, sizeof(statusBuffer), "Gold: %d", gold);
     sys->print(statusBuffer);
 
     sys->gotoxy(0, 3);
     sys->snprintf(statusBuffer, sizeof(statusBuffer), "Steps: %d", steps);
     sys->print(statusBuffer);
 
     sys->gotoxy(0, 4);
     sys->snprintf(statusBuffer, sizeof(statusBuffer), "Enemies defeated: %d", enemiesDefeated);
     sys->print(statusBuffer);
 
     sys->gotoxy(0, 5);
     sys->snprintf(statusBuffer, sizeof(statusBuffer), "Floor: %d", floor);
     sys->print(statusBuffer);
 }
 
 /*
  * displayInventory: shows the player's inventory on the screen.
  */
 void displayInventory() {
     char invBuffer[128];
     sys->gotoxy(60, 0);
     sys->print("Inventory:");
     sys->gotoxy(60, 1);
     sys->snprintf(invBuffer, sizeof(invBuffer), "Health Potions: %d", healthPotions);
     sys->print(invBuffer);
     sys->gotoxy(60, 2);
     sys->snprintf(invBuffer, sizeof(invBuffer), "Stamina Potions: %d", staminaPotions);
     sys->print(invBuffer);
     sys->gotoxy(60, 3);
     sys->snprintf(invBuffer, sizeof(invBuffer), "Elixirs: %d", elixirs);
     sys->print(invBuffer);
     sys->gotoxy(60, 4);
     sys->snprintf(invBuffer, sizeof(invBuffer), "Arrows: %d", arrows);
     sys->print(invBuffer);
 }
 
 /*
  * navigateMenu: displays a menu using an array of string options and returns the selected index.
  */
 int navigateMenu(char **options, int numOptions) {
     int selection = 0;
     int key;
     int menuStartLine = 20;
     sys->clear_screen();
     while (1) {
         displayStatus();
         displayInventory();
         displayText();
         sys->gotoxy(0, menuStartLine - 1);
         for (int i = 0; i < MAX_WIDTH; i++)
             sys->print("-");
         for (int i = 0; i < numOptions; i++) {
             int x = 5 + (i % 2) * 40;
             int y = menuStartLine + (i / 2);
             sys->gotoxy(x, y);
             if (i == selection)
                 sys->print("-> ");
             else
                 sys->print("   ");
             sys->print(options[i]);
             sys->update_cursor();
         }
         key = sys->getch();
         if (key == UP_ARROW)
             selection = (selection - 2 + numOptions) % numOptions;
         if (key == DOWN_ARROW)
             selection = (selection + 2) % numOptions;
         if (key == LEFT_ARROW)
             selection = (selection - 1 + numOptions) % numOptions;
         if (key == RIGHT_ARROW)
             selection = (selection + 1) % numOptions;
         if (key == '\n') {
             sys->clear_screen();
             return selection;
         }
     }
 }
 
 /*
  * useItemMenu: allows the player to select and use an item from their inventory.
  */
 void useItemMenu() {
     char *itemOptions[] = {"Use Health Potion", "Use Stamina Potion", "Use Elixir", "Cancel"};
     int selection = navigateMenu(itemOptions, 4);
     int heal, stamina;
     if (selection == 0) {
         if (healthPotions > 0) {
             healthPotions--;
             heal = sys->rand(30) + 20;
             playerHealth += heal;
             if (playerHealth > maxHealth + permanentHealthBonus)
                 playerHealth = maxHealth + permanentHealthBonus;
             flash_heal();
             sys->snprintf(msg, sizeof(msg), "Used health potion! Healed %d health.", heal);
             updateText(msg);
         } else {
             updateText("No health potions left!");
         }
     } else if (selection == 1) {
         if (staminaPotions > 0) {
             staminaPotions--;
             stamina = sys->rand(20) + 15;
             playerStamina += stamina;
             if (playerStamina > maxStamina + permanentStaminaBonus)
                 playerStamina = maxStamina + permanentStaminaBonus;
             flash_heal();
             sys->snprintf(msg, sizeof(msg), "Used stamina potion! Restored %d stamina.", stamina);
             updateText(msg);
         } else {
             updateText("No stamina potions left!");
         }
     } else if (selection == 2) {
         if (elixirs > 0) {
             elixirs--;
             playerHealth = maxHealth + permanentHealthBonus;
             playerStamina = maxStamina + permanentStaminaBonus;
             flash_heal();
             updateText("Used elixir! Fully restored health and stamina.");
         } else {
             updateText("No elixirs left!");
         }
     } else {
         updateText("Canceled item menu.");
     }
     displayText();    
 }
 
 /*
  * endGame: handles game termination, prints play duration, and resets the system.
  */
 void endGame() {
     sys->clear_screen();
     // record end time and calculate play duration
     uint8_t rtc_end_hour, rtc_end_minute, rtc_end_second, rtc_day, rtc_month, rtc_year;
     sys->read_rtc_datetime(&rtc_end_hour, &rtc_end_minute, &rtc_end_second, &rtc_day, &rtc_month, &rtc_year);
     int startTotal = rtc_start_hour * 3600 + rtc_start_minute * 60 + rtc_start_second;
     int endTotal = rtc_end_hour * 3600 + rtc_end_minute * 60 + rtc_end_second;
     int playTime = endTotal - startTotal;
     int hours = playTime / 3600;
     int minutes = (playTime % 3600) / 60;
     int seconds = playTime % 60;
     sys->print("\nGame Over! Thanks for playing.\n\n");
     char timeBuffer[64];
     sys->snprintf(timeBuffer, sizeof(timeBuffer), "Time played: %d:%d:%d\n", hours, minutes, seconds);
     sys->print(timeBuffer);
     sys->print("\nMade by Xander Gomez (tuvalutortorture)\n\nPress any key to exit...\n");
     sys->getch();
     sys->reset();
 }
 
 /*
  * encounterShop: handles shop encounters with options to buy items, rest, or leave.
  */
 void encounterShop() {
     // set shop color: white on blue
     change_color(15, 1);
     char *shopOptions[] = {"Buy Items", "Rest", "Buy Ticket", "Leave"};
     int healthPotionCost = sys->rand(30) + 40;
     int staminaPotionCost = sys->rand(10) + 25;
     int elixirCost = sys->rand(50) + 180;
     int arrowCost = sys->rand(20) + 25;
     int ticketCost = 50;
     int selection;
     int purchaseSelection;
     int shopRestUsed = 0;
   
     showText("You encounter a shopkeeper!");
     while (1) {
         selection = navigateMenu(shopOptions, 4);
         if (selection == 0) {
             char *purchaseOptions[] = {"Buy Health Potion", "Buy Stamina Potion", "Buy Elixir", "Buy Arrows", "Cancel"};
             showText("What will ye buy?");
             showText("Health pot: %dg, Stamina pot: %dg, Elixir: %dg, Arrows: %dg.",
                      healthPotionCost, staminaPotionCost, elixirCost, arrowCost);
             purchaseSelection = navigateMenu(purchaseOptions, 5);
             if (purchaseSelection == 0 && gold >= healthPotionCost) {
                 healthPotions++;
                 gold -= healthPotionCost;
                 updateText("Ye bought a health potion.");
             } else if (purchaseSelection == 1 && gold >= staminaPotionCost) {
                 staminaPotions++;
                 gold -= staminaPotionCost;
                 updateText("Ye bought a stamina potion.");
             } else if (purchaseSelection == 2 && gold >= elixirCost) {
                 elixirs++;
                 gold -= elixirCost;
                 updateText("Ye bought an elixir.");
             } else if (purchaseSelection == 3 && gold >= arrowCost) {
                 arrows += 5;
                 gold -= arrowCost;
                 updateText("Ye bought a bundle o' arrows.");
             } else if (purchaseSelection == 4) {
                 updateText("Ye canceled yer purchase.");
             } else {
                 updateText("Ye dinnae have enough gold for that!");
             }
         } else if (selection == 1) { // Rest option
             if (shopRestUsed < shopRestLimit) {
                 playerStamina += 20;
                 if (playerStamina > maxStamina + permanentStaminaBonus)
                     playerStamina = maxStamina + permanentStaminaBonus;
                 playerHealth += 10;
                 if (playerHealth > maxHealth + permanentHealthBonus)
                     playerHealth = maxHealth + permanentHealthBonus;
                 shopRestUsed++;
                 updateText("Ye rested in the shop and feel rejuvinated.");
             } else {
                 updateText("Ye've used all yer free rests at this shop!");
             }
         } else if (selection == 2) { // Buy Ticket option
             if (gold >= ticketCost) {
                 shopRestLimit++;
                 gold -= ticketCost;
                 updateText("Ye purchased a rest ticket. Shop rests increased permanently!");
             } else {
                 updateText("Not enough gold to buy a ticket, ye starving numpty!");
             }
         } else {
             updateText("Ye left the shop.");
             break;
         }
         displayText();
     }
     change_color(2, 15); // reset shop color to default green on white
 }
 
 /*
  * encounterShrine: handles shrine encounters where the player can sacrifice resources for upgrades.
  */
 void encounterShrine() {
     char *sacrificeOptions[] = {"Sacrifice Gold (100)", "Sacrifice an Item", "Sacrifice Health", "Sacrifice Stamina", "Return"};
     int selection, upgradeSelection;
     showText("Ye stumble upon a mysterious shrine. The air is thick wi' ancient dread.");
     selection = navigateMenu(sacrificeOptions, 5);
   
     int sacrificeMade = 0;
   
     if (selection == 0) { // Sacrifice Gold
         if (gold >= 100) {
             gold -= 100;
             sacrificeMade = 1;
             updateText("100 gold sacrificed.");
         } else {
             updateText("Ye dinnae have enough gold to sacrifice!");
         }
     } else if (selection == 1) { // Sacrifice an Item
         if (healthPotions > 2) {
             healthPotions -= 2;
             sacrificeMade = 1;
             updateText("A set of health potions has been sacrificed.");
         } else if (staminaPotions > 4) {
             staminaPotions -= 4;
             sacrificeMade = 1;
             updateText("A set of stamina potions has been sacrificed.");
         } else if (elixirs > 0) {
             elixirs--;
             sacrificeMade = 1;
             updateText("An elixir has been sacrificed.");
         } else {
             updateText("Ye have no items to sacrifice!");
         }
     } else if (selection == 2) { // Sacrifice 10 Max Health
         if (maxHealth + permanentHealthBonus > 10) {
             permanentHealthBonus -= 10;
             sacrificeMade = 1;
             updateText("Ye sacrificed 10 max health.");
         } else {
             updateText("Ye dinnae have enough max health to sacrifice!");
         }
     } else if (selection == 3) { // Sacrifice 10 Max Stamina
         if (maxStamina + permanentStaminaBonus > 10) {
             permanentStaminaBonus -= 10;
             sacrificeMade = 1;
             updateText("Ye sacrificed 10 max stamina.");
         } else {
             updateText("Ye dinnae have enough max stamina to sacrifice!");
         }
     } else {
         updateText("Ye chose to back off from the ominous shrine.");
     }
     displayText();
     if (!sacrificeMade) {
         return;
     }
     // Now present upgrade options
     char *upgradeOptions[] = {"Increase Max Health (+10)", "Increase Damage (+3)", "Increase Max Stamina (+10)"};
     showText("Choose your boon...");
     upgradeSelection = navigateMenu(upgradeOptions, 3);
   
     if (upgradeSelection == 0) {
         permanentHealthBonus += 10;
         updateText("Ye've increased yer max health permanently by 10.");
     } else if (upgradeSelection == 1) {
         damageBonus += 3;
         updateText("Ye've increased yer damage permanently by 3.");
     } else if (upgradeSelection == 2) {
         permanentStaminaBonus += 10;
         updateText("Ye've increased yer max stamina permanently by 10.");
     }
     displayText();
 }
 
 /*
  * regenerateStamina: restores 10 stamina, up to the maximum.
  */
 void regenerateStamina() {
     playerStamina += 10;
     if (playerStamina > maxStamina + permanentStaminaBonus)
         playerStamina = maxStamina + permanentStaminaBonus;
 }
 
 /*
  * runAway: attempts to escape combat, possibly incurring damage if unsuccessful.
  */
 void runAway() {
     int damage;
     if (playerStamina < 10) {
         escapeFailed = 1;
         sys->snprintf(msg, sizeof(msg), "Not enough stamina to run away!");
     } else if (sys->rand(10) < 2) {
         escapeFailed = 1;
         damage = sys->rand(11) + 5;
         playerHealth -= damage;
         flash_damage();
         sys->snprintf(msg, sizeof(msg), "Failed to escape! Took %d damage.", damage);
     } else {
         escapeFailed = 0;
         sys->snprintf(msg, sizeof(msg), "Successfully escaped! Lost 10 stamina. Lost %d gold.", gold / 5);
         playerStamina -= 10;
         if (playerStamina < 0)
             playerStamina = 0;
         gold = gold - (gold / 5);
         change_color(2, 15);
     }
     showText(msg);
 }
 
 /*
  * attackMenu: allows the player to select an attack and computes damage dealt.
  */
 void attackMenu(int *monsterHealth, int *retaliate) {
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
             playerStamina -= cost[selection];
             if (selection == 3)
                 arrows--;
             damage = sys->rand(range[selection]) + baseDamage[selection] + damageBonus;
             miss = (sys->rand(10) == 0);
             crit = (sys->rand(10) == 0);
             if (miss) {
                 updateText("Ye completely missed!");
                 damage = 0;
             } else if (crit) {
                 damage *= 2;
                 char temp[128];
                 sys->snprintf(temp, sizeof(temp), "%s scored a critical hit and dealt %d damage!", attackOptions[selection], damage);
                 updateText(temp);
             } else {
                 char temp[128];
                 sys->snprintf(temp, sizeof(temp), "%s dealt %d damage!", attackOptions[selection], damage);
                 updateText(temp);
             }
             *monsterHealth -= damage;
         }
     }
     displayText();
 }
 
 /*
  * encounterEnemy: handles combat against an enemy (or boss).
  */
 void encounterEnemy(int isBoss) {
     int retaliate = 1, defended = 0, action;
     int baseHealth, monsterHealth;
     int isMimic = 0;
 
     if (isBoss) {
         char *bosses[] = {"Ironclad King", "Ravager of Souls", "Stormbringer", "The Herald of Despair", "Voidcaller"};
         int bossCount = 5;
         baseHealth = sys->rand(120) + 150;
         monsterHealth = baseHealth;
         sys->set_color(0, 4); // boss mode: black on red
         sys->repaint_screen(0, 4);
         current_fg_color = 0;
         current_bg_color = 4;
         sys->snprintf(msg, sizeof(msg), "Ye encounter the %s!", bosses[sys->rand(bossCount) % bossCount]);
     } else {
         const char *monsterName;
         if (sys->rand(10) < 2) {  // 20% chance for mimic
             isMimic = 1;
             monsterName = "Mimic";
             baseHealth = sys->rand(20) + 100 + (floor * 10);
             change_color(14, 0); // yellow on black for mimic
         } else {
             const char *normalMonsters[] = {"Goblin", "Troll", "Skeleton", "Wizard", "Zombie-sized Chicken", "Bandit", "Rabid Dog"};
             int normalCount = 7;
             int idx = sys->rand(normalCount) % normalCount;
             monsterName = normalMonsters[idx];
             baseHealth = sys->rand(60) + 30 + (floor * 5);
             change_color(15, 0); // white on black for normal monsters
         }
         monsterHealth = baseHealth;
         sys->snprintf(msg, sizeof(msg), "A wild %s appeared!", monsterName);
     }
     showText(msg);
     while (monsterHealth > 0 && playerHealth > 0) {
         char *combatOptions[] = {"Attack Menu", "Item Menu", "Defend", "Run", "Scan"};
         displayStatus();
         displayInventory();
         displayText();
         action = navigateMenu(combatOptions, 5);
         retaliate = 1;
         if (action == 0) {
             defended = 0;
             attackMenu(&monsterHealth, &retaliate);
         } else if (action == 1) {
             useItemMenu();
             retaliate = 0;
         } else if (action == 2) {
             defended = 1;
             playerStamina += 5;
             if (playerStamina > maxStamina + permanentStaminaBonus)
                 playerStamina = maxStamina + permanentStaminaBonus;
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
             sys->snprintf(msg, sizeof(msg), "Enemy has %d / %d health.", monsterHealth, baseHealth);
             retaliate = 0;
             updateText(msg);
         }
         if (monsterHealth <= 0) {
             int loot = sys->rand(20) + 10;
             if (isMimic) {
                 loot *= 2;
                 healthPotions++;
                 sys->snprintf(msg, sizeof(msg), "Ye defeated the mimic! Gained %d gold and a health potion.", loot);
             } else if (isBoss) {
                 sys->snprintf(msg, sizeof(msg), "Congrats! Ye slayed the boss! Health, stamina, and dmg increased!");
                 loot *= 3;
                 healthPotions += 1;
                 floor++;
                 permanentHealthBonus += 10;
                 permanentStaminaBonus += 10;
                 damageBonus += 3;
                 enemiesDefeated = 0;
                 playerHealth = maxHealth + permanentHealthBonus;
                 playerStamina = maxStamina + permanentStaminaBonus;
             } else {
                 sys->snprintf(msg, sizeof(msg), "Ye defeated the monster! Gained %d gold.", loot);
             }
             gold += loot;
             enemiesDefeated++;
             showText(msg);
             if (enemiesDefeated == 10) {
                 showText("The air grows thick, you feel a sinister presence...");
             }
         }
         if (retaliate && monsterHealth > 0) {
             int baseDmg = sys->rand(isBoss ? 15 : 8) + (isBoss ? 10 : 3);
             float dmg = baseDmg;
             if (sys->rand(10) == 0) {
                 dmg *= 2;
                 sys->snprintf(msg, sizeof(msg), "The monster scored a critical hit and dealt %d damage!", (int)dmg);
             } else {
                 if (sys->rand(10) == 0) {
                     sys->snprintf(msg, sizeof(msg), "The monster completely missed!");
                     dmg = 0;
                 } else if (defended == 1) {
                     sys->snprintf(msg, sizeof(msg), "The monster attacks, but ye block some of it! Ye take %d damage.", (int)dmg / 2);
                 } else {
                     sys->snprintf(msg, sizeof(msg), "The monster retaliates and deals %d damage!", (int)dmg);
                 }
             }
             playerHealth -= (int)dmg;
             if (dmg >= 1)
                 flash_damage();
             showText(msg);
             if (playerHealth <= 0) {
                 is_playing = 0;
                 endGame();
             }
         }
     }
     change_color(2, 15); // reset color to default
 }
 
 /*
  * exploreDungeon: the main game loop that handles exploring, encountering enemies,
  * shops, shrines, and rests.
  */
 void exploreDungeon() {
     char *dungeonOptions[] = {"Delve further", "Rest", "Item", "Quit"};
     int action;
     while (1) {
         displayStatus();
         displayInventory();
         displayText();
   
         if (enemiesDefeated == 10) {
             encounterShop();
         }
   
         if (enemiesDefeated >= 10) {
             encounterEnemy(1);
         }
   
         if (sys->rand(20) == 0) {
             encounterShrine();
         }
   
         if (floor == 6) {
             showText("Congratulations! You have cleared all the dungeon floors.");
             endGame();
         }
   
         action = navigateMenu(dungeonOptions, 4);
         if (action == 3) {
             is_playing = 1;
             endGame();
         }
   
         if (action == 2) {
             useItemMenu();
         }
         steps++;
         if (action == 1) {
             regenerateStamina();
             playerHealth += 5;
             if (playerHealth > maxHealth + permanentHealthBonus)
                 playerHealth = maxHealth + permanentHealthBonus;
             if (playerStamina > maxStamina + permanentStaminaBonus)
                 playerStamina = maxStamina + permanentStaminaBonus;
             updateText("Ye rested, regaining 10 stamina and 5 health.");
         } else {
             switch (sys->rand(4)) {
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
     }
     sys->reset();
 }
 
 /*
  * startingItem: presents the player with a starting item selection.
  */
 void startingItem() {
     showText("Pick a starting item...");
     char *startingOptions[] = {"Stamina potion", "Health potion", "Bundle of arrows", "Rest ticket", "Nothing"};
     int selection = navigateMenu(startingOptions, 5);
     if (selection == 0) {
         staminaPotions++;
     } else if (selection == 1) {
         healthPotions++;
     } else if (selection == 2) {
         arrows += 5;
     } else if (selection == 3) {
         shopRestLimit++;
     }
     showText("Well, shall we begin?");
 }
 
 /*
  * start_dungeon: sets up the game and starts the dungeon exploration loop.
  */
 int start_dungeon() {
     sys->srand(sys->extra_rand());
     sys->disable_cursor();
     sys->clear_screen();
     uint8_t dummyDay, dummyMonth, dummyYear;
     sys->read_rtc_datetime(&rtc_start_hour, &rtc_start_minute, &rtc_start_second, &dummyDay, &dummyMonth, &dummyYear);
     if (is_playing == 0) {
         dungeon_reset();
         showText("Welcome to the Dungeon...");
         startingItem();
     }
     is_playing = 1;
     displayStatus();
     displayInventory();
     exploreDungeon();
     return 0;
 }
 