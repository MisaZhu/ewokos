all: basic_libs
	@echo "all done."

DIRS = socket ntpc wolfssl libwebsockets
basic_libs: $(DIRS)

# header dependencies: everything needs socket's sys/socket.h installed
ntpc wolfssl libwebsockets: socket

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all basic_libs clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
