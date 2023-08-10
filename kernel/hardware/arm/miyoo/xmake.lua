
local target_dir = os.scriptdir().."/"

function build()
    set_toolchain("arm")
    set_kind("binary")

    add_kernel_src()
    add_arch_src("arm", "v7")

    add_defines("KERNEL_SMP", "KCONSOLE")

    add_files(
        "bsp/*.c"
    )
     add_cflags("-march=armv7ve")
     add_asflags("-march=armv7ve")
end

target("kernel")
    build()
    add_ldflags("-T "..os.scriptdir().."/mkos.lds.S",  {force = true})

    after_build(function (target)
        os.run("arm-none-eabi-objcopy -O binary "..target:targetfile().." "..target_dir.."kernel7.img")
    end)
target_end()

target("qemu")
    build()

    add_ldflags("-T "..os.scriptdir().."/mkos.lds.qemu.S",  {force = true})

    after_build(function (target)
        os.run("arm-none-eabi-objcopy -O binary "..target:targetfile().." "..target_dir.."kernel7.qemu.img")
    end)

    on_run(function (target)
        os.run("qemu-system-arm -cpu cortex-a7 -M raspi2b -m 1024M -serial mon:stdio -sd system/root.ext2 -kernel "..target_dir.."kernel7.qemu.img")
    end)

    add_deps("rootfs")
target_end()



