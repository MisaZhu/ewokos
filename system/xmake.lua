local system_dir = os.scriptdir().."/"
set_policy("check.auto_ignore_flags", false)

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
    on_run(function (target)end)
end


function install_dir(path)
    set_targetdir(system_dir.."build/rootfs/"..path)
    before_build(function (target)
        if not os.exists(system_dir.."/build/rootfs/"..path) then
            os.run("mkdir -p %s/build/rootfs/%s", system_dir, path)
        end
        if os.exists(target:scriptdir().."/res") then
            os.run("cp -rf %s/res %s/build/rootfs/%s", target:scriptdir(), system_dir, path)
        end
    end)

    --after_build(function (target)
        -- os.run("cp %s %s/build/rootfs/%s", target:targetfile(), system_dir, path)
    --end)
end 

function build_system()
end

target("rootfs")
    set_type("phony")
    before_build(function (target)
        if not os.exists(system_dir.."build/include") then
            os.run("mkdir -p %s/build/include", system_dir)
        end
        os.run("cp -rf %s../kernel/kernel/include %s/build/", system_dir,system_dir)
        os.run("cp -rf %s/data %s/build/rootfs/", system_dir, system_dir)
        os.run("mkdir -p %s/build/rootfs/dev", system_dir)
        os.run("mkdir -p %s/build/rootfs/home/root", system_dir)
    end)

    on_build(function (target)
        os.run("rm -f %s/root.ext2", system_dir)
        os.run("dd if=/dev/null of=%s/root.ext2 bs=1 count=1 seek=256M", system_dir)
        os.run("mke2fs  -b 1024 -I 128  %s/root.ext2", system_dir)
        os.run("%s../script/mkrootfs %s/build/rootfs %s/root.ext2", system_dir, system_dir, system_dir)
    end)

    -- baseic system
    add_deps("libewokc")
    add_deps("xserverd","consoled","fontd","displayd","anim","gtest","xconsole",
            "finder","book","wtest","launcher","png","ttyjoy","xim_vkey","xim_none",
            "xmoused","xwm_macos7","xjoystickd","xtouchd","fbd","etc","fbd","stated",
            "sysinfod","timerd","nulld","ramfsd","cat","svcinfo","dump","echo","pwd",
            "sleep","shell","uname","grep","kill","ps","mkdir","mount","rundev","ls",
            "rm","rx","login","session","init","sdfsd","core","vfsd"
    )
target_end()

includes("basic/**/xmake.lua")
includes("full/**/xmake.lua")

if is_plat("raspi1", "raspi2", "raspi3", "raspi4") then
    includes("hardware/arm/raspix/**/xmake.lua")
    target("system")
        set_type("phony")
        add_deps("rootfs")
    target_end()
elseif is_plat("miyoo") then
    includes("hardware/arm/miyoo/**/xmake.lua")
     target("system")
        set_type("phony")
        add_deps("rootfs")
    target_end()
end
