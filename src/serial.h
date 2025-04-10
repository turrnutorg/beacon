#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h> // because you're not a total moron (hopefully)

// typedef for the feedthru callback function
typedef void (*serial_feedthru_callback_t)(char);

// initialize the serial port
void serial_init(void);

// check if a byte has been received; returns nonzero if data is available
int serial_received(void);

// perform a nonblocking read; returns 0 if no data available
char serial_read(void);

// check if the transmit buffer is empty; returns nonzero if it is
int serial_is_transmit_empty(void);

// write a single byte to the serial port
void serial_write(char a);

// write a null-terminated string to the serial port
void serial_write_string(const char* str);

// set the flag to enable command processing (nonzero to process commands)
void set_serial_command(int enabled);

// set the serial waiting flag indicating that some program is waiting on serial input
void set_serial_waiting(int waiting);

// register a feedthru callback for passing input directly when commands are disabled
void set_serial_feedthru_callback(serial_feedthru_callback_t callback);

// nonblocking poll function; always polls for serial input and either processes
// it as a command (if enabled) or passes it straight through
void serial_poll(void);

// toggle the serial port on/off
void serial_toggle(void);

// 💾 load a .csa file over serial, interpret header, store binary into memory
void start_csa_load(void);

// 🚀 run previously loaded .csa by jumping to its entrypoint
void run_csa_entry(void);

#endif // SERIAL_H
