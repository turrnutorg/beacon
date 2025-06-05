/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "port.h"		/* Port specific functions */
#include <stdint.h>

/* ATA Register I/O Base Address for Primary Channel */
#define ATA_IO_BASE 0x1F0
#define ATA_CTRL_BASE 0x3F6

/* ATA Command Registers */
#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30
#define ATA_CMD_IDENTIFY 0xEC

/* Status Register Flags */
#define ATA_STATUS_BUSY 0x80
#define ATA_STATUS_READY 0x40
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_ERROR 0x01

/* Command Register */
#define ATA_CMD_PORT (ATA_IO_BASE + 7)

/* Definitions of physical drive number for each drive */
#define DEV_ATA      0   /* Map ATA to physical drive 0 */

/* Wait for the ATA controller to be ready */
static void wait_for_ready() {
    while (inb(ATA_IO_BASE + 7) & ATA_STATUS_BUSY);
}

/* Read data from ATA port (256 bytes) */
static void ata_read_data(uint16_t *buffer) {
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(ATA_IO_BASE);
    }
}

/* Write data to ATA port (256 bytes) */
static void ata_write_data(const uint16_t *buffer) {
    for (int i = 0; i < 256; i++) {
        outw(ATA_IO_BASE, buffer[i]);
    }
}

/* Send a command to the ATA drive */
static void ata_send_command(uint8_t command) {
    outb(ATA_CMD_PORT, command);
}

/* Read status and handle errors */
static uint8_t ata_read_status() {
    return inb(ATA_IO_BASE + 7);
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != DEV_ATA) return STA_NOINIT;

    uint8_t status = ata_read_status();
    if (status & ATA_STATUS_BUSY) return STA_NOINIT;  // Busy, cannot initialize.

    return 0;  // Ready
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                     */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != DEV_ATA) return STA_NOINIT;

    // Identify the disk with an IDENTIFY command
    ata_send_command(ATA_CMD_IDENTIFY);
    wait_for_ready();

    uint8_t status = ata_read_status();
    if (status & ATA_STATUS_ERROR) return STA_NOINIT; // Error during IDENTIFY.

    return 0;  // Drive ready.
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != DEV_ATA) return RES_PARERR;

    // Select the disk and prepare for read
    outb(ATA_IO_BASE + 6, 0xE0 | ((sector >> 24) & 0x0F));  // LBA mode

    // Set the sector and execute read
    outb(ATA_IO_BASE + 2, 0x01);  // Number of sectors to read
    outb(ATA_IO_BASE + 3, (sector & 0xFF));  // LSB of sector
    outb(ATA_IO_BASE + 4, ((sector >> 8) & 0xFF));  // Mid byte of sector
    outb(ATA_IO_BASE + 5, ((sector >> 16) & 0xFF));  // MSB of sector

    // Send the command
    ata_send_command(ATA_CMD_READ);
    wait_for_ready();

    // If we got the correct DRQ (Data Request) flag, start reading
    uint8_t status = ata_read_status();
    if (!(status & ATA_STATUS_DRQ)) return RES_ERROR;  // No Data Request

    // Read the data into buffer
    ata_read_data((uint16_t *)buff);

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != DEV_ATA) return RES_PARERR;

    // Select the disk and prepare for write
    outb(ATA_IO_BASE + 6, 0xE0 | ((sector >> 24) & 0x0F));  // LBA mode

    // Set the sector and execute write
    outb(ATA_IO_BASE + 2, 0x01);  // Number of sectors to write
    outb(ATA_IO_BASE + 3, (sector & 0xFF));  // LSB of sector
    outb(ATA_IO_BASE + 4, ((sector >> 8) & 0xFF));  // Mid byte of sector
    outb(ATA_IO_BASE + 5, ((sector >> 16) & 0xFF));  // MSB of sector

    // Send the write command
    ata_send_command(ATA_CMD_WRITE);
    wait_for_ready();

    // If we got the correct DRQ (Data Request) flag, start writing
    uint8_t status = ata_read_status();
    if (!(status & ATA_STATUS_DRQ)) return RES_ERROR;  // No Data Request

    // Write the data from buffer
    ata_write_data((const uint16_t *)buff);

    return RES_OK;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    DRESULT res = RES_OK;

    switch (cmd) {
        case CTRL_SYNC:
            // If there's anything to sync, do it here (not needed for ATA)
            break;
        case GET_SECTOR_COUNT:
            *(DWORD*)buff = 0;  // Return the total number of sectors (if known)
            break;
        case GET_SECTOR_SIZE:
            *(WORD*)buff = 512;  // Standard for ATA
            break;
        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 1;  // Standard for ATA (no block abstraction)
            break;
        default:
            res = RES_PARERR;
            break;
    }

    return res;
}

DWORD get_fattime(void) {
    return 0;  // Just return 0, it will not affect your functionality right now
}


