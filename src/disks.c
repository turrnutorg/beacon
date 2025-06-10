/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * disks.c
 */

#include "disks.h"    // <-- include our own header first
#include "ff.h"
#include "diskio.h"
#include "stdlib.h"
#include "string.h"
#include "console.h"
#include "screen.h"
#include "keyboard.h"
#include <stdint.h>

// Global file system objects (one per logical drive)
static FATFS fs_array[MAX_LOGICAL_DRIVES];

// Store current directory for each logical drive (e.g., "0:/MYDIR/SUBDIR")
char cwd[MAX_LOGICAL_DRIVES][MAX_PATH_LEN];

// Track the active drive when user omits drive letter (default to 0)
int current_drive = 0;

// Number of lines per page for directory listing
#define LINES_PER_PAGE 25

// Indicates whether at least one drive was mounted
static int mounted_any = 0;

// Forward‐declare the low‐level probe function (must be implemented elsewhere)
extern void probe_all_ata_drives(void);

// Prototypes for internal helpers
static void init_cwd(void);

size_t input_col_offset = 0; // this must be global for keyboard.c to use

BYTE logical_to_physical[MAX_LOGICAL_DRIVES] = { 0xFF, 0xFF, 0xFF, 0xFF };

//------------------------------------------------------------
// Initialize the cwd[] array so each drive starts at "X:/"
// Call this exactly once at the very start of mount_all_filesystems()
//------------------------------------------------------------
static void init_cwd(void) {
    for (int i = 0; i < MAX_LOGICAL_DRIVES; i++) {
        snprintf(cwd[i], MAX_PATH_LEN, "%d:/", i);
    }
    current_drive = 0;
}

//------------------------------------------------------------
// Build an absolute path by combining cwd and user‐supplied path
// - If path starts with "X:/", use that drive directly.
// - Otherwise, use current_drive and append a relative path.
// 'out_path' must have room for MAX_PATH_LEN characters.
//------------------------------------------------------------
void get_full_path(const char *path, char *out_path) {
    int drv;
    const char *sub;

    if (path
        && path[0] >= '0'
        && path[0] <= '0' + (MAX_LOGICAL_DRIVES - 1)
        && path[1] == ':'
        && path[2] == '/') {
        drv = path[0] - '0';
        sub = path + 3;  // ← FIXED HERE
    } else {
        drv = current_drive;
        sub = path;
    }

    if (!sub || sub[0] == '\0') {
        strncpy(out_path, cwd[drv], MAX_PATH_LEN - 1);
        out_path[MAX_PATH_LEN - 1] = '\0';
        goto maybe_add_slash;
    }

    if (sub[0] == '/' || sub[0] == '\\') {
        snprintf(out_path, MAX_PATH_LEN, "%d:%s", drv, sub);
    } else {
        size_t len = strlen(cwd[drv]);
        char temp[MAX_PATH_LEN];

        strncpy(temp, cwd[drv], MAX_PATH_LEN - 1);
        temp[MAX_PATH_LEN - 1] = '\0';

        if (len > 3 && (temp[len - 1] == '/' || temp[len - 1] == '\\')) {
            temp[len - 1] = '\0';
        }

        snprintf(out_path, MAX_PATH_LEN, "%s/%s", temp, sub);
    }

    maybe_add_slash:{
        size_t len = strlen(out_path);
        if (len < MAX_PATH_LEN - 1 && out_path[len - 1] != '/') {
            out_path[len] = '/';
            out_path[len + 1] = '\0';
        }
    }
}

