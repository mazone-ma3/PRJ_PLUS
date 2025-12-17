/* rainamg.c By m@3 2025. */

#include <stdio.h>
#include <stdlib.h>

#include <hardware/Custom.h>
#include <hardware/dmabits.h>

#include <hardware/cia.h>

#include <proto/exec.h>
#include <proto/lowlevel.h>

#include <libraries/dos.h>
#include <proto/dos.h>
#include <clib/intuition_protos.h>

#include <clib/graphics_protos.h>

/************************************************************************/
/*		BIT操作マクロ定義												*/
/************************************************************************/

/* BITデータ算出 */
#define BITDATA(n) (1 << (n))

/* BITセット */
#define BITSET2(BITNUM, NUMERIC) {	\
	NUMERIC |= BITDATA(BITNUM);		\
}

/* BITクリア */
#define BITCLR2(BITNUM, NUMERIC) {	\
	NUMERIC &= ~BITDATA(BITNUM);	\
}

/* BITチェック */
#define BITTST(BITNUM, NUMERIC) (NUMERIC & BITDATA(BITNUM))

/* BIT反転 */
#define BITNOT(BITNUM, NUMERIC) {	\
	NUMERIC ^= BITDATA(BITNUM);		\
}

#define Custom_BASE 0xDFF000
#define Custom (*(volatile struct Custom *)Custom_BASE)

#define bplpt 0x0e0
#define sprpt2 0x120

enum {
	BPL1PTH = bplpt + 0x00,
	BPL1PTL = bplpt + 0x02,

	BPL2PTH = bplpt + 0x04,
	BPL2PTL = bplpt + 0x06,

	BPL3PTH = bplpt + 0x08,
	BPL3PTL = bplpt + 0x0a,

	BPL4PTH = bplpt + 0x0c,
	BPL4PTL = bplpt + 0x0e,

	SPR0PT  = sprpt2  + 0x00,
	SPR0PTH = SPR0PT + 0x00,
	SPR0PTL = SPR0PT + 0x02,

	SPR1PT  = sprpt2  + 0x04,
	SPR1PTH = SPR1PT + 0x00,
	SPR1PTL = SPR1PT + 0x02,

	SPR2PT  = sprpt2  + 0x08,
	SPR2PTH = SPR2PT + 0x00,
	SPR2PTL = SPR2PT + 0x02,

	SPR3PT  = sprpt2  + 0x0C,
	SPR3PTH = SPR3PT + 0x00,
	SPR3PTL = SPR3PT + 0x02,

	SPR4PT  = sprpt2  + 0x10,
	SPR4PTH = SPR4PT + 0x00,
	SPR4PTL = SPR4PT + 0x02,

	SPR5PT  = sprpt2  + 0x14,
	SPR5PTH = SPR5PT + 0x00,
	SPR5PTL = SPR5PT + 0x02,

	SPR6PT  = sprpt2  + 0x18,
	SPR6PTH = SPR6PT + 0x00,
	SPR6PTL = SPR6PT + 0x02,

	SPR7PT  = sprpt2  + 0x1C,
	SPR7PTH = SPR7PT + 0x00,
	SPR7PTL = SPR7PT + 0x02,
};

