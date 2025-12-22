/* plustaker68.c By m@3 with Grok 2025. */

//#define XSP

#define SCREEN_WIDTH (256)
#define SCREEN_HEIGHT (256)

#include <stdint.h>
#include <stdio.h>
#include <x68k/iocs.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include <x68k/dos.h>

#include <math.h>

#ifdef XSP

#include "XSP2lib.H"
#include "PCM8Afnc.H"

#endif

#define CHRPAL_NO 0
#define REVPAL_NO 1
#define BGPAL_NO 2

#ifdef XSP
#define MAX_SPRITE 512
#else
#define MAX_SPRITE 128
#endif

#define SHIFT_NUM	0 //3

enum {
	SPR_OFS_X = -16,
	SPR_OFS_Y = -16
};

#define PCG_SPACE (0x40 - '0')
#define PCGPARTS (16 * 4)
#define PCG_SPACE (0x40 - '0')
#define SPRPARTS 128

#ifdef XSP
#define CHR_TOP 0
#else
#define CHR_TOP (PCGPARTS / 4)
#endif

#define SCREEN_MAX_Y 256

/* スプライト表示デ－タをVRAMに書き込むマクロ */
/* 座標をシフトして書き込みデ－タは随時インクリメント */
/*パレット＋カラーコード＋反転フラグ */
/* BGとのプライオリティ設定 */
#define PUT_SP( x, y, no, atr) {\
	*(spdata++) = x; \
	*(spdata++) = y; \
	*(spdata++) = no; \
	*(spdata++) = atr; \
}

#ifdef XSP
/* スプライト PCG パターン最大使用数 */
#define	PCG_MAX		256
/*
	XSP 用 PCG 配置管理テーブル
	スプライト PCG パターン最大使用数 + 1 バイトのサイズが必要。
*/
char pcg_alt[PCG_MAX + 1];
/* PCG データファイル読み込みバッファ */
char pcg_dat[PCG_MAX * 128];
#endif

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

// max/min マクロ
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))


// 構造体
typedef struct {
	int x, y;
	int active;
} Entity;
/*
Entity player = {120, 160};
Entity bullets[3] = {{0}};
Entity enemies[4] = {{0}};
Entity pluses[8] = {{0}};  // Plus最大8個
*/
int score = 0, hiscore = 5000;
int combo = 0;
/*
int spawn_timer = 0;
int combo_timer = 0;
int wave = 1;
int enemies_killed_this_wave = 0;
int game_over = 0;
*/

unsigned short *vram;
unsigned short *spram;
unsigned short *spdata;

volatile int spr_count;
int total_count = 0;
int tmp_spr_count = 0;
int old_count[2];
unsigned char spr_page = 0; //1;

/* 各キャラクタの構造体(CHR_PARA) */
typedef struct chr_para{
	short x,y, pat_num,atr;
} CHR_PARA;

CHR_PARA chr_data[MAX_SPRITE * 2];

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

/*パレット・セット*/
void pal_set(int pal_no, unsigned short color, unsigned short red, unsigned short green,
	unsigned short blue)
{
	unsigned short *pal_port;
	unsigned char mode = 0;
	if(color)
		mode = 1;

	green = ((green + 1)*2-1)*(green!=0);
	blue = ((blue + 1)*2-1)*(blue!=0);
	red = ((red + 1)*2-1)*(red!=0);

	switch(pal_no){
		case CHRPAL_NO:
			pal_port = (unsigned short *)(0xe82200); // + color * 2);
			*(pal_port+color) = (green * 32 * 32 + red * 32 + blue) * 2 + mode;
//			pal_port = (unsigned short *)(0xe82220); // + color * 2);
//			*(pal_port+color) = (green * 32 * 32 + red * 32 + blue) * 2 + mode;
			break;
		case BGPAL_NO:
			pal_port = (unsigned short *)(0xe82000); // + color * 2);
			*(pal_port+color) = (green * 32 * 32 + red * 32 + blue) * 2 + mode;
			break;
		case REVPAL_NO:
			pal_port = (unsigned short *)(0xe82220); // + color * 2);
			*(pal_port+color) = (green * 32 * 32 + red * 32 + blue) * 2 + mode;
			break;
	}
}

