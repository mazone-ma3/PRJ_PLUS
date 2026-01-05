# --- 設定 ---
TARGET   = plustaker64
CC       = mos-c64-clang.bat

# -Os (サイズ最適化)
CFLAGS   = -Os
LDFLAGS  = -Os

# 中間ファイル
OBJS     = $(TARGET).o

# --- ルール ---

all: $(TARGET).prg

# 1. パターンルール (.c -> .o)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 2. パターンルール (.s -> .o)
%.o: %.s
	$(CC) -c $< -o $@

# 3. リンク（.prg生成）
$(TARGET).prg: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

# --- 依存関係 ---
$(TARGET).o: $(TARGET).c

clean:
	rm -f *.o $(TARGET).prg
