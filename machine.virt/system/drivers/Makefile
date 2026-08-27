BASIC_DIRS = ttyd virtfsd powerd
basic: $(BASIC_DIRS)

GUI_DIRS = fbdisplayd moused keybd snd
gui: $(GUI_DIRS)

NET_DIRS = net
network: $(NET_DIRS)

$(BASIC_DIRS) $(GUI_DIRS) $(NET_DIRS):
	@$(MAKE) -C $@

.PHONY: basic gui network clean $(BASIC_DIRS) $(GUI_DIRS) $(NET_DIRS)

CLEAN_DIRS = $(BASIC_DIRS) $(GUI_DIRS) $(NET_DIRS)
clean:	
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
