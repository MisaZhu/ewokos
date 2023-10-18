target("libgraph")
    set_type("library")
	if is_plat("raspix","raspi1", "raspi2", "raspi3", "raspi4") then
		add_cflags("-mfpu=neon-vfpv4", {force=true})
	end
    add_files("**.c")        
    add_deps("libsoftfloat", "libewokc")
    add_includedirs("include",  {public = true})
target_end()
