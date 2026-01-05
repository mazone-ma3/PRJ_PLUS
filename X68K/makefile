# --- 設定 ---
TARGET      = plustaker68
CC          = m68k-xelf-gcc
AS          = m68k-xelf-as

# X68000(68000)ターゲット、最適化、文字コード変換（SJIS出力）
CFLAGS      = -m68000 -O2 -finput-charset=utf-8 -fexec-charset=cp932
# リンクオプション：0x6800番地開始、シンボル削除、数学ライブラリ
LDFLAGS     = -Ttext=0x6800 -s -lm

# 中間ファイル
XSPLIBFILES =  xsp2lib.o pcm8afnc.o
OBJS        = $(TARGET).o

# --- ルール ---

all: $(TARGET).X

# 1. パターンルール
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) -o $@ $<

# 2. リンク工程
$(TARGET).X: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

# 3. (おまけ) XSP対応にする場合
xsp:  $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET)_XSP.X $(OBJS) $(XSPLIBFILES)

# --- 依存関係 ---
$(TARGET).o: $(TARGET).c inkey.h

clean:
	rm -f $(TARGET).o $(TARGET).X
