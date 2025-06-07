/*-----------------------------------------------------------------------*/
/* low level disk I/O module for FatFs   (c)Chan, 2019, rewrit by a daft git */
/*-----------------------------------------------------------------------*/
/* scans all 4 classic ATA positions (pri/sec × mstr/slv),
   uses delay_ms, and uses proper LBA head select (0xE0/0xF0) */
/*-----------------------------------------------------------------------*/

#include "ff.h"      /* obtains integer types */
#include "diskio.h"  /* declarations of disk functions */
#include "port.h"    /* port specific functions: inb, outb, inw, outw, delay_ms */
#include <stdint.h>
#include <stdio.h>   /* for debug printing, if available */
#include "time.h"    /* for prototype of delay_ms if needed */

/* constants for channels and drives */

/* physical I/O base addresses */
static const uint16_t ATA_IO_BASES[2]   = { 0x1F0, 0x170 };
static const uint16_t ATA_CTRL_BASES[2] = { 0x3F6, 0x376 };

/* drive select bits (master/slave) with LBA bit set */
static const uint8_t ATA_DRIVE_SEL[2] = { 0xE0, 0xF0 };  /* 0: master (bit7=1,bit6=1,bit5=0), 1: slave (bit7=1,bit6=1,bit5=1) */

/* ATA Commands */
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_READ     0x20
#define ATA_CMD_WRITE    0x30

/* status flags */
#define ATA_STATUS_BUSY  0x80
#define ATA_STATUS_DRQ   0x08
#define ATA_STATUS_ERR   0x01

/* array to remember which drives are actually present (physical indices) */
uint8_t drive_present[MAX_DRIVES] = { 0, 0, 0, 0 };

/* store total sectors for each drive (physical indices) */
static DWORD total_sectors[MAX_DRIVES]  = { 0, 0, 0, 0 };

/* mapping from logical (0..MAX_DRIVES-1) to physical (0..3) */
extern BYTE logical_to_physical[MAX_DRIVES];

/*-----------------------------------------------------------------------*/
/* low-level I/O helpers for physical pdrv  (0..3)                       */
/*-----------------------------------------------------------------------*/

static inline uint8_t ata_read_status(uint16_t io_base) {
    return inb(io_base + 7);
}

/* alternate status port read (avoids clearing pending IRQ) */
static inline uint8_t ata_read_alt_status(uint16_t ctrl_base) {
    return inb(ctrl_base);
}

static inline void ata_send_command(uint16_t io_base, uint8_t cmd) {
    outb(io_base + 7, cmd);
}

/* wait until BSY clears or timeout, checking alternate status port */
static int wait_for_bsy_clear(uint16_t ctrl_base) {
    uint8_t status;
    for (int t = 0; t < 100000; t++) {
        status = ata_read_alt_status(ctrl_base);
        if (!(status & ATA_STATUS_BUSY)) {
            return status;  /* return final status */
        }
    }
    return 0xFF;  /* timed out */
}

/* wait until DRQ sets or timeout */
static int wait_for_drq_set(uint16_t io_base) {
    uint8_t status;
    for (int t = 0; t < 100000; t++) {
        status = ata_read_status(io_base);
        if (status & ATA_STATUS_DRQ) {
            return 0;  /* DRQ is set */
        }
        if (status & ATA_STATUS_ERR) {
            return -1; /* error */
        }
    }
    return -1; /* timed out */
}

/* read 256 words (512 bytes) from data port */
static void ata_read_data(uint16_t io_base, uint16_t *buf) {
    for (int i = 0; i < 256; i++) {
        buf[i] = inw(io_base);
    }
}

/* write 256 words (512 bytes) to data port */
static void ata_write_data(uint16_t io_base, const uint16_t *buf) {
    for (int i = 0; i < 256; i++) {
        outw(io_base, buf[i]);
    }
}

/*-----------------------------------------------------------------------*/
/* translate physical pdrv (0..3) to channel and drive select bits       */
/*-----------------------------------------------------------------------*/
static void pdrv_to_ata(uint8_t pdrv, uint16_t *io_base, uint16_t *ctrl_base, uint8_t *drive_sel) {
    uint8_t channel = (pdrv / 2) & 0x01;      /* 0 = primary, 1 = secondary */
    uint8_t drv     = (pdrv % 2) & 0x01;      /* 0 = master, 1 = slave */
    *io_base   = ATA_IO_BASES[channel];
    *ctrl_base = ATA_CTRL_BASES[channel];
    *drive_sel = ATA_DRIVE_SEL[drv];
}

