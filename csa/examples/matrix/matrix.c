#include "syscall.h"

#define WIDTH 80
#define HEIGHT 25
#define MAX_STREAMS 60  // Upped for full chaos

typedef struct {
    int x;
    int y;
    int speed;
    int counter;
    int length;
} stream_t;

void main(void) {
    syscall_table_t* sys = *(syscall_table_t**)0x200000;
    sys->disable_cursor();
    sys->set_color(2, 0);         // green on black
    sys->repaint_screen(2, 0);    // black bg BEFORE any rain
    sys->clear_screen();
    sys->srand(sys->extra_rand());

    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+=-";
    stream_t streams[MAX_STREAMS];

    for (int i = 0; i < MAX_STREAMS; i++) {
        streams[i].x = sys->rand(WIDTH);
        streams[i].y = sys->rand(HEIGHT);
        streams[i].speed = 1 + sys->rand(3); // lower is faster
        streams[i].length = 3 + sys->rand(6);
        streams[i].counter = 0;
    }

    while (1) {
        for (int i = 0; i < MAX_STREAMS; i++) {
            stream_t* s = &streams[i];
            s->counter++;

            if (s->counter >= s->speed) {
                s->counter = 0;
                s->y = (s->y + 1) % (HEIGHT + s->length);
            }

            for (int j = 0; j < s->length; j++) {
                int draw_y = s->y - j;
                if (draw_y >= 0 && draw_y < HEIGHT) {
                    sys->gotoxy(s->x, draw_y);
                    char c[2] = { charset[sys->rand(sizeof(charset) - 1)], '\0' };

                    if (j == 0) sys->set_color(15, 0);       // bright white head
                    else if (j == 1) sys->set_color(10, 0);  // neon green trail
                    else sys->set_color(2, 0);               // fading green

                    sys->print(c);
                }
            }

            int tail_y = s->y - s->length;
            if (tail_y >= 0 && tail_y < HEIGHT) {
                sys->gotoxy(s->x, tail_y);
                sys->print(" ");
            }
        }

        // slower update — gives a more cinematic crawl
        sys->delay_ms(300); 

        int ch = sys->getch_nb();
        if (ch == ' ') {
            break;
        }
    }

    sys->reset();
}
