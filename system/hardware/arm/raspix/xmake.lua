includes("**/xmake.lua")

target("rootfs")
    rootfs_common("root.raspix.ext2")
    add_deps("soundd", "mbox_actledd", "gpio_actledd", "gpio_actledd", 
             "uartd", "usbd", "hat13_joystickd","epaperd",
			 "hat13_joykeybd","gpio_joystickd","lcdhatd","rpi_lcdd","ili9486d",
			"gt911_touchd","xpt2046d","gpio_joykeybd"
			)
target_end()