//------------------------------------------------------------
// Change current directory to 'path', updating cwd[drive] & current_drive.
// Supports:
//   "X:/DIR/SUB"  => absolute on drive X
//   "DIR/SUB"     => relative to current_drive
//   "X:" or "X:/" => reset to root of drive X
//------------------------------------------------------------
FRESULT change_directory(const char *path) {
    char full[MAX_PATH_LEN];
    int drv;

    // check if path explicitly specifies a drive letter
    if (path
        && path[0] >= '0'
        && path[0] <= '0' + (MAX_LOGICAL_DRIVES - 1)
        && path[1] == ':') {

        drv = path[0] - '0';

        // jump straight to drive root if it's just "X:" or "X:/"
        if (path[2] == '\0' || (path[2] == '/' && path[3] == '\0')) {
            snprintf(cwd[drv], MAX_PATH_LEN, "%d:/", drv);
            current_drive = drv;
            return FR_OK;
        }
    } else {
        drv = current_drive;
    }

    // build the full absolute path based on cwd and input
    get_full_path(path, full);

    // normalize ".", "..", etc
    normalize_path(full);

    // test if it's a real directory
    DIR dir;
    FRESULT res = f_opendir(&dir, full);
    if (res == FR_OK) {
        size_t l = strlen(full);
        if (l > 0 && full[l - 1] != '/') {
            strncat(full, "/", MAX_PATH_LEN - l - 1);
        }

        strncpy(cwd[drv], full, MAX_PATH_LEN - 1);
        cwd[drv][MAX_PATH_LEN - 1] = '\0';
        current_drive = drv;

        f_closedir(&dir);
    }

    return res;
}


//------------------------------------------------------------
// Return the current working directory string, e.g. "2:/MYDIR/"
//------------------------------------------------------------
const char* get_current_directory(void) {
    if (mounted_any) {
        return cwd[current_drive];
    } else {
        static char empty_path[] = "";
        return empty_path;
    }
}

//------------------------------------------------------------
// List files in a directory with paging. Resolves relative or absolute.
// 'path' examples:
//   NULL or ""   => list cwd[current_drive]
//   "SUBDIR"     => list cwd[current_drive]/SUBDIR
//   "1:/DIR"     => list 1:/DIR
//------------------------------------------------------------
void list_directory_with_paging(const char *path) {
    char full_path[MAX_PATH_LEN];

    if (!path || path[0] == '\0') {
        // No explicit path: use cwd of current_drive
        snprintf(full_path, MAX_PATH_LEN, "%s", cwd[current_drive]);
    } else {
        get_full_path(path, full_path);
    }

    DIR dir;
    FILINFO fno;
    FRESULT res = f_opendir(&dir, full_path);
    if (res != FR_OK) {
        print("Failed to open directory: ");
        println(full_path);
        return;
    }

    uint32_t line_count = 0;
    print("Listing directory: ");
    println(full_path);

    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break;

        if (fno.fattrib & AM_DIR) {
            print("[DIR]   ");
        } else {
            print("        ");
        }
        println(fno.fname);
        curs_row++;
        line_count++;

        if (line_count >= LINES_PER_PAGE) {
            getch();  // Wait for user input to continue
            print("Press any key to continue...");
            line_count = 0;
            clear_screen();
        }
    }

    f_closedir(&dir);
}

//------------------------------------------------------------
// List the root (cwd) directory on each mounted logical drive
// (calls list_directory_with_paging("X:/"))
//------------------------------------------------------------
void list_all_root_directories(void) {
    for (int i = 0; i < MAX_LOGICAL_DRIVES; i++) {
        char drv_root[4];
        snprintf(drv_root, sizeof(drv_root), "%d:/", i);
        print("----\nListing ");
        println(drv_root);
        list_directory_with_paging(drv_root);
    }
}

//------------------------------------------------------------
// Legacy: just list the cwd of the default drive
//------------------------------------------------------------
void list_root_directory(void) {
    list_directory_with_paging(NULL);
}

//------------------------------------------------------------
// Print the contents of a file, resolving drive letters or relative paths
//------------------------------------------------------------
void print_file(const char *filename) {
    char full_path[MAX_PATH_LEN];
    get_full_path(filename, full_path);

    FIL fil;
    FRESULT res = f_open(&fil, full_path, FA_READ);
    if (res != FR_OK) {
        print("Failed to open file: ");
        println(full_path);
        return;
    }

    print("Reading file: ");
    println(full_path);

    char buffer[128];
    unsigned int bytes_read;
    while (1) {
        res = f_read(&fil, buffer, sizeof(buffer) - 1, &bytes_read);
        if (res != FR_OK || bytes_read == 0) break;
        buffer[bytes_read] = '\0';
        println(buffer);
    }

    f_close(&fil);
    curs_row++;
}

