gccparams = -m32 -nostdlib -fno-builtin -fno-exceptions -fno-leading-underscore -ffreestanding
asmparams = --32
ldparams = -melf_i386 -s

objs = obj/boot.o obj/os.o obj/console.o obj/keyboard.o obj/keyboard_asm.o obj/port.o obj/screen.o obj/command.o obj/speaker.o obj/string.o obj/velocity.o obj/time.o obj/serial.o obj/csa.o obj/simas.o obj/dungeon.o

compile:
	rm -rf out
	rm -rf obj
	mkdir out
	mkdir obj
	rm -rf $(objs)
	as $(asmparams) -o obj/boot.o src/boot.asm
	as $(asmparams) -o obj/keyboard_asm.o src/keyboard.asm

	gcc $(gccparams) -o obj/console.o -c src/console.c
	gcc $(gccparams) -o obj/port.o -c src/port.c
	gcc $(gccparams) -o obj/keyboard.o -c src/keyboard.c
	gcc $(gccparams) -o obj/os.o -c src/os.c
	gcc $(gccparams) -o obj/screen.o -c src/screen.c
	gcc $(gccparams) -o obj/command.o -c src/command.c
	gcc $(gccparams) -o obj/speaker.o -c src/speaker.c
	gcc $(gccparams) -o obj/time.o -c src/time.c
	gcc $(gccparams) -o obj/velocity.o -c src/velocity.c
	gcc $(gccparams) -o obj/string.o -c src/string.c
	gcc $(gccparams) -o obj/serial.o -c src/serial.c
	gcc $(gccparams) -o obj/csa.o -c src/csa.c
	gcc $(gccparams) -o obj/simas.o -c src/simas.c
	gcc $(gccparams) -o obj/dungeon.o -c src/dungeon.c

	ld $(ldparams) -T link.ld -o out/os.bin $(objs)
	cp out/os.bin build/boot/os.bin
	grub-mkrescue --output=out/os.iso build