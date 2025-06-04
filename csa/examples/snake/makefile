CC = gcc
LD = ld -m elf_i386
OBJCOPY = objcopy

CFLAGS = -m32 -march=i386 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-pic -fno-pie
LDFLAGS = -T link.ld

SRC = main.c snake.c
OBJS = $(SRC:.c=.o)

all: cleanup

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

payload.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

payload.bin: payload.elf
	$(OBJCOPY) -O binary $< $@

script.py: payload.bin
	python3 script.py $<

cleanup: script.py
	rm -f *.elf *.bin *.o

clean:
	rm -f *.o *.elf *.bin
