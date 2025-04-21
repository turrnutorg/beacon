/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * csa.c
 */

#include "serial.h"
#include "string.h"
#include "command.h"
#include "console.h"
#include <stdint.h>
#include "syscall.h"
#include "port.h"
#include "screen.h"
#include "time.h"
#include "keyboard.h"
#include "speaker.h"
#include "os.h"
#include "math.h"
#include "csa.h"

syscall_table_t syscall_table = {
    // --- core sys ---
    .print = print,
    .gotoxy = gotoxy,
    .clear_screen = clear_screen,
    .repaint_screen = repaint_screen,
    .set_color = set_color,
    .delay_ms = delay_ms,
    .disable_cursor = disable_cursor,
    .update_cursor = update_cursor,
    .getch = getch,
    .srand = srand,
    .rand = rand,
    .extra_rand = extra_rand,
    .read_rtc_datetime = read_rtc_datetime,
    .reset = reset,
    .beep = beep,
    .getch_nb = getch_nb,

    // --- string/format/mem ---
    .snprintf = snprintf,
    .vsnprintf = vsnprintf,
    .strcpy = strcpy,
    .strncpy = strncpy,
    .strcat = strcat,
    .strncat = strncat,
    .strlen = strlen,
    .strcmp = strcmp,
    .strncmp = strncmp,
    .strchr = strchr,
    .strrchr = strrchr,
    .strstr = strstr,
    .strtok = strtok,
    .memcpy = memcpy,
    .memset = memset,
    .memcmp = memcmp,
    .itoa = itoa,
    .itoa_base = itoa_base,
    .atoi = atoi,
    .strtol = strtol,

    // --- rtc / datetime ---
    .display_datetime = display_datetime,
    .set_rtc_date = set_rtc_date,
    .set_rtc_time = set_rtc_time,
    .get_month_name = get_month_name,

    // --- serial fuckery ---
    .serial_init = serial_init,
    .serial_received = serial_received,
    .serial_read = serial_read,
    .serial_is_transmit_empty = serial_is_transmit_empty,
    .serial_write = serial_write,
    .serial_write_string = serial_write_string,
    .set_serial_command = set_serial_command,
    .set_serial_waiting = set_serial_waiting,
    .set_serial_feedthru_callback = set_serial_feedthru_callback,
    .serial_poll = serial_poll,
    .serial_toggle = serial_toggle,

    // --- console i/o extras ---
    .putchar = putchar,
    .printc = printc,
    .println = println,
    .newline = newline,
    .move_cursor_left = move_cursor_left,
    .enable_cursor = enable_cursor,
    .enable_bright_bg = enable_bright_bg,

    // ─── fixed-point math ───────────────
    .fabs = fabs,
    .sqrt = sqrt,
    .pow = pow,
    .exp = exp,
    .log = log,
    .fmod = fmod,
    .sin = sin,
    .cos = cos,
    .tan = tan,
    .asin = asin,
    .acos = acos,
    .atan = atan,
    .sinh = sinh,
    .cosh = cosh,
    .tanh = tanh,
    .floor = floor,
    .ceil = ceil,
    .round = round,
    .signbit = signbit,
    .isnan = isnan,
    .isinf = isinf,

    // ─── float math ───────────────
    .fabsf = fabsf,
    .sqrtf = sqrtf,
    .powf = powf,
    .expf = expf,
    .logf = logf,
    .fmodf = fmodf,
    .sinf = sinf,
    .cosf = cosf,
    .tanf = tanf,
    .asinf = asinf,
    .acosf = acosf,
    .atanf = atanf,
    .sinhf = sinhf,
    .coshf = coshf,
    .tanhf = tanhf,
    .floorf = floorf,
    .ceilf = ceilf,
    .roundf = roundf,
    .signbitf = signbitf,
    .isnanf = isnanf,
    .isinff = isinff
};

#define CSA_MAGIC 0xC0DEFACE
#define CSA_HEADER_SIZE 12
#define CSA_MAX_SIZE 65536

static uint8_t csa_buffer[CSA_HEADER_SIZE + CSA_MAX_SIZE];
static int csa_recv_index = 0;
static int csa_expected_size = -1;
void* csa_entrypoint = 0;

extern syscall_table_t syscall_table;

static int csa_done = 0;   // signals that loading is done
static int csa_failed = 0;

void csa_feedthru(char byte) {
    if (csa_recv_index >= sizeof(csa_buffer)) {
        println("CSA: Buffer overflow. You absolute donkey.");
        csa_failed = 1;
        set_serial_waiting(0);
        set_serial_command(1);
        return;
    }

    csa_buffer[csa_recv_index++] = byte;

    if (csa_recv_index == CSA_HEADER_SIZE) {
        uint32_t magic = *(uint32_t*)&csa_buffer[0];
        if (magic != CSA_MAGIC) {
            println("CSA: Invalid header magic. Get fucked.");
            csa_failed = 1;
            set_serial_waiting(0);
            set_serial_command(1);
            return;
        }

        uint32_t addr = *(uint32_t*)&csa_buffer[4];
        uint32_t size = *(uint32_t*)&csa_buffer[8];

        if (size > CSA_MAX_SIZE) {
            println("CSA: Payload too fuckin big. Die.");
            csa_failed = 1;
            set_serial_waiting(0);
            set_serial_command(1);
            return;
        }

        if (addr < 0x200000 || addr > 0x800000) { // adjust these as per your memory map
            println("CSA: Invalid memory address, get tae fuck.");
            csa_failed = 1;
            set_serial_waiting(0);
            set_serial_command(1);
            return;
        }

        csa_expected_size = size;
        csa_entrypoint = (void*)addr;
    }

    if (csa_expected_size > 0 && csa_recv_index == CSA_HEADER_SIZE + csa_expected_size) {
        csa_done = 1;
        set_serial_waiting(0);
        set_serial_command(1);
    }
}

void csa_tick(void) {
    if (csa_done && !csa_failed) {
        // clear the destination memory and copy payload
        memset(csa_entrypoint, 0, csa_expected_size);
        memcpy(csa_entrypoint, &csa_buffer[12], csa_expected_size);

        // set the syscall table pointer (must match CSA binary expectations)
        syscall_table_t** syscall_ptr_slot = (syscall_table_t**)0x200000;
        *syscall_ptr_slot = &syscall_table;

        println("CSA: Loaded successfully.");

        csa_done = 0;
        csa_recv_index = 0;
        curs_row ++;
        curs_col = 0;
        update_cursor();
        println("CSA: Ready to execute. Type 'program run' to start.");
        curs_row++;
        update_cursor();
        execute_csa();
    }
}

void command_load(char* args) {
    (void)args;
    println("CSA loader waiting...");

    csa_recv_index = 0;
    csa_expected_size = -1;
    csa_entrypoint = 0;
    csa_done = 0;
    csa_failed = 0;

    set_serial_command(0);
    set_serial_waiting(1);
    set_serial_feedthru_callback(csa_feedthru);
}

void execute_csa(void) {
    println("JUMPING TO ENTRYPOINT NOW");
    void (*entry_func)(void) = (void (*)(void))csa_entrypoint;
    entry_func();
    println("CSA: ERROR! Payload returned... what the fuck?");
    reset(); // or enter an infinite halt loop
}
