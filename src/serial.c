#include "serial.h"
#include "port.h"
#include "string.h"
#include "console.h"
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include "stdlib.h"
#include "time.h"
#include "ff.h"   // Include FATFS
#include "diskio.h" // Include Disk I/O for fatfs

#define FILENAME_MAX_LENGTH 64

static FIL file; // FATFS file object for writing received file
static uint32_t received_file_size = 0;
static uint32_t received_data_count = 0;

#define SERIAL_INPUT_BUFFER_SIZE 256

// buffer tae store serial input for file transfer
static char serial_input_buffer[SERIAL_INPUT_BUFFER_SIZE];
static int serial_input_index = 0;

#define COM1_PORT 0x3F8

// Disable serial command processing
static int serial_enabled = 1; // Set to 0 to disable serial completely

// pointer for feedthru callback for any waiting process
typedef void (*serial_feedthru_callback_t)(char);
static serial_feedthru_callback_t feedthru_callback = NULL;

void serial_toggle() {
    serial_enabled = !serial_enabled;
}

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);       // disable interrupts
    outb(COM1_PORT + 3, 0x80);       // enable DLAB for baud rate
    outb(COM1_PORT + 0, 0x03);       // set divisor lo byte (38400 baud)
    outb(COM1_PORT + 1, 0x00);       // set divisor hi byte
    outb(COM1_PORT + 3, 0x03);       // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);       // enable FIFO, clear 'em, 14-byte threshold
    outb(COM1_PORT + 4, 0x0B);       // IRQs enabled, RTS/DSR set
}

int serial_received(void) {
    return inb(COM1_PORT + 5) & 1;
}

// nonblocking read: returns 0 if no data is available
char serial_read(void) {
    if (!serial_enabled) return 0; // don't read if serial is disabled
    if (serial_received())
        return inb(COM1_PORT);
    return 0;
}

int serial_is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_write(char a) {
    if (!serial_enabled) return; // don't write if serial is disabled
    while (!serial_is_transmit_empty())
        ; // wait for transmit buffer to empty
    outb(COM1_PORT, a);
}

void serial_write_string(const char* str) {
    while (*str)
        serial_write(*str++);
}

// simplified polling function without command processing
void serial_poll(void) {
    if (!serial_enabled) return; // don't poll if serial is disabled
    if (serial_received()) {
        char data = inb(COM1_PORT);
        // if some program is waiting or command processing is disabled,
        // pass the byte straight through via feedthru callback if available
        if (feedthru_callback != NULL) {
            feedthru_callback(data);
        }
    }
}

void serial_receive_file(const char* filename) {
    if (!filename || !*filename) {
        println("no filename provided.");
        return;
    }

    FIL file;
    if (f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        println("couldn't open file for writing.");
        return;
    }

    println("waiting for file transfer...");
    print("receiving: ");
    println(filename);

    UINT written;
    uint8_t buffer[128];  // chunked buffer
    size_t pos = 0;
    int idle = 0, total = 0;
    bool got_data = false;
    const int idle_threshold = 3000;  // ms of silence _after_ first byte

    while (1) {
        if (serial_received()) {
            char byte = inb(COM1_PORT);
            buffer[pos++] = byte;
            got_data = true;
            idle = 0;
            total++;

            if (pos == sizeof(buffer)) {
                // Write the current buffer to the file
                FRESULT res = f_write(&file, buffer, pos, &written);
                if (res != FR_OK || written != pos) {
                    println("Error writing to file. Transfer aborted.");
                    f_close(&file);
                    return;
                }
                pos = 0;  // reset buffer position
                serial_write('.');  // progress marker
            }
        } else {
            delay_ms(1);
            if (got_data) {
                idle++;
                if (idle > idle_threshold) {
                    // If there's any remaining data, write it
                    if (pos > 0) {
                        FRESULT res = f_write(&file, buffer, pos, &written);
                        if (res != FR_OK || written != pos) {
                            println("Error writing last chunk to file. Transfer aborted.");
                            f_close(&file);
                            return;
                        }
                    }
                    println("\ntransfer complete.");
                    break;
                }
            }
        }
    }

    f_close(&file);

    char msg[128];
    snprintf(msg, sizeof(msg), "saved: %s (%d bytes)\n", filename, total);
    println(msg);
}

void serial_send_file(const char* filename) {
    if (!filename || !*filename) {
        println("no filename provided.");
        return;
    }

    FIL file;
    if (f_open(&file, filename, FA_READ) != FR_OK) {
        println("failed to open file.");
        return;
    }

    println("sending file...");
    print("sending: ");
    println(filename);

    uint8_t buffer[128];
    UINT read;
    int total = 0;

    while (1) {
        if (f_read(&file, buffer, sizeof(buffer), &read) != FR_OK || read == 0)
            break;

        for (UINT i = 0; i < read; i++)
            serial_write(buffer[i]);

        total += read;
        serial_write('.');  // progress blip
    }

    f_close(&file);

    char msg[128];
    snprintf(msg, sizeof(msg), "\ndone sending %s (%d bytes)\n", filename, total);
    serial_write_string(msg);
}
