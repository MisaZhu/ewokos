
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

target("pi1")
    build()

    add_ldflags("-T "..os.scriptdir().."/mkos.lds.S",  {force = true})

    after_build(function (target)
        os.run("arm-none-eabi-objcopy -O binary "..target:targetfile().." "..target_dir.."kernel.img")
    end)
target_end()

target("pi1.qemu")
    build()

    add_ldflags("-T "..os.scriptdir().."/mkos.lds.qemu.S",  {force = true})

    after_build(function (target)
        os.run("arm-none-eabi-objcopy -O binary "..target:targetfile().." "..target_dir.."kernel.qemu.img")
    end)

    on_run(function (target)
        os.run("qemu-system-arm -cpu arm1176 -M raspi0 -m 512M -serial mon:stdio -sd system/root.ext2 -kernel "..target_dir.."kernel.qemu.img")
    end)
target_end()



