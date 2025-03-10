CFLAGS = -Wall -I/opt/homebrew/Cellar/libusb/1.0.27/include -L/opt/homebrew/lib
LDFLAGS = -lusb-1.0
CC = gcc

OBJECTS = lab2.o fbputchar.o usbkeyboard.o

TARFILES = Makefile lab2.c \
	fbputchar.h fbputchar.c \
	usbkeyboard.h usbkeyboard.c

lab2 : $(OBJECTS)
	$(CC) $(CFLAGS) -o lab2 $(OBJECTS) $(LDFLAGS) -pthread

lab2.tar.gz : $(TARFILES)
	rm -rf lab2
	mkdir lab2
	ln $(TARFILES) lab2
	tar zcf lab2.tar.gz lab2
	rm -rf lab2

lab2.o : lab2.c fbputchar.h usbkeyboard.h
fbputchar.o : fbputchar.c fbputchar.h
usbkeyboard.o : usbkeyboard.c usbkeyboard.h

.PHONY : clean
clean :
	rm -rf *.o lab2