unsigned short COPPERL[] = {
	BPL1PTH,0x0002, //;Bitplane 1 pointer = $22000
	BPL1PTL,0x2000,

	BPL2PTH,0x0002, //;Bitplane 2 pointer = $28000
	BPL2PTL,0x6000,

	BPL3PTH,0x0002, //;Bitplane 3 pointer = $2a000
	BPL3PTL,0xa000,

	BPL4PTH,0x0002, //;Bitplane 4 pointer = $2e000
	BPL4PTL,0xe000,


	SPR0PTH,0x0002, //;Sprite 0 pointer = $21000
	SPR0PTL,0x1000,
	SPR1PTH,0x0002, //;Sprite 1 pointer = $21100
	SPR1PTL,0x1100,
	SPR2PTH,0x0002, //;Sprite 2 pointer = $21200
	SPR2PTL,0x1200,
	SPR3PTH,0x0002, //;Sprite 3 pointer = $21300
	SPR3PTL,0x1300,
	SPR4PTH,0x0002, //;Sprite 4 pointer = $21400
	SPR4PTL,0x1400,
	SPR5PTH,0xd002, //;Sprite 5 pointer = $21500
	SPR5PTL,0x1500,
	SPR6PTH,0x0002, //;Sprite 6 pointer = $21600
	SPR6PTL,0x1600,
	SPR7PTH,0x0002, //;Sprite 7 pointer = $21700
	SPR7PTL,0x1700,
	0xFFFF,0xFFFE, //End of Copper list
};
//	;;
//	;;  Sprite data
//	;;
unsigned short SPRITE[] = {
	0x6D60,0x7200, //VSTART, HSTART, VSTOP
/*
	0x0000, 0x0000,
	0x0FF0, 0x07E0,
	0x1FF8, 0x0FF0,
	0x3FFC, 0x1FF8,
	0x7FFE, 0x3FFC,
	0xFFFF, 0x7FFE,
	0xFFFF, 0x7FFE,
	0x7FFE, 0x3FFC,
	0x3FFC, 0x1FF8,
	0x1FF8, 0x0FF0,
	0x0FF0, 0x07E0,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
*/
	 0x07f0, 0x0808,	// RAIN CHR
	 0x0808, 0x17f4,
	 0x13e4, 0x2c1a,
	 0x1ffc, 0x2002,
	 0x16ec, 0x2912,
	 0x16ec, 0x2912,
	 0x1ddc, 0x2ffa,
	 0x1ddc, 0x0ff8,
	 0x0ff8, 0x03e0,
	 0x1ffc, 0x1ffc,
	 0x300f, 0x6ff3,
	 0x3013, 0x6fef,
	 0x1024, 0x1fde,
	 0x0ff8, 0x01e0,
	 0x0000, 0x0ff8,
	 0x0630, 0x19cc,

	0x0000,0x0000, //End of sprite data
};

unsigned short SPRITE2[] = {
	0x6D60,0x7200, //VSTART, HSTART, VSTOP

	0x03C0, 0x03C0,
	0x07E0, 0x07E0,
	0x07E0, 0x07E0,
	0x03C0, 0x03C0,

	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,

	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,

	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,

	0x0000, 0x0000  // short
};

unsigned short SPRITE3[] = {
/*	0x6D60,0x7200, //VSTART, HSTART, VSTOP

	0x7FFE, 0x3FFC,
	0xFFFF, 0x7FFE,
	0xFFFF, 0x7FFE,
	0x7FFE, 0x3FFC,
	0x3FFC, 0x1FF8,
	0x1FF8, 0x0FF0,
	0x0FF0, 0x07E0,
	0x0000, 0x0000,

	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,

	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,*/

	0x8080,0x8D00,		// TOFU
	0xffff,0x0000,
	0xffff,0x0000,
	0xffff,0x0000,
	0xffff,0x0000,

	0xffff,0x0000,
	0xffff,0x0000,
	0xffff,0x0000,
	0xffff,0x0000,

	0xffff,0x0000,
	0xffff,0x0000,
	0xffff,0x0000,
	0xffff,0x0000,

	0xffff,0x0000,
	0xffff,0x0000,
	0xffff,0x0000,
	0xffff,0x0000,

	0x0000, 0x0000
};


#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200 // PAL

// max/min マクロ
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

// シンプルスプライトデータ (16x16, 2色)
// プレイヤー船 (sprite0)
UWORD player_sprite[] = {
	0x0000, 0x0000,
	0x0FF0, 0x07E0,
	0x1FF8, 0x0FF0,
	0x3FFC, 0x1FF8,
	0x7FFE, 0x3FFC,
	0xFFFF, 0x7FFE,
	0xFFFF, 0x7FFE,
	0x7FFE, 0x3FFC,
	0x3FFC, 0x1FF8,
	0x1FF8, 0x0FF0,
	0x0FF0, 0x07E0,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000  // end
};

