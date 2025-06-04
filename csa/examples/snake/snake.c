/*
 * My implementation of the beloved snake game
 * made for this lovely project
 *
 * snake.csa file included
 *
 * Written by Ivan Kazhnov (kazhnov)
 * Copyright (c) 2025 Ivan Kazhnov
 * Licensed under the GPL v3 License
 */

#include "snake.h"
#include <string.h>
 
#define UP_ARROW	-1 // Using arrows causes
#define DOWN_ARROW	-2 // snake to always go up
#define LEFT_ARROW	-3 // due to getch_nb()
#define RIGHT_ARROW	-4 // returning -1
#define ESCAPE		'`'

#define WIDTH 80
#define HEIGHT 25

#define DIR_NORTH 0
#define DIR_WEST 1
#define DIR_SOUTH 2
#define DIR_EAST 3

#define BLUE 1
#define GREEN 2
#define RED 4
#define YELLOW 14
#define WHITE 15

#define SPEED 1 // Overall game speed

#define BACKGROUND WHITE

// Screen borders
#define BORDER_NORTH	2
#define BORDER_WEST	2
#define BORDER_SOUTH	2
#define BORDER_EAST	2

#define NAME		"Snake"
#define LOST_STRING	"You lost"
#define TRY_AGAIN	"Try Again? (Y/n)"
#define WON_STRING	"You won!"
#define PLAY_AGAIN	"Play Again? (Y/n)"

syscall_table_t* sys;
typedef uint8_t segment_t;

typedef struct {
	// Each segment stores direction to the next one,
	// starting from the snake's head
	segment_t segments[WIDTH*HEIGHT]; 
	uint8_t dir; 
	uint32_t length; // Number of segments (including the head)
	uint8_t x;
	uint8_t y;
} Snake;

typedef struct {
	uint8_t x;
	uint8_t y;
} Apple;

uint8_t debug = 0;
uint8_t map[HEIGHT][WIDTH]; // Storing the positions occupied by the snake

uint8_t player_won = 0;
uint8_t draw_borders = 1;

Apple apple;
Snake snake;

const int8_t x_from_dir[] = {
	[DIR_NORTH]	= 0,
	[DIR_WEST]	= -1,
	[DIR_SOUTH]	= 0,
	[DIR_EAST]	= 1,
};

const int8_t y_from_dir[] = {
	[DIR_NORTH]	= -1,
	[DIR_WEST]	= 0,
	[DIR_SOUTH]	= 1,
	[DIR_EAST]	= 0,
};

uint8_t is_in_borders(uint8_t x, uint8_t y) {
	if (
			x < BORDER_WEST ||
			x >= WIDTH-BORDER_EAST ||
			y <= BORDER_NORTH ||
			y > HEIGHT-BORDER_SOUTH
	   ) 
	{
		return 0;
	}
	return 1;
}

// Get random unoccupied position (used for apple generation)
void get_random_free_pos(uint8_t *x, uint8_t *y) {
	size_t tries = 0;
	while(tries < 10) {
		*x = (uint8_t)sys->rand() % (WIDTH - 1);
		*y = (uint8_t)sys->rand() % (HEIGHT - 1);

		if (map[*y][*x] != 0 || !is_in_borders(*x, *y + 1)) {
			// The position is occupied, go look somewhere else
			tries++;
		}
		else 
			return;
		
	}
        if (tries >= 9) {
		// If we give up trying random position,
		// pick one nearest to the upper left corner
		// (but not near the borders)
        	for (size_t i = BORDER_WEST + 1; 
				i < WIDTH - BORDER_EAST - 1; i++) {
			for (size_t j = BORDER_NORTH + 1; 
					j < HEIGHT - BORDER_SOUTH - 1; j++) {
        			if(map[j][i] == 0){
        				*x = i;
        				*y = j;
        				return;
        			}
			}
        	}
        }
	// If the game couldn't find a free tile, player wins
	player_won = 1;
}

