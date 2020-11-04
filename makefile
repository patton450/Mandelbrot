CFLAGS=-std=c++17 -Wall -Werror -pthread

all:
	g++ $(CFLAGS) main.cpp -o mandelbrot
