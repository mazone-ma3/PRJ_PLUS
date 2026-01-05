# --- 設定 ---
TARGET   = plustakerva
CC       = wcl
AS       = wasm
LD       = wlink

# CFLAGS: -ms (Smallモデル), -bt=dos (DOSターゲット), -ox (最大最適化), -0 (8086用)
CFLAGS   = -ms -bt=dos -ox -0
ASFLAGS  = -ms -bt=dos -0

# 中間ファイル
OBJS     = $(TARGET).obj

# --- ルール ---

all: $(TARGET).EXE

# 1. パターンルール (.c -> .obj)
# wcl は -c オプションを付けないとリンクまで行こうとするので注意
%.obj: %.c
	$(CC) -c -fo=$@ $(CFLAGS) $<

# 2. パターンルール (.asm -> .obj)
%.obj: %.asm
	$(AS) $(ASFLAGS) -fo=$@ $<

# 3. リンク工程
# wlink の独自記法を整理
$(TARGET).EXE: $(OBJS)
	$(LD) system Dos option quiet name $@ file { $(OBJS) }

# --- 依存関係 ---
$(TARGET).obj: $(TARGET).c inkey.h

clean:
	rm -f *.obj *.EXE *.err *.map