// 弾 (sprite1-3: 小四角)
UWORD bullet_sprite[] = {
	0x03C0, 0x03C0,
	0x07E0, 0x07E0,
	0x07E0, 0x07E0,
	0x03C0, 0x03C0,

	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,

	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,

	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,
	0x0000, 0x0000,

	0x0000, 0x0000
};

// 敵 (sprite4-7: 逆船)
UWORD enemy_sprite[] = {
	0x7FFE, 0x3FFC,
	0xFFFF, 0x7FFE,
	0xFFFF, 0x7FFE,
	0x7FFE, 0x3FFC,
	0x3FFC, 0x1FF8,
	0x1FF8, 0x0FF0,
	0x0FF0, 0x07E0,
	0x0000, 0x0000,
	0x0000, 0x0000
};

// 構造体
typedef struct {
	int x, y;
	int active;
} Entity;

Entity player = {120, 160};
Entity bullets[3] = {{0}};
Entity enemies[4] = {{0}};
Entity pluses[8] = {{0}};  // Plus最大8個

int score = 0;
int combo = 0;
int spawn_timer = 0;
int combo_timer = 0;
int wave = 1;
int enemies_killed_this_wave = 0;
int game_over = 0;

unsigned char org_pal[16][3] = {
	{  0,  0,  0},
	{  0,  0,  0},
	{  3, 13,  3},
	{  7, 15,  7},
	{  3,  3, 15},
	{  5,  7, 15},
	{ 11,  3,  3},
	{  5, 13, 15},
	{ 15,  3,  3},
	{ 15,  7,  7},
	{ 13, 13,  3},
	{ 13, 13,  7},
	{  3,  9,  3},
	{ 13,  5, 11},
	{ 11, 11, 11},
	{ 15, 15, 15},
};

#define MSXWIDTH 256
#define MSXLINE 212

unsigned char pattern[10];
unsigned char msxcolor[MSXWIDTH / 2][MSXLINE];
unsigned short *vram_adr;

unsigned char read_pattern[MSXWIDTH * MSXLINE * 2+ 2];

BPTR stream[1];

unsigned char conv_tbl[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 , 15};

short sc5_load(char *loadfil, short x, short y, short msxline)
{
	long i, j, count, count2;
	int k=0, l=0, m=0;

	unsigned short amgcolor[4];
	unsigned char msxcolor[8];
	unsigned char color;
	unsigned short vram_adr;

	if ((stream[0] = Open( loadfil, MODE_READWRITE)) == NULL) {
//		printf("Can\'t open file %s.", loadfil);

		Close(stream[0]);
		return 1;
	}
	Read(stream[0], pattern, 1);	/* MSX先頭を読み捨てる */
	Read(stream[0], pattern, 4);	/* MSXヘッダも読み捨てる */

	Read(stream[0], pattern, 2);	/* MSXヘッダを読み捨てる */


	i = Read(stream[0], read_pattern, MSXWIDTH * msxline);
	m = 0;
//	if(i < 1)
//		break;
	for(count2 = 0; count2 < MSXWIDTH * msxline / 4 / 2; ++count2){
	
		/* 色分解 */
		msxcolor[0] = (read_pattern[m] >>4) & 0x0f;
		msxcolor[1] = read_pattern[m++] & 0x0f;
		msxcolor[2] = (read_pattern[m] >> 4) & 0x0f;
		msxcolor[3] = read_pattern[m++] & 0x0f;
		msxcolor[4] = (read_pattern[m] >>4) & 0x0f;
		msxcolor[5] = read_pattern[m++] & 0x0f;
		msxcolor[6] = (read_pattern[m] >> 4) & 0x0f;
		msxcolor[7] = read_pattern[m++] & 0x0f;

		for(i = 0; i < 4; ++i){
			amgcolor[i] = 0;
		}
		for(j = 0; j < 8; ++j){
			for(i = 0; i < 4; ++i){
				color = conv_tbl[msxcolor[j]];	/* 色変換 */
				if(BITTST(i, color)){
					BITSET2(7-j, amgcolor[i]);
				}else{
					BITCLR2(7-j, amgcolor[i]);
				}
			}
		}
		for(i = 0; i < 4; ++i){
			pattern[i] = amgcolor[i];
		}
		vram_adr = k + l + (x + y * 32); //); // * 2;
		*((unsigned char *)(vram_adr + 0x22000L)) = pattern[0];
		*((unsigned char *)(vram_adr + 0x26000L)) = pattern[1];
		*((unsigned char *)(vram_adr + 0x2a000L)) = pattern[2];
		*((unsigned char *)(vram_adr + 0x2e000L)) = pattern[3];

		k += 1;
		if(k >= (32)){
			k = 0;
			l += 40;
		}
	}
	Close(stream[0]);

	return 0;
}

