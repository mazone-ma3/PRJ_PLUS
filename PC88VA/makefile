# plustaker2 makefile for PC-88VA OpenWatcom

TARGET=plustakerva

#マクロ
CC= wcl

CFLAGS = -ms -bt=dos -ox -0

# Cソ－スからobj作成

.SUFFIXES: .obj .c
.c.obj:
	$(CC) -s $< $(CFLAGS)
#-c

.SUFFIXES: .obj .asm
.asm.obj:
	wasm $< -ms -bt=dos -0

#生成手順

all:$(TARGET).EXE

clean:
	rm $(TARGET).EXE $(TARGET).obj

OBJFILES =  $(TARGET).obj
# test.obj

$(TARGET).EXE : $(OBJFILES)
	wlink  system Dos option quiet name $(TARGET) file { $(OBJFILES) }

$(TARGET).obj : $(TARGET).c

#test.obj : test.asm
