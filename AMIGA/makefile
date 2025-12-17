# plustaker2 makefile for AMIGA OS GCC

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

OBJFILES = plustaker2.o

plustaker2 : $(OBJFILES)
	$(CC) $(LINKOPTS) $(OBJFILES) -o plustaker2 -lamiga
	adf.bat plustaker2 RAINCHR5.SC5

plustaker2.o : plustaker2.c
