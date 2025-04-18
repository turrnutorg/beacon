/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * games.c
 */

#include "games.h"
#include "console.h"
#include "screen.h"

void guessnum() {
    println("Welcome to the Beacon number guessing game!");
    println("Enter a number(0-100):");
    curs_row ++;
}
