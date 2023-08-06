
local target_dir = os.scriptdir().."/"

function build()
    set_toolchain("riscv")
    set_kind("binary")

    add_kernel_src()
    add_arch_src("riscv", "rv64")

    add_files(
        "bsp/*.c"
    )
    
    add_cflags("-march=rv64g_zifencei") 
    add_asflags("-march=rv64g_zifencei") 
end

target("virt")
    build()

    add_ldflags("-T "..os.scriptdir().."/mkos.lds.S",  {force = true})

    after_build(function (target)
        os.run("riscv64-unknown-elf-objcopy -O binary "..target:targetfile().." "..target_dir.."kernel7.img")
    end)
target_end()

target("virt.qemu")
    build()

    add_ldflags("-T "..os.scriptdir().."/mkos.lds.qemu.S",  {force = true})

    after_build(function (target)
        os.run("riscv64-unknown-elf-objcopy -O binary "..target:targetfile().." "..target_dir.."kernel7.qemu.img")
    end)

    on_run(function (target)
        os.run("qemu-system-riscv64 -cpu rv64 -nographic -M virt -smp 1 -m 2G -bios default -serial mon:stdio -device loader,file=system/root.ext2,addr=0xe0000000 -kernel "..target_dir.."kernel7.qemu.img")
    end)
target_end()



