CC := gcc
AS := as
LD := ld

SRC_DIR := src
OBJ_DIR := obj
OUT_DIR := out

C_FLAGS := -m32 -nostdlib -fno-builtin -fno-exceptions -fno-leading-underscore -ffreestanding -mno-sse -mno-mmx
AS_FLAGS = --32
LD_FLAGS = -m elf_i386 -s

C_SRC := $(wildcard $(SRC_DIR)/*.c)
HEADERS := $(wildcard $(SRC_DIR)/*.h)
ASM_SRC := $(wildcard $(SRC_DIR)/*.asm)

# To avoid conflicts between files with same name but different extensions
# we turn file.asm into file_asm.o
ASM_OBJS := $(addsuffix .o,$(subst .,_,$(subst $(SRC_DIR),$(OBJ_DIR),$(ASM_SRC))))

C_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SRC))

OS_BIN := $(OUT_DIR)/os.bin
OS_ISO := $(OUT_DIR)/os.iso
