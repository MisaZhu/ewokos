target("libewokc")
	set_toolchain("arm")
    set_kind("static")

    add_files("**.c")
    add_files("sys/src/syscall_arm.S")
    add_files("sys/src/arm32_aeabi_divmod_a32.S")
    add_files("sys/src/arm32_aeabi_ldivmod_a32.S")

	add_includedirs("../../../build/include")
    add_includedirs("include", "sys/include", {public = true})

	add_cflags("-O2")
    on_run(function (target)end)
target_end()