void pal_all(int pal_no, unsigned char pal[16][3])
{
	unsigned char i;
	for(i = 0; i < 16; i++)
		pal_set(pal_no, i, pal[i][0], pal[i][1], pal[i][2]);
}

//パレットを暗転する。
void pal_allblack(int pal_no)
{
	char j;
	for(j = 0; j < 16; j++)
		pal_set(pal_no, j, 0, 0, 0);
}

void paint_text(unsigned short color)
{
	unsigned short i, j;
	unsigned char *vram_adr = (unsigned char *)0xe00000; // * 2;

	for (i = 0; i < 512; ++i){
		for (j = 0; j < 0x80; ++j){
			*(vram_adr + j + i * 0x80 + 0x20000 * 0) = color; /* bit */;
			*(vram_adr + j + i * 0x80 + 0x20000 * 1) = color; /* bit */;
			*(vram_adr + j + i * 0x80 + 0x20000 * 2) = color; /* bit */;
			*(vram_adr + j + i * 0x80 + 0x20000 * 3) = color; /* bit */;
		}
	}
}

void paint_grp(unsigned short color)
{
	unsigned short i, j;

	for (i = 0; i < 256*2; ++i){
		for (j = 0; j < 256; ++j){
			*(vram + j + i * 512) = color; /* bit */;
		}
	}
}

void paint_bg1(unsigned short color)
{
	unsigned short i, j;
	unsigned short *bgram = (unsigned short *)0xebe000; /* BG1 */
	for(j = 0; j < (256 / 8); j++){
		for(i = 0; i < 32; i++){
			*(bgram + (i * 2 + j * 0x80) / 2) = color;
		}
	}
}

void paint_bg0(unsigned short color)
{
	unsigned short i, j;
	unsigned short *bgram = (unsigned short *)0xebc000; /* BG0 */
	for(j = 0; j < (256 / 8); j++){
		for(i = 0; i < 32; i++){
			*(bgram + (i * 2 + j * 0x80) / 2) = color;
		}
	}
}

#define SND_BUFFSIZE 30000

char SNDBUFF[4][SND_BUFFSIZE];
long pcmsize[4]; 
unsigned char seflag;

long SND_load(char *fn, char*SNDBUFF){
	FILE *fp;
	long size;
	struct stat statBuf;


	if ((fp = fopen(fn,"rb")) == NULL)
		return 0; //NULL;
	
	fread(SNDBUFF, SND_BUFFSIZE, 1, fp);

	fclose(fp);

	if (stat(fn, &statBuf) == 0)
		return statBuf.st_size;

	return -1;
}

#define MSXWIDTH 256
#define MSXLINE 212
#define PCGSIZEX 4
#define PCGSIZEY 8
#define MAXSPRITE 128

unsigned char pattern[10];
unsigned short *vram_adr;

unsigned char read_pattern[MSXWIDTH * MSXLINE * 2+ 2];

unsigned char conv_tbl[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 , 15};

FILE *stream[2];

short font_load(char *loadfil, short offset, short pcgparts)
{
	unsigned char msxcolor[MSXWIDTH / 2][MSXLINE];
	long i, j,k,y, x, xx, yy, no, max_xx;

	unsigned short *spram;

	if ((stream[0] = fopen( loadfil, "rb")) == NULL) {
		fprintf(stderr, "Can\'t open file %s.", loadfil);

		fclose(stream[0]);
		return 1;
	}

	fread(pattern, 1, 1, stream[0]);	/* MSX先頭を読み捨てる */
	fread(pattern, 1, 4, stream[0]);	/* MSXヘッダも読み捨てる */
	fread(pattern, 1, 2, stream[0]);	/* MSXヘッダを読み捨てる */

	for(y = 0; y < MSXLINE; ++y){
		for(x = 0; x < MSXWIDTH / 2 ; ++x){
			msxcolor[x][y] = 0;
		}
	}
	for(y = 0; y < MSXLINE; ++y){
		for(x = 0; x < MSXWIDTH / 8; ++x){
			i = fread(read_pattern, 1, 4, stream[0]);	/* 8dot分 */
			if(i < 1)
				break;

			/* 色分解 */
			msxcolor[0 + x * 4][y] = read_pattern[0]; 
			msxcolor[1 + x * 4][y] = read_pattern[1]; 
			msxcolor[2 + x * 4][y] = read_pattern[2];
			msxcolor[3 + x * 4][y] = read_pattern[3];
		}
	}
	fclose(stream[0]);
	max_xx = 64;
	spram  = (unsigned short *)0xeb8000;
	spram += ((64 * offset));

	j = 0;
	xx=0;
	yy=0;
	x=0;
	for(no = 0; no < pcgparts; ++no){
//		printf("\nno =%d ",no);
		for(y = 0; y < PCGSIZEY; ++y){
			for(x = 0; x < PCGSIZEX; x+=2){

				if((x+xx) >= max_xx) {
					xx=0;
					yy+=PCGSIZEY;
				}

				*(spram++) = msxcolor[x + xx][y + yy] * 256 + msxcolor[x + xx + 1][y + yy];
			}
		}
		xx+=PCGSIZEX;
	}

//	pal_all();

	return 0;
}

