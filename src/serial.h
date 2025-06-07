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

// nonblocking poll function; always polls for serial input and either processes
// it or passes it straight through (depending on the feedthru callback)
void serial_poll(void);

// toggle the serial port on/off
void serial_toggle(void);

// Function to start receiving a file via the serial port
// This function listens for file data, saves the file to FAT32 using FATFS
void serial_receive_file(const char* filename);

// Function to send a file over the serial port
// This function reads the file and sends it byte by byte to the serial port
void serial_send_file(const char* filename);

#endif // SERIAL_H
