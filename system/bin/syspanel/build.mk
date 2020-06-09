SYS_PANEL_OBJS = $(ROOT_DIR)/bin/syspanel/syspanel.o

SYS_PANEL = $(TARGET_DIR)/$(ROOT_DIR)/bin/syspanel

PROGS += $(SYS_PANEL)
CLEAN += $(SYS_PANEL_OBJS)

$(SYS_PANEL): $(SYS_PANEL_OBJS)
	$(LD) -Ttext=100 $(SYS_PANEL_OBJS) -o $(SYS_PANEL) $(LDFLAGS) -lx -lsconf  -lgraph -lewokc -lc -ltga