int msxspconv(char *loadfil, short offset, short sprparts)
{
	FILE *stream[2];
	unsigned char pattern[10];
	unsigned char msxcolor[MSXWIDTH / 2][MSXLINE];

	long i, j,k,y, x, xx, yy, no, max_xx;

	unsigned short data;

//#ifndef XSP
	unsigned short *spram;
	spram  = (unsigned short *)0xeb8000;
	spram += ((64 * offset));
//#endif
#ifdef XSP
	short *ppcg_data = (short *)pcg_dat;
	xsp_pcgmask_on(0, 64/4-1); //short start_no, short end_no);
	xsp_pcgmask_on(128, 256-1); //short start_no, short end_no);
#endif


	if ((stream[0] = fopen( loadfil, "rb")) == NULL) {
		fprintf(stderr, "Can\'t open file %s.", loadfil);

		fclose(stream[0]);
		return 1;
	}

	fread(pattern, 1, 1, stream[0]);	/* MSX先頭を読み捨てる */
	fread(pattern, 1, 4, stream[0]);	/* MSXヘッダも読み捨てる */
	fread(pattern, 1, 2, stream[0]);	/* MSXヘッダを読み捨てる */

	for(y = 0; y < MSXLINE; ++y){
		for(x = 0; x < MSXWIDTH / 2 ; ++x){
			msxcolor[x][y] = 0;
		}
	}
	for(y = 0; y < MSXLINE; ++y){
		for(x = 0; x < MSXWIDTH / 8; ++x){
			i = fread(pattern, 1, 4, stream[0]);	/* 8dot分 */
			if(i < 1)
				break;

			/* 色分解 */
			msxcolor[0 + x * 4][y] = pattern[0]; 
			msxcolor[1 + x * 4][y] = pattern[1]; 
			msxcolor[2 + x * 4][y] = pattern[2];
			msxcolor[3 + x * 4][y] = pattern[3];
		}
	}
	fclose(stream[0]);
	max_xx = 128;


	j = 0;
	xx=0;
	yy=0;
	x=0;
	for(no = 0; no < sprparts; ++no){
		for(i = 0; i < 2; ++i){
			for(j = 0; j < 2; ++j){
//				printf("\nno =%d ",no);
				for(y = 0; y < PCGSIZEY; ++y){
					for(x = 0; x < PCGSIZEX; x+=2){

						if((x+xx) >= max_xx) {
							xx=0;
							yy+=PCGSIZEY*2;
						}

						data = msxcolor[x + xx][y + yy] * 256 + msxcolor[x + xx + 1][y + yy];
//#ifndef XSP
						*(spram++) = data;
#ifdef XSP
						*ppcg_data++ = data;
#endif
					}
				}
				yy+=PCGSIZEY;
			}
			yy-=PCGSIZEY*2;
			xx+=PCGSIZEX;
		}
	}
#ifdef XSP
	/* PCG データと PCG 配置管理をテーブルを指定 */
	xsp_pcgdat_set(pcg_dat, pcg_alt, sizeof(pcg_alt));
#endif

	return 0;
}
int sprite_pattern_no[8], old_sprite_pattern_no[8], spr_x[8], spr_y[8];

