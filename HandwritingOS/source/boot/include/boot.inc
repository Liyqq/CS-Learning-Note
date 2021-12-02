;------------------- loader和kernel -------------------;
;内核加载器占用硬盘的扇区数
LOADER_CONSUMED_SECTOR_NUM equ 4

;内核加载器的扇区LBA地址：2扇区
LOADER_START_SECTOR_LBA_ADDR equ 0x02

;内核加载器被加载到内存后的开始内存地址
LOADER_BASE_MEM_ADDR equ 0x0900
