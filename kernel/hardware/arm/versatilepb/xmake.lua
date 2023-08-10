
local target_dir = os.scriptdir().."/"

function build()
    set_toolchain("arm")
    set_kind("binary")

    add_kernel_src()
    add_arch_src("arm", "v6")

    add_defines("KCONSOLE", "ARM_V6")

    add_files(
        "bsp/*.c"
    )
    add_cflags("-mcpu=arm926ej-s")
    add_asflags("-mcpu=arm926ej-s")
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
        os.run("qemu-system-arm -cpu arm926 -M versatilepb -m 256M -serial mon:stdio -sd system/root.ext2 -kernel "..target_dir.."kernel7.qemu.img")
    end)
target_end()