//------------------------------------------------------------
// Create or overwrite a file with the given data
//------------------------------------------------------------
void create_and_write_file(const char *filename, const char *data) {
    char full_path[MAX_PATH_LEN];
    get_full_path(filename, full_path);

    FIL fil;
    FRESULT res = f_open(&fil, full_path, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        print("Failed to open or create file: ");
        println(full_path);
        return;
    }

    res = f_write(&fil, data, strlen(data), NULL);
    println("Data written successfully.");

    f_close(&fil);
}

//------------------------------------------------------------
// Delete a file (given a path or relative name)
//------------------------------------------------------------
void delete_file(const char *filename) {
    char full_path[MAX_PATH_LEN];
    get_full_path(filename, full_path);

    FRESULT res = f_unlink(full_path);
    if (res == FR_OK) {
        println("File deleted successfully.");
    } else {
        print("Failed to delete file: ");
        println(full_path);
    }
    curs_row++;
}

//------------------------------------------------------------
// Check if a file exists (returns 1 if yes, 0 if no)
//------------------------------------------------------------
int file_exists(const char *filename) {
    char full_path[MAX_PATH_LEN];
    get_full_path(filename, full_path);

    FIL fil;
    FRESULT res = f_open(&fil, full_path, FA_READ);
    if (res == FR_OK) {
        f_close(&fil);
        return 1;
    } else {
        return 0;
    }
}

//------------------------------------------------------------
// Rename a file from old_name to new_name (supports drive letters or relative)
//------------------------------------------------------------
void rename_file(const char *old_name, const char *new_name) {
    char full_old[MAX_PATH_LEN], full_new[MAX_PATH_LEN];
    get_full_path(old_name, full_old);
    get_full_path(new_name, full_new);

    FRESULT res = f_rename(full_old, full_new);
    if (res == FR_OK) {
        println("File renamed successfully.");
    } else {
        print("Failed to rename file: ");
        println(full_old);
    }
    curs_row++;
}

//------------------------------------------------------------
// Check disk usage for current_drive’s root
//------------------------------------------------------------
void check_disk_usage(void) {
    DWORD fre_clust, total_clust;
    FATFS *fs_ptr;
    char drv_root[3] = { '0' + current_drive, ':', '\0' };
    f_getfree(drv_root, &fre_clust, &fs_ptr);

    total_clust = fs_ptr->n_fatent - 2;
    DWORD total_size = total_clust * fs_ptr->csize * 512;
    DWORD free_size = fre_clust * fs_ptr->csize * 512;

    printf("Drive %d total space: %lu bytes\n", current_drive, total_size);
    printf("Drive %d free space: %lu bytes\n", current_drive, free_size);
}

//------------------------------------------------------------
// Format a disk (format a specific drive, e.g. "1:")
//------------------------------------------------------------
void format_disk(const char *drive_spec) {
    char drv_root[3] = { '0' + current_drive, ':', '\0' };
    if (drive_spec
        && drive_spec[0] >= '0'
        && drive_spec[0] <= '0' + (MAX_LOGICAL_DRIVES - 1)
        && drive_spec[1] == ':') {
        drv_root[0] = drive_spec[0];
    }

    MKFS_PARM opt = {
        .fmt = FM_FAT | FM_FAT32,
        .n_fat = 1,
        .align = 0,
        .n_root = 512,
        .au_size = 0
    };

    BYTE workbuf[FF_MAX_SS];

    println("formatting disk... (please be patient; this may take a while)");
    FRESULT res = f_mkfs(drv_root, &opt, workbuf, sizeof(workbuf));
    if (res == FR_OK) {
        println("disk formatted successfully.");
        f_mount(NULL, drv_root, 1);  // unmount just to be safe
        f_mount(&fs_array[current_drive], drv_root, 1);  // remount
    } else {
        print("disk format failed. error code: ");
        printf("%d\n", res);
    }
}


