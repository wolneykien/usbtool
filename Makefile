# Name: Makefile
# Project: usbtool
# Author: Christian Starkjohann, Andrey Tolstoy
# Creation Date: 2016-05-26
# Tabsize: 4
# Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
#            (c) 2016 Andrey Tolstoy, LLC Enistek
# License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)


# Concigure the following definitions according to your system.
# This Makefile has been tested on Mac OS X, Linux and Windows.

# Use the following 3 lines on Unix (uncomment the framework on Mac OS X):
USBFLAGS = `pkg-config --cflags libusb-1.0`
USBLIBS = `pkg-config --libs libusb-1.0`
EXE_SUFFIX =

# Use the following 3 lines on Windows and comment out the 3 above. You may
# have to change the include paths to where you installed libusb-win32
#USBFLAGS = -I/usr/local/include
#USBLIBS = -L/usr/local/lib -lusb
#EXE_SUFFIX = .exe

NAME = usbtool

OBJECTS = opendevice.o $(NAME).o

CC		= gcc
CFLAGS	= $(CPPFLAGS) $(USBFLAGS) -O -g -Wall -std=c99 -Wno-pointer-sign
LIBS	= $(USBLIBS)

PROGRAM = $(NAME)$(EXE_SUFFIX)
INSTALL = install
bindir = /usr/bin


all: $(PROGRAM)

.c.o:
	$(CC) $(CFLAGS) -c $<

$(PROGRAM): $(OBJECTS)
	$(CC) -o $(PROGRAM) $(OBJECTS) $(LIBS)

install: $(PROGRAM)
	$(INSTALL) -D -m0755 $(PROGRAM) $(DESTDIR)$(bindir)/$(PROGRAM)

strip: $(PROGRAM)
	strip $(PROGRAM)

clean:
	rm -f *.o $(PROGRAM)
