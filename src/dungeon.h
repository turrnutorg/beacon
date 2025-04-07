/** 
* Copyright (c) 2025 Turrnut Open Source Organization
* Under the GPL v3 License
* See COPYING for information on how you can use this file
* 
* dungeon.c
**/

#ifndef DUNGEON_H
#define DUNGEON_H

#include <stdint.h>

// global state
extern int gold;
extern int healthPotions;
extern int staminaPotions;
extern int elixirs;
extern int arrows;
extern int escapeFailed;
extern int steps;
extern int enemiesDefeated;
extern int bossesDefeated;
extern int floor;
extern int playerHealth;
extern int playerStamina;
extern int maxHealth;
extern int maxStamina;

// UI buffers
extern char currentText[1024];
extern char msg[1024];

// main game functions
void updateText(const char *format, ...);
void displayText();
void displayStatus();
void displayInventory();
void exploreDungeon();
void encounterEnemy(int isBoss);
void encounterShop();
void useItemMenu();
void attackMenu(int *monsterHealth, int *retaliate, int isBoss, int baseHealth);
void runAway();
void regenerateStamina();
void endGame();
void startingItem();
int navigateMenu(char **options, int numOptions);
int start_dungeon();

// VGA position cursor emulation
void gotoxy(size_t x, size_t y);

#endif // DUNGEON_H
