base_dir = os.scriptdir().."/"

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


includes("kernel/xmake.lua")
