cmd_arch/arm/boot/ewokos.bin := arm-none-eabi-objcopy -O binary -R .note -R .note.gnu.build-id -R .comment -S  ewokos.strip arch/arm/boot/ewokos.bin
