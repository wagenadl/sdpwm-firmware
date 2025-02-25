# This Makefile just documents some generally useful actions.
# Actual build process is through cmake.

all: prep
	+PICO_BOARD=pico cmake --build build 

prep:
	+PICO_BOARD=pico cmake -S . -B build

upload: all
	cp build/sigmadelta.uf2 /media/wagenaar/RPI-RP2/

clean:; rm -rf build

.PHONY: all prep clean upload

