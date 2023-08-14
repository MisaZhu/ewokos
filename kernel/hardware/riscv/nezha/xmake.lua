
local target_dir = os.scriptdir().."/"

function build()
    set_toolchain("riscv")
    set_kind("binary")

    add_kernel_src()
    add_arch_src("riscv", "rv64")

    add_files(
        "bsp/*.c"
    )
    
    add_defines("C906_EXTEND")
    add_cflags("-march=rv64g_zifencei")
    add_asflags("-march=rv64g_zifencei")
end

target("kernel")
    build()

    add_ldflags("-T "..os.scriptdir().."/mkos.lds.S",  {force = true})

    after_build(function (target)
        os.run("riscv64-unknown-elf-objcopy -O binary "..target:targetfile().." "..target_dir.."kernel7.img")
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
        os.run("riscv64-unknown-elf-objcopy -O binary "..target:targetfile().." "..target_dir.."kernel7.qemu.img")
    end)

    after_clean(function (target)
        os.rm(target_dir.."kernel7.qemu.img")
    end)

    qemu("-cpu rv64 -nographic -M virt -smp 1 -m 2G -bios default -serial mon:stdio", target_dir.."kernel7.qemu.img")
target_end()



