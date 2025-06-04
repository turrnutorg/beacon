/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * Written by Xander Gomez (tuvalutortorture)
 * 
 * dungeon.c - Ported dungeon game for Beacon with SFX and color effects
 * Just a little tech demo I wrote for Beacon.
 *
 */

 #include "os.h"
 #include "console.h"
 #include "command.h"
 #include "screen.h"
 #include "speaker.h"
 #include "keyboard.h"
 #include "time.h"
 #include "string.h"
 #include <stdarg.h>
 #include <stdint.h>
 
 #define MAX_WIDTH 80
 #define MAX_HEIGHT 25
 
 #define UP_ARROW    -1
 #define DOWN_ARROW  -2
 #define LEFT_ARROW  -3
 #define RIGHT_ARROW -4
 
 // NEW FEATURE: Equipment definitions & battle position
 #define MAX_EQUIP 10

 extern void main_menu_loop(void);
 
 typedef struct {
     int uses;  // remaining uses
 } Equipment;
 
 // perishable armor: gives 1 dmg reduction per hit (15 uses)
 Equipment armorEquip[MAX_EQUIP];
 int armorCount = 0;
 int armorEquipped = 0;
 
 // sharpening tools: gives +2 damage per attack (15 uses)
 Equipment sharpTools[MAX_EQUIP];
 int sharpCount = 0;
 int sharpsEquipped = 0;
 
 // shields: lasts 10 hits; only ONE shield allowed.
 Equipment shields[MAX_EQUIP];
 int shieldCount = 0;
 
 // Manual activation bonus variables (active until used)
 int activeSharpBonus = 0;  // bonus damage for next attack when activated
 int activeArmorBonus = 0;  // damage reduction for next hit when activated
 
 // battle positioning: 0 = front row, 1 = back row
 int battlePosition = 0; // default front row
 
 /* 
  * GLOBAL GAME STATE
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
 int dungeon_floor = 1;
 int textDisplayed = 0;
 int playerHealth = 100;
 int playerStamina = 50;
 int maxHealth = 100;
 int maxStamina = 50;
 int damageBonus = 0;
 int permanentHealthBonus = 0;
 int permanentStaminaBonus = 0;

 // special case for arrow fired from back row
 int arrowFiredFromBackRow = 0;
 
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
  * change_color: sets the foreground and background colors and repaints the screen.
  */
 void change_color(uint8_t fg, uint8_t bg) {
     current_fg_color = fg;
     current_bg_color = bg;
     set_color(fg, bg);
     repaint_screen(fg, bg);
 }
 
 /*
  * dungeon_reset: resets all global game variables.
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
     dungeon_floor = 1;
     textDisplayed = 0;
     playerHealth = 100;
     playerStamina = 50;
     maxHealth = 100;
     maxStamina = 50;
     damageBonus = 0;
     permanentHealthBonus = 0;
     permanentStaminaBonus = 0;
     shopRestLimit = 1;
 
     armorCount = 0;
     sharpCount = 0;
     shieldCount = 0;
     battlePosition = 0;
     activeSharpBonus = 0;
     activeArmorBonus = 0;
 }
 
 void regenerateStamina() {
    playerStamina += 10;
    if (playerStamina > maxStamina + permanentStaminaBonus)
        playerStamina = maxStamina + permanentStaminaBonus;
}


 /*
  * flash_damage: flashes screen red.
  */
 void flash_damage() {
     set_color(4, 4);
     repaint_screen(4, 4);
     beep(300, 200);
     delay_ms(25);  // reduced delay for quicker feedback
     set_color(current_fg_color, current_bg_color);
     repaint_screen(current_fg_color, current_bg_color);
 }
 
 /*
  * flash_heal: flashes screen green.
  */
 void flash_heal() {
     set_color(2, 2);
     repaint_screen(2, 2);
     beep(1000, 100);
     delay_ms(25);
     set_color(current_fg_color, current_bg_color);
     repaint_screen(current_fg_color, current_bg_color);
 }
 
 /*
  * updateText: formats text into currentText buffer.
  */
 void updateText(const char *format, ...) {
     va_list args;
     va_start(args, format);
     vsnprintf(currentText, sizeof(currentText), format, args);
     va_end(args);
     textDisplayed = 0;
 }
 
 /*
  * displayText: clears portion of screen and displays currentText.
  * Reduced per-character delay for clarity.
  */
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
             // reduced delay per character for better responsiveness
             for (int j = 0; j < lineLength; j++) {
                 char ch[2] = { buffer[j], '\0' };
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
     getch();
 }
 
 /*
  * showText: sets currentText and calls displayText.
  */
 void showText(const char *format, ...) {
     va_list args;
     va_start(args, format);
     vsnprintf(currentText, sizeof(currentText), format, args);
     va_end(args);
     textDisplayed = 0;
     displayText();
 }
 
 /*
  * displayStatus: shows player's status and current battle position.
  */
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
     snprintf(statusBuffer, sizeof(statusBuffer), "Floor: %d", dungeon_floor);
     print(statusBuffer);
 
     gotoxy(0, 6);
     snprintf(statusBuffer, sizeof(statusBuffer), "Position: %s", (battlePosition == 0 ? "Front" : "Back"));
     print(statusBuffer);
 }
 
 /*
  * displayInventory: shows inventory including new equipment.
  */
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
     gotoxy(60, 5);
     snprintf(invBuffer, sizeof(invBuffer), "Armor Plates: %d", armorCount);
     print(invBuffer);
     gotoxy(60, 6);
     snprintf(invBuffer, sizeof(invBuffer), "Sharpening Tools: %d", sharpCount);
     print(invBuffer);
     gotoxy(60, 7);
     snprintf(invBuffer, sizeof(invBuffer), "Shield: %s", (shieldCount > 0 ? "Equipped" : "None"));
     print(invBuffer);
 }
 
 /*
  * navigateMenu: displays a menu from an array and returns selection.
  */
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
             update_cursor();
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
 
 /*
  * new functions to manually use equipment:
  */
 void useSharpeningTool() {
     if (sharpCount > 0) {
         activeSharpBonus += 2;
         sharpsEquipped++;
         // consume one use from the first available tool
         sharpTools[0].uses--;
         if (sharpTools[0].uses <= 0) {
             print("Yer sharpening tool dulls and breaks!\n");
             // shift remaining tools up
             for (int i = 0; i < sharpCount - 1; i++) {
                 sharpTools[i] = sharpTools[i+1];
             }
             sharpCount--;
             activeSharpBonus--; // reset bonus if no tools left
             sharpsEquipped--; // reset equipped count
         }
         updateText("Ye sharpen yer blade, gaining +2 damage (15 hits).");
     } else {
         updateText("Nae sharpening tools available!");
     }
     displayText();
 }
 
 void useArmorPlate() {
     if (armorCount > 0) {
         armorEquipped++;
         activeArmorBonus += 1;
         armorEquip[0].uses--;
         if (armorEquip[0].uses <= 0) {
             print("Yer armor plate crumbles tae dust!\n");
             for (int i = 0; i < armorCount - 1; i++) {
                 armorEquip[i] = armorEquip[i+1];
             }
             armorCount--;
             activeArmorBonus--; // reset bonus if no armor left
             armorEquipped--; // reset equipped count
         }
         updateText("Ye strap on an extra plate of armor, gaining 1 damage reduction (15 hits).");
     } else {
         updateText("Nae armor plates available!");
     }
     displayText();
 }
 
 /*
  * Extended useItemMenu: now includes options to use sharpening tools and armor plates.
  */
 void useItemMenu() {
     char *itemOptions[] = {"Use Health Potion", "Use Stamina Potion", "Use Elixir", "Use Sharpening Tool", "Use Armor Plate", "Cancel"};
     int selection = navigateMenu(itemOptions, 6);
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
     } else if (selection == 3) { // Use sharpening tool manually
         if (sharpsEquipped < sharpCount)
            useSharpeningTool();
         else {
             updateText("Ye already used yer sharpening tool!");
         }
     } else if (selection == 4) { // Use armor plate manually
            if (armorEquipped < armorCount)
                useArmorPlate();
            else {
                updateText("Ye already used yer armor plate!");
            }
     } else {
         updateText("Canceled item menu.");
     }
     displayText();    
 }
 
 /*
  * togglePosition: switches battle rows.
  */
 void togglePosition() {
     battlePosition = 1 - battlePosition;
     if (battlePosition == 0)
         updateText("Ye move tae the front row! Full power restored.");
     else
         updateText("Ye skid back tae the rear, but yer attacks feel weaker.");
     displayText();
 }

  /*
  * runAway: remains unchanged.
  */
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
        change_color(2, 15);
    }
    showText(msg);
}
 
 void endGame() {
    clear_screen();
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
    print("\nMade by Xander Gomez (tuvalutortorture)\n\nPress any key to exit...\n");
    getch();
    main_menu_loop(); // return to main menu
    dungeon_reset(); // reset game state
}