//------------------------------------------------------------
// Mount all filesystems (detect drives, mount each in order).
// The first HDD found becomes logical "0:", next is "1:", etc.
// If none found, prints a message and waits for a key.
//------------------------------------------------------------
void mount_all_filesystems(void) {
    init_cwd();  // reset working dirs
    probe_all_ata_drives();  // detect drives
    mounted_any = 0;
    int logical_index = 0;

    for (int pdrv = 0; pdrv < MAX_DRIVES; pdrv++) {
        if (!drive_present[pdrv]) continue;

        // build logical name like "0:"
        TCHAR path[3] = { (TCHAR)('0' + logical_index), ':', '\0' };
        logical_to_physical[logical_index] = (BYTE)pdrv;

        FRESULT res = f_mount(&fs_array[logical_index], path, 1);
        if (res == FR_OK) {
            print("Mounted logical drive ");
            print(path);
            print(" from physical drive ");
            switch (pdrv) {
                case 0: println("(primary master)"); break;
                case 1: println("(primary slave)"); break;
                case 2: println("(secondary master)"); break;
                case 3: println("(secondary slave)"); break;
                default: println("(unknown?)"); break;
            }

            if (!mounted_any) {
                current_drive = logical_index;
                snprintf(cwd[logical_index], MAX_PATH_LEN, "%s/", path);
            }

            mounted_any = 1;
            logical_index++;
        } else {
            print("Failed to mount physical drive ");
            putchar('0' + pdrv);
            println("");
        }
    }

    if (!mounted_any) {
        println("No available hard drives found. Press any key to continue.");
        getch();
    }
}

//------------------------------------------------------------
// File operations for ease of use
//------------------------------------------------------------

//------------------------------------------------------------
// Create an empty (zero‐length) file (or truncate if it exists)
//------------------------------------------------------------
FRESULT file_create(const char *path) {
    char full_path[MAX_PATH_LEN];
    get_full_path(path, full_path);

    FIL fil;
    /* FA_WRITE | FA_CREATE_ALWAYS => create new or overwrite */
    FRESULT res = f_open(&fil, full_path, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        return res; /* could not create/truncate */
    }

    /* no need to write anything; just close to leave zero length */
    f_close(&fil);
    return FR_OK;
}

//------------------------------------------------------------
// Write a NUL‐terminated string into a file (overwrite or create)
//------------------------------------------------------------
FRESULT file_write(const char *path, const char *data) {
    char full_path[MAX_PATH_LEN];
    get_full_path(path, full_path);

    FIL fil;
    /* FA_WRITE | FA_CREATE_ALWAYS => overwrite or create anew */
    FRESULT res = f_open(&fil, full_path, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        return res; /* open/create failed */
    }

    UINT bw;
    res = f_write(&fil, data, (UINT)strlen(data), &bw);
    if (res != FR_OK) {
        f_close(&fil);
        return res; /* write‐error */
    }
    /* You could check “if (bw < strlen(data))” for a partial‐write, but
       in most builds of FatFs that either writes everything or fails. */

    f_close(&fil);
    return FR_OK;
}

//------------------------------------------------------------
// Open an existing file at `path` with FatFs mode “mode”.
// Caller must call f_close() on `fil` when done.
// e.g. mode = FA_READ, or FA_READ|FA_WRITE, etc.
//------------------------------------------------------------
FRESULT file_open(const char *path, FIL *fil, BYTE mode) {
    char full_path[MAX_PATH_LEN];
    get_full_path(path, full_path);

    /* simply wrap f_open; caller is responsible for f_close */
    return f_open(fil, full_path, mode);
}

//------------------------------------------------------------
// Read up to bufsize bytes from `path` into `buffer`. 
// *read will be set to the actual bytes read.
//------------------------------------------------------------
FRESULT file_read(const char *path, void *buffer, UINT bufsize, UINT *read) {
    char full_path[MAX_PATH_LEN];
    get_full_path(path, full_path);

    FIL fil;
    FRESULT res = f_open(&fil, full_path, FA_READ);
    if (res != FR_OK) {
        return res; /* could not open */
    }

    res = f_read(&fil, buffer, bufsize, read);
    f_close(&fil);
    return res;
}

int directory_exists(const char *path) {
    char full[MAX_PATH_LEN];
    get_full_path(path, full);

    DIR dir;
    FRESULT res = f_opendir(&dir, full);
    if (res == FR_OK) {
        f_closedir(&dir);
        return 1;
    }
    return 0;
}

