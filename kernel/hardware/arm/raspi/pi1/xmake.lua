
local target_dir = os.scriptdir().."/"

function build()
    set_toolchain("arm")
    set_kind("binary")

    add_kernel_src()
    add_arch_src("arm", "v6")

    add_defines("KCONSOLE")

    add_includedirs(
        "../lib/bcm283x/include"
    )

    add_files(
        "bsp/*.c",
        "../lib/bcm283x/**.c"
    )

     add_cflags("-mcpu=arm1176jzf-s")
     add_asflags("-mcpu=arm1176jzf-s")
end

target("kernel")
    build()

    add_ldflags("-T "..os.scriptdir().."/mkos.lds.S",  {force = true})

    after_build(function (target)
        os.run("arm-none-eabi-objcopy -O binary "..target:targetfile().." "..target_dir.."kernel.img")
    end)

    after_clean(function (target)
        os.rm(target_dir.."kernel.img")
    end)

    on_run(function(target)end)
target_end()

target("qemu")
    build()

    add_ldflags("-T "..os.scriptdir().."/mkos.lds.qemu.S",  {force = true})

    after_build(function (target)
        os.run("arm-none-eabi-objcopy -O binary "..target:targetfile().." "..target_dir.."kernel.qemu.img")
    end)

    after_clean(function (target)
        os.rm(target_dir.."kernel.qemu.img")
    end)

    qemu("-cpu arm1176 -M raspi0 -m 512M -serial mon:stdio", target_dir.."kernel.qemu.img")
target_end()



