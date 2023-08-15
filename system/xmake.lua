local system_dir = os.scriptdir().."/"
local rootfs_dir =  os.scriptdir().."/build/rootfs/"

set_policy("check.auto_ignore_flags", false)
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
        set_toolchains("riscv64-unknown-elf")
        add_cflags("-mcmodel=medany -fno-optimize-sibling-calls -mstrict-align  -fpie -fno-stack-protector")
    else
        set_toolchains("arm-none-eabi")
        add_cflags(  "-marm -msoft-float -mapcs-frame -std=c99")
        add_cxxflags("-marm -msoft-float -mapcs-frame -std=c++11")
    end

    add_cflags(
        "-Wall -Wextra -fPIC",
        "-ffreestanding",
        "-Wstrict-prototypes",
        "-Wno-overlength-strings"
    )

    add_cxxflags(
        "-pedantic -Wall -Wextra  -fPIC",
        "-ffreestanding",
        "-Wno-overlength-strings",
        "-fno-exceptions",
        "-fno-rtti"
    )

end

function set_type(type)
    set_toolchain("arm")

    if type == "library" then
        set_kind("static")
        add_includedirs(system_dir.."build/include")
    elseif type == "application" then
        set_kind("binary")
        add_includedirs(system_dir.."build/include")
        add_cflags("-Isystem/basic/libs/libc/include -Isystem/basic/libs/libc/sys/includ")
        add_ldflags("-Ttext=100 -lc -lnosys", {force=true})
        add_deps("libewokc")
    else
        set_kind("phony")
    end
    add_cflags("-O2")
    add_deps("system")

    on_run(function (target)end)

end


function install_dir(path)
    before_build(function (target)
        if not os.exists(rootfs_dir..path) then
            os.run("mkdir -p %s", rootfs_dir..path)
        end
        if os.exists(target:scriptdir().."/res") then
            os.cp(target:scriptdir().."/res", rootfs_dir..path)
        end
    end)


    after_link(function (target)
        os.cp(target:targetfile(), rootfs_dir..path)
    end)

    after_clean(function(target)
        os.rm(rootfs_dir..path.."/"..target:name())
    end)
end

function build_system()
end

function rootfs_common(image)
    set_type("phony")

    on_build(function (target)
        os.iorun("%s/script/mkrootfs %s %s/../root.ext2", system_dir, rootfs_dir, rootfs_dir)
    end)

    -- baseic system
    add_deps("libewokc")
    add_deps("xserverd","consoled","fontd","displayd","anim","gtest","xconsole",
            "finder","book","wtest","launcher","png","ttyjoy","xim_vkey","xim_none",
            "xmoused","xwm_macos7","xjoystickd","xtouchd","fbd","etc","fbd","stated",
            "sysinfod","timerd","nulld","ramfsd","cat","svcinfo","dump","echo","pwd",
            "sleep","shell","uname","grep","kill","ps","mkdir","mount","rundev","ls",
            "rm","rx","login","session","init","sdfsd","core","vfsd", "emu", "snake"
    )

    after_clean(function (target)
        os.rm(system_dir.."build/")
        os.rm(rootfs_dir.."../root.ext2")
    end)
end

function system_common()
target("system")
    set_kind("phony")

    on_load(function (target)
        print(rootfs_dir) 

        if not os.exists(system_dir.."build/include") then
            os.run("mkdir -p %s/build/include", system_dir)
        end
        os.run("cp -rf %s../kernel/kernel/include %s/build/", system_dir,system_dir)

        if not os.exists(target:targetdir()) then
            os.mkdir(target:targetdir())
        end

        if not os.exists(rootfs_dir) then
            os.mkdir(rootfs_dir)
            os.run("cp -rf %s/data %s", system_dir, rootfs_dir)
            os.run("mkdir -p %s/dev", rootfs_dir)
            os.run("mkdir -p %s/home/root", rootfs_dir)
        end
    end)

    on_build(function (target)
        printf(rootfs_dir)
    end)

    after_clean(function (target)
    end)
target_end()
end

function get_rootfsdir()
    return rootfs_dir
end


includes("basic/**/xmake.lua")
includes("full/**/xmake.lua")
includes("extra/**/xmake.lua")

if is_plat("raspix","raspi1", "raspi2", "raspi3", "raspi4") then
    set_arch("")
    rootfs_dir =  os.scriptdir().."/build/raspix/rootfs/" 
    includes("hardware/arm/raspix/xmake.lua")
	includes("hardware/3rd/raspix/xmake.lua")
    system_common()
elseif is_plat("miyoo") then
    set_arch("")
    rootfs_dir =  os.scriptdir().."/build/miyoo/rootfs/" 
    includes("hardware/arm/miyoo/xmake.lua")
    system_common()
end


