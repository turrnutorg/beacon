#include "serial.h"
#include "string.h"
#include "command.h"
#include "console.h"
#include "csa.h"
#include <stdint.h>
#include "syscall.h"
#include "screen.h"
#include "time.h"
#include "keyboard.h"
#include "speaker.h"
#include "os.h"  // assuming reset() + extra_rand() are here

// set up the syscall table
syscall_table_t syscall_table = {
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
    .snprintf = snprintf,
    .vsnprintf = vsnprintf,
    .strcpy = strcpy,
    .strcmp = strcmp,
    .strlen = strlen,
    .beep = beep,
    .strncmp = strncmp,
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

        if (addr < 0x10000 || addr > 0x800000) { // adjust these as per your memory map
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
        // dump first 16 payload bytes from the buffer for debug
        for (int i = 12; i < 12 + 16 && i < CSA_HEADER_SIZE + csa_expected_size; ++i) {
            char hex[64];
            snprintf(hex, sizeof(hex), "buffer[%d] = 0x%d", i, csa_buffer[i]);
            println(hex);
        }

        // clear the destination memory and copy payload
        memset(csa_entrypoint, 0, csa_expected_size);
        memcpy(csa_entrypoint, &csa_buffer[12], csa_expected_size);

        // set the syscall table pointer (must match CSA binary expectations)
        syscall_table_t** syscall_ptr_slot = (syscall_table_t**)0x200000;
        *syscall_ptr_slot = &syscall_table;

        println("CSA: Loaded successfully.");
        {
            char msg[64];
            snprintf(msg, sizeof(msg), "Entrypoint @ 0x%d", (uint32_t)csa_entrypoint);
            println(msg);
        }

        csa_done = 0;
        csa_recv_index = 0;
        curs_row += 2;
        curs_col = 0;
        update_cursor();

        delay_ms(10000); // give it a second to breathe
        println("CSA: Ready to execute. Type 'run' to start.");
        curs_row++;
        update_cursor();
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
    // dump first 16 bytes from the loaded payload for debug
    unsigned char* code = (unsigned char*)csa_entrypoint;
    for (int i = 0; i < 16 && i < csa_expected_size; ++i) {
        char msg[64];
        snprintf(msg, sizeof(msg), "code[%d] = 0x%d", i, code[i]);
        println(msg);
    }

    println("JUMPING TO ENTRYPOINT NOW");
    void (*entry_func)(void) = (void (*)(void))csa_entrypoint;
    entry_func();
    println("BACK FROM ENTRY??!?!? WTF");
    println("CSA: ERROR! Payload returned... what the fuck?");
    reset(); // or enter an infinite halt loop
}