void update_map() {
	memset(map, 0, WIDTH*HEIGHT);
	uint8_t x = snake.x;
	uint8_t y = snake.y;

	// Gradually going through the segments,
	// filling map one by one
	for (size_t i = 0; i < snake.length; i++) {
		map[y][x] = 1;
		x += x_from_dir[snake.segments[i]];
		y += y_from_dir[snake.segments[i]];
	}
}

void snake_new() {
	memset(snake.segments, -1, WIDTH*HEIGHT);
	snake.length = 1;
	snake.x = WIDTH/2;
	snake.y = HEIGHT/2;
	snake.dir = DIR_EAST;
	snake.segments[0] = (snake.dir + 2) % 4;
	player_won = 0;
}

void snake_expand() {
	snake.length++;
}

void spawn_apple() {
	get_random_free_pos(&apple.x, &apple.y);
}

void draw_apple(uint8_t x, uint8_t y) {
	sys->gotoxy(x, y);
	sys->set_color(WHITE, RED);
	sys->putchar('*');
}

void undraw_apple(uint8_t x, uint8_t y) {
	sys->gotoxy(x, y);
	sys->set_color(BACKGROUND, BACKGROUND);
	sys->putchar(' ');
}

int snake_move() {
	uint8_t next_x = snake.x + x_from_dir[snake.dir];
	uint8_t next_y = snake.y + y_from_dir[snake.dir];

	if(next_x == apple.x && next_y == apple.y){
		// Snake eats the apple
		snake_expand();
		update_map();
		// Give apple a new position	
		spawn_apple();
	}
	if(!is_in_borders(next_x, next_y+1)) {
		return 1;
	}
	if(map[next_y][next_x]==1) {
		// Snake hit itself
		return 1;
	}
	// We shift segments by one from head to tail
	// to simulate movement
	for (size_t i = snake.length-1; i > 0; i--) {
		snake.segments[i] = snake.segments[i-1];
	}
	// The head segment will point away from 
	// the snake's direction
	snake.segments[0] = (snake.dir + 2) % 4;
	snake.x = next_x;
	snake.y = next_y;
	return 0;
}

void draw_score() {
	sys->set_color(WHITE, GREEN);
	sys->gotoxy(1, 0);
	sys->print("score: ");
	char* score;
	sys->itoa(snake.length, score);
	sys->print(score);
}

void undraw_score() {
	sys->set_color(WHITE, GREEN);
	sys->gotoxy(11, 0);
	sys->move_cursor_left();
	sys->putchar(' ');
	sys->move_cursor_left();
	sys->putchar(' ');
	sys->move_cursor_left();
	sys->putchar(' ');
	sys->move_cursor_left();
	sys->putchar(' ');
}

void draw_segment(uint8_t x, uint8_t y, uint8_t dir, uint32_t i){
	if (x > WIDTH || y > HEIGHT || dir > 3) return;
	char c = ' ';
	if (i == 0) {
		switch(dir) {
			case DIR_NORTH:
				c = 'v';
				break;
			case DIR_WEST:
				c = '>';
				break;
			case DIR_SOUTH:
				c = '^';
				break;
			case DIR_EAST:
				c = '<';
				break;
		}
	}
	sys->gotoxy(x, y);
	sys->set_color(WHITE, GREEN);
	sys->putchar(c);
}

void undraw_segment(uint8_t x, uint8_t y){
	sys->gotoxy(x, y);
	sys->set_color(BACKGROUND, BACKGROUND);
	sys->putchar(' ');
}

void draw_segments() {
	uint8_t x = snake.x;
	uint8_t y = snake.y;
	
	// The same as in update_map()	
	for (size_t i = 0; i < snake.length; i++) {
		draw_segment(x, y, snake.segments[i], i);
		x += x_from_dir[snake.segments[i]];
		y += y_from_dir[snake.segments[i]];
	}
}

void undraw_segments() {
	uint8_t x = snake.x;
	uint8_t y = snake.y;
	
	// The same as in update_map()	
	for (size_t i = 0; i < snake.length; i++) {
		undraw_segment(x, y);
		x += x_from_dir[snake.segments[i]];
		y += y_from_dir[snake.segments[i]];
	}
}

