includes("**/xmake.lua")

target("rootfs")
    add_deps("ps2keybd", "ps2moused", "ttyd")
    rootfs_common("root.versatilepb.ext2")
target_end()
