includes("**/xmake.lua")

target("rootfs")
    add_deps("gpio_joykeybd", "rk_uartd")
    rootfs_common("root.rk3128.ext2")
target_end()
