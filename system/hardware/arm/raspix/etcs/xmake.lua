target("etc")
    set_type("phony")
    install_dir("etc")    
    on_build(function (target)
        if not os.exists(target:targetdir()) then
            os.run("mkdir -p %s", target:targetdir())
        end
        os.run("cp -rf %s/qemu/full/init.rd %s",target:scriptdir(), target:targetdir())
        os.run("cp -rf %s/qemu/full/console.conf %s",target:scriptdir(), target:targetdir())
        os.run("cp -rf %s/qemu/full/passwd %s",target:scriptdir(), target:targetdir())
        os.run("cp -rf %s/qemu/full/x %s",target:scriptdir(), target:targetdir())
        os.run("cp -rf %s/qemu/full/arch %s",target:scriptdir(), target:targetdir())
    end)
target_end()
