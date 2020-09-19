LOGIN_OBJS = $(ROOT_DIR)/bin/login/login.o

LOGIN = $(TARGET_DIR)/$(ROOT_DIR)/bin/login

PROGS += $(LOGIN)
CLEAN += $(LOGIN_OBJS)

$(LOGIN): $(LOGIN_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(LOGIN_OBJS) -o $(LOGIN) $(LDFLAGS) -lewokc -lc
