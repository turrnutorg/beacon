#ifndef DUNGEON_H
#define DUNGEON_H

#include <stdint.h>

/*
 * Prototype for the dungeon starter function.
 * This is the main game launcher – it contains all the logic, global variables, and helper
 * functions needed for your dungeon crawler.
 */
int start_dungeon(void);

/*
 * Prototype for zeroing out the .bss section.
 */
void zero_bss(void);

#endif
