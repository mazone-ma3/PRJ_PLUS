# --- 設定 ---
TARGET   = yokoshtm2
CC       = zcc

# コンパイルオプション（SDCCを使用）
CFLAGS   = +msx -compiler sdcc --opt-code-speed --sdcccall0
# リンクオプション（subtypeで切り替え）
LDFLAGS  = +msx -create-app -lmsxbios
#-lmath32 

# 中間ファイル
OBJS     = com_stg.o sin_table.o $(TARGET).o PSGMSXD.o

# --- ルール ---

all: $(TARGET).bin

# 1. パターンルール（.c から .o を作成）
%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.o: %.asm
	z80asm -b $<

# 2. 生成ルール
$(TARGET).bin: $(OBJS)
	$(CC) $(LDFLAGS) -subtype=disk -zorg=0x83c0 -lmsxbios -o $(TARGET).bin $(OBJS) -m

# 3. (おまけ) DOS用COMファイルやBINも作りたい場合
com: $(OBJS)
	$(CC) $(LDFLAGS) -subtype=msxdos -lndos -lmsxbios -o $(TARGET).com $(OBJS)

bin: $(OBJS)
	$(CC) $(LDFLAGS) -subtype=disk -zorg=0x83c0 -lmsxbios -o $(TARGET).bin $(OBJS)

#rom: $(OBJS)
#	$(CC) $(LDFLAGS) -subtype=rom -o $(TARGET) $(OBJS) -m -v

# 依存関係
$(TARGET).o: $(TARGET).c inkey.h yokoshtm2.h PSGMSXD.H spr_col.h
com_stg.o: com_stg.c yokoshtm2.h
sin_table.o: sin_table.c
PSGMSXD.o: PSGMSXD.asm PSGTONE.INC C_6_2.INC

clean:
	rm -f *.o *.rom *.map *.com *.bin *.map *.ihx *.cas
