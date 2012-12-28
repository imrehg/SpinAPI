# This Makefile builds spinapi as a static library on Linux.

# Name of the library
SPINAPI = spinapi

CC = gcc
CFLAGS = -Wall -I/usr/include -I./FTD2XX -I./fftw -lusb -lm -lftd2xx -lfftw $(DEFINES)
COMPILE=$(CC) $(CFLAGS) -c
OBJS=spinapi.o util.o caps.o if.o usb.o driver-linux-usb.o driver-linux-direct.o 

# "all" is the default target. Simply make it point to SpinAPI.
all: SpinAPI

SpinAPI:$(OBJS)
	-mkdir .temp
	(cd .temp && ar x ../fftw/libfftw.a)
	(cd .temp && ar x ../FTD2XX/libftd2xx.a)
	ar rc libspinapi.a $(OBJS) ./.temp/*.o


# Rule to make object files from C files
%.o:%.c
	$(COMPILE) $<
#	-sudo cp spinapi.h /usr/include/

clean:
	-rm $(OBJS)
	-rm libspinapi.a
	-rm -r ./.temp
#	-sudo rm /usr/include/spinapi.h
