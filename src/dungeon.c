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
 #include "time.h"
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
 int maxHealth = 100;    // base max health
 int maxStamina = 50;    // base max stamina
 int damageBonus = 0;    // permanent bonus added to all attacks
 int permanentHealthBonus = 0;  // permanent boost from shrines
 int permanentStaminaBonus = 0; // permanent boost from shrines
 
 // shop rest mechanic: base 1 free rest per shop, can be increased with tickets
 int shopRestLimit = 1;
 
 char currentText[1024];
 char msg[1024];
 
 int current_fg_color = 2; // default text color (green)
 int current_bg_color = 15; // default background (white)
 
 int is_playing = 0; // flag to check if in playing state
 
 // RTC start time globals for playtime tracking
 uint8_t rtc_start_hour = 0, rtc_start_minute = 0, rtc_start_second = 0;
 
 /*
  * gotoxy has been removed from here – it's now defined in console.c,
  * so all calls to gotoxy are assumed to reference the external implementation.
  */

 void change_color(uint8_t fg, uint8_t bg) {
     current_fg_color = fg;
     current_bg_color = bg;
     set_color(fg, bg);
     repaint_screen(fg, bg);
 }
 
 void dungeon_reset() {
     is_playing = 0; // reset play state
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
 
 // new functions for sfx and color flashes
 void flash_damage() {
     set_color(4, 4); // red on black for damage flash
     repaint_screen(4, 4);
     beep(300, 200);
     delay_ms(500);
     set_color(current_fg_color, current_bg_color);
     repaint_screen(current_fg_color, current_bg_color);
 }
 
 void flash_heal() {
     set_color(2, 2); // green on black for healing flash
     repaint_screen(2, 2);
     beep(1000, 100);
     delay_ms(500);
     set_color(current_fg_color, current_bg_color);
     repaint_screen(current_fg_color, current_bg_color);
 }
 
 // updateText: format text into currentText buffer
 void updateText(const char *format, ...) {
     va_list args;
     va_start(args, format);
     vsnprintf(currentText, sizeof(currentText), format, args);
     va_end(args);
     textDisplayed = 0;
 }
 
 // displayText: show the currentText centered at row 15 with character delay
 void displayText() {
     for (int i = 15; i < MAX_HEIGHT; i++) {
         gotoxy(0, i);
         for (int j = 0; j < MAX_WIDTH; j++) {
             print(" ");
         }
     }
     gotoxy(0, 15);
     static char lastDisplayedText[1024] = "";
     int yOffset = 15;
     int lineStart = 0;
     int len = strlen(currentText);
     char buffer[MAX_WIDTH + 1];
 
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
                 delay_ms(1500);
             }
             lineStart = i + 1;
             while (currentText[lineStart] == ' ' && currentText[lineStart] != '\0')
                 lineStart++;
         }
     }
     strcpy(lastDisplayedText, currentText);
     textDisplayed = 1;
     getch(); // wait for user to press a key
 }
 
 void showText(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(currentText, sizeof(currentText), format, args);
    va_end(args);
    textDisplayed = 0;
    displayText();
 }

 // displayStatus: shows player info at top-left
 void displayStatus() {
     char statusBuffer[128];
     int healthBars = playerHealth / 10;
     int staminaBars = playerStamina / 10;
 
     gotoxy(0, 0);
     snprintf(statusBuffer, sizeof(statusBuffer), "Health: [");
     print(statusBuffer);
     for (int i = 0; i < (maxHealth + permanentHealthBonus) / 10; i++) {
         print(i < healthBars ? "|" : " ");
     }
     print("]");
 
     gotoxy(0, 1);
     snprintf(statusBuffer, sizeof(statusBuffer), "Stamina: [");
     print(statusBuffer);
     for (int i = 0; i < (maxStamina + permanentStaminaBonus) / 10; i++) {
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
             update_cursor(); // sync the cursor position
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
 
 void useItemMenu() {
    char *itemOptions[] = {"Use Health Potion", "Use Stamina Potion", "Use Elixir", "Cancel"};
    int selection = navigateMenu(itemOptions, 4);
    int heal, stamina;
    if (selection == 0) {
        if (healthPotions > 0) {
            healthPotions--;
            heal = rand(30) + 20;
            playerHealth += heal;
            if (playerHealth > maxHealth + permanentHealthBonus)
                playerHealth = maxHealth + permanentHealthBonus;
            flash_heal();
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
            if (playerStamina > maxStamina + permanentStaminaBonus)
                playerStamina = maxStamina + permanentStaminaBonus;
            flash_heal();
            snprintf(msg, sizeof(msg), "Used stamina potion! Restored %d stamina.", stamina);
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

void endGame() {
    clear_screen();
    // record end time and calculate play duration
    uint8_t rtc_end_hour, rtc_end_minute, rtc_end_second, rtc_day, rtc_month, rtc_year;
    read_rtc_datetime(&rtc_end_hour, &rtc_end_minute, &rtc_end_second, &rtc_day, &rtc_month, &rtc_year);
    int startTotal = rtc_start_hour * 3600 + rtc_start_minute * 60 + rtc_start_second;
    int endTotal = rtc_end_hour * 3600 + rtc_end_minute * 60 + rtc_end_second;
    int playTime = endTotal - startTotal;
    int hours = playTime / 3600;
    int minutes = (playTime % 3600) / 60;
    int seconds = playTime % 60;
    print("\nGame Over! Thanks for playing.\n\n");
    char timeBuffer[64];
    snprintf(timeBuffer, sizeof(timeBuffer), "Time played: %d:%d:%d\n", hours, minutes, seconds);
    print(timeBuffer);
    print("\nMade by Xander Gomez (tuvalutorture)\n\nPress any key to exit...\n");
    getch();
    reset();
}

 // encounterShop: shop encounter with new rest and ticket mechanics, and raised prices
 void encounterShop() {
     // set shop color: white on blue
     change_color(15, 1);
 
     char *shopOptions[] = {"Buy Items", "Rest", "Buy Ticket", "Leave"};
     int healthPotionCost = rand(30) + 40;
     int staminaPotionCost = rand(10) + 25;
     int elixirCost = rand(50) + 180;
     int arrowCost = rand(20) + 25;
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
                 // rest recovers 20 stamina and 10 health
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
     // reset shop color to default green on white
     change_color(2, 15);
 }
 
 // encounterShrine: rare shrine where ye sacrifice for permanent upgrades
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
 
 // regenerateStamina: restore 10 stamina up to max
 void regenerateStamina() {
     playerStamina += 10;
     if (playerStamina > maxStamina + permanentStaminaBonus)
         playerStamina = maxStamina + permanentStaminaBonus;
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
         flash_damage();
         snprintf(msg, sizeof(msg), "Failed to escape! Took %d damage.", damage);
     } else {
         escapeFailed = 0;
         snprintf(msg, sizeof(msg), "Successfully escaped! Lost 10 stamina. Lost %d gold.", gold / 5);
         playerStamina -= 10;
         if (playerStamina < 0)
             playerStamina = 0;
         gold = gold - (gold / 5);
         change_color(2, 15); // reset color to default
     }
     showText(msg);
 }
 
 // attackMenu: let the player choose an attack and calculate damage
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
             playerStamina -= cost[selection];
             if (selection == 3)
                 arrows--;
             damage = rand(range[selection]) + baseDamage[selection] + damageBonus;
             miss = (rand(10) == 0);
             crit = (rand(10) == 0);
             if (miss) {
                 updateText("Ye completely missed!");
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
 }
 
 // encounterEnemy: handle combat; includes special mimic logic & new colors
 void encounterEnemy(int isBoss) {
     int retaliate = 1, defended = 0, action;
     int baseHealth, monsterHealth;
     int isMimic = 0;

     if (isBoss) {
         char *bosses[] = {"Ironclad King", "Ravager of Souls", "Stormbringer", "The Herald of Despair", "Voidcaller"};
         int bossCount = 5;
         baseHealth = rand(120) + 150;
         monsterHealth = baseHealth;
         set_color(0, 4); // boss mode: black on red
         repaint_screen(0, 4);
         current_fg_color = 0;
         current_bg_color = 4;
         snprintf(msg, sizeof(msg), "Ye encounter the %s!", bosses[rand(bossCount)]);
     } else {
         const char *monsterName;
         if (rand(10) < 2) {  // 20% chance for mimic
             isMimic = 1;
             monsterName = "Mimic";
             baseHealth = rand(20) + 100 + (floor * 10);  // tougher mimic
             change_color(14, 0); // yellow on black for mimic
         } else {
             const char *normalMonsters[] = {"Goblin", "Troll", "Skeleton", "Wizard", "Zombie-sized Chicken", "Bandit", "Rabid Dog"};
             int normalCount = 7;
             int idx = rand(normalCount);
             monsterName = normalMonsters[idx];
             baseHealth = rand(60) + 30 + (floor * 5); // tougher monsters on higher floors
             change_color(15, 0); // white on black for normal monsters
         }
         monsterHealth = baseHealth;
         snprintf(msg, sizeof(msg), "A wild %s appeared!", monsterName);
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
             attackMenu(&monsterHealth, &retaliate, isBoss, baseHealth);
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
             snprintf(msg, sizeof(msg), "Enemy has %d / %d health.", monsterHealth, baseHealth);
             retaliate = 0;
             updateText(msg);
         }
         if (monsterHealth <= 0) {
             int loot = rand(20) + 10;
             if (isMimic) {
                 loot *= 2;  // double gold for mimics
                 healthPotions++;  // mimic drops an extra health potion
                 snprintf(msg, sizeof(msg), "Ye defeated the mimic! Gained %d gold and a health potion.", loot);
             } else if (isBoss) {
                 snprintf(msg, sizeof(msg), "Congrats! Ye slayed the boss! Health, stamina, and dmg increased!");
                 loot *= 3;  // triple gold for bosses
                 healthPotions += 1;  // bosses drop health potions
                 floor++;
                 permanentHealthBonus += 10;
                 permanentStaminaBonus += 10;
                 damageBonus += 3;
                 enemiesDefeated = 0;
                 playerHealth = maxHealth + permanentHealthBonus;
                 playerStamina = maxStamina + permanentStaminaBonus;
             } else {
                 snprintf(msg, sizeof(msg), "Ye defeated the monster! Gained %d gold.", loot);
             }
             gold += loot;
             enemiesDefeated++;
             showText(msg);
             if (enemiesDefeated == 10) {
                showText("The air grows thick, you feel a sinister presence...");
            } 
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
                     snprintf(msg, sizeof(msg), "The monster attacks, but ye block some of it! Ye take %d damage.", (int)damage / 2);
                 } else {
                     snprintf(msg, sizeof(msg), "The monster retaliates and deals %d damage!", (int)damage);
                 }
             }
             playerHealth -= (int)damage;
             if (damage >= 1)
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
 
 void exploreDungeon() {
     char *dungeonOptions[] = {"Delve further", "Rest", "Item", "Quit"};
     int action;
     while (1) {
         displayStatus();
         displayInventory();
         displayText();
 
         // Guaranteed shop before boss if enemies defeated reaches 10
         if (enemiesDefeated == 10) {
             encounterShop();
         }
 
         // Boss encounter every 10 enemies
         if (enemiesDefeated >= 10) {
             encounterEnemy(1);
         }
 
         // Random rare shrine encounter
         if (rand(20) == 0) {
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
         if (action == 1) {  // rest outside shop
             regenerateStamina();
             playerHealth += 5;
             if (playerHealth > maxHealth + permanentHealthBonus)
                 playerHealth = maxHealth + permanentHealthBonus;
             if (playerStamina > maxStamina + permanentStaminaBonus)
                 playerStamina = maxStamina + permanentStaminaBonus;
             updateText("Ye rested, regaining 10 stamina and 5 health.");
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
     }
     reset();
 }
 
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
 
 int start_dungeon() {
     srand(extra_rand());
     disable_cursor();
     clear_screen();
     // record RTC start time
     uint8_t dummyDay, dummyMonth, dummyYear;
     read_rtc_datetime(&rtc_start_hour, &rtc_start_minute, &rtc_start_second, &dummyDay, &dummyMonth, &dummyYear);
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