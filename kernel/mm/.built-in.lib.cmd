cmd_mm/built-in.lib :=  arm-none-eabi-ld -EL   -r -o mm/built-in.lib mm/startup.o mm/mmu.o mm/kalloc.o mm/trunkmalloc.o mm/kmalloc.o mm/shm.o
