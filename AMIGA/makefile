# plustaker2 makefile for AMIGA OS GCC

TARGET = plustakeramg

LIBPATH = c:/amiga-gcc/m68k-amigaos/lib/
LIBPATH2 = c:/amiga-gcc/m68k-amigaos/lib/libb/
LIBPATH3 = c:/amiga-gcc/lib/gcc/m68k-amigaos/6.5.0b/

#É}ÉNÉç
CC = m68k-amigaos-gcc

CFLAGS = -c -Os -m68000 -msoft-float
LINKOPTS = -noixemul -Wl,-Map,output.map -m68000

.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) -o $*.o $<
#	$(CC) $(CFLAGS) -S -o $*.s $<
#	m68k-amigaos-as -o $*.o $*.s

.SUFFIXES: .o .s
.s.o:
	m68k-amigaos-as -o $*.o $*.s

#ê∂ê¨éËèá

all:$(TARGET)

clean:
	rm $(TARGET) $(TARGET).o

OBJFILES = $(TARGET).o muldiv.o


$(TARGET) : $(OBJFILES)
	$(CC) $(LINKOPTS)  $(OBJFILES) -o $(TARGET) 
	adf.bat $(TARGET) RAINCHR5.SC5 FONTYOKO.SC5 se1.raw

$(TARGET).o : $(TARGET).c
muldiv.o : muldiv.s