int sprite_pattern_no[8], old_sprite_pattern_no[8], spr_x[8], spr_y[8];

// スプライトポインタ設定
void set_sprite(int num, int posx, int posy) {
	spr_x[num] = posx;
	spr_y[num] = posy;
}

void set_sprite_all(void) {
	int no;
	unsigned long *pointer, *pointer2;
//	Custom.spr[num].pos = (posy << 8) | (posx & 0xff);
//	Custom.spr[num].ctl = ((posy & 0xff) << 8) | ((posx >> 8) & 1) | 0x80;  // attach
//	Custom.sprpt[num] = data;
	for(int i = 0; i < 8; ++i){

		if((no = sprite_pattern_no[i]) != old_sprite_pattern_no[i]){
			old_sprite_pattern_no[i] = no;

			pointer = (unsigned long *)((unsigned char *)0x21000L + 0x100 * i);
//			sprite_pattern_no[i] = no;
			switch(no){
				case 0:
					pointer2 = (unsigned long *)SPRITE;
					break;

				case 1:
					pointer2 = (unsigned long *)SPRITE2;
					break;

				case 2:
					pointer2 = (unsigned long *)SPRITE3;
					break;
			}

			pointer++;
			pointer2++;
			for(int j = 1; j < 17; ++j){
				*(pointer++) = *(pointer2++);
			}
		}

		*((unsigned char *)0x21001 + 0x100 * i) =spr_x[i]+ 64 - 16;	// H_START

		*((unsigned char *)0x21000 + 0x100 * i) = spr_y[i] + 44 - 16;		// V_START

		*((unsigned char *)0x21002 + 0x100 * i) = spr_y[i] + 16 + 44 - 16;	// V_STOP
	}
}


void set_sprite_pattern(int num, int no) {
	sprite_pattern_no[num] = no;
}

void set_sprite_pattern_all(int num, int no) {

}

