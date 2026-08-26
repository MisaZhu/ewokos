BASIC_DIRS = ttyd virtfsd powerd
basic: 
	@for dir in $(BASIC_DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

GUI_DIRS = fbdisplayd moused keybd snd
gui:
	@for dir in $(GUI_DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

network:
	cd net; $(MAKE)

CLEAN_DIRS = ttyd virtfsd fbdisplayd g2dd moused keybd snd powerd net
clean:	
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
