/* plustakerva.c  By m@3 3 with Grok 2025. */

#define SCREEN_WIDTH (320)
#define SCREEN_HEIGHT (200)

#define MAIN
#define PC88VA

#define SPROW_ON 1

#include <dos.h>
#include <conio.h>
//#include <spr.h>
//#include <FMCFRB.H>

//#include "sp_init.h"
//#include "FONTVA.h"

enum {
	BGMMAX = 2,
	SEMAX = 4
};

void put_strings(int scr, int x, int y,  char *str, char pal);
void put_numd(long j, char digit);
char str_temp[9];

void __interrupt __far (*keepvector)(void);


#define SPR_ATR 0x7e00
#define SPR_PAT_ADR 0xaa00
#define SPR_SIZE(X, Y) (X / 2 * Y)
#define LINE 16

#define TVRAM_ON() 	outp(0x153, 0x51)		/* T-VRAM選択 */
#define GVRAM_ON() 	outp(0x153, 0x54)		/* G-VRAM選択 */

#define CHRPAL_NO 0
#define REVPAL_NO 1
#define BGPAL_NO 2

#define FONTSIZEX 4
#define FONTSIZEY 8
#define FONTPARTS 64
#define CHRPARTS 32

#define FALSE 0
#define TRUE 1

#define SCREEN1 0
#define SCREEN2 1

#define SHIFT_NUM	0		/* 座標系をシフトする回数(固定小数点演算用) */
enum {
	SPR_OFS_X = -16,
	SPR_OFS_Y = -16
};

#define CONST

typedef struct{
	int x;
	int y;
	int xx;
	int yy;
	int patno;
	int count;
} CHR_OBJ;


typedef struct {
	short patno;
	short x;
	short y;
	unsigned short atr;
	unsigned short x_size;
	unsigned short y_size;
	unsigned short adr;
} SPR_COMB;

typedef struct {
	CONST int patmax;
	CONST SPR_COMB *data;
	unsigned short x_size;
	unsigned short y_size;
	unsigned short adr;
} SPR_INFO;

/************************************************************************/
/*		BIT操作マクロ定義												*/
/************************************************************************/

/* BITデータ算出 */
#define BITDATA(n) (1 << (n))

/* BITセット */
#define BITSET(BITNUM, NUMERIC) {	\
	NUMERIC |= BITDATA(BITNUM);		\
}

/* BITクリア */
#define BITCLR(BITNUM, NUMERIC) {	\
	NUMERIC &= ~BITDATA(BITNUM);	\
}

/* BITチェック */
#define BITTST(BITNUM, NUMERIC) (NUMERIC & BITDATA(BITNUM))

/* BIT反転 */
#define BITNOT(BITNUM, NUMERIC) {	\
	NUMERIC ^= BITDATA(BITNUM);		\
}

// max/min マクロ
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

/* スプライト表示デ－タをVRAMに書き込むマクロ */
#define PUT_SP( X, Y, X_SIZE, Y_SIZE, ADR) {\
	*(spram++) = (Y_SIZE) | (Y & 0x01ff) | 0x0200;\
	*(spram++) = (X_SIZE) | (X & 0x03ff);\
	*(spram++) = (ADR);\
	*(spram++) = 0;\
}

#define EOIDATA 0x20
#define EOI 0x188

#define WIDTH 32
#define CONV_LINE 32 //128 //200

#define MAX_SPRITE 32			/* 最大許容スプライト表示数 */

/* 各キャラクタの構造体(CHR_PARA) */
typedef struct chr_para{
	struct chr_para *next,*prev;
	int x,y,xx,yy,pat_num,atr,count,hp;
	unsigned short x_size,y_size;
	unsigned short adr;
} CHR_PARA;

CHR_PARA chr_data[MAX_SPRITE];
CHR_PARA *pchr_data;

#define DEF_SP_SINGLE( NO, X, Y, PAT, PAL, ATR, X_SIZE, Y_SIZE, ADR) {\
	pchr_data = &chr_data[NO];\
	pchr_data->x = (((X >> SHIFT_NUM) + SPR_OFS_X) * 2 + 1024L) % 1024 ; \
	pchr_data->y = (((Y >> SHIFT_NUM) + SPR_OFS_Y) + 512L) % 512; \
	pchr_data->x_size = X_SIZE; \
	pchr_data->y_size = Y_SIZE; \
	pchr_data->adr = ADR; \
	if(pchr_data->y > 204L){\
		if((512L - pchr_data->y) < ((Y_SIZE / 256 / 4 + 1) * 4)){\
			pchr_data->adr += ( ((X_SIZE / 256 / 8 + 1) * 2) * (512L - pchr_data->y)); \
		}else{\
			pchr_data->y = 204L;\
		}\
	}\
	NO++; \
}


short test_h_f = TRUE;
short soundflag = FALSE;

volatile unsigned char __far vs_count;
unsigned char keepport;
//unsigned short title_index;

// 構造体
typedef struct {
	int x, y;
	int active;
} Entity;