/*-----------------------------------------------------------------------*/
/* Debug logging (optional)                                              */
/*-----------------------------------------------------------------------*/
#ifdef DEBUG
#define DBG_PRINTF(fmt, ...) printf("[ATA DEBUG] " fmt, ##__VA_ARGS__)
#else
#define DBG_PRINTF(fmt, ...) /* no-op */
#endif

/*-----------------------------------------------------------------------*/
/* Physical drive initialize (0..3) – returns STA_NOINIT or 0           */
/*-----------------------------------------------------------------------*/
static DSTATUS phys_disk_initialize(BYTE pdrv) {
    if (pdrv >= MAX_DRIVES) return STA_NOINIT;

    uint16_t io_base, ctrl_base;
    uint8_t drive_sel;
    pdrv_to_ata(pdrv, &io_base, &ctrl_base, &drive_sel);

    DBG_PRINTF("phys init pdrv %d (I/O=0x%X, CTRL=0x%X, SEL=0x%X)\n", pdrv, io_base, ctrl_base, drive_sel);

    /* reset Device Control register: de-assert SRST, enable IRQs */
    outb(ctrl_base, 0x00);
    delay_ms(50);  /* let control port settle */

    /* select drive (master/slave) with LBA bit set */
    outb(io_base + 6, drive_sel);
    delay_ms(1000);  /* give real hardware time to spin up (1s) */

    /* clear LBA registers */
    outb(io_base + 2, 0x00);
    outb(io_base + 3, 0x00);
    outb(io_base + 4, 0x00);
    outb(io_base + 5, 0x00);

    /* send IDENTIFY */
    ata_send_command(io_base, ATA_CMD_IDENTIFY);

    /* wait for BSY clear using alternate status port */
    uint8_t status = wait_for_bsy_clear(ctrl_base);
    if (status == 0xFF) {
        /* timed out or no drive */
        drive_present[pdrv] = 0;
        total_sectors[pdrv] = 0;
        DBG_PRINTF("pdrv %d: no response or timeout\n", pdrv);
        return STA_NOINIT;
    }

    /* check if ERROR */
    if (status & ATA_STATUS_ERR) {
        drive_present[pdrv] = 0;
        total_sectors[pdrv] = 0;
        DBG_PRINTF("pdrv %d: IDENTIFY reported error (status=0x%X)\n", pdrv, status);
        return STA_NOINIT;
    }

    /* check for ATAPI signature: read LBA mid/high registers */
    {
        uint8_t lbamid  = inb(io_base + 4);
        uint8_t lbahigh = inb(io_base + 5);
        DBG_PRINTF("pdrv %d: LBAMID=0x%X, LBAHIGH=0x%X\n", pdrv, lbamid, lbahigh);
        /* ATAPI devices set LBAMID=0x14, LBAHIGH=0xEB or 0x69/0x96 */
        if ((lbamid == 0x14 && lbahigh == 0xEB) ||
            (lbamid == 0x69 && lbahigh == 0x96)) {
            /* atapi or packet device, skip it */
            drive_present[pdrv] = 0;
            total_sectors[pdrv] = 0;
            DBG_PRINTF("pdrv %d: ATAPI detected or unsupported device\n", pdrv);
            return STA_NOINIT;
        }
    }

    /* wait for DRQ */
    if (!(status & ATA_STATUS_DRQ)) {
        /* sometimes need to poll again for DRQ */
        if (wait_for_drq_set(io_base) < 0) {
            drive_present[pdrv] = 0;
            total_sectors[pdrv] = 0;
            DBG_PRINTF("pdrv %d: DRQ never set\n", pdrv);
            return STA_NOINIT;
        }
    }

    /* read IDENTIFY data */
    uint16_t identify_buf[256];
    ata_read_data(io_base, identify_buf);

    /* extract total 28-bit sectors from words 60 and 61 */
    uint32_t low  = identify_buf[60];
    uint32_t high = identify_buf[61];
    total_sectors[pdrv] = (high << 16) | low;

    /* mark drive present */
    drive_present[pdrv] = 1;
    DBG_PRINTF("pdrv %d: present, total sectors = %lu\n", pdrv, total_sectors[pdrv]);
    return 0;  /* success */
}

/*-----------------------------------------------------------------------*/
/* Physical drive status (0..3)                                         */
/*-----------------------------------------------------------------------*/
static DSTATUS phys_disk_status(BYTE pdrv) {
    if (pdrv >= MAX_DRIVES) return STA_NOINIT;
    if (!drive_present[pdrv]) return STA_NOINIT;
    return 0;  /* drive ready */
}

/*-----------------------------------------------------------------------*/
/* Physical Read Sector(s)                                                */
/*-----------------------------------------------------------------------*/
static DRESULT phys_disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv >= MAX_DRIVES)      return RES_PARERR;
    if (!drive_present[pdrv])    return RES_NOTRDY;
    if (count == 0)              return RES_PARERR;

    uint16_t io_base, ctrl_base;
    uint8_t drive_sel;
    pdrv_to_ata(pdrv, &io_base, &ctrl_base, &drive_sel);

    for (UINT i = 0; i < count; i++) {
        LBA_t current_sector = sector + i;

        /* de-assert SRST, enable IRQs before every operation */
        outb(ctrl_base, 0x00);
        delay_ms(10);

        /* select drive + head bits (LBA high 4 bits) */
        uint8_t head_byte = drive_sel | (uint8_t)((current_sector >> 24) & 0x0F);
        outb(io_base + 6, head_byte);
        delay_ms(500);  /* let the drive process head select */

        /* send sector count = 1 */
        outb(io_base + 2, 0x01);

        /* set LBA low/mid/high */
        outb(io_base + 3, (uint8_t)(current_sector & 0xFF));          /* LBA bits 0..7 */
        outb(io_base + 4, (uint8_t)((current_sector >> 8) & 0xFF));   /* LBA bits 8..15 */
        outb(io_base + 5, (uint8_t)((current_sector >> 16) & 0xFF));  /* LBA bits 16..23 */

        /* send READ command */
        ata_send_command(io_base, ATA_CMD_READ);

        /* wait for BSY clear */
        uint8_t status2 = wait_for_bsy_clear(ctrl_base);
        if (status2 == 0xFF) {
            DBG_PRINTF("pdrv %d: read sector %lu timed out on BSY\n", pdrv, current_sector);
            return RES_ERROR;
        }

        /* wait for DRQ */
        if (wait_for_drq_set(io_base) < 0) {
            DBG_PRINTF("pdrv %d: read sector %lu DRQ error\n", pdrv, current_sector);
            return RES_ERROR;
        }

        /* read one sector (512 bytes) */
        ata_read_data(io_base, (uint16_t *)(buff + (i * 512)));
    }

    return RES_OK;
}

#if FF_FS_READONLY == 0
/*-----------------------------------------------------------------------*/
/* Physical Write Sector(s)                                               */
/*-----------------------------------------------------------------------*/
static DRESULT phys_disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv >= MAX_DRIVES)      return RES_PARERR;
    if (!drive_present[pdrv])    return RES_NOTRDY;
    if (count == 0)              return RES_PARERR;

    uint16_t io_base, ctrl_base;
    uint8_t drive_sel;
    pdrv_to_ata(pdrv, &io_base, &ctrl_base, &drive_sel);

    for (UINT i = 0; i < count; i++) {
        LBA_t current_sector = sector + i;

        /* de-assert SRST, enable IRQs before every operation */
        outb(ctrl_base, 0x00);
        delay_ms(10);

        /* select drive + head bits (LBA high 4 bits) */
        uint8_t head_byte = drive_sel | (uint8_t)((current_sector >> 24) & 0x0F);
        outb(io_base + 6, head_byte);
        delay_ms(500);  /* let the drive process head select */

        /* send sector count = 1 */
        outb(io_base + 2, 0x01);

        /* set LBA low/mid/high */
        outb(io_base + 3, (uint8_t)(current_sector & 0xFF));
        outb(io_base + 4, (uint8_t)((current_sector >> 8) & 0xFF));
        outb(io_base + 5, (uint8_t)((current_sector >> 16) & 0xFF));

        /* send WRITE command */
        ata_send_command(io_base, ATA_CMD_WRITE);

        /* wait for BSY clear */
        uint8_t status2 = wait_for_bsy_clear(ctrl_base);
        if (status2 == 0xFF) {
            DBG_PRINTF("pdrv %d: write sector %lu timed out on BSY\n", pdrv, current_sector);
            return RES_ERROR;
        }

        /* wait for DRQ */
        if (wait_for_drq_set(io_base) < 0) {
            DBG_PRINTF("pdrv %d: write sector %lu DRQ error\n", pdrv, current_sector);
            return RES_ERROR;
        }

        /* write one sector (512 bytes) */
        ata_write_data(io_base, (const uint16_t *)(buff + (i * 512)));
    }

    return RES_OK;
}
#endif

