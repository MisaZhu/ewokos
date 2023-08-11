local kernel_dir = os.scriptdir().."/"
set_defaultplat("miyoo")

toolchain("arm-none-eabi")
    -- mark as standalone toolchain
    set_kind("standalone")

    -- set toolset
    set_toolset("cc", "arm-none-eabi-gcc")
    set_toolset("cxx","arm-none-eabi-c++")
    set_toolset("ld", "arm-none-eabi-ld")
    set_toolset("ar", "arm-none-eabi-ar")
    set_toolset("ex", "arm-none-eabi-ar")
    set_toolset("strip", "arm-none-eabi-strip")
    set_toolset("as", "arm-none-eabi-gcc")
    set_toolset("objcopy", "arm-none-eabi-objcopy")
toolchain_end()

toolchain("riscv64-unknown-elf")
    -- mark as standalone toolchain
    set_kind("standalone")

    -- set toolset
    set_toolset("cc", "riscv64-unknown-elf-gcc")
    set_toolset("cxx","riscv64-unknown-elf-c++")
    set_toolset("ld", "riscv64-unknown-elf-ld")
    set_toolset("ar", "riscv64-unknown-elf--ar")
    set_toolset("ex", "riscv64-unknown-elf-ar")
    set_toolset("strip", "riscv64-unknown-elf-strip")
    set_toolset("as", "riscv64-unknown-elf-gcc")
    set_toolset("objcopy", "riscv64-unknown-elf-objcopy")
toolchain_end()


function set_toolchain(arch)
    if arch == "riscv" then
        set_plat("riscv")
        set_toolchains("riscv64-unknown-elf")
        add_cflags("-mcmodel=medany -fno-optimize-sibling-calls -mstrict-align  -fpie -fno-stack-protector")
    else
        set_plat("arm")
        set_toolchains("arm-none-eabi")
        add_cflags("-msoft-float -mapcs-frame -std=c99")
    end
        
    add_cflags(
        "-Wall -Wextra -fPIC -pedantic",
        "-Wstrict-prototypes",
        "-fno-builtin-printf",
        "-fno-builtin-strcpy",
        "-Wno-overlength-strings",
        "-fno-builtin-exit",
        "-fno-builtin-stdio",
        "-fno-builtin-memset",
        "-fno-builtin-memcpy",
        "-fno-builtin-strchr",
        "-fno-builtin-strcmp",
        "-fno-builtin-strlen",
        "-fno-builtin-strncpy",
        "-fno-builtin-strncmp"
    )

end

function add_kernel_src(arch)
    add_includedirs(
        kernel_dir.."dev/include",
        kernel_dir.."kernel/include",
        kernel_dir.."lib/include",
        kernel_dir.."lib/graph/include",
        kernel_dir.."lib/console/include"
    )
    add_files(
       kernel_dir.."kernel/**.c",
       kernel_dir.."lib/**.c", 
       kernel_dir.."loadinit/**.c"
    )
end


function add_arch_src(arch, v)
    add_includedirs(
        kernel_dir.."hardware/"..arch.."/arch/common/include"
    )

    add_files(
        kernel_dir.."hardware/"..arch.."/arch/common/src/*.c",
        kernel_dir.."hardware/"..arch.."/arch/common/src/*.S",
        kernel_dir.."hardware/"..arch.."/arch/"..v.."/*.c",
        kernel_dir.."hardware/"..arch.."/arch/"..v.."/*.S"
    )
end

function qemu(args, kernel)
    on_run(function (target)
        if is_plat("raspi1", "raspi2", "raspi3", "raspi2.3", "raspi4") then
            os.run("qemu-system-arm %s -kernel %s -sd ../system/root.raspix.ext2", args, kernel)
        elseif is_plat("miyoo") then
            os.run("qemu-system-arm %s -kernel %s -sd ../system/root.miyoo.ext2", args, kernel)
        elseif is_plat("rk3128") then
            os.run("qemu-system-arm %s -kernel %s -sd ../system/root.rk3128.ext2", args, kernel)
         elseif is_plat("versatilepb") then
            os.run("qemu-system-arm %s -kernel %s -sd ../system/root.versatilepb.ext2", args, kernel)
        elseif is_plat("nezha") then
            os.run("qemu-system-riscv64 %s -kernel %s  -device loader,file=../system/root.nezha.ext2,addr=0xe0000000", args, kernel)
        elseif is_plat("virt") then
            os.run("qemu-system-riscv64 %s -kernel %s  -device loader,file=../system/root.virt.ext2,addr=0xe0000000", args, kernel)
        end
    end)
end

if is_plat("raspi2", "raspi3", "raspi2.3") then
    includes("hardware/arm/raspi/pi2.3/xmake.lua")
elseif is_plat("raspi1") then
    includes("hardware/arm/raspi/pi1/xmake.lua")
elseif is_plat("raspi4") then
    includes("hardware/arm/raspi/pi4/xmake.lua")
elseif is_plat("miyoo") then
    includes("hardware/arm/miyoo/xmake.lua")
elseif is_plat("rk3128") then
    includes("hardware/arm/rk3128/xmake.lua")
elseif is_plat("versatilepb") then
    includes("hardware/arm/versatilepb/xmake.lua")
elseif is_plat("nezha") then
    includes("hardware/riscv/nezha/xmake.lua")
elseif is_plat("virt") then
    includes("hardware/riscv/virt/xmake.lua")
end
