all: x_libs
	@echo "all done."

DIRS = x x++ widget++ SDL2
x_libs: $(DIRS)

# build chain: x -> x++ -> widget++; SDL2's video backend sits on libx
x++: x
widget++: x++
SDL2: x

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all x_libs clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done