char chr;
unsigned char str_temp[11];
#define SCREEN2 0
#define CHRPAL_NO 0


void put_strings(int scr, int x, int y,  char *str, char pal)
{
	char chr;
	unsigned short i = 0;
	unsigned short *bgram;
	bgram = (unsigned short *)0xebc000; /* BG0 */
//	bgram = (unsigned short *)0xebe000; /* BG1 */
	bgram += (x * 2 + (y) * 0x80) / 2;

	while((chr = *(str++)) != '\0'){
		if((chr < 0x30)) //|| (chr > 0x5f))
			chr = PCG_SPACE;
		else
			chr -= '0';
		*(bgram++) = chr;
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
	put_strings(SCREEN2, 15, 25 , str_temp, CHRPAL_NO);
	if(score >= hiscore){
		if((score % 10) == 0){
			hiscore = score;
			put_strings(SCREEN2, 8, 25, "HIGH ", CHRPAL_NO);
		}
	}
	else
		put_strings(SCREEN2, 8, 25, "SCORE", CHRPAL_NO);
}

void combo_display(void)
{
	put_numd(combo, 8);
	put_strings(SCREEN2, 15, 27 , str_temp, CHRPAL_NO);
		put_strings(SCREEN2, 8, 27, "COMBO", CHRPAL_NO);
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

	put_strings(SCREEN2, 8, 27, "HIGH ", CHRPAL_NO);
	put_strings(SCREEN2, 15, 27, str_temp, CHRPAL_NO);
}

void hiscore_display_clear(void)
{
	put_strings(SCREEN2, 8, 27, "     ", CHRPAL_NO);
	put_strings(SCREEN2, 15, 27, "        ", CHRPAL_NO);
}

CHR_PARA *pchr_data;

#ifndef XSP
/* スプライト位置を定義するマクロ */
#define DEF_SP_SINGLE( NO, X, Y, PAT, PAL, ATR, PRO) {\
	pchr_data = &chr_data[NO];\
	pchr_data->x = (X >> SHIFT_NUM) + SPR_OFS_X + 16; \
	pchr_data->y = (Y >> SHIFT_NUM) + SPR_OFS_Y + 16; \
	pchr_data->pat_num = PAT + (PAL << 8) + (ATR << 14); \
	pchr_data->atr = 0x0002; \
	NO++; \
}
#else
#define DEF_SP_SINGLE( NO, X, Y, PAT, PAL, ATR, PRO) {\
	pchr_data = &chr_data[NO];\
	pchr_data->x = (X >> SHIFT_NUM) + SPR_OFS_X + 16; \
	pchr_data->y = (Y >> SHIFT_NUM) + SPR_OFS_Y + 16; \
	pchr_data->pat_num = PAT & 0x3fff; \
	pchr_data->atr = (PAL << 8) | (ATR << 14) | 0x20 | PRO | (PAT & 0xc000);\
	NO++; \
}
#endif

// スプライトポインタ設定
void set_sprite(int num, int posx, int posy) {
	spr_x[num] = posx;
	spr_y[num] = posy;
}

#ifdef XSP

/*void set_sprite_all(void)
{
}*/

void set_sprite_all(void)
{
	int i;

	for(i = 0; i < spr_count; i++)
		/* スプライトの表示登録 */
//		xsp_set(chr_data[i].x, chr_data[i].y, chr_data[i].pat_num,  chr_data[i].atr);//0x013F

		xsp_set_st(&chr_data[i]);
//		xsp_set(player.x, player.y, player.pt, player.info);
		/*
			↑ここは、
				xsp_set_st(&player);
			と記述すれば、より高速に実行できる。
		*/

	/* スプライトを一括表示する */
	xsp_out();

//	if(seflag){
//		if(soundflag == TRUE)
//			if(se_check())
//				se_stop();
//		S_IL_FUNC(se_play(sndtable[0], seflag - 1));	/* 効果音 */
//		if(mcd_status >= 0){
//			pcm_play(&SNDBUFF[seflag - 1][0], pcmsize[seflag - 1]);
//		}
//		_iocs_adpcmmod(0);
//		_iocs_adpcmout(&SNDBUFF[seflag - 1][0], 4 * 256 + 3, pcmsize[seflag - 1]);
//		seflag = 0;
//	}
}