void attackMenu(int *monsterHealth, int *retaliate) {
    char *attackOptions[] = {
        "Normal Attack (10 stamina)", 
        "Heavy Attack (20 stamina)", 
        "Light Attack (free)", 
        "Fire Arrow (5 stamina)", 
        "Cancel", 
        "Toggle Position"
    };

    int selection = navigateMenu(attackOptions, 6);
    int damage = 0;

    if (selection == 5) {
        togglePosition();
        *retaliate = 0;
        return;
    }

    if (selection == 4) {
        *retaliate = 0;
        updateText("Attack canceled.");
        displayText();
        return;
    } else {
        int cost[] = {10, 20, 0, 5};
        int baseDamage[] = {10, 20, 5, 15};
        int range[] = {6, 11, 4, 16};

        if ((selection == 3 && (arrows <= 0 || playerStamina < cost[selection])) ||
            (playerStamina < cost[selection])) {
            updateText(selection == 3 ? "Not enough stamina or no arrows left!" : "Not enough stamina!");
            *retaliate = 0;
            displayText();
            return;
        }

        int arrowFired = 0;
        playerStamina -= cost[selection];

        if (selection == 3) {
            arrows--;
            arrowFired = 1;
            arrowFiredFromBackRow = 0;
        }

        damage = rand(range[selection]) + baseDamage[selection];

        // apply manual sharpening bonus if activated
        if (activeSharpBonus > 0) {
            damage += activeSharpBonus;
            updateText("Your sharpened blade slices with extra fury (+%d damage)!", activeSharpBonus);
        }

        // Adjust for back row
        if (battlePosition == 1)
            damage /= 2;

        if (battlePosition == 1 && arrowFired)
            arrowFiredFromBackRow = 1;
        else
            arrowFiredFromBackRow = 0;

        // Handle miss/crit chance
        int missRoll = rand(100);
        int critRoll = rand(100);

        int missChance = 10; // base miss chance %
        int critChance = 10; // base crit chance %

        if (selection == 1) { // heavy
            missChance = (int)(missChance * 1.5);  // 15%
            critChance = (int)(critChance * 1.5);  // 15%
        } else if (selection == 2) { // light
            missChance = (int)(missChance * 0.5);  // 5%
            critChance = (int)(critChance * 0.5);  // 5%
        }

        if (battlePosition == 1)
            missChance += 20;

        if (missRoll < missChance) {
            damage = 0;
            updateText("Ye completely missed!");
        } else {
            if (critRoll < critChance) {
                damage *= 2;
                snprintf(msg, sizeof(msg), "Critical hit! Ye dealt %d damage!", damage);
            } else {
                snprintf(msg, sizeof(msg), "Ye dealt %d damage!", damage);
            }
            *monsterHealth -= damage;
            updateText(msg);
        }
    }

    displayText();
}

 /*
  * encounterEnemy: handles combat against an enemy (or boss).
  * Now adjusted for proper text and shield removal.
  */
 void encounterEnemy(int isBoss) {
    int retaliate = 1, defended = 0, action;
    int baseHealth, monsterHealth;
    int isMimic = 0;
    int crit = 0;
    battlePosition = 0; // reset battle position to front row

    if (isBoss) {
        char *bosses[] = {"Ironclad King", "Ravager of Souls", "Stormbringer", "The Herald of Despair", "Voidcaller"};
        int bossCount = 5;
        baseHealth = rand(120) + 150;
        monsterHealth = baseHealth;
        set_color(0, 4);
        repaint_screen(0, 4);
        current_fg_color = 0;
        current_bg_color = 4;
        snprintf(msg, sizeof(msg), "Ye encounter the %s!", bosses[rand(bossCount) % bossCount]);
        showText(msg);
    } else {
        const char *monsterName;
        if (rand(10) < 2) {
            isMimic = 1;
            monsterName = "Mimic";
            baseHealth = rand(20) + 100 + (dungeon_floor * 10);
            change_color(14, 0);
        } else {
            const char *normalMonsters[] = {"Goblin", "Troll", "Skeleton", "Wizard", "Zombie-sized Chicken", "Bandit", "Rabid Dog"};
            int normalCount = 7;
            int idx = rand(normalCount) % normalCount;
            monsterName = normalMonsters[idx];
            baseHealth = rand(60) + 30 + (dungeon_floor * 5);
            change_color(15, 0);
        }
        monsterHealth = baseHealth;
        snprintf(msg, sizeof(msg), "A wild %s appeared!", monsterName);
        showText(msg);
    }

    while (monsterHealth > 0 && playerHealth > 0) {
        char *combatOptions[] = {"Attack Menu", "Item Menu", "Defend", "Run", "Scan", "Toggle Position"};
        displayStatus();
        displayInventory();
        displayText();
        retaliate = 1;

        action = navigateMenu(combatOptions, 6);

        switch (action) {
            case 0:
                defended = 0;
                attackMenu(&monsterHealth, &retaliate);
                break;

            case 1:
                useItemMenu();
                retaliate = 0;
                break;

            case 2:
                defended = 1;
                playerStamina += 5;
                if (playerStamina > maxStamina + permanentStaminaBonus)
                    playerStamina = maxStamina + permanentStaminaBonus;
                break;

            case 3:
                if (isBoss) {
                    showText("Can't run away from a boss!");
                } else {
                    runAway();
                    if (!escapeFailed)
                        return;
                }
                retaliate = 0;
                break;

            case 4:
                snprintf(msg, sizeof(msg), "Enemy has %d / %d health.", monsterHealth, baseHealth);
                showText(msg);
                retaliate = 0;
                break;

            case 5:
                togglePosition();
                retaliate = 0;
                break;
        }

        if (monsterHealth <= 0) {
            int loot = rand(20) + 10;
            if (isMimic) {
                loot *= 2;
                healthPotions++;
                snprintf(msg, sizeof(msg), "Ye defeated the mimic! Gained %d gold and a health potion.", loot);
            } else if (isBoss) {
                snprintf(msg, sizeof(msg), "Congrats! Ye slayed the boss! Yer attributes increase!");
                loot *= 3;
                healthPotions += 1;
                dungeon_floor++;
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
                showText("The air grows thick, ye feel a sinister presence...");
            }
        }

        if (retaliate && monsterHealth > 0) {
            int baseDmg = rand(isBoss ? 15 : 8) + (isBoss ? 10 : 3);
            float dmg = baseDmg;
            crit = 0;

            if (arrowFiredFromBackRow) {
                arrowFiredFromBackRow = 0;
                dmg = 0;
                snprintf(msg, sizeof(msg), "The monster tried to attack, but couldn't reach ye!");
            } else if (rand(10) == 0) {
                dmg *= 2;
                crit = 1;
                snprintf(msg, sizeof(msg), "The monster scored a critical hit and dealt %d damage!", (int)dmg);
            } else if (rand(10) == 0) {
                dmg = 0;
                snprintf(msg, sizeof(msg), "The monster completely missed!");
            } else if (defended == 1) {
                if (!crit && shieldCount > 0) {
                    shields[0].uses--;
                    if (shields[0].uses <= 0) {
                        print("Yer shield snaps like a twig in a hurricane!\n");
                        shieldCount = 0;
                    }
                    dmg = 0;
                    snprintf(msg, sizeof(msg), "The monster attacked, but yer shield blocked it all!");
                    playerStamina += 10;
                    if (playerStamina > maxStamina + permanentStaminaBonus)
                        playerStamina = maxStamina + permanentStaminaBonus;
                } else {
                    dmg /= 2;
                    snprintf(msg, sizeof(msg), "The monster attacks, but ye block some of it! Ye take %d damage.", (int)dmg);
                }
            } else {
                snprintf(msg, sizeof(msg), "The monster retaliates and deals %d damage!", (int)dmg);
            }

            if (battlePosition == 1)
                dmg /= 2;

            if (!crit && activeArmorBonus > 0) {
                dmg -= activeArmorBonus;
                if (dmg < 0)
                    dmg = 0;
                snprintf(msg, sizeof(msg), "Yer extra armor reduces the damage by %d!", activeArmorBonus);
            }

            playerHealth -= (int)dmg;
            if ((int)dmg >= 1)
                flash_damage();
            showText(msg);

            if (playerHealth <= 0) {
                is_playing = 0;
                endGame();
            }
        }
    }

    change_color(2, 15);
}


 /*
  * encounterShop: handles shop encounters.
  * Added price variance for equipment and restricts shield to one.
  */
 void encounterShop() {
     change_color(15, 1);
     char *shopOptions[] = {"Buy Items", "Rest", "Buy Ticket", "Leave"};
     int healthPotionCost = rand(30) + 40;
     int staminaPotionCost = rand(10) + 25;
     int elixirCost = rand(50) + 180;
     int arrowCost = rand(20) + 25;
     int selection;
     int purchaseSelection;
     int shopRestUsed = 0;
   
     showText("Ye encounter a shopkeeper!");
     while (1) {
         selection = navigateMenu(shopOptions, 4);
         if (selection == 0) {
             // Purchase menu with 8 options.
             char *purchaseOptions[] = {"Buy Health Potion", "Buy Stamina Potion", "Buy Elixir", "Buy Arrows", "Buy Armor", "Buy Sharpening Tool", "Buy Shield", "Cancel"};
             showText("What will ye buy?");
             // Calculate variable prices for new equipment:
             int armorCost = 50 + (rand(11) - 5);       // 45-55g
             int sharpCost = 50 + (rand(11) - 5);         // 45-55g
             int shieldCost = 50 + (rand(11) - 5);        // 45-55g
             snprintf(msg, sizeof(msg),
                 "Health pot: %dg, Stamina pot: %dg, Elixir: %dg, Arrows: %dg, Armor: %dg, Sharpening: %dg, Shield: %dg.",
                 healthPotionCost, staminaPotionCost, elixirCost, arrowCost, armorCost, sharpCost, shieldCost);
             updateText(msg);
             purchaseSelection = navigateMenu(purchaseOptions, 8);
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
             } else if (purchaseSelection == 4 && gold >= armorCost) {
                 if (armorCount < MAX_EQUIP) {
                     armorEquip[armorCount].uses = 15;
                     armorCount++;
                     gold -= armorCost;
                     updateText("Ye bought a sturdy plate o' armor.");
                 } else {
                     updateText("Ye can't carry any more armor plates!");
                 }
             } else if (purchaseSelection == 5 && gold >= sharpCost) {
                 if (sharpCount < MAX_EQUIP) {
                     sharpTools[sharpCount].uses = 15;
                     sharpCount++;
                     gold -= sharpCost;
                     updateText("Ye bought a sharpening tool for yer blade.");
                 } else {
                     updateText("Yer already maxed out on sharpening tools!");
                 }
             } else if (purchaseSelection == 6 && gold >= shieldCost) {
                 if (shieldCount > 0) {
                     updateText("Ye already got a shield! Only one allowed.");
                 } else {
                     shields[0].uses = 10;
                     shieldCount = 1;
                     gold -= shieldCost;
                     updateText("Ye bought a shield.");
                 }
             } else if (purchaseSelection == 7) {
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
             if (gold >= 50) {
                 shopRestLimit++;
                 gold -= 50;
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
     change_color(2, 15);
 }
 
void encounterShrine() {
    char *sacrificeOptions[] = {"Sacrifice Gold (125)", "Sacrifice an Item", "Sacrifice Health", "Sacrifice Stamina", "Return"};
    int selection, upgradeSelection;
    showText("Ye stumble upon a mysterious shrine. The air is thick wi' ancient dread.");
    selection = navigateMenu(sacrificeOptions, 5);

    int sacrificeMade = 0;

    if (selection == 0) {
        if (gold >= 125) {
            gold -= 125;
            sacrificeMade = 1;
            updateText("125 gold sacrificed.");
        } else {
            updateText("Ye dinnae have enough gold to sacrifice!");
        }
    } else if (selection == 1) {
        if (healthPotions >= 3) {
            healthPotions -= 3;
            sacrificeMade = 1;
            updateText("A set of 2 health potions has been sacrificed.");
        } else if (staminaPotions >= 5) {
            staminaPotions -= 5;
            sacrificeMade = 1;
            updateText("A set of 4 stamina potions has been sacrificed.");
        } else if (elixirs >= 1) {
            elixirs--;
            sacrificeMade = 1;
            updateText("An elixir has been sacrificed.");
        } else {
            updateText("Ye have no items to sacrifice!");
        }
    } else if (selection == 2) {
        if (maxHealth + permanentHealthBonus > 10) {
            permanentHealthBonus -= 10;
            sacrificeMade = 1;
            updateText("Ye sacrificed 10 max health.");
        } else {
            updateText("Ye dinnae have enough max health to sacrifice!");
        }
    } else if (selection == 3) {
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

    if (!sacrificeMade)
        return;

    char *upgradeOptions[] = {"Increase Max Health (+10)", "Increase Damage (+3)", "Increase Max Stamina (+10)", "Gain 150 Gold"};
    showText("Choose your boon...");
    upgradeSelection = navigateMenu(upgradeOptions, 4);

    if (upgradeSelection == 0) {
        permanentHealthBonus += 10;
        updateText("Ye've increased yer max health permanently by 10.");
    } else if (upgradeSelection == 1) {
        damageBonus += 3;
        updateText("Ye've increased yer damage permanently by 3.");
    } else if (upgradeSelection == 2) {
        permanentStaminaBonus += 10;
        updateText("Ye've increased yer max stamina permanently by 10.");
    } else if (upgradeSelection == 3) {
        gold += 150;
        updateText("150 gold spews from the shrine.");
    } 
    displayText();
}
 
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

        if (rand(20) == 0) {
            encounterShrine();
        }

        if (dungeon_floor == 6) {
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


    /*
    * startingItem: allows the player to choose a starting item.
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
     srand(extra_rand());
     disable_cursor();
     clear_screen();
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
 