/*-----------------------------------------------------------------------*/
/* Physical I/O control                                                  */
/*-----------------------------------------------------------------------*/
static DRESULT phys_disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    if (pdrv >= MAX_DRIVES)      return RES_PARERR;
    if (!drive_present[pdrv])    return RES_NOTRDY;

    DRESULT res = RES_OK;
    switch (cmd) {
        case CTRL_SYNC:
            /* nothing to do for ATA */
            break;
        case GET_SECTOR_COUNT:
            /* return the sector count detected */
            *(DWORD *)buff = total_sectors[pdrv];
            break;
        case GET_SECTOR_SIZE:
            *(WORD *)buff = 512;
            break;
        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1;
            break;
        default:
            res = RES_PARERR;
            break;
    }
    return res;
}

/*-----------------------------------------------------------------------*/
/* Public FatFs API wrappers (logical → physical mapping)                */
/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/* Probe all 4 possible ATA drives and initialize their presence status */
/*-----------------------------------------------------------------------*/
void probe_all_ata_drives(void) {
    for (BYTE pdrv = 0; pdrv < MAX_DRIVES; pdrv++) {
        /* call physical init to detect the drive */
        phys_disk_initialize(pdrv);
    }
}

/*-----------------------------------------------------------------------*/
/* Get logical drive status                                              */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status(BYTE logical_drv) {
    if (logical_drv >= MAX_DRIVES) return STA_NOINIT;

    BYTE pdrv = logical_to_physical[logical_drv];
    if (pdrv == 0xFF) return STA_NOINIT;       /* unmapped logical */ 
    return phys_disk_status(pdrv);
}

/*-----------------------------------------------------------------------*/
/* Initialize logical drive                                               */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize(BYTE logical_drv) {
    if (logical_drv >= MAX_DRIVES) return STA_NOINIT;

    BYTE pdrv = logical_to_physical[logical_drv];
    if (pdrv == 0xFF) return STA_NOINIT;       /* unmapped logical */
    return phys_disk_initialize(pdrv);
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s) by logical drive                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read(BYTE logical_drv, BYTE *buff, LBA_t sector, UINT count) {
    if (logical_drv >= MAX_DRIVES) return RES_PARERR;

    BYTE pdrv = logical_to_physical[logical_drv];
    if (pdrv == 0xFF || !drive_present[pdrv]) return RES_NOTRDY;

    return phys_disk_read(pdrv, buff, sector, count);
}

#if FF_FS_READONLY == 0
/*-----------------------------------------------------------------------*/
/* Write Sector(s) by logical drive                                       */
/*-----------------------------------------------------------------------*/
DRESULT disk_write(BYTE logical_drv, const BYTE *buff, LBA_t sector, UINT count) {
    if (logical_drv >= MAX_DRIVES)      return RES_PARERR;

    BYTE pdrv = logical_to_physical[logical_drv];
    if (pdrv == 0xFF || !drive_present[pdrv]) return RES_NOTRDY;

    return phys_disk_write(pdrv, buff, sector, count);
}
#endif

/*-----------------------------------------------------------------------*/
/* I/O control by logical drive                                          */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl(BYTE logical_drv, BYTE cmd, void *buff) {
    if (logical_drv >= MAX_DRIVES) return RES_PARERR;

    BYTE pdrv = logical_to_physical[logical_drv];
    if (pdrv == 0xFF || !drive_present[pdrv]) return RES_NOTRDY;

    return phys_disk_ioctl(pdrv, cmd, buff);
}

/*-----------------------------------------------------------------------*/
/* Get FAT time (unchanged)                                              */
/*-----------------------------------------------------------------------*/
DWORD get_fattime(void) {
    /* not implemented, just return 0 */
    return 0;
}
