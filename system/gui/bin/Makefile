all: builtin_apps
	@echo "all done."

builtin_apps:
	cd splash;$(MAKE)
	cd disptest;$(MAKE)
	cd g2dtest;$(MAKE)
	cd wavplayer;$(MAKE)
	cd mp3player;$(MAKE)
	cd oggplayer;$(MAKE)
	cd load_font;$(MAKE)

clean:
	cd splash;$(MAKE) clean
	cd disptest;$(MAKE) clean
	cd g2dtest;$(MAKE) clean
	cd wavplayer;$(MAKE) clean
	cd mp3player;$(MAKE) clean
	cd oggplayer;$(MAKE) clean
	cd load_font;$(MAKE) clean
