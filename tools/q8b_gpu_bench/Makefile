ROOT := $(abspath ../..)
SDK := $(ROOT)/system/build_aarch64/q8b
ADRENO := $(ROOT)/machines/q8b/system/libs/adreno
FE := $(ROOT)/projects/soft3d/libs/gles/pglcompiler
OUT ?= /tmp/q8b-gpu-bench
COMPILER_OUT ?= /tmp/q8b-builtin-shaders
.PHONY: all shaders check
all: $(OUT)/bench $(OUT)/clock $(OUT)/input-latency $(OUT)/shm-pool shaders
$(OUT):
	mkdir -p $@
$(OUT)/bench.o: bench.c | $(OUT)
	aarch64-none-elf-gcc -O2 -march=armv8-a -mno-outline-atomics -fno-builtin -Wall -Wextra \
	 -I$(SDK)/include -I$(ADRENO)/include -c $< -o $@
$(OUT)/bench: $(OUT)/bench.o $(SDK)/lib/libadreno.a $(SDK)/lib/libewoksys.a
	aarch64-none-elf-ld -Ttext=100 $< -o $@ -L$(SDK)/lib \
	 --start-group -ladreno -lewoksys -lc -lgloss --end-group
$(OUT)/clock.o: clock.c | $(OUT)
	aarch64-none-elf-gcc -O2 -march=armv8-a -mno-outline-atomics -Wall -Wextra \
	 -I$(SDK)/include -c $< -o $@
$(OUT)/clock: $(OUT)/clock.o $(SDK)/lib/libewoksys.a
	aarch64-none-elf-ld -Ttext=100 $< -o $@ -L$(SDK)/lib \
	 --start-group -lewoksys -lc -lgloss --end-group
$(OUT)/input-latency.o: input_latency.c | $(OUT)
	aarch64-none-elf-gcc -O2 -march=armv8-a -mno-outline-atomics -Wall -Wextra \
	 -I$(SDK)/include -c $< -o $@
$(OUT)/input-latency: $(OUT)/input-latency.o $(SDK)/lib/libewoksys.a
	aarch64-none-elf-ld -Ttext=100 $< -o $@ -L$(SDK)/lib \
	 --start-group -lewoksys -lc -lgloss --end-group
$(OUT)/shm-pool.o: shm_pool.c | $(OUT)
	aarch64-none-elf-gcc -O2 -march=armv8-a -mno-outline-atomics -Wall -Wextra \
	 -I$(SDK)/include -c $< -o $@
$(OUT)/shm-pool: $(OUT)/shm-pool.o $(SDK)/lib/libewoksys.a
	aarch64-none-elf-ld -Ttext=100 $< -o $@ -L$(SDK)/lib \
	 --start-group -lewoksys -lc -lgloss --end-group
shaders: | $(OUT)
	$(MAKE) -C $(ADRENO) -f shaders/Makefile.host compiler OUT=$(COMPILER_OUT)
	clang -O2 -I$(FE)/include -I$(ADRENO)/include -c compile.c -o $(OUT)/compile.o
	clang++ $(OUT)/compile.o $(COMPILER_OUT)/host_support.o \
	 $(COMPILER_OUT)/libbackend.a $(COMPILER_OUT)/libfrontend.a $(COMPILER_OUT)/libbackend.a \
	 -pthread -lm -o $(OUT)/compile
	$(OUT)/compile texture.comp $(OUT)/texture.bin
check: | $(OUT)
	clang -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	 -I$(ROOT)/kernel/kernel/include clock_anchor_test.c -o $(OUT)/clock-anchor-test
	$(OUT)/clock-anchor-test
	clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	 memory_pool_test.c -o $(OUT)/memory-pool-test
	$(OUT)/memory-pool-test
