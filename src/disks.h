/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * disks.h
 */

#ifndef DISKS_H
#define DISKS_H

#include "ff.h"      // FatFs file system types
#include "diskio.h"  // Low-level disk I/O
#include <stddef.h>

// Max supported logical drives
#define MAX_LOGICAL_DRIVES 4
#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 256
#endif

extern char cwd[MAX_LOGICAL_DRIVES][MAX_PATH_LEN];
extern int current_drive;
extern size_t input_col_offset; // this must be global for keyboard.c to use

// Mount all available file systems (0: to 3:), pick first HDD as 0:, etc.
// Prints a message if no HDD is found.
void mount_all_filesystems(void);

// List files in a directory with paging (resolves relative or absolute paths)
void list_directory_with_paging(const char *path);

// List root directories of all mounted drives
void list_all_root_directories(void);

// (Legacy) List the current root directory of the default drive
void list_root_directory(void);

// Print the contents of a file (drive letter + path or relative path)
void print_file(const char *filename);

// Create or overwrite a file with given data
void create_and_write_file(const char *filename, const char *data);

// Delete a file
void delete_file(const char *filename);

// Check if a file exists (returns 1 if yes, 0 if no)
int file_exists(const char *filename);

// Rename a file (old and new paths accepted, with drive letters)
void rename_file(const char *old_name, const char *new_name);

// Format a drive (e.g. "0:", "2:") — WARNING: wipes all data
void format_disk(const char *drive_spec);

// Check disk usage for current_drive
void check_disk_usage(void);

// Clear screen function (used during paging)
void clear_screen(void);

// Wait for user input to continue (for paging prompts)
void wait_for_user_input(void);

// Change current directory (supports drive letters, absolute and relative)
FRESULT change_directory(const char *path);

// Return the current working directory (e.g. "0:/MYDIR/")
const char* get_current_directory(void);

int directory_exists(const char *path);
FRESULT make_directory(const char *path);
FRESULT list_directory_recursive(const char *path);
DWORD get_file_size(const char *path);
FRESULT copy_file(const char *src, const char *dst);
FRESULT append_to_file(const char *path, const char *data);

/* create an empty (zero‐length) file at `path`. 
   If it already exists, truncate it. */
FRESULT file_create(const char *path);

/* write `data` (a NUL‐terminated C string) into `path`, 
   overwriting any existing contents (or creating it). */
FRESULT file_write(const char *path, const char *data);

/* open an existing file at `path` with the given `mode`, returning 
   the FIL object (caller must f_close when done). */
FRESULT file_open(const char *path, FIL *fil, BYTE mode);

/* read up to `bufsize` bytes from `path` into `buffer`; 
   `*read` will be set to the actual number of bytes read. */
FRESULT file_read(const char *path, void *buffer, UINT bufsize, UINT *read);

extern uint8_t drive_present[MAX_DRIVES];

void get_full_path(const char *input, char *output);

size_t print_prompt(void);

void normalize_path(char *path);

#endif  // DISKS_H
