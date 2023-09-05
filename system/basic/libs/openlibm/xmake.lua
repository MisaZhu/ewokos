target("libm")
    set_type("library")
	
    add_files("src/**.c")
    add_files("arm/**.c")
	add_includedirs("src", {public = true})
    add_includedirs("include", {public = true})

    on_run(function (target)end)
target_end()