FRESULT make_directory(const char *path) {
    char full[MAX_PATH_LEN];
    get_full_path(path, full);
    return f_mkdir(full);
}

FRESULT list_directory_recursive(const char *path) {
    char full_path[MAX_PATH_LEN];
    get_full_path(path, full_path);

    DIR dir;
    FILINFO fno;
    FRESULT res = f_opendir(&dir, full_path);
    if (res != FR_OK) {
        print("Failed to open directory: ");
        println(full_path);
        return res;
    }

    print("Recursive listing of ");
    println(full_path);

    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break;

        char entry_path[MAX_PATH_LEN];
        snprintf(entry_path, MAX_PATH_LEN, "%s/%s", full_path, fno.fname);

        if (fno.fattrib & AM_DIR) {
            print("[DIR] ");
            println(entry_path);
            // recursive call
            if (strcmp(fno.fname, ".") && strcmp(fno.fname, "..")) {
                list_directory_recursive(entry_path);
            }
        } else {
            print("      ");
            println(entry_path);
        }
    }

    f_closedir(&dir);
    return FR_OK;
}

DWORD get_file_size(const char *path) {
    char full[MAX_PATH_LEN];
    get_full_path(path, full);

    FIL fil;
    FRESULT res = f_open(&fil, full, FA_READ);
    if (res != FR_OK) return 0;

    DWORD size = f_size(&fil);
    f_close(&fil);
    return size;
}

FRESULT copy_file(const char *src, const char *dst) {
    char full_src[MAX_PATH_LEN];
    char full_dst[MAX_PATH_LEN];
    get_full_path(src, full_src);
    get_full_path(dst, full_dst);

    FIL fsrc, fdst;
    FRESULT res = f_open(&fsrc, full_src, FA_READ);
    if (res != FR_OK) return res;

    res = f_open(&fdst, full_dst, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        f_close(&fsrc);
        return res;
    }

    BYTE buffer[512];
    UINT br, bw;
    while (1) {
        res = f_read(&fsrc, buffer, sizeof(buffer), &br);
        if (res != FR_OK || br == 0) break;

        res = f_write(&fdst, buffer, br, &bw);
        if (res != FR_OK || bw < br) break;
    }

    f_close(&fsrc);
    f_close(&fdst);
    return res;
}

FRESULT append_to_file(const char *path, const char *data) {
    char full[MAX_PATH_LEN];
    get_full_path(path, full);

    FIL fil;
    FRESULT res = f_open(&fil, full, FA_WRITE | FA_OPEN_ALWAYS);
    if (res != FR_OK) return res;

    // move to end
    res = f_lseek(&fil, f_size(&fil));
    if (res != FR_OK) {
        f_close(&fil);
        return res;
    }

    UINT bw;
    res = f_write(&fil, data, strlen(data), &bw);
    f_close(&fil);
    return res;
}

size_t print_prompt() {
    const char* dir = get_current_directory(); // e.g. "0:/"
    print(dir);
    print(">");
    input_col_offset = strlen(dir) + 1;
    curs_col = input_col_offset;
    curs_row = row;
    return input_col_offset;
}

void normalize_path(char *path) {
    char temp[MAX_PATH_LEN];
    char *tokens[64];
    int depth = 0;

    // tokenize the path
    char *tok = strtok(path + 2, "/");  // skip drive (e.g., "0:/")
    while (tok && depth < 64) {
        if (strcmp(tok, "..") == 0) {
            if (depth > 0) depth--;
        } else if (strcmp(tok, ".") != 0 && strlen(tok) > 0) {
            tokens[depth++] = tok;
        }
        tok = strtok(NULL, "/");
    }

    // rebuild path
    snprintf(temp, sizeof(temp), "%c:/", path[0]);
    for (int i = 0; i < depth; i++) {
        strcat(temp, tokens[i]);
        if (i != depth - 1) strcat(temp, "/");
    }

    if (temp[strlen(temp) - 1] != '/') {
        strcat(temp, "/");
    }

    strncpy(path, temp, MAX_PATH_LEN - 1);
    path[MAX_PATH_LEN - 1] = '\0';
}
