all: basic_libs
	@echo "all done."

DIRS = socket ntpc wolfssl libtinyhttpsc libwebsockets
basic_libs: $(DIRS)

# header dependencies: everything needs socket's sys/socket.h installed,
# libtinyhttpsc also compiles against the installed wolfssl headers
ntpc wolfssl libwebsockets: socket
libtinyhttpsc: socket wolfssl

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all basic_libs clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
