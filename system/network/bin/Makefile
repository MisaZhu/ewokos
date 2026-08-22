
DIRS = ipconfig ping host https_test telnet ssh scp ws_test
all:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = ipconfig ping host https_test telnet ssh scp ws_test
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
