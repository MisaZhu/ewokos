includes("**/xmake.lua")

target("rootfs")
    rootfs_common("root.raspix.ext2")
    add_deps("soundd", "mbox_actledd", "gpio_actledd", "gpio_actledd", 
             "mini_uartd", "usbd", "pl011_uartd"
    )
target_end()
