/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 *
 * bf.c
 */

#include "console.h"
#include "keyboard.h"
#include "screen.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

static int loop(char *text, int i, int mode) {
    int sum = 0;
    int movIdx = i;
    while (sum > -1) {
        movIdx -= mode;
        if (text[movIdx] == ']')
            sum += mode;
        else if (text[movIdx] == '[')
            sum -= mode;
    }
    return movIdx;
}

void bf(char text[]) {
    uint8_t arr[60000];
    int pointer = 0;
    int i = 0;
    while (true) {
        switch (text[i]) {
        case '\0':
            return;
        case '>':
            pointer++;
            break;
        case '<':
            pointer--;
            break;
        case '+':
            if (arr[pointer] >= 255)
                arr[pointer] = 0;
            else
                arr[pointer]++;
            break;
        case '-':
            if (arr[pointer] <= 0)
                arr[pointer] = 0;
            else
                arr[pointer]--;
            break;
        case '.':
            printc((char)arr[pointer]);
            curs_col ++;
            break;
        case ',':
            arr[pointer] = getch();
            break;
        case ']':
            if (arr[pointer] != 0)
            {
                i = loop(text, i, 1);
            }
            break;
        case '[':
            if (arr[pointer] == 0) {
                i = loop(text, i, -1);
            }
            break;
        default: 
            break;
        }
        i++;
    }
    return;
}