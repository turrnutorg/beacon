#ifndef DISKS_H
#define DISKS_H

#include "ff.h"  // For FatFs file system types
#include "diskio.h"  // For low-level disk I/O

// Function declarations

// Mount the filesystem
void mount_filesystem();

// List files in a directory with paging
void list_directory_with_paging(const char *path);

// List files in the root directory with paging
void list_root_directory();

// Print the contents of a file
void print_file(const char *filename);

// Create a file and write data to it
void create_and_write_file(const char *filename, const char *data);

// Delete a file
void delete_file(const char *filename);

// Check if a file exists
int file_exists(const char *filename);

// Rename a file
void rename_file(const char *old_name, const char *new_name);

// Format the disk (WARNING: This will erase all data)
void format_disk();

// Check disk usage (total space and free space)
void check_disk_usage();

// Clear screen function (implement in your console)
void clear_screen();

// Wait for user input to continue (used for paging)
void wait_for_user_input();

#endif  // DISKS_H
