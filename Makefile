CXX ?= g++
CCEXT ?= cpp
CCFLAGS ?= -D_GLIBCXX_DEBUG -g -O2 -std=c++11 -Wall -Wextra -Werror -pedantic `pkg-config --cflags SDL2_image` `pkg-config --cflags sdl2`
CCLINK ?= `pkg-config --libs SDL2_image` `pkg-config --libs sdl2`

bin/main: src/main.cpp Makefile
	mkdir -p bin
	g++ -o $@ $(CCFLAGS) $< $(CCLINK)

clean:
	rm -f bin/main