#else

void set_sprite_all(void)
{
	int i, j;

//	wait_vsync();

/* スプライト表示 */
	spdata = //(unsigned short *)&chr_data2[spr_page]; //
		(unsigned short *)0xeb0000;

//	printf("\x1b[0;0H%d \n", spr_count);

/* 表示数ぶん書き込む */
	if(spr_count > MAX_SPRITE){
		if(total_count & 1){
/*			for(i = spr_count - MAX_SPRITE, j = 0; j < MAX_SPRITE; i++, j++){
				PUT_SP(chr_data[i].x, chr_data[i].y, chr_data[i].pat_num, chr_data[i].atr);
			}*/
			memcpy(spdata, &chr_data[spr_count - MAX_SPRITE], MAX_SPRITE * 4 * 2);
		}else{
/*			for(i = 0; i < MAX_SPRITE; i++){
				PUT_SP(chr_data[i].x, chr_data[i].y, chr_data[i].pat_num, chr_data[i].atr);
			}*/
			memcpy(spdata, chr_data, MAX_SPRITE * 4 * 2);
		}
		tmp_spr_count = MAX_SPRITE;
	}else{
/*		for(i = 0; i < spr_count; i++){
			PUT_SP(chr_data[i].x, chr_data[i].y, chr_data[i].pat_num, chr_data[i].atr);
		}*/
		memcpy(spdata, chr_data, spr_count * 4 * 2);
		spdata += (spr_count * 4);

/* スプライトの表示数が減った場合､減った分を画面外に消去する */
/* 増える分には問題ない */
		if (old_count[spr_page] > spr_count){
			for(i = 0;i < (old_count[spr_page] - spr_count); i++){
				PUT_SP(0,SCREEN_MAX_Y,0,0);
			}
		}
		tmp_spr_count = spr_count;
	}
/* このフレ－ムで表示したスプライトの数を保存 */
	old_count[spr_page] = tmp_spr_count;
	tmp_spr_count = 0;

//	spr_flag = 1;
//	spr_next = spr_page;
//	spr_page ^= 0x01;

	++total_count;
}

#endif

void set_sprite_pattern(int num, int no) {
	sprite_pattern_no[num] = no + CHR_TOP;
}

void se(void)
{
//	seflag = 1;
	_iocs_adpcmmod(0);
	_iocs_adpcmout(&SNDBUFF[seflag - 1][0], 4 * 256 + 3, pcmsize[seflag - 1]);
}

void seoff(void)
{
	unsigned short *pp;

	_iocs_adpcmmod(0);
}

/* 画面設定 */
void grp_set(void){
/*	CRTMOD(10); */
	_iocs_crtmod(6);
	_iocs_ms_init();
	_iocs_skey_mod(0, 0, 0);
	_iocs_ms_curof();
	_iocs_b_curoff();
	_iocs_g_clr_on();
}

/* 画面を戻す */
void grp_term(void){
//	_iocs_ms_curon();
	_iocs_skey_mod(-1, 0, 0);
	_iocs_b_curon();
	_iocs_crtmod(16);
}

