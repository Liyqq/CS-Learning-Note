BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I ./lib -I ./lib/kernel -I ./lib/user -I ./kernel -I ./device \
      -I ./thread
ASFLAGS = -f elf
CFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -fno-stack-protector \
		 -W -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
	   $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel_interrupt.o \
	   $(BUILD_DIR)/print.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o \
	   $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/thread.o \
	   $(BUILD_DIR)/list.o $(BUILD_DIR)/switch.o

########## C code compile ##########
$(BUILD_DIR)/main.o: ./kernel/main.c ./lib/kernel/print.h \
	./lib/stdint.h ./kernel/init.h ./kernel/memory.h \
	./lib/string.h ./lib/kernel/bitmap.h ./thread/thread.h \
	./kernel/debug.h ./kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: ./kernel/init.c ./kernel/init.h \
	./lib/kernel/print.h ./kernel/interrupt.h ./lib/stdint.h \
	./device/timer.h ./kernel/memory.h ./thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: ./kernel/interrupt.c ./kernel/interrupt.h \
	./lib/stdint.h ./kernel/global.h ./lib/kernel/io.h ./lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: ./device/timer.c ./device/timer.h \
	./lib/stdint.h ./lib/kernel/io.h ./lib/kernel/print.h \
	./kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o: ./kernel/debug.c ./kernel/debug.h \
	./lib/stdint.h ./kernel/interrupt.h ./lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/string.o: ./lib/string.c ./lib/string.h \
	./lib/stdint.h ./kernel/global.h ./kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o: ./lib/kernel/bitmap.c ./lib/kernel/bitmap.h \
	./lib/stdint.h ./lib/string.h ./kernel/debug.h ./lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o: ./kernel/memory.c ./kernel/memory.h \
	./lib/stdint.h ./lib/kernel/print.h ./lib/kernel/bitmap.h \
	./lib/string.h ./kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/list.o: ./lib/kernel/list.c ./lib/kernel/list.h \
	./kernel/interrupt.h ./kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/thread.o: ./thread/thread.c ./thread/thread.h \
	./lib/stdint.h ./lib/string.h ./kernel/global.h ./kernel/memory.h \
	./lib/kernel/list.h ./kernel/debug.h ./kernel/interrupt.h \
	./lib/kernel/print.h ./lib/kernel/list.h 
	$(CC) $(CFLAGS) $< -o $@


########## Assembly code compile ##########
$(BUILD_DIR)/kernel_interrupt.o: ./kernel/kernel_interrupt.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/print.o: ./lib/kernel/print.S 
	$(AS) $(ASFLAGS) -I ./lib/kernel/ $< -o $@

$(BUILD_DIR)/switch.o: ./thread/switch.S
	$(AS) $(ASFLAGS) $< -o $@



########## link all object file ##########
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@


.PHONY : mk_dir hd clean all

mk_dir:
	if [[ ! -d $(BUILD_DIR) ]]; then mkdir $(BUILD_DIR); fi

hd:
	dd if=/home/oneim/Desktop/HandwritingOS/src/build/kernel.bin \
   	of=/home/oneim/Desktop/HandwritingOS/localdebug/bochs/bin/hd60M.img \
   	bs=512 count=200 seek=9 conv=notrunc

clean:
	cd $(BUILD_DIR) && rm -f ./*

build: $(BUILD_DIR)/kernel.bin

all: mk_dir build hd

