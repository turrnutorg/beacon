gccparams = -m32 -nostdlib -fno-builtin -fno-exceptions -fno-leading-underscore
asmparams = --32
ldparams = -melf_i386 -s

objs = obj/boot.o obj/os.o obj/console.o obj/keyboard.o obj/keyboard_asm.o obj/port.o obj/screen.o obj/command.o obj/speaker.o obj/string.o

.PHONY: all bin cd floppy clean

all: floppy cd 

bin:
	rm -rf out obj
	mkdir -p out obj

	as $(asmparams) -o obj/boot.o src/boot.asm
	as $(asmparams) -o obj/keyboard_asm.o src/keyboard.asm

	gcc $(gccparams) -o obj/console.o -c src/console.c
	gcc $(gccparams) -o obj/port.o -c src/port.c
	gcc $(gccparams) -o obj/keyboard.o -c src/keyboard.c
	gcc $(gccparams) -o obj/os.o -c src/os.c
	gcc $(gccparams) -o obj/screen.o -c src/screen.c
	gcc $(gccparams) -o obj/command.o -c src/command.c
	gcc $(gccparams) -o obj/speaker.o -c src/speaker.c
	gcc $(gccparams) -o obj/string.o -c src/string.c

	ld $(ldparams) -T link.ld -o out/os.bin $(objs)
	cp out/os.bin build/boot/os.bin

cd: bin
	mkdir -p out/boot/grub
	cp out/os.bin out/boot/
	cp out/floppy.img out/boot/
	cp build/boot/grub/grub.cfg out/boot/grub/
	grub-mkrescue -o out/os.iso out

floppy: bin
	dd if=/dev/zero of=out/floppy.img bs=512 count=2880
	mkfs.vfat out/floppy.img
	mmd -i out/floppy.img ::/boot
	mcopy -i out/floppy.img out/os.bin ::/boot/os.bin

clean:
	rm -rf out obj
