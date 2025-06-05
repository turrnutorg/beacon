#include "ff.h"
#include "diskio.h"
#include "stdlib.h"
#include "string.h"
#include "console.h"
#include "screen.h"
#include "keyboard.h"

// Global file system object
extern FATFS fs;     // FatFs file system object

#define LINES_PER_PAGE 25  // The number of lines to show on one screen (adjust based on your screen size)

void clear_screen(); // Clear screen function (implement it in your console if not already)

void wait_for_user_input() {
    uint8_t scancode;
    println("Press any key to continue...");
    while (!buffer_get(&scancode)) {
        // Wait for user input (getch)
    }
    // Consume the keypress so we can go on with the next page
}

// Function to list files in a directory with paging
void list_directory_with_paging(const char *path) {
    DIR dir;
    FILINFO fno;
    FRESULT res;

    res = f_opendir(&dir, path);  // Open directory
    if (res != FR_OK) {
        println("Failed to open directory.");
        return;
    }

    uint32_t line_count = 0;
    println("Listing directory:");
    
    while (1) {
        res = f_readdir(&dir, &fno);  // Read entry
        if (res != FR_OK || fno.fname[0] == 0) break;  // End of directory or error

        // Display files and directories
        if (fno.fattrib & AM_DIR) {
            print("[DIR]   ");
        } else {
            print("        ");
        }

        // Print the file/folder name
        println(fno.fname);
        curs_row++;  // Move to the next line
        line_count++;

        // If we've reached the max lines per page, wait for user input
        if (line_count >= LINES_PER_PAGE) {
            wait_for_user_input();
            line_count = 0;
            clear_screen();  // Clear screen for next batch of files
        }
    }
    f_closedir(&dir);  // Close the directory after listing
}

// Function to list the root directory with paging
void list_root_directory() {
    list_directory_with_paging("/");  // List files in the root directory
}

// Function to print a file's contents
void print_file(const char *filename) {
    FIL fil;
    FRESULT res;
    char buffer[128];  // Buffer to hold file data
    unsigned int bytes_read;

    res = f_open(&fil, filename, FA_READ);  // Open the file
    if (res != FR_OK) {
        println("Failed to open file.");
        return;
    }

    println("Reading file:");
    while (1) {
        res = f_read(&fil, buffer, sizeof(buffer) - 1, &bytes_read);  // Read the file
        if (res != FR_OK || bytes_read == 0) break;

        buffer[bytes_read] = '\0';  // Null-terminate string
        println(buffer);  // Print file data
    }

    f_close(&fil);  // Close the file after reading
    curs_row++;  // Move to the next line after printing
}

// Function to create and write some data to a file
void create_and_write_file(const char *filename, const char *data) {
    FIL fil;
    FRESULT res;

    res = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);  // Open or create file
    if (res != FR_OK) {
        println("Failed to open or create file.");
        return;
    }

    res = f_write(&fil, data, strlen(data), NULL);  // Write data to the file
    println("Data written successfully.");

    f_close(&fil);  // Close the file after writing
}

// Function to delete a file
void delete_file(const char *filename) {
    FRESULT res = f_unlink(filename);  // Delete the file
    if (res == FR_OK) {
        println("File deleted successfully.");
    } else {
        println("Failed to delete file.");
    }
    curs_row++;  // Move to the next line after deletion
}

// Function to check if a file exists
int file_exists(const char *filename) {
    FIL fil;
    FRESULT res = f_open(&fil, filename, FA_READ);  // Try to open the file for reading
    if (res == FR_OK) {
        f_close(&fil);  // Close if found
        return 1;  // File exists
    } else {
        return 0;  // File does not exist
    }
}

// Function to rename a file
void rename_file(const char *old_name, const char *new_name) {
    FRESULT res = f_rename(old_name, new_name);  // Rename the file
    if (res == FR_OK) {
        println("File renamed successfully.");
    } else {
        println("Failed to rename file.");
    }
    curs_row++;  // Move to the next line after renaming
}

// Function to check disk usage
void check_disk_usage() {
    DWORD fre_clust, total_clust;
    FATFS *fs;
    f_getfree("", &fre_clust, &fs);
    total_clust = fs->n_fatent - 2; // Total number of clusters
    DWORD total_size = total_clust * fs->csize * 512;  // Size in bytes
    DWORD free_size = fre_clust * fs->csize * 512;  // Free space in bytes

    printf("Total space: %lu bytes\n", total_size);
    printf("Free space: %lu bytes\n", free_size);
}

void format_disk() {
    FRESULT res;

    // Perform the format with default options (NULL) and no working buffer
    res = f_mkfs("", 0, NULL, 0);  // Format the disk (empty string for root path)

    if (res == FR_OK) {
        println("Disk formatted successfully.");
    } else {
        println("Disk format failed.");
    }
}

// Function to mount the filesystem (called in start or initialization)
void mount_filesystem() {
    FRESULT res = f_mount(&fs, "", 1);  // Mount filesystem at root
    if (res != FR_OK) {
        println("Failed to mount filesystem.");
        getch();  // Wait for user input before continuing
    } else {
        println("Filesystem mounted.");
    }
}
