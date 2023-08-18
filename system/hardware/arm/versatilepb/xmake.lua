includes("**/xmake.lua")

target("rootfs")
    rootfs_common("root.versatilepb.ext2")
    add_deps("fbd", "ps2keybd", "ps2moused", "ttyd"
			)
target_end()
