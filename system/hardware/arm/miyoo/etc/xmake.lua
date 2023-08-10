target("etc")
    set_type("phony")
    install_dir("etc")    
    on_build(function (target)
        if not os.exists(target:targetdir()) then
            os.run("mkdir -p %s", target:targetdir())
        end
        os.run("cp -rf %s/init.rd %s",target:scriptdir(), target:targetdir())
        os.run("cp -rf %s/console.conf %s",target:scriptdir(), target:targetdir())
        os.run("cp -rf %s/passwd %s",target:scriptdir(), target:targetdir())
        os.run("cp -rf %s/x %s",target:scriptdir(), target:targetdir())
    end)
target_end()
