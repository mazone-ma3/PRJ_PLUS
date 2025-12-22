# plustaker2 makefile for FM TOWNS-gcc クロスコンパイル環境

TARGET = plustakerfmt

#マクロ

GCCROOT=C:/tgcc
LIBRARY_PATH=$(GCCROOT)/lib
C_INCLUDE_PATH=$(GCCROOT)/include

LINK = $(GCCROOT)/bin/ld_towns -N -S -o $@ $(LIBRARY_PATH)/crt0.o
LINKOPTS = -L$(LIBRARY_PATH) -lTN -lce -lme -lpc 

# Cソ－スからobj作成
.SUFFIXES: .o .c
.c.o:
	$(GCCROOT)/bin/cpp  -D__GNUC__ -DTOWNS -I. -I $(C_INCLUDE_PATH) $*.c $*.i
	$(GCCROOT)/bin/cc1 -O -quiet -msoft-float $*.i
	$(GCCROOT)/bin/as -o $*.o $*.s

.SUFFIXES: .o .s
.s.o:
	$(GCCROOT)/bin/as -o $*.o $*.s

#生成手順

all:$(TARGET).EXP

OBJFILES = $(TARGET).o VSYNC.o

$(TARGET).EXP : $(TARGET).bin
	$(GCCROOT)/bin/genexp3 $(TARGET).bin $(TARGET).EXP

$(TARGET).bin : $(OBJFILES)
	$(LINK) $(OBJFILES) $(LINKOPTS)

$(TARGET).o : $(TARGET).c
VSYNC.o : VSYNC.s
