#include "serial.h"
#include "port.h"
#include "stdtypes.h"
#include "string.h"
#include "command.h"
#include <stddef.h>

#define SERIAL_INPUT_BUFFER_SIZE 256

// buffer tae store serial input for command processing
static char serial_input_buffer[SERIAL_INPUT_BUFFER_SIZE];
static int serial_input_index = 0;

#define COM1_PORT 0x3F8

// global flags: set serial_command_enabled to 1 tae process commands,
// set it tae 0 tae bypass command processing.
// serial_waiting flag tells the main loop that some program is waiting on serial input.
static int serial_command_enabled = 1;
static int serial_waiting = 0;

// pointer for feedthru callback for any waiting process
typedef void (*serial_feedthru_callback_t)(char);
static serial_feedthru_callback_t feedthru_callback = NULL;

// functions tae set flags and the feedthru callback
void set_serial_command(int enabled) {
    serial_command_enabled = enabled;
}

void set_serial_waiting(int waiting) {
    serial_waiting = waiting;
}

void set_serial_feedthru_callback(serial_feedthru_callback_t callback) {
    feedthru_callback = callback;
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
    if (serial_received())
        return inb(COM1_PORT);
    return 0;
}

int serial_is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_write(char a) {
    while (!serial_is_transmit_empty())
        ; // wait for transmit buffer to empty
    outb(COM1_PORT, a);
}

void serial_write_string(const char* str) {
    serial_write('\r'); // move to the start of the line
    serial_write('\n'); // move to the next line
    while (*str)
        serial_write(*str++);
}

// this callback processes serial input as command line input
void serial_command_handler(char data) {
    if (data == '\n' || data == '\r') {
        serial_input_buffer[serial_input_index] = '\0'; // null-terminate
        if (serial_input_index > 0) {
            process_command(serial_input_buffer);
        }
        serial_input_index = 0;
        serial_write_string("\r\n> ");
        return;
    }
    if (data == '\b' || data == 127) {
        if (serial_input_index > 0) {
            serial_input_index--;
            serial_write_string("\b \b");
        }
        return;
    }
    if (serial_input_index < SERIAL_INPUT_BUFFER_SIZE - 1) {
        serial_write(data);
        serial_input_buffer[serial_input_index++] = data;
    }
}

// call this in yer main loop; it nonblockingly polls for serial input always
void serial_poll(void) {
    if (serial_received()) {
        char data = inb(COM1_PORT);
        // if some program is waiting or command processing is disabled,
        // pass the byte straight through via feedthru callback if available
        if (serial_waiting || !serial_command_enabled) {
            if (feedthru_callback != NULL) {
                feedthru_callback(data);
            }
        } else {
            // otherwise process it as a command
            serial_command_handler(data);
        }
    }
}
