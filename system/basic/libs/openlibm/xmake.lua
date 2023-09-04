target("libm")
	set_toolchain("arm")
    set_kind("static")
	
    add_files("src/**.c")
    add_files("arm/**.c")
	add_cflags("-fno-gnu89-inline -fno-builtin")
	add_includedirs("src", {public = true})
    add_includedirs("include", {public = true})

	add_cflags("-O2")
    on_run(function (target)end)
target_end()
