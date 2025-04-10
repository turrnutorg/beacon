#include "velocity.h"
#include "port.h"
#include "console.h"
#include "string.h"

extern void* mb_info;

#define FB_COLOR_COUNT 256

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t reserved[2];
} __attribute__((packed)) multiboot_info_t;

static void draw_screen(uint8_t* fb, uint32_t pitch, uint32_t width, uint32_t height) {
    uint32_t color = 0;
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            fb[y * pitch + x] = color;
            color = (color + 1) % FB_COLOR_COUNT;
        }
    }
}

void velocity_init(void* mb_info_ptr) {
    
    multiboot_info_t* mbi = (multiboot_info_t*) mb_info;

    char buf[32];
    itoa((uint32_t)mb_info, buf);
    println("mb_info addr: ");
    println(buf);

    itoa(mbi->framebuffer_addr, buf);
    println("fb addr: ");
    println(buf);

    itoa(mbi->framebuffer_width, buf);
    println("width: ");
    println(buf);

    itoa(mbi->framebuffer_height, buf);
    println("height: ");
    println(buf);

    itoa(mbi->framebuffer_bpp, buf);
    println("bpp: ");
    println(buf);

    itoa(mbi->framebuffer_type, buf);
    println("type: ");
    println(buf);


    if (!(mbi->flags & (1 << 12))) {
        // no framebuffer info, go cry in BIOS mode
        return;
    }

    if (mbi->framebuffer_bpp != 8 || mbi->framebuffer_type != 0) {
        // not 8-bit indexed color mode
        return;
    }

    uint8_t* fb = (uint8_t*) (uintptr_t) mbi->framebuffer_addr;
    draw_screen(fb, mbi->framebuffer_pitch, mbi->framebuffer_width, mbi->framebuffer_height);
}
