if [ ! -d "./build" ]; then
    mkdir build
fi

echo -e "\033[32m========================================\033[0m"
echo -e "\033[32m build main.o \033[0m"
gcc -m32 -I ./lib/kernel/ -I ./lib/ -I ./kernel/ \
    -c -fno-builtin -fno-stack-protector \
    -o ./build/main.o ./kernel/main.c

echo -e "\033[32m========================================\033[0m"
echo -e "\033[32m build print.o \033[0m"
nasm -f elf -I ./lib/kernel/ -o ./build/print.o ./lib/kernel/print.S

echo -e "\033[32m========================================\033[0m"
echo -e "\033[32m build kernel_interrupt.o \033[0m"
nasm -f elf -o ./build/kernel_interrupt.o ./kernel/kernel_interrupt.S

echo -e "\033[32m========================================\033[0m"
echo -e "\033[32m build interrupt.o \033[0m"
gcc -m32 -I ./lib/kernel/ -I ./lib -I ./kernel/ \
    -c -fno-builtin -fno-stack-protector \
    -o ./build/interrupt.o ./kernel/interrupt.c

echo -e "\033[32m========================================\033[0m"
echo -e "\033[32m build init.o \033[0m"
gcc -m32 -I ./lib/kernel/ -I ./lib -I ./kernel/ -I ./device \
    -c -fno-builtin -fno-stack-protector \
    -o ./build/init.o ./kernel/init.c

echo -e "\033[32m========================================\033[0m"
echo -e "\033[32m build timer.o \033[0m"
gcc -m32 -I ./device -I ./lib/kernel \
    -c -fno-builtin -fno-stack-protector \
    -o ./build/timer.o ./device/timer.c


echo -e "\033[32m========================================\033[0m"
echo -e "\033[32m build kernel.bin \033[0m"
ld -m elf_i386 -Ttext 0xc0001500 -e main -o ./build/kernel.bin \
      ./build/main.o ./build/init.o ./build/interrupt.o ./build/print.o \
      ./build/kernel_interrupt.o ./build/timer.o

echo -e "\033[32m========================================\033[0m"
echo -e "\033[32m copy kernel.bin to hd60M.img \033[0m"
dd if=/home/oneim/Desktop/HandwritingOS/src/build/kernel.bin \
   of=/home/oneim/Desktop/HandwritingOS/localdebug/bochs/bin/hd60M.img \
   bs=512 count=200 seek=9 conv=notrunc

tree ./build