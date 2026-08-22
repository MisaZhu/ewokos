all: basic_libs
	@echo "all done."

DIRS = socket ntpc wolfssl libtinyhttpsc libwebsockets
basic_libs:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = socket ntpc wolfssl libtinyhttpsc libwebsockets
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
