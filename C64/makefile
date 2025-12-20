# plustake2 makefile for C64 llvm-cmos

TARGET = plustaker64

#É}ÉNÉç
CC = mos-c64-clang.bat

CFLAGS2 = -o $*.s -Wl,--lto-emit-asm
CFLAGS = -Os -o $*.o -c

#LINKOPTS = -Os
#-noixemul

.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) $<
#	$(CC) $(CFLAGS2) $<
#	$(CC) -o $*.o $*.s.lto.s -c

.SUFFIXES: .o .s
.s.o:
	$(CC) -o $*.o $*.s -c

#ê∂ê¨éËèá

all:$(TARGET).prg

clean:
	rm $(TARGET).prg $(TARGET).o

OBJFILES = $(TARGET).o

$(TARGET).prg : $(OBJFILES)
	$(CC) $(LINKOPTS) $(OBJFILES) -o $(TARGET).prg

#$(TARGET).o : $(TARGET).s
$(TARGET).o : $(TARGET).c
