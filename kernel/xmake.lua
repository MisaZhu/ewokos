local kernel_dir = os.scriptdir().."/"

function set_toolchain(arch)
    if arch == "riscv" then
        set_toolchains("riscv64-unknown-elf")
        add_cflags("-mcmodel=medany -fno-optimize-sibling-calls -mstrict-align  -fpie -fno-stack-protector")
    else
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

if is_plat("raspi2", "raspi3") then
    includes("hardware/arm/raspi/pi2.3/xmake.lua")
elseif is_plat("raspi1") then
    includes("hardware/arm/raspi/pi1/xmake.lua")
elseif is_plat("raspi4") then
    includes("hardware/arm/raspi/pi4/xmake.lua")
elseif is_plat("miyoo") then
    includes("hardware/arm/miyoo/xmake.lua")
end
