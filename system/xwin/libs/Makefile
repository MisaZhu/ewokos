all: x_libs
	@echo "all done."

DIRS = x x++ widget++
x_libs: $(DIRS)

# build chain: x -> x++ -> widget++
x++: x
widget++: x++

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all x_libs clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done