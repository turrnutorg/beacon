include config.mk

compile: $(OS_ISO)

$(OS_ISO): $(OS_BIN)
	cp $< build/boot/os.bin
	grub-mkrescue --output=$@ build

$(OS_BIN): $(OBJ_DIR)/blob.o $(ASM_OBJS)
	$(LD) $(LD_FLAGS) -T link.ld -o $@ $^

$(OBJ_DIR)/blob.o: build_c.mk
	$(MAKE) -f $<

# Make sure that the correct files are recompiled at header update
build_c.mk: $(C_SRC) $(HEADERS)
	rm -f $@
	echo "$(OBJ_DIR)/blob.o: $(C_OBJS)" >> $@
	echo "\t" $(LD) $(LD_FLAGS) $$^ "-r -o" $$\@ >> $@
	echo >> $@
	# Generate a rule for each source file
	for file in $(C_SRC) ; do \
		echo -n $(OBJ_DIR)/ >> $@; \
		$(CC) $$file -MM -I $(SRC_DIR) >> $@; \
		echo "\t" $(CC) $(C_FLAGS) -ffreestanding -c $$\< -I $(SRC_DIR) -o $$\@ >> $@; \
		echo >> $@; \
	done

$(ASM_OBJS): $(OBJ_DIR)/%_asm.o: $(SRC_DIR)/%.asm
	$(AS) $(AS_FLAGS) -o $@ $<

clean:
	rm -rf $(OBJ_DIR)/*.o
	rm -rf $(OUT_DIR)/*.bin
	rm -rf $(OUT_DIR)/*.iso
	rm -f build_c.mk

# We assume image.img already exists
run: $(OS_ISO)
	qemu-system-i386 -hda image.img -cdrom $(OS_ISO) -boot d -serial pty