/* スプライトを全て画面外に移す */
void spr_clear(void){
	int i;
#ifndef XSP
	spram = (unsigned short *)0xeb0000;
#endif
#ifdef XSP
	for(i = 0;i < MAX_SPRITE; i++){
		/* スプライトの表示登録 */
//		xsp_set(0, SCREEN_MAX_Y, 0, 0);
		chr_data[i].x = 0;
		chr_data[i].y = SCREEN_MAX_Y;
		chr_data[i].pat_num = 0;
		chr_data[i].atr = 0;
#else
	for(i = 0;i < 128; i++){
		*(spram++) = 0;
		*(spram++) = SCREEN_MAX_Y; //256;
		*(spram++) = 0;
		*(spram++) = 0; 
#endif
	}
}

void spr_on(int num){
	_iocs_sp_on();
}

void spr_off(void){
	_iocs_sp_off();
}

/* スプライトパタ－ン設定 */
void spr_set(void){
	spr_off();

/* スプライト定義 */
/*	SPR_init(); */
	spr_clear(); 
/*	spr_fill(SPBACKCOLOR); */
/*	spr_make(); */
}

#include "inkey.h"

unsigned char keyscan(void)
{
	unsigned char k5, k6, k7, k8, k9, st, pd;
	unsigned char *reg = (unsigned char *)0xe9a001;
	unsigned short paddata;
	unsigned char keycode = 0;

	k5 = _iocs_bitsns(5);
	k6 = _iocs_bitsns(6);
	k7 = _iocs_bitsns(7);
	k8 = _iocs_bitsns(8);
	k9 = _iocs_bitsns(9);

	paddata = reg[0];
	st = (paddata & 0x0f); // ^ 0x0f;
	pd = ((paddata >> 5) & 0x03); // ^ 0x03;

	if((k5 & 0x04) || (k6 & 0x20) || !(pd & 0x01)) /* Z,SPACE */
		keycode |= KEY_A;
	if((k5 & 0x08) || !(pd & 0x02)) /* X */
		keycode |= KEY_B;
	if((k8 & 0x10) || (k7 & 0x10) || !(st & 0x01)) /* 8 */
		keycode |= KEY_UP1;
	if((k9 & 0x10) || (k7 & 0x40) || !(st & 0x02)) /* 2 */
		keycode |= KEY_DOWN1;

	if(!(st & 0x0c)){ /* RL */
		keycode |= KEY_START;
	}else{
		if((k8 & 0x80) || (k7 & 0x08) || !(st & 0x04)) /* 4 */
			keycode |= KEY_LEFT1;
		if((k9 & 0x02) || (k7 & 0x20) || !(st & 0x08)) /* 6 */
			keycode |= KEY_RIGHT1;
	}

	return keycode;
}

#ifdef XSP

void set_sprite_new(void);

void wait_vsync(void)
{
	/* 垂直同期 */
	xsp_vsync2(2);
//	set_sprite_new();
}

#else

volatile unsigned char vsync_flag = 0;

void wait_vsync(void)
{

	unsigned char volatile *vsync = (unsigned char *)0xe88001;	/* MFP */
/* VSYNC待ち */
	while(!((*vsync) & 0x10));	/* 調査中 */
	while((*vsync) & 0x10);		/* 調査中 */
//	while(!vsync_flag);
//	vsync_flag = 0;

//	set_sprite_new();
}

#endif

int	main(int argc,char **argv)
{
	long *lp;
	unsigned short *bg_priority = (unsigned short *)0xeb0808;
	unsigned short *grp_priority = (unsigned short *)0xe82500;
	unsigned short *bgram;
	short i, j;
	short errlv;
//	register unsigned short *scroll_grp_x = (unsigned short *)0xe8001c;	/* */
//	register unsigned short *scroll_grp_y = (unsigned short *)0xe8001e;	/* */

//	register unsigned short *scroll_bg0_x = (unsigned short *)0xeb0800;	/* */
//	register unsigned short *scroll_bg0_y = (unsigned short *)0xeb0802;	/* */

	register unsigned short *scroll_bg1_x = (unsigned short *)0xeb0806;	/* */
	register unsigned short *scroll_bg1_y = (unsigned short *)0xeb0804;	/* */

	register unsigned short *scroll_x = (unsigned short *)0xe80014;	/* */
	register unsigned short *scroll_y = (unsigned short *)0xe80016;	/* */

	long part1,part2;

	unsigned char keycode;
	unsigned char x,y;

	char *title_filename;

dum:	_iocs_b_super(0);		/* スーパーバイザモード 最適化防止にラベルを付ける */
	spram = (unsigned short *)0xeb0000;

//	crtc = (short *)0xe80000;
//	crtcr20 = (short *)0xe80028;

//	mcd_filename = "C_1_68.MDC";	/* 引数がなかった場合 */

/* 実行時引数が設定されているかどうか調べる */
/*	if (argc < 2 ) */
	/*	return 1;*/
	if (argv[1] == NULL)
		title_filename = "";
	else
		title_filename = argv[1];

//	mcd_status = -1; //check_mcd();


//	for(i = 0; i <  256; ++i)
//		sin_table[i] = sin_table[i + 256] = (16 * sin(2 * M_PI * i / 256));

//	load_fmdbgm("dummy.ob2");

//	mcd_status = -1;
//	if(mcd_status >= 0){
//		printf("se load:\n");
		if ((pcmsize[0] = SND_load("se1.pcm", &SNDBUFF[0][0])) < 1)
			exit(1);
//		if ((pcmsize[1] = SND_load("se2.pcm", &SNDBUFF[1][0])) < 1)
//			exit(1);
//		if ((pcmsize[2] = SND_load("se3.pcm", &SNDBUFF[2][0])) < 1)
//			exit(1);
//		if ((pcmsize[3] = SND_load("se4.pcm", &SNDBUFF[3][0])) < 1)
//			exit(1);
//	}

//	pcm_play(&SNDBUFF[0][0], pcmsize[0]);
//	seflag = 1;
//	se();

	grp_set();		/* ゲーム用に画面初期化 */
/*	screen(1,1,1,1);*/

/*	crtc[0] = 69;
	crtc[1] = 6;
	crtc[2] = 11; 
	crtc[3] = 59 ;
	crtc[4] = 567;
	crtc[5] = 5 ;
	crtc[6] = 40 ;
	crtc[7] = 552;
*/
/*	printf("%X\n",*crtcr20);
	*crtcr20 = 0x0110;*/
/*	*crtcr20 = 0x0111;*/
/*	printf("%X\n",*crtcr20);*/

#ifndef XSP
	spr_set();		/* スプライトの定義 */
#endif
//	bgram = (unsigned short *)0xebc000; /* BG0 */
//	for(j = 0; j < 32; j++){
//		for(i = 0; i < 32; i++){
//			*(bgram + (i * 2 + j * 0x80) / 2) = 0x0a; //(i + j * 32) % 256;
//		}
//	}

//	bgram = (unsigned short *)0xebe000; /* BG1 */
//	for(j = 0; j < 32; j++){
//		for(i = 0; i < 32; i++){
//			*(bgram + (i * 2 + j * 0x80) / 2) = 0x0a; //(i + j * 32) % 256;
//		}
//	}

//	bgram = (unsigned short *)0xebc000; /* BG0 */
//	for(j = 0; j < 32; j++){
//		for(i = 0; i < 32; i++){
//			*(bgram + (i * 2 + j * 0x80) / 2) = 0x0a; //(i + j * 32) % 256;
//		}
//	}

//	for(j = 0; j < 8; j++){
//		for(i = 0; i < 32; i++){
//			*(bgram + (i * 2 + j * 0x80) / 2) = (i + j * 32) % 256;
//		}
//	}

/*	printf("mode = %d\n",mcd_status);*/

/*	if(mcd_status >= 0){
		mcd_load(mcd_filename, playbuffer ,MAX_MCD_SIZE);
		lp = (long *)&playbuffer[0x1c];
		pcm_filename = (char *)&playbuffer[*lp];
		pcm_load(pcm_filename, pcmbuffer, MAX_PCM_SIZE); /* "TEST.PDX" */
//	}*/


#ifdef XSP
	/* XSP の初期化 */
	xsp_on();
	/* PCM8A との衝突を回避 */
	pcm8a_vsyncint_on();
#endif

	msxspconv("PLUSTAKE.SC5", PCGPARTS / 4, 256 - PCGPARTS / 4);
	font_load("FONTYOKO.SC5", 0, PCGPARTS);

//	paint_text(0);
	pal_allblack(CHRPAL_NO);
	pal_allblack(BGPAL_NO);
////	title_load("TITLE.SC5", (128-48-16 + 256) / 8, 48-16, 16 * 4);
//	vram = (unsigned short *)0xc8000;
//	if(bg_mode = title_load2(title_filename, vram)){
//		vram =  (unsigned short *)0xc00000;
//		paint_grp(0x0);	/* GRP1 */
//	}
//	pal_all(CHRPAL_NO, org_pal);

//	(BG0 > BG1)
//	*bg_priority = 0x021a;	/*  BG0=OFF BG1=ON BG0=BG1 BG1=BG0 */
	*bg_priority = 0x0219;	/*  BG0=ON BG1=ON BG0=BG0 BG1=BG1 */

	*grp_priority = 0x12e1;	/* SP=1 TEXT=0 GRP=0 GRP0=1 GRP1=0 GRP2=2 GRP3=3 */
//	*grp_priority = 0x18e4;	/* SP=1 TEXT=2 GRP=0 GRP0=0 GRP1=1 GRP2=2 GRP3=3 */

	*scroll_bg1_x = 0;
	*scroll_bg1_y = 0;
//	*scroll_bg0_x = 0;
//	*scroll_bg0_y = 0;

/*	if(mcd_status >= 0){
		mcd_stop();
		mcd_play();
	}
*/
	spr_on(MAX_SPRITE);	/* スプライト表示の開始 */

//	game_run();		/* ゲームの実行 */
/* ゲ－ムを実行 */
//	scrl_spd = SCRL_MIN;
//	scrl = 0;

/* ゲ－ムを実行 */

	spr_clear();
#ifndef XSP
//	if(init_int())
//		goto end;
#endif
//	init_star();

	vram =  (unsigned short *)0xc00000;
	paint_grp(0x0);
	paint_bg1(PCG_SPACE);
	paint_bg0(PCG_SPACE);

	pal_all(CHRPAL_NO, org_pal);
	pal_all(BGPAL_NO, org_pal);

	for(y = 21; y < 23; ++y){
		x = 0;
		bgram = (unsigned short *)0xebe000;
		bgram += (x * 2 + (y) * 0x80) / 2; /* BG1 */
		for(x = 0; x < 32; ++x){
			*(bgram++) = (0x18)*4;
		}
	}
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

	for(;;){
		Entity player = {160 - 8, 160-16+8+16};
		Entity bullets[3] = {{0}};
		Entity enemies[4] = {{0}};
		Entity pluses[8] = {{0}};  // Plus最大8個

		score = 0;
		combo = 0;
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
		unsigned char keycode;
		spr_count = 0;

		score_displayall();
//		combo_display();

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
				for (int i = 0; i < 3; i++) {
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
						enemies[i].x = 16 + ((rand() & ((SCREEN_WIDTH - 16))));// * 4 / 5 + 1)) * 5 / 4);
						enemies[i].y = 0;
						break;
					}
				}
			}

			for (int i = 0; i < 4; i++) {
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
			for (int b = 0; b < 3; b++) {
				if (!bullets[b].active) continue;
				for (int e = 0; e < 4; e++) {
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
//						for (int p = 0; p < 1; p++) {
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
			for (int p = 0; p < 4; p++) {
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
			for (int e = 0; e < 4; e++) {
				if (enemies[e].active && abs(enemies[e].x - player.x) < 16 && abs(enemies[e].y - player.y) < 16) {
					if(!pluses[e].active)
						game_over = 1;
				}
			}

			spr_count = 0;
			for(i = 0; i < 8; ++i){
				DEF_SP_SINGLE(spr_count, spr_x[i], spr_y[i], sprite_pattern_no[i], CHRPAL_NO, 0, 0);
				++spr_count;
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
		for (int i = 0; i < 8; i++){
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

#ifndef XSP
//	reset_int();
#endif

//	if(mcd_status >= 0){
//		mcd_fadeout();
/*		mcd_release();*/
//	}

	pal_allblack(CHRPAL_NO);
	pal_allblack(BGPAL_NO);
	spr_off();
#ifdef XSP
	/* XSP の終了処理 */
	xsp_off();
	/* PCM8A との衝突を回避 */
	pcm8a_vsyncint_off();
#endif

	paint_text(0);

//	if(mcd_status >= 0){
//		while(mcd_setfadelvl(-1));

//		do{
//			mcd_getplayflg(&part1, &part2);
//			printf("%x %x\n", part1, part2);
//		}while(part1 | part2);
//	}
	grp_term();		/* 画面再初期化 */

/*dum2:	B_SUPER(1);*/

	while(_iocs_b_keysns())
		_iocs_b_keyinp();

	exit(0);

	return(0);
}

