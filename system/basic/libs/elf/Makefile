SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_ELF_OBJS = $(ARCH)/src/elf.o

ELF = $(TARGET_DIR)/lib/libelf.a

# make.rule clears .DEFAULT_GOAL, so this is what `$(MAKE) -C elf` runs.
#
# Headers get their own target rather than riding on the archive rule: a
# header-only edit leaves $(ELF) up to date, and if the install were only a
# recipe line of $(ELF) the edited header would never be copied. Consumers then
# compile against a stale $(TARGET_DIR)/include copy, which is invisible at
# build time and only shows up as wrong behaviour at run time - libs/dl hit
# exactly that when Elf64_Sym changed size.
all: $(ELF) headers

headers:
	@mkdir -p $(TARGET_DIR)/include
	cp -r include/* $(TARGET_DIR)/include

$(ELF): $(LIB_ELF_OBJS)
	$(AR) rcs $(ELF) $(LIB_ELF_OBJS)

.PHONY: all headers


clean:
	rm -f $(LIB_ELF_OBJS)
	rm -f $(ELF)