void draw_name() {
	sys->set_color(WHITE, GREEN);
	sys->gotoxy(WIDTH/2-3, 0);
	sys->print(NAME);
}

void draw_ui() {
	// This draws the borders, only call this once per game
	// to avoid excessive flickering
	sys->gotoxy(WIDTH, -1);
	sys->set_color(WHITE, GREEN);
	if(draw_borders) {
		for (uint8_t i = 0; i < WIDTH; i++) {
			for (uint8_t j = 0; j < HEIGHT; j++) {
				if (!is_in_borders(i, j)) {
					sys->gotoxy(i, j);
					sys->putchar(' ');
				}
			}
		}
	}
	sys->putchar(' ');
	draw_name();
}

void update_score() {
	// dynamicaly draw score, the only part of the
	// UI that changes
	undraw_score();
	draw_score();
}

void you_lost() {
	// Draw the "You lose" screen
	sys->set_color(GREEN, BACKGROUND);
	sys->gotoxy(WIDTH/2-4, HEIGHT/2);
	sys->print(LOST_STRING);
	sys->gotoxy(WIDTH/2-8, HEIGHT/2+1);
	sys->print(TRY_AGAIN);
	char c = sys->getch();
	switch(c) {
		case 'n': 
			sys->reset();  // Quit
			return;
		default:
			snake_start(); // Restart

	}
}

void you_won() {
	// Draw the "You win" screen
	sys->set_color(GREEN, BACKGROUND);
	sys->set_color(GREEN, BACKGROUND);
	sys->gotoxy(WIDTH/2-4, HEIGHT/2);
	sys->print(WON_STRING);
	sys->gotoxy(WIDTH/2-8, HEIGHT/2+1);
	sys->print(PLAY_AGAIN);
	char c = sys->getch();
	switch(c) {
		case 'n': 
			sys->reset();  // Quit
			return;
		default:
			snake_start(); // Restart

	}
}

int snake_start() {
	sys->disable_cursor();
	sys->srand(sys->extra_rand());
	sys->clear_screen();

	snake_new();
	uint8_t current_dir = snake.dir;
	update_map();

	spawn_apple();
	draw_ui();
	draw_score();
	uint8_t ticks = 0;
	uint32_t expand = 0; // this value + 1 is the initial length
	while(1) {
		if (player_won) {
			you_won();
			continue;
		}
		sys->set_color(WHITE, WHITE);
		
		int key = sys->getch_nb();
		switch(key) {
		case 'w':
			// Prevent turning 180 degrees
			if (snake.dir != DIR_SOUTH)
				current_dir = DIR_NORTH;
			break;
		case 's':
			if (snake.dir != DIR_NORTH)
				current_dir = DIR_SOUTH;
			break;
		case 'a':
			if (snake.dir != DIR_EAST)
				current_dir = DIR_WEST;
			break;
		case 'd':
			if (snake.dir != DIR_WEST)
				current_dir = DIR_EAST;
			break;

		//case 'e':		  Uncomment for debug purposes
		//	expand = !expand;
		//	break;
		
		case ESCAPE:
			sys->reset();
			return 0;
		}

		if (ticks%4==0) {
			// We check for input 4 times per movement
			// for improved feedback
			snake.dir = current_dir;
			if (expand > 0) {
				snake_expand();
				expand--;
			}

			// We undraw stuff instead of
			// clearing the screen to prevent
			// too much flickering
			undraw_apple(apple.x, apple.y);
			undraw_segments();

			if (snake_move() && !player_won) {
				// snake_move() returns 1 when 
				// snake hits something
				you_lost();
				continue;
			}
			// Redraw, when updated
			draw_segments();
			draw_apple(apple.x, apple.y);
			
			ticks = 0;
		}
		sys->delay_ms(75/SPEED); // Adjust if you need
		update_map();
		update_score();
		ticks++;
	}
	return 0;
}
