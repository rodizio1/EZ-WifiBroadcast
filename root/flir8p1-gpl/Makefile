CC = gcc -I/usr/include/libusb-1.0
GXX = g++
CXXFLAGS      = -pipe -O2 -Wall -W -D_REENTRANT -lusb-1.0 -lm
INCPATH = -I. -I/usr/include/libusb-1.0

all: flir8p1.o flir8p1



flir8p1.o: flir8p1.c plank.h

flir8p1: flir8p1.o
	${CC} -o flir8p1 flir8p1.o -lusb-1.0 -lm -Wall


clean:
	rm -f flir8p1.o flir8p1

