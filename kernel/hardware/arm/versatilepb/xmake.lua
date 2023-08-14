
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

    after_clean(function (target)
        os.rm(target_dir.."kernel7.img")
    end)

    on_run(function(target)end)
target_end()

target("qemu")
    build()

    add_ldflags("-T "..os.scriptdir().."/mkos.lds.qemu.S",  {force = true})

    after_build(function (target)
        os.run("arm-none-eabi-objcopy -O binary "..target:targetfile().." "..target_dir.."kernel7.qemu.img")
    end)

    after_clean(function (target)
        os.rm(target_dir.."kernel7.qemu.img")
    end)

    qemu(" -cpu arm926 -M versatilepb -m 256M -serial mon:stdio", target_dir.."kernel7.qemu.img")
target_end()



