# --- 設定 ---
TARGET       = plustakerfmt
GCCROOT      = C:/tgcc
BIN          = $(GCCROOT)/bin
LIB_PATH     = $(GCCROOT)/lib
INC_PATH     = $(GCCROOT)/include

# ツール定義
CPP          = $(BIN)/cpp
CC1          = $(BIN)/cc1
AS           = $(BIN)/as
LD           = $(BIN)/ld_towns
GENEXP       = $(BIN)/genexp3

# フラグ定義
CPPFLAGS     = -D__GNUC__ -DTOWNS -I. -I$(INC_PATH)
CC1FLAGS     = -O -quiet -msoft-float
LDFLAGS      = -N -S -L$(LIB_PATH) -lTN -lce -lme -lpc

# 中間ファイル
OBJS         = $(TARGET).o VSYNC.o
CRT0         = $(LIB_PATH)/crt0.o

# --- ルール ---

all: $(TARGET).EXP

# 1. パターンルール（.c -> .o）
# 中間ファイル .i や .s を自動で処理する流れを定義
%.o: %.c
	$(CPP) $(CPPFLAGS) $< $*.i
	$(CC1) $(CC1FLAGS) $*.i -o $*.s
	$(AS) -o $@ $*.s
	@rm -f $*.i $*.s

# 2. パターンルール（.s -> .o）
%.o: %.s
	$(AS) -o $@ $<

# 3. リンク（.bin生成）
$(TARGET).bin: $(OBJS)
	$(LD) -o $@ $(CRT0) $(OBJS) $(LDFLAGS)

# 4. EXPファイル変換
$(TARGET).EXP: $(TARGET).bin
	$(GENEXP) $< $@

# --- 依存関係 ---
$(TARGET).o: $(TARGET).c
VSYNC.o: VSYNC.s

clean:
	rm -f *.o *.bin *.EXP *.i *.s