/*Entity player = {120, 160};
Entity bullets[3] = {{0}};
Entity enemies[4] = {{0}};
Entity pluses[8] = {{0}};  // Plus最大8個
*/
long score = 0, hiscore = 5000;
int combo = 0;
/*
int spawn_timer = 0;
int combo_timer = 0;
int wave = 1;
int enemies_killed_this_wave = 0;
int game_over = 0;
*/

int total_count = 0;
unsigned char seflag;

int spr_count,old_count;

#define VECT 0x0a

unsigned char __far *vram;		/* VRAM操作用変数 */
unsigned short __far *spram;		/* スプライトRAM操作用変数 */

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <i86.h>
#include <fcntl.h>
#include <dos.h>
#include <unistd.h>

/*#include "sp.h"
#include "sp_main.h"
#include "sp_init.h"

#include "FONTVA.h"
*/

#define MAXCOLOR 16
#define ON 1
#define OFF 0

/*
void paint(unsigned char color);
void paint2(unsigned char color);
void paint3(unsigned char color);
void erase_allsprite(void);
*/

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

//unsigned short rev_adr = 0;

#define MSXWIDTH 256
#define MSXLINE 212

FILE *fontstream[2];

unsigned char pattern[10];
unsigned char msxcolor[MSXWIDTH / 2][MSXLINE];

unsigned short fontdata[FONTPARTS + CHRPARTS][FONTSIZEX][FONTSIZEY];

unsigned short font_load(char *loadfil, unsigned short fontdata[][FONTSIZEX][FONTSIZEY], int fontparts, int start_x)
{
	long i, j,k,y, x, xx, yy, no, max_xx;
//	unsigned short *fontram = fontdata;

	if ((fontstream[0] = fopen( loadfil, "rb")) == NULL) {
		fprintf(stderr, "Can\'t open file %s.", loadfil);

		fclose(fontstream[0]);
		return 1;
	}

	fread(pattern, 1, 1, fontstream[0]);	/* MSX先頭を読み捨てる */
	fread(pattern, 1, 4, fontstream[0]);	/* MSXヘッダも読み捨てる */
	fread(pattern, 1, 2, fontstream[0]);	/* MSXヘッダを読み捨てる */

	for(y = 0; y < MSXLINE; ++y){
		for(x = 0; x < MSXWIDTH / 2 ; ++x){
			msxcolor[x][y] = 0;
		}
	}
	for(y = 0; y < MSXLINE; ++y){
		for(x = 0; x < MSXWIDTH / 8; ++x){
			i = fread(pattern, 1, 4, fontstream[0]);	/* 8dot分 */
			if(i < 1)
				break;

			/* 色分解 */
			msxcolor[0 + x * 4][y] = pattern[0]; 
			msxcolor[1 + x * 4][y] = pattern[1]; 
			msxcolor[2 + x * 4][y] = pattern[2];
			msxcolor[3 + x * 4][y] = pattern[3];
		}
	}
	fclose(fontstream[0]);
	max_xx = 64;

	j = 0;
	xx=start_x;
	yy=0;
	x=0;
	for(no = 0; no < fontparts; ++no){
//		printf("\nno =%d ",no);
		for(y = 0; y < FONTSIZEY; ++y){
			for(x = 0; x < FONTSIZEX; x+=1){

				if((x+xx) >= (max_xx + start_x)) {
					xx=start_x;
					yy+=FONTSIZEY;
				}

//				*(fontram++) = 
				fontdata[no][x][y]=msxcolor[x + xx][y + yy]; // * 256 + msxcolor[x + xx + 1][y + yy];
			}
		}
		xx+=FONTSIZEX;
	}
	return 0;
}


unsigned short __far *vram2;

void paint(unsigned char color)
{
	unsigned short i, j;

	GVRAM_ON();
	vram2 = (unsigned short __far *)_MK_FP(0xa000, 0);

	for (i = 0; i < ((80 * 408L)); ++i){
		*(vram2 + i) = color + color *256;
	}
}

void paint2(unsigned char color)
{
	unsigned short i, j;

	GVRAM_ON();
//	unsigned short __far *vram;
	vram2 = (unsigned short __far *)_MK_FP(0xc000, 0);

	for (i = 0; i < ((80 * 408L)); ++i){
		*(vram2 + i) = color + color *256;
	}
}

void paint3(unsigned char color)
{
	unsigned short i, j;

	GVRAM_ON();
//	unsigned short __far *vram;
	vram2 = (unsigned short __far *)
		MK_FP(0xc000, 0);

	for (i = ((80 * (100L))); i < ((80 * (204L))); ++i){
		*(vram2 + i) = color + color *256;
	}
}

/*テキスト画面及びグラフィック画面の消去*/
void clear(unsigned short type)
{
	if(type & 1){
		paint(0x0);
		paint2(0x0);
	}

//	if(type & 2)
//		printf("\x1b*");
}

/*パレット・セット*/
void pal_set(unsigned char pal_no, unsigned short color, unsigned char red, unsigned char green,
	unsigned char blue)
{
	switch(pal_no){
		case CHRPAL_NO:
			outpw(0x300 + color * 2, (unsigned short)green * 4096 + red * 64 + blue * 2);
			break;
		case BGPAL_NO:
			outpw(0x320 + color * 2, (unsigned short)green * 4096 + red * 64 + blue * 2);
			break;
	}
}

void pal_all(unsigned char pal_no, unsigned char color[16][3])
{
	unsigned short i;
	for(i = 0; i < MAXCOLOR; i++)
		pal_set(pal_no, i, color[i][0], color[i][1], color[i][2]);
}


#define EOIDATA 0x20
#define EOI_M 0x188
#define EOI_S 0x184
#define IMR_M 0x18a
#define IMR_S 0x186
#define VECT 0x0a

void m_eoi(void)
{
/*	unsigned char a;

	outp(EOI_S,	EOIDATA);
	outp(EOI_S, 0x0b);
	a = inp(EOI_S);
	a &= a;
	if(!a)*/
		outp(EOI_M, EOIDATA);
}

void __interrupt __far ip_v_sync(void)
{
	++vs_count;
//	printf("%d\n", vs_count);
	m_eoi();
}

void init_v_sync(void)
{
	_disable();
	keepport = inp(IMR_M);
	keepvector = _dos_getvect(VECT);
	outp(IMR_M, keepport & 0xfb);

	_dos_setvect(VECT, ip_v_sync);

	m_eoi();
	_enable();
}

void term_v_sync(void)
{
	_disable();
	_dos_setvect(VECT, keepvector);
	outp(0x18a, keepport);
	_enable();
}


/*タイマウェイト*/
void wait(unsigned short wait)
{
	while(1){
		_disable();
		if(vs_count >= wait)
			break;
		_enable();
	}
	_disable();
	vs_count = 0;
	_enable();
}

void write_psg(int reg, int tone)
{
	while(inp(0x44) & 0x80);
	outp(0x44,reg);
	while(inp(0x44) & 0x80);
	outp(0x45, tone);
}

FILE *stream[2];


short set_sprite_pattern1(char *loadfil, unsigned short adr, unsigned short width, unsigned short line, unsigned short spr_x_size, unsigned short spr_y_size, unsigned char spr_x_max, unsigned char spr_y_max)
{
	long i, j,count, count2;
	unsigned short k=0, l=0, m;
	unsigned char pattern[100];
	unsigned short header;

	unsigned short spr_x = 0, spr_y = 0, spr_no = 0, spr_no2 = 0;
	unsigned short index = 0;
	unsigned char __far *tvram = (unsigned char __far *)
		MK_FP(0xa000, 0);
	unsigned short spr_size = SPR_SIZE(spr_x_size, spr_y_size);

	if ((stream[0] = fopen( loadfil, "rb")) == NULL) {
		printf("Can\'t open file %s.", loadfil);

		fclose(stream[0]);
		return -1;
	}
	fread(pattern, 1, 1, stream[0]);	/* MSX先頭を読み捨てる */
	fread(pattern, 1, 4, stream[0]);	/* MSXヘッダも読み捨てる */

	fread(pattern, 1, 2, stream[0]);	/* MSXヘッダを読み捨てる */

	for(spr_no2 = 0; spr_no2 < spr_y_max; ++spr_no2){
		spr_y = spr_no2 * (spr_y_size) * spr_x_max;
		for(count = 0; count < line; ++count){

			for(count2 = 0; count2 < (width / 4); ++count2){

				i = fread(pattern, 1, 2, stream[0]);	/* 8dot分 */
				if(i < 1)
					break;

				index = ((spr_no) * spr_size) + spr_x + (spr_y * (spr_x_size / 2)) ;

				/* 横を2倍する */
				for(m = 0; m < 1; ++m){
					tvram[adr + index] = 
						(((pattern[m * 2 + 0]) >> 4) & 0x0f) |
						(((pattern[m * 2 + 0]) >> 4) & 0x0f) * 16;
					++index;
					tvram[adr + index] = 
						(((pattern[m * 2 + 0]) & 0x0f)) |
						(((pattern[m * 2 + 0]) & 0x0f) * 16);
					++index;
					tvram[adr + index] = 
						(((pattern[m * 2 + 1]) >> 4) & 0x0f) |
						(((pattern[m * 2 + 1]) >> 4) & 0x0f) * 16;
					++index;
					tvram[adr + index] = 
						(((pattern[m * 2 + 1]) & 0x0f)) |
						(((pattern[m * 2 + 1]) & 0x0f) * 16);
					++index;
				}

				spr_x += 4;
				if(spr_x >= (spr_x_size / 2)){
					spr_x = 0;
					++spr_no;
					if(spr_no >= spr_x_max){
						spr_no = 0;
						++spr_y;
					}
				}
			}
			for(count2 = 0; count2 < ((256 - width) / 4); ++count2){

				i = fread(pattern, 1, 2, stream[0]);	/* 8dot分 */
				if(i < 1)
					break;
			}
		}
	}
	fclose(stream[0]);

	return index;
}


void erase_allsprite(void)
{
	unsigned char i;
	TVRAM_ON();
//	unsigned short __far  *spram;
	spram = (unsigned short __far *)
		MK_FP(0xa000, SPR_ATR);
	for(i = 0; i < MAX_SPRITE; ++i){
		*(spram++) = 0;
		*(spram++) = 0;
		*(spram++) = 0;
		*(spram++) = 0;
//		set_sprite_locate(i, 0, 0, 0,(SCREEN_MAX_Y), 0, 0);
	}
}


void g_init(void)
{
	union REGS reg;
	union REGS reg_out;

	reg.h.ah = 0x2a;
	int86(0x83, &reg, &reg_out);	/* テキスト初期化 */

	reg.h.ah = 0x00;
	reg.x.bx = 0xe00e;
	reg.h.cl=4;
	reg.h.ch=4;
	int86(0x8f, &reg, &reg_out);	/* グラフィックBIOS初期化 */

//	reg.h.ah = 0x00;
//	reg.h.al=0;
//	reg.x.dx=SPR_ATR;
//	int86(0x84, &reg, &reg_out);	/* スプライトBIOS初期化 */
//	reg.h.ah = 0x16;
//	int86(0x84, &reg, &reg_out);	/* スプライトBIOSスリープ */

//	outpw(0x100, 0xb000);	/* none-interless Graphic-on 400dot(400line) */
//	outpw(0x100, 0xb020);	/* none-interless Graphic-on 200dot(200line) */
//	outpw(0x100, 0xb062);	/* none-interless Graphic-on 400dot(200line) */

	outpw(0x100, 0xbc62);	/* none-interless Graphic-on 400dot(200line) */
								/* 画面ON single-plane 2画面 */
//	outpw(0x100, 0xb021);

//	outpw(0x102, 0x0101);	/* graphic0/1 Width640 4dot/pixel */
// 	outpw(0x102, 0x1111);	/* graphic0/1 Width320 4dot/pixel */

//	outp(0x153, 0x54);		/* G-VRAM選択 *
//	outp(0x153, 0x51);		/* T-VRAM選択 */

//	outpw(0x106, 0xab89);	/* パレット指定画面割当て指定 */
//	outpw(0x106, 0xa89b);	/* パレット指定画面割当て指定 */
	outpw(0x106, 0xa89b);	/* パレット指定画面割当て指定 */
	outpw(0x108, 0x0000);	/* 直接色指定画面割当て設定 */
	outpw(0x110, 0x008f);	/* 4ビットピクセル */
	outpw(0x10a, 0);
//	outp(0x500, 0);	/* 独立アクセス */
//	outp(0x512, 0);	/* ブロック0 */
//	outp(0x516, 0);	/* 書き込みプレーン選択 */

	outp(0x580, 0x10);

//	outp(0x10d, 0x01);
//	outpw(0x10c, 0x0100);	/* カラーパレットモード (パレット0) */
//	outpw(0x10c, 0x01b0);	/* カラーパレットモード 混在/バレット1:GRP1 */
	outpw(0x10c, 0x01a0);	/* カラーパレットモード 混在/バレット1:GRP0 */

	outpw(0x124,0x00);	/* 透明色 GRP0 */
	outpw(0x126,0x01);	/* 透明色 GRP1 */
	outpw(0x12e,0x01);	/* 透明色 TEXT/SPRITE */
}

/*終了処理*/
void term()
{
	union REGS reg;
	union REGS reg_out;

	erase_allsprite();

	paint(0);
	paint2(0);

	reg.h.ah = 0x00;
	reg.x.bx = 0x2000;
	reg.h.cl=4;
	reg.h.ch=4;
	int86(0x8f, &reg, &reg_out);	/* グラフィックBIOS初期化 */

	reg.h.ah = 0x0a;
	int86(0x8f, &reg, &reg_out);	/* パレット初期化 */

// 	outpw(0x100, 0xb020);	/* none-interless Graphic-on 400dot(400line) */
// 	outpw(0x102, 0x0101);	/* graphic0 Width640 4dot/pixel */
//	TVRAM_ON();
	outpw(0x106, 0xab89);	/* パレット指定画面割当て指定 */
	outpw(0x10c, 0x0180);	/* カラーパレットモード */
//	outpw(0x10c, 0x0110);	/* カラーパレットモード */

	reg.h.ah = 0x2a;
	int86(0x83, &reg, &reg_out);	/* テキスト初期化 */

	_disable();
	reg.h.ah = 0x0c;
	int86(0x82, &reg, &reg_out);	/* キーキュー初期化 */
	_enable();
}


//unsigned char __far *spr_atr;

//	*(spram++) = y % 256;				/* Y */
//	*(spram++) = (spr_y_size / 4 - 1) * 4 | 0x02 | ((y / 256) & 0x01);
//	*(spram++) = x % 256;				/* X */
//	*(spram++) = (spr_x_size / 8 - 1) * 8 | ((x / 256) & 0x03);
	/* データアドレス(TSP) */
//	*(spram++) = ((SPR_PAT_ADR + index + spr_size * i) / 2) % 256;
//	*(spram++) = ((SPR_PAT_ADR + index + spr_size * i) / 2) / 256;
//	*(spram++) = 0;
//	*(spram++) = 0;

void set_sprite_locate(unsigned short pat_no, unsigned short index, unsigned char i,  unsigned short x, unsigned short y, unsigned short spr_x_size, unsigned short spr_y_size){
	unsigned short spr_size = SPR_SIZE(spr_x_size, spr_y_size);
	spram = (unsigned short __far  *)
		MK_FP(0xa000, ((SPR_ATR) + (pat_no * 8)));
//	PUT_SP( x, y, ((spr_x_size / 4 - 1) * 4 | 0x02), ((spr_y_size / 8 - 1) * 8), ((SPR_PAT_ADR + index + spr_size * i) / 2));

	PUT_SP( x, y, (spr_x_size / 8 - 1) * 8 * 256, (spr_y_size / 4 - 1) * 4 * 256,
		(SPR_PAT_ADR + index + spr_size * i) / 2);

//	*(spram++) = y % 256;				/* Y */
//	*(spram++) = (spr_y_size / 4 - 1) * 4 | 0x02 | ((y / 256) & 0x01);
//	*(spram++) = x % 256;				/* X */
//	*(spram++) = (spr_x_size / 8 - 1) * 8 | ((x / 256) & 0x03);
	/* データアドレス(TSP) */
//	*(spram++) = ((SPR_PAT_ADR + index + spr_size * i) / 2) % 256;
//	*(spram++) = ((SPR_PAT_ADR + index + spr_size * i) / 2) / 256;
//	*(spram++) = 0;
//	*(spram++) = 0;
}

SPR_INFO spr_info[3];

void set_spr_info(unsigned short count, unsigned short index, unsigned short i, unsigned char spr_x_size, unsigned char spr_y_size)
{
	unsigned short spr_size = SPR_SIZE(spr_x_size, spr_y_size);

	spr_info[count].x_size = (spr_x_size / 8 - 1) * 8 * 256;
	spr_info[count].y_size = (spr_y_size / 4 - 1) * 4 * 256;
	spr_info[count].adr = (SPR_PAT_ADR + index + spr_size * i) / 2;
//	printf("(%d %x)\n",count, spr_info[count].adr);
}

unsigned short read_sprite(void)
{
	unsigned short index = 0, index2 = 0, index3 = 0, count = 0;
	unsigned char i, j, k;
	unsigned char spr_no = 0;

	TVRAM_ON();

//	if((index2 = set_sprite_pattern1("PLUSTAKE.SC5", SPR_PAT_ADR + index, 8 * 3 * 3, 16, 16 * 3, 16, 3, 1)) == -1) {
	if((index2 = set_sprite_pattern1("PLUSTAKE.SC5", SPR_PAT_ADR + index, 256, LINE, 32, 16, 16, 1)) == -1) {
		term();
		exit(1);
	}
	for(i = 0; i < 3; ++i){
//		set_sprite_locate(spr_no++, index, i, i * 16 * 3, 64, 16 * 3, 16);
		set_spr_info(count++, index, i, 32, 16);
	}
	index += index2;

	index3 = index;
	

	return index3;
}

char str_temp[9];

void put_strings(int scr, int x, int y,  char *str, char pal)
{
	char chr;
	unsigned short i = 0;
	unsigned char __far *vram;
	unsigned char xx, yy;

	if(scr == SCREEN1)
		vram = (unsigned char __far *)MK_FP(0x0a000, 0);
	else// if(scr == SCREEN2)
		vram = (unsigned char __far *)MK_FP(0x0c000, 0);
	vram += (x * FONTSIZEX + y * FONTSIZEY * 160);

	GVRAM_ON();

	while((chr = *(str++)) != '\0'){
		if((chr < 0x30)) // || (chr > 0x5f))
			chr = 0x40;
		for(yy = 0; yy < FONTSIZEY; ++yy){
			for(xx = 0; xx < FONTSIZEX; ++xx){
				*vram++ = fontdata[chr - '0'][xx][yy];
			}
			vram += (160 - FONTSIZEX);
		}
		vram += (FONTSIZEX - 160 * FONTSIZEY);
	}
}


void put_numd(long j, char digit)
{
	char i = digit;

	while(i--){
		str_temp[i] = j % 10 + 0x30;
		j /= 10;
	}
	str_temp[digit] = '\0';
}

void score_display(void)
{
	put_numd(score, 8);
	put_strings(SCREEN2, 19, 22 , str_temp, CHRPAL_NO);
	if(score >= hiscore){
//		if((get_mod10(score)) == 0){
		if((score % 10) == 0){
			hiscore = score;
			put_strings(SCREEN2, 12, 22, "HIGH ", CHRPAL_NO);
		}
	}
	else
		put_strings(SCREEN2, 12, 22, "SCORE", CHRPAL_NO);
}

void combo_display(void)
{
	put_numd(combo, 8);
	put_strings(SCREEN2, 19, 24 , str_temp, CHRPAL_NO);
		put_strings(SCREEN2, 12, 24, "COMBO", CHRPAL_NO);
}

void score_displayall(void)
{
//	put_strings(SCREEN2, 9, 22, "SCORE", CHRPAL_NO);
	score_display();
}

void hiscore_display(void)
{
/*	if(score > hiscore)
		if((score % 10) == 0)
			hiscore = score;
*/
	put_numd(hiscore, 8);

	put_strings(SCREEN2, 12, 24, "HIGH ", CHRPAL_NO);
	put_strings(SCREEN2, 19, 24, str_temp, CHRPAL_NO);
}

void hiscore_display_clear(void)
{
	put_strings(SCREEN2, 12, 24, "     ", CHRPAL_NO);
	put_strings(SCREEN2, 19, 24, "        ", CHRPAL_NO);
}

#include "inkey.h"

unsigned char keyscan(void)
{
	unsigned char k0, k1, k8, ka, st, pd, k5, k9;
	unsigned char keycode = 0;

	k0 = inp(0x00);
	k1 = inp(0x01);
	k8 = inp(0x08);
	ka = inp(0x0a);
	k5 = inp(0x05);
	k9 = inp(0x09);
	_disable();
	outp(0x44, 0x0e);
	st = inp(0x45);
	outp(0x44, 0x0f);
	pd = inp(0x45);
	_enable();

	if(!(k5 & 0x04) || !(k9 & 0x40) || !(pd & 0x01)) /* Z,SPACE */
		keycode |= KEY_A;
	if(!(k5 & 0x01) || !(pd & 0x02)) /* X */
		keycode |= KEY_B;
//		keycode |= KEY_START;
	if(!(k1 & 0x01) || !(k8 & 0x02) || !(st & 0x01)) /* 8 */
		keycode |= KEY_UP1;
	if(!(k0 & 0x04) || !(ka & 0x02) || !(st & 0x02)) /* 2 */
		keycode |= KEY_DOWN1;

	if(!(st & 0x0c)){ /* RL */
		keycode |= KEY_START;
	}else{
		if(!(k0 & 0x10) || !(ka & 0x04) || !(st & 0x04)) /* 4 */
			keycode |= KEY_LEFT1;
		if(!(k0 & 0x40) || !(k8 & 0x04) || !(st & 0x08)) /* 6 */
			keycode |= KEY_RIGHT1;
	}
	return keycode;
}

void wait_vsync(void)
{
/* 2回見ないと誤判断する可能性がある */
//	while(!(inp(0x0040) & 0x20)); /* WAIT VSYNC */
//	while(inp(0x0040) & 0x20));
	while((inp(0x0142) & 0x40)); /* WAIT VSYNC */
	while(!(inp(0x0142) & 0x40));
}

void sys_wait(unsigned char wait)
{
	unsigned char i;
	for(i = 0; i < wait; ++i)
		wait_vsync();
}


void set_sprite_all(void)
{
	int i, j;

/* スプライト表示 */
/* BIOSは使わずスプライトRAMに直接書き込む */
/* 	SPR_display(2, 0); */
/* SPRAM先頭アドレスをスプライト表示最大数から算出 */
/* 最大数が可変の時はどうなる? */
//	_FP_OFF(spram) = (1024 - MAX_SPRITE) * 8;
	spram = (unsigned short __far  *)
		MK_FP(0xa000, SPR_ATR); // + i * 8);

	TVRAM_ON();

/* 表示数ぶん書き込む */
	if(spr_count > MAX_SPRITE){
//		spr_count = MAX_SPRITE ;
		if(total_count & 1){
			for(i = spr_count - MAX_SPRITE, j = 0; j < MAX_SPRITE; i++, j++){
				pchr_data = &chr_data[i];\
				PUT_SP(pchr_data->x, pchr_data->y, pchr_data->x_size, pchr_data->y_size, pchr_data->adr );
			}
		}else{
			for(i = 0; i < MAX_SPRITE; i++){
				pchr_data = &chr_data[i];\
				PUT_SP(pchr_data->x, pchr_data->y, pchr_data->x_size, pchr_data->y_size, pchr_data->adr );
			}
		}
		old_count = MAX_SPRITE;
	}else{
//		for(i = spr_count - 1; i >= 0; i--){
		for(i = 0; i < spr_count; ++i){
//		spram = (unsigned short __far *)MK_FP(0xa000, SPR_ATR + i * 8);
			pchr_data = &chr_data[i];\
			PUT_SP(pchr_data->x, pchr_data->y, pchr_data->x_size, pchr_data->y_size, pchr_data->adr );
//		printf("(%d:X=%d Y=%d X_SIZE=%d Y_SIZE=%d ADR=%x %x)\n", i, chr_data[i].x, chr_data[i].y, chr_data[i].x_size, chr_data[i].y_size,chr_data[i].adr, spram);
		}
/* スプライトの表示数が減った場合､減った分を画面外に消去する */
/* 増える分には問題ない */
		if (old_count > spr_count){
			for(i = 0;i < (old_count - spr_count); i++){
//				PUT_SP(0,(SCREEN_MAX_Y),0,0,0);
				*(spram++) = 0L;
				*(spram++) = 0;
				*(spram++) = 0;
				*(spram++) = 0;
			}
		}
		old_count = spr_count;
	}

/* このフレ－ムで表示したスプライトの数を保存 */
//	old_count = spr_count;

	++total_count;
}

void se(void)
{
	if(seflag == 1){
		_disable();
		write_psg(6,127);
		write_psg(11,0);
		write_psg(12,15);
		write_psg(7,0x1c);  // 00011100
		write_psg(13,9);
		write_psg(10,0x10);
		_enable();
	}
	seflag = 0;
}



//パレットを暗転する。
void pal_allblack(int pal_no)
{
	char j;
	for(j = 0; j < 16; j++)
		pal_set(pal_no, j, 0, 0, 0);
}

int sprite_pattern_no[8], old_sprite_pattern_no[8], spr_x[8], spr_y[8];

// スプライトポインタ設定
void set_sprite(int num, int posx, int posy) {
	spr_x[num] = posx;
	spr_y[num] = posy;
}

void set_sprite_pattern(int num, int no) {
	sprite_pattern_no[num] = no;
}
/*メインルーチン
　初期設定とメインループ*/
void main()
{
	short i, j;
	short errlv;

	g_init();
	paint(0);
	paint2(0);
//	pal_all(CHRPAL_NO, org_pal);

	/* TSP coommand */
	while(inp(0x142) & 0x05);
	outp(0x142, 0x82);	/* SPRON スプライトON */
	while(inp(0x142) & 0x05);
	outp(0x146, (SPR_ATR) / 256);	/* スプライト制御テーブルアドレス上位 */
	while(inp(0x142) & 0x05);
	outp(0x146, 0x00);
	while(inp(0x142) & 0x05);
	outp(0x146, 32 * 4 | 0x02)	/* 横32枚 縦方向2倍 */;


	while(inp(0x142) & 0x05);
	outp(0x142, 0x15);	/* CURDEF */
	while(inp(0x142) & 0x05);
	outp(0x146, 0x02);	/* カーソルOFF */

	GVRAM_ON();

	font_load("FONTYOKO.SC5", fontdata, FONTPARTS, 0);
	font_load("PLUSTAKE.SC5", &fontdata[FONTPARTS], CHRPARTS, 16 * FONTSIZEX);

//	rev_adr = 
	read_sprite();


	_disable();
	outp(0x44, 0x07);
	outp(0x45, 0x00);
	_enable();

//	init_v_sync();

/* ゲ－ムを実行 */

	erase_allsprite();

	wait_vsync();
	pal_allblack(CHRPAL_NO);
	pal_allblack(BGPAL_NO);
	paint(0x0);
	paint2(0x0);
	wait_vsync();
	pal_all(BGPAL_NO, org_pal);
	pal_all(CHRPAL_NO, org_pal);

	for(i = 0; i < 8; ++i){
		old_sprite_pattern_no[i] = 255;
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

	str_temp[0] = 0x30 + FONTPARTS + 16;
	str_temp[1] = '\0';
	for(i = 0; i < 40; ++i){
		put_strings(SCREEN1, i, 18, str_temp, CHRPAL_NO);
		put_strings(SCREEN1, i, 19, str_temp, CHRPAL_NO);
	}


	for(;;){
		Entity player = {160, 160-16};
		Entity bullets[3] = {{0}};
		Entity enemies[4] = {{0}};
		Entity pluses[8] = {{0}};  // Plus最大8個
/*
		player.x = 160 - 8;
		player.y = 160 - 16;
		for(i = 0; i < 4; ++i)
			bullets[i].active = 0;
		for(i = 0; i < 4; ++i)
			enemies[i].active = 0;
		for(i = 0; i < 8; ++i)
			pluses[i].active = 0;  // Plus最大8個
*/
		int spawn_timer = 0;
		int combo_timer = 0;
		int wave = 1;
		int enemies_killed_this_wave = 0;
		int game_over = 0;
		int score_disp_flag = 0;
		int combo_display_flag = 0;
		int enemy_speed = 2;

		int count = 0;
//		int seflag = 0;
		int secounter = 0;

		int e,b,p;

		unsigned char keycode;

		score = 0;
		combo = 0;

		spr_count = 0;

		score_displayall();
//		combo_display();

		GVRAM_ON();

		while (!game_over) {
			++count;
			if(combo)
				++combo_timer;
//			ULONG joy =ReadJoyPort(1);
			keycode = keyscan();

			// 移動 (キーボードor joy direct)
			if (keycode & KEY_UP1) player.y -= 3;  // up
			if (keycode & KEY_DOWN1) player.y += 3;  // down
			if (keycode & KEY_LEFT1) player.x -= 3;  // left
			if (keycode & KEY_RIGHT1) player.x += 3;  // right

			player.x = MAX(16, MIN(((SCREEN_WIDTH)), player.x));
			player.y = MAX(16, MIN(((SCREEN_HEIGHT)), player.y));

			set_sprite(0, player.x, player.y);

			// 射撃 (ボタン押したら弾発射)
			if (keycode & KEY_A) {  // adjust bit
				for (i = 0; i < 3; i++) {
					if (!bullets[i].active) {
						bullets[i].active = 1;
						bullets[i].x = player.x;
						bullets[i].y = player.y;// - 16;
						break;
					}
				}
			}

			if (keycode & KEY_B)
				goto end;

			// 弾更新
			for (i = 0; i < 3; i++) {
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
				for (i = 0; i < 4; i++) {
					if (!enemies[i].active) {
						enemies[i].active = 1;
						enemies[i].x = 16 + ((rand() & ((SCREEN_WIDTH - 16) * 4 / 5 + 1)) * 5 / 4);
						enemies[i].y = 0;
						break;
					}
				}
			}

			for (i = 0; i < 4; i++) {
				if (enemies[i].active) {
					if (!pluses[i].active) {
						set_sprite_pattern(i+4, 2);
						enemies[i].y += enemy_speed;
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
			for (b = 0; b < 3; b++) {
				if (!bullets[b].active) continue;
				for (e = 0; e < 4; e++) {
					if (pluses[e].active) continue;
					if (!enemies[e].active) continue;
					if (abs(bullets[b].x - enemies[e].x) < 16 && abs(bullets[b].y - enemies[e].y) < 16) {
						bullets[b].active = 0;
//						enemies[e].active = 0;
						set_sprite(b+1, 0, 0);
						set_sprite(e+4, 0, 0);

//						if(!secounter)
//						if(!seflag)
//							seoff();
							seflag = 1;
						// Plus生成
//						for (p = 0; p < 1; p++) {
//							if (!pluses[e].active) {
								pluses[e].active = 1;
								pluses[e].x = enemies[e].x;
								pluses[e].y = enemies[e].y;
//								break;
//							}
//						}

						combo++;
						combo_timer = 0;
						combo_display_flag = 1;
						score += 20 * combo;
						score_disp_flag = 1;
						enemies_killed_this_wave++;

						// WAVEクリア判定
						if (enemies_killed_this_wave >= 5 * wave) {
							wave++;
							enemies_killed_this_wave = 0;
							++enemy_speed;
						}
						break;
					}
				}
			}

			// Plus取得
			for (p = 0; p < 4; p++) {
				if (pluses[p].active) {
					set_sprite_pattern(p+4, 0);
					pluses[p].y += 1;
					if (abs(pluses[p].x - player.x) < 16 && abs(pluses[p].y - player.y) < 16) {
						pluses[p].active = 0;
						enemies[p].active = 0;
						score += 50 * combo;
						score_disp_flag = 1;
						combo_timer = 0;
						combo_display_flag = 1;
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
				combo_display_flag = 1;
				combo_timer = 0;
			}

			// ゲームオーバー (敵接触)
			for (e = 0; e < 4; e++) {
				if (enemies[e].active && abs(enemies[e].x - player.x) < 16 && abs(enemies[e].y - player.y) < 16) {
					if(!pluses[e].active)
						game_over = 1;
				}
			}

			spr_count = 0;
			for(i = 0; i < 8; ++i){
				j = sprite_pattern_no[i];
				DEF_SP_SINGLE(spr_count, spr_x[i], spr_y[i], 0, CHRPAL_NO, 0,  spr_info[j].x_size, spr_info[j].y_size, spr_info[j].adr);
			}

			wait_vsync();

			set_sprite_all();
			if(seflag){
				se();
				seflag = 0;
				secounter = 8;
			}
			if(secounter){
				--secounter;
//				if(!secounter)
//					seoff();
			}

			if(score_disp_flag){
				score_disp_flag = 0;
				score_display();
			}
			else if(combo_display_flag){
				combo_display_flag = 0;
				if(combo){
					combo_display();
				}else{
					hiscore_display_clear();
				}
			}

//			put_strings(SCREEN2, 8, 22, "SCORE", CHRPAL_NO);
//	put_numd(score, 8);

		}
//		seoff();
		for(;;){
			keycode = keyscan();
			if(!(keycode & KEY_A))
				break;
		}
		// ゲームオーバー画面
		/* ここにテキスト描画追加可能 */
		hiscore_display();
		for(;;){
			keycode = keyscan();
			if (keycode & KEY_A)
				break;
		}
		hiscore_display_clear();
		// (スプライト全消し)
		for (i = 0; i < 8; i++){
			set_sprite(i, 0, 0);
			set_sprite_all();
		}
		for(;;){
			keycode = keyscan();
			if (!(keycode & KEY_A))
				break;
		}
	}

end:
	wait_vsync();
	erase_allsprite();
	wait_vsync();

	pal_allblack(CHRPAL_NO);
	pal_allblack(BGPAL_NO);

	term();
}