int main(void)
{
	long i = 0;
	unsigned long *pointer, *pointer2;

	struct Library *LowLevelBase = OpenLibrary("lowlevel.library", 39L);
	if (!LowLevelBase) {
//		printf("cannot open lowlevel.library\n");
		return 10;
	}

	/* 一時停止 */
	Custom.dmacon = 0x8120;
	Custom.intena = 0x8000;

	/* 画面設定 */
	Custom.bplcon0 = 0x4200;
	Custom.bpl1mod = 0x0000; //MOVE.W  #$0000,BPL1MOD(a0) ;Modulo = 0
	Custom.bplcon1 = 0x0000; //MOVE.W  #$0000,BPLCON1(a0) ;Horizontal scroll value = 0

	Custom.bplcon2 = 0x0024; //	MOVE.W  #$0024,BPLCON2(a0) ;Sprites have priority over playfields
	Custom.ddfstrt = 0x0038; //	MOVE.W  #$0038,DDFSTRT(a0) ;Set data-fetch start
	Custom.ddfstop = 0x00d0; //	MOVE.W  #$00D0,DDFSTOP(a0) ;Set data-fetch stop

	Custom.diwstrt = 0x2c81;
	Custom.diwstop = 0xf4c1;


	// set sprite
	pointer = (unsigned long *)0x20000L;
	pointer2 = (unsigned long *)COPPERL;

	do{
		*(pointer++) = *(pointer2++);
	}while(*pointer2 != 0xfffffffe);

	for(i = 0; i < 8; ++i)
		*((unsigned long *)((unsigned char *)0x21000L + 0x100 * i)) = 0;


	for(i = 0; i < 8; ++i){
		old_sprite_pattern_no[i] = 255;
		pointer = (unsigned long *)((unsigned char *)0x21000L + 0x100 * i);
		switch(i){
			case 0:
				set_sprite_pattern(i, 0);
				break;

			case 1:
			case 2:
			case 3:
				set_sprite_pattern(i, 1);
				break;

			case 4:
			case 5:
			case 6:
			case 7:
				set_sprite_pattern(i, 2);
				break;
		}
	}


	/* COPPERLへのポインタセット */
	Custom.cop1lc = 0x20000L; //MOVE.L  #$20000,COP1LC(a0)
	Custom.copjmp1 = 0;
	Custom.dmacon = 0x83a0;

	/* pallet */
	for(i = 0; i < 16; i+=4){
		Custom.color[17 + i] = 0xff0;//0x0ff0;
		Custom.color[18 + i] = 0xf00;//0x00ff;
		Custom.color[19+i] = 0xfff;//0x0f0f;
	}
	for(i = 0; i < 16; ++i)
		Custom.color[i] = org_pal[i][2] / 1 | ((org_pal[i][1]/1) << 4) | ((org_pal[i][0]/1) << 8);

	for(i = 0; i < 8; ++i)
		set_sprite(i, i * 8 + 16, i * 16 + 16);
	set_sprite_all();
	Custom.dmacon = DMAF_SETCLR | DMAF_SPRITE;

	// 画面クリア
	pointer = (unsigned long *)0x22000L;
	i = SCREEN_WIDTH * SCREEN_HEIGHT / 8 - 4;
	do{
		*(unsigned long *)((unsigned char *)(i + 0x22000L)) = 0;
		*(unsigned long *)((unsigned char *)(i + 0x26000L)) = 0xffffffffL;
		*(unsigned long *)((unsigned char *)(i + 0x2a000L)) = 0;
		*(unsigned long *)((unsigned char *)(i + 0x2e000L)) = 0;
	}while((i-=4) >= 0);

	sc5_load("RAINCHR5.SC5", 0, 0, 16*3); //212);

	for(;;){
//	while (!game_over) {
		ULONG joy =ReadJoyPort(1);

		// 移動 (仮にキーボードor joy direct)
		if (joy & JPF_JOY_UP) player.y -= 3;  // up
		if (joy & JPF_JOY_DOWN) player.y += 3;  // down
		if (joy & JPF_JOY_LEFT) player.x -= 3;  // left
		if (joy & JPF_JOY_RIGHT) player.x += 3;  // right

		player.x = MAX(16, MIN(((SCREEN_WIDTH + 16) / 2), player.x));
		player.y = MAX(16, MIN(((SCREEN_HEIGHT)), player.y));

		set_sprite(0, player.x, player.y);

		// 射撃 (ボタン押したら弾発射)
		if (joy & JPF_BTN1) {  // adjust bit
			for (int i = 0; i < 3; i++) {
				if (!bullets[i].active) {
					bullets[i].active = 1;
					bullets[i].x = player.x;
					bullets[i].y = player.y - 16;
					break;
				}
			}
		}

		// 弾更新
		for (int i = 0; i < 3; i++) {
			if (bullets[i].active) {
				bullets[i].y -= 8;
				if (bullets[i].y < 0) bullets[i].active = 0;
				set_sprite(i+1, bullets[i].x, bullets[i].y);
			} else {
				set_sprite(i+1, 0, 0);  // hide
			}
		}

		// 敵スポーン&更新
		if (++spawn_timer > 10) {
			spawn_timer = 0;
			for (int i = 0; i < 4; i++) {
				if (!enemies[i].active) {
					enemies[i].active = 1;
					enemies[i].x = 32/2 + (i*64)/2;
					enemies[i].y = 0;
					break;
				}
			}
		}

		for (int i = 0; i < 4; i++) {
			if (enemies[i].active) {
				if (!pluses[i].active) {
					set_sprite_pattern(i+4, 2);
					enemies[i].y += 2;
					if (enemies[i].y > (SCREEN_HEIGHT + 16)) enemies[i].active = 0;
					set_sprite(i+4, enemies[i].x, enemies[i].y);
				} else {
					set_sprite(i+4, 0, 0);
				}
			}else{
				set_sprite(i+4, 0, 0);
			}
		}

		// 衝突判定: 弾 vs 敵
		for (int b = 0; b < 3; b++) {
			if (!bullets[b].active) continue;
			for (int e = 0; e < 4; e++) {
				if (pluses[e].active) continue;
				if (!enemies[e].active) continue;
				if (abs(bullets[b].x - enemies[e].x) < 16 && abs(bullets[b].y - enemies[e].y) < 16) {
					bullets[b].active = 0;
//					enemies[e].active = 0;
					set_sprite(b+1, 0, 0);
					set_sprite(e+4, 0, 0);

					// Plus生成
//					for (int p = 0; p < 1; p++) {
//						if (!pluses[e].active) {
							pluses[e].active = 1;
							pluses[e].x = enemies[e].x;
							pluses[e].y = enemies[e].y;
//							break;
//						}
//					}

					combo++;
					combo_timer = 0;
					score += 20 * combo;
					enemies_killed_this_wave++;

					// WAVEクリア判定
					if (enemies_killed_this_wave >= 5 * wave) {
						wave++;
						enemies_killed_this_wave = 0;
					}
					break;
				}
			}
		}

		// Plus取得
		for (int p = 0; p < 4; p++) {
			if (pluses[p].active) {
				set_sprite_pattern(p+4, 0);
				pluses[p].y += 1;
				if (abs(pluses[p].x - player.x) < 16 && abs(pluses[p].y - player.y) < 16) {
					pluses[p].active = 0;
					enemies[p].active = 0;
					score += 50 * combo;
					combo_timer = 0;
				}else{
					set_sprite(p+4, pluses[p].x, pluses[p].y);
					if (pluses[p].y > (SCREEN_HEIGHT + 16)){
						pluses[p].active = 0;
						enemies[p].active = 0;
					}
				}
			}
		}

		// コンボ切れ
		if (combo_timer > 60) {
			combo = 0;
		}

		// ゲームオーバー (敵接触)
		for (int e = 0; e < 4; e++) {
			if (enemies[e].active && abs(enemies[e].x - player.x) < 20 && abs(enemies[e].y - player.y) < 20) {
				game_over = 1;
			}
		}
//		while(((Custom.vhposr / 256))); // == 0x20));
		WaitTOF();
		set_sprite_all();

//		*((unsigned char *)0x25001) = player.x;	// H_START
//		*((unsigned char *)0x25000) = player.y;		// V_START
//		*((unsigned char *)0x25002) = player.y + 16;	// V_STOP
	}
	// ゲームオーバー画面 (スプライト全消し)
//	for (int i = 0; i < 8; i++)
//		set_sprite(i, player_sprite, 0, 0);

	// ここにテキスト描画追加可能
//	}
	return(0);
}

