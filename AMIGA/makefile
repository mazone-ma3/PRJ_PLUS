# plustaker2 makefile for AMIGA OS GCC

TARGET = plustakeramg

LIBPATH = c:\amiga-gcc/m68k-amigaos/lib/

#É}ÉNÉç
CC = m68k-amigaos-gcc

CFLAGS = -c
LINKOPTS = -Os -noixemul

.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) -c $<

.SUFFIXES: .o .s
.s.o:
	m68k-amigaos-as -o $*.o $*.s

#ê∂ê¨éËèá

all:plustaker2

clean:
	rm $(TARGET) $(TARGET).o

OBJFILES = $(TARGET).o

plustaker2 : $(OBJFILES)
	$(CC) $(LINKOPTS) $(OBJFILES) -o $(TARGET)
# -lamiga
	adf.bat $(TARGET) RAINCHR5.SC5 FONTYOKO.SC5

$(TARGET) : $(TARGET).c
