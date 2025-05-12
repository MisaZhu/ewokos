CPU = cortex-a55
CONSOLE_MODE = -nographic -display none
GRAPHIC_MODE = -display cocoa 
QEMU_FLAGS = -M raspi3b  $(GRAPHIC_MODE) -smp 4 -serial null -serial mon:stdio

#QEMU_FLAGS += -d trace:gic_update_set_irq 

ARCH_VER=v8
ARCH_CFLAGS=-Wa,-march=armv8-a 
DEBUG=no
SMP=yes
