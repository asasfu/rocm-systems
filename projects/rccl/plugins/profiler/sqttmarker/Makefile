# Simple Makefile wrapper for CMake build
# For backward compatibility with legacy build systems

.PHONY: all clean

all:
	@mkdir -p build
	@cd build && cmake .. && $(MAKE)

clean:
	@rm -rf build
