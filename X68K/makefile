# plustaker2 makefile for X680x0 GCC (elf2x68k) (XSPëIëéÆ)

TARGET= plustaker68

#É}ÉNÉç
CC = m68k-xelf-gcc

#CFLAGS =  -O -Wall -g
CFLAGS = -m68000 -O2 -finput-charset=utf-8 -fexec-charset=cp932

#LINKOPTS = -liocs -ldos
# -lsignal
LINKOPTS = -Ttext=0x6800 -s
# -specs=x68knodos.specs

.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) -c $<

.SUFFIXES: .o .s
.s.o:
	m68k-xelf-as -o $*.o $*.s

#ê∂ê¨éËèá

all:$(TARGET).X

clean:
	rm $(TARGET).o $(TARGET).X

OBJFILES =$(TARGET).o

#XSPLIBFILES = xsp2lib.o pcm8afnc.o

$(TARGET).X : $(OBJFILES)
	$(CC) $(LINKOPTS) -o $(TARGET).X $(OBJFILES) $(XSPLIBFILES) -lm
# $(CFLAGS) $(LINKOPTS)

$(TAEGET).o : $(TARGET).c
