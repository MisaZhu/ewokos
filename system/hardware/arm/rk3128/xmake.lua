includes("**/xmake.lua")

target("rootfs")
    add_deps("audctrl", "gpio_joykeybd", "ms_uartd")
    rootfs_common("root.rk3128.ext2")
target_end()
