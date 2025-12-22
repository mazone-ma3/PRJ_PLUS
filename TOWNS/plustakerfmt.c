/* plustakerfmt.c By m@3 with Grok 2025. */

#define SCREEN_WIDTH (256)
#define SCREEN_HEIGHT (240)

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <spr.h>
#include <conio.h>
#include <snd.h>
#include <TOWNS/segment.h>
#include <egb.h>

#define _disable() asm("cli\n")
#define _enable() asm("sti\n")

char egb_work[1536];

#define CHRPAL_NO 0
#define REVPAL_NO 1
#define BGPAL_NO 2

#define MAX_SPRITE 227

#define SHIFT_NUM	0 //3

enum {
	IMG_SIZE_X = 16,
	SPR_OFS_X = -16,
	SPR_OFS_Y = -16
};

#define SCREEN_MAX_Y 240

#define CHR_TOP 128

#define SPBACKCOLOR	0x8000	/* スプライトの未表示部分を塗りつぶす色 */

#define FONTPARTS (16 * 4)
#define TITLEPARTS 0 //(8 * 4)
#define SPRPARTS 128

/* スプライト表示デ－タをVRAMに書き込むマクロ */
/* 座標をシフトして書き込みデ－タは随時インクリメント */
/*パレット＋カラーコード＋反転フラグ */
/* BGとのプライオリティ設定 */
#define PUT_SP( x, y, no, atr) {\
	_poke_word(0x130, spram, x); \
	spram += 2; \
	_poke_word(0x130, spram, y); \
	spram += 2; \
	_poke_word(0x130, spram, no); \
	spram += 2; \
	_poke_word(0x130, spram, atr); \
	spram += 2; \
}

#define IMG_SET(X, Y) (CHR_TOP + FONTPARTS + TITLEPARTS + X + IMG_SIZE_X * Y)

extern void int_vsync_ent(void);
extern void init_vsync_ent(void);
extern void reset_vsync_ent(void);


/* サウンド設定 */
char sndwork[16384];

unsigned int encode;

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

volatile int spr_count;
int total_count = 0;
int tmp_spr_count = 0;
int old_count;

/* 各キャラクタの構造体(CHR_PARA) */
typedef struct chr_para{
	int x,y,xx,yy,pat_num,atr,count,hp;
} CHR_PARA;

typedef struct chr_para2{
	short x , y, pat_num, atr;
} CHR_PARA2;

CHR_PARA2 chr_data[MAX_SPRITE * 2];

int vram;		/* VRAM操作用変数 */
int spram;		/* スプライトRAM操作用変数 */

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

unsigned char rev_pal[16][3] = {
	{  0,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
	{  15,  0,  0},
};

/*パレット・セット*/
void pal_set(int pal_no, unsigned short color, unsigned short red, unsigned short green,
	unsigned short blue)
{
	int palram;
	switch(pal_no){
		case CHRPAL_NO:
//			_Far unsigned short *palram;

//			_FP_SEG(palram) = 0x130;
//			_FP_OFF(palram) = 0x2000;
			palram = 0x2000;

			green = ((green + 1)*2-1)*(green!=0);
			blue = ((blue + 1)*2-1)*(blue!=0);
			red = ((red + 1)*2-1)*(red!=0);

//			palram[color] = green * 32 * 32 + red * 32 + blue;
			_poke_word(0x130, palram + color * 2,  green * 32 * 32 + red * 32 + blue);
			break;

		case BGPAL_NO:
			green = ((green + 1)*16-1)*(green!=0);
			blue = ((blue + 1)*16-1)*(blue!=0);
			red = ((red + 1)*16-1)*(red!=0);

			outp(0x448,0x01);
			outp(0x44a,0x01);	/* priority register */

			outp(0xfd90, color);
			outp(0xfd92, blue);
			outp(0xfd94, red);
			outp(0xfd96, green);
			break;

		case REVPAL_NO:
//			_Far unsigned short *palram;

//			_FP_SEG(palram) = 0x130;
//			_FP_OFF(palram) = 0x2000;
			palram = 0x2000+32;

			green = ((green + 1)*2-1)*(green!=0);
			blue = ((blue + 1)*2-1)*(blue!=0);
			red = ((red + 1)*2-1)*(red!=0);

//			palram[color] = green * 32 * 32 + red * 32 + blue;
			_poke_word(0x130, palram + color * 2,  green * 32 * 32 + red * 32 + blue);
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

void paint(unsigned short color)
{
	unsigned short i, j;

	_disable();
	for (i = 0; i < (512); ++i){
		for (j = 0; j < 512; j+=2){
//			_FP_OFF(vram) = (j + i * 1024 + 32) / 2;
//			*vram = color;
			VRAM_putPixelW((j + i * 512 + 32 * 0) / 1, color);
		}
	}
	_enable();
}

volatile int vsync_flag = 0;

int init_vsync(void)
{
	int ret;
	vsync_flag = 0;

	init_vsync_ent();

//	stackAddress = stack+1000;

//	outportb(0x04ea, 0xff);
//	outportb(0x22,0x40);
	outportb(0x5ca,0);
	return 0;
}

void reset_vsync(void)
{
	reset_vsync_ent();
}

#define SND_BUFFSIZE 55000

char SNDBUFF[4][SND_BUFFSIZE];
unsigned char seflag;

char *SND_load(char *fn, char*SNDBUFF){
	FILE *fp;
	
	if ((fp = fopen(fn,"rb")) == NULL)
		return NULL;
	
	fread(SNDBUFF, SND_BUFFSIZE, 1, fp);
	fclose(fp);
	
	return SNDBUFF;
}

#define MSXWIDTH 256
#define MSXLINE 212

unsigned char pattern[10];

unsigned char read_pattern[MSXWIDTH * MSXLINE * 2+ 2];

unsigned char conv_tbl[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 , 15};

FILE *stream[2];


#define SPRSIZEX 8
#define SPRSIZEY 16

int msxspconv(char *loadfil, short sprparts)
{
	FILE *stream[2];
	unsigned char pattern[10];
	unsigned char msxcolor[MSXWIDTH / 2][MSXLINE];

	long i, j,k,y, x, xx, yy, no, max_xx;

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
			msxcolor[0 + x * 4][y] =
				((pattern[1] >>4) & 0x0f) | ((pattern[1] & 0x0f) * 16); 
			msxcolor[1 + x * 4][y] =
				((pattern[0] >>4) & 0x0f) | ((pattern[0] & 0x0f) * 16); 
			msxcolor[2 + x * 4][y] =
				((pattern[3] >>4) & 0x0f) | ((pattern[3] & 0x0f) * 16); 
			msxcolor[3 + x * 4][y] =
				((pattern[2] >>4) & 0x0f) | ((pattern[2] & 0x0f) * 16); 
		}
	}
	fclose(stream[0]);
	max_xx = 128;

	j = 0;
	xx=0;
	yy=0;
	x=0;
	for(no = 0; no < sprparts; ++no){
//		printf("\nno =%d ",no);
		for(y = 0; y < SPRSIZEY; ++y){
			for(x = 0; x < SPRSIZEX; x+=2){

				if((x+xx) >= max_xx) {
					xx=0;
					yy+=SPRSIZEY;
				}
				_poke_word(0x130, spram, msxcolor[x + xx][y + yy] * 256 + msxcolor[x + xx + 1][y + yy]);
				spram += 2;
			}
		}
		xx+=SPRSIZEX;
	}

	return 0;
}

FILE *fontstream[2];

unsigned char pattern[10];
unsigned char msxcolor[MSXWIDTH / 2][MSXLINE];

short font_load(char *loadfil, unsigned short sprparts)
{

	long i, j,k,y, x, xx, yy, no, max_xx;

//	_Far unsigned short *spram;
//	int spram;

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
			msxcolor[0 + x * 4][y] =
				((pattern[1] >>4) & 0x0f) | ((pattern[1] & 0x0f) * 16); 
			msxcolor[1 + x * 4][y] =
				((pattern[0] >>4) & 0x0f) | ((pattern[0] & 0x0f) * 16); 
			msxcolor[2 + x * 4][y] =
				((pattern[3] >>4) & 0x0f) | ((pattern[3] & 0x0f) * 16); 
			msxcolor[3 + x * 4][y] =
				((pattern[2] >>4) & 0x0f) | ((pattern[2] & 0x0f) * 16); 
		}
	}
	fclose(fontstream[0]);
	max_xx = 64;

//	_FP_SEG(spram) = 0x130;
//	_FP_OFF(spram) = 0x4000 + 256*SPRSIZEX*SPRSIZEY;
//	spram = (0x4000 + 256*SPRSIZEX*SPRSIZEY);
	j = 0;
	xx=0;
	yy=0;
	x=0;
	for(no = 0; no < sprparts; ++no){
//		printf("\nno =%d ",no);
		for(y = 0; y < SPRSIZEY / 2; ++y){
			for(x = 0; x < SPRSIZEX / 2; x+=2){

				if((x+xx) >= max_xx) {
					xx=0;
					yy+=SPRSIZEY / 2;
				}

//				*(spram++) = msxcolor[x + xx][y + yy] * 256 + msxcolor[x + xx + 1][y + yy];
				_poke_word(0x130, spram,  msxcolor[x + xx][y + yy] * 256 + msxcolor[x + xx + 1][y + yy]);
				spram += 2;
			}
			for(x = 0; x < SPRSIZEX / 2; x+=2){
//				*(spram++) = 0;
				_poke_word(0x130, spram,  0);
				spram += 2;
			}
		}
		for(y = 0; y < SPRSIZEY / 2; ++y){
			for(x = 0; x < SPRSIZEX; x += 2){
//				*(spram++) = 0;
				_poke_word(0x130, spram,  0);
				spram += 2;
			}
		}
		xx+=SPRSIZEX / 2;
	}

	return 0;
}



int sprite_pattern_no[8], old_sprite_pattern_no[8], spr_x[8], spr_y[8];

char chr;
unsigned char str_temp[11];
#define SCREEN2 0
#define CHRPAL_NO 0


unsigned char message_count = 0;

void put_strings(int scr, int x, int y,  char *str, char pal)
{
	char chr;
	unsigned short i = 0;

	while((chr = *(str++)) != '\0'){
		if(spr_count >= MAX_SPRITE)
			break;
		if((chr >= 0x30)){ // && (chr <= 0x5f)){
			chr_data[spr_count].x = (x + i) * 8 + SPR_OFS_X * 0; \
			chr_data[spr_count].y = (y) * 8 + SPR_OFS_Y * 0 + 2; \
			chr_data[spr_count].pat_num = (chr - '0' + CHR_TOP); \
			chr_data[spr_count].atr = (CHRPAL_NO + 256) | 0x8000; \
			spr_count++; \
		}
		i++;
	}
	message_count = spr_count;
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
	put_strings(SCREEN2, 15, 25, str_temp, CHRPAL_NO);
	if((score >= hiscore) && ((score % 10) == 0)){
		hiscore = score;
		put_strings(SCREEN2, 8, 25, "HIGH", CHRPAL_NO);
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
//	if(score < hiscore)
//		put_strings(SCREEN2, 8, 25, "SCORE", CHRPAL_NO);
	score_display();
}

void hiscore_display(void)
{
	if(score > hiscore)
		if((score % 10) == 0)
			hiscore = score;

	put_numd(hiscore, 8);

	put_strings(SCREEN2, 8, 27, "HIGH", CHRPAL_NO);
	put_strings(SCREEN2, 15, 27, str_temp, CHRPAL_NO);
}
/*
void hiscore_display_clear(void)
{
	put_strings(SCREEN2, 8, 27, "     ", CHRPAL_NO);
	put_strings(SCREEN2, 15, 27, "        ", CHRPAL_NO);
}
*/
CHR_PARA2 *pchr_data;

/* スプライト位置を定義するマクロ */
#define DEF_SP_SINGLE( NO, X, Y, PAT, PAL, ATR) {\
	pchr_data = &chr_data[NO];\
	pchr_data->x = (X >> SHIFT_NUM) + SPR_OFS_X; \
	pchr_data->y = (Y >> SHIFT_NUM) + SPR_OFS_Y + 2; \
	pchr_data->pat_num = PAT | (ATR << 10); \
	pchr_data->atr = (PAL + 256) | 0x8000; \
	NO++; \
}

void spr_on(int num){
//	SPR_display(1, num);
	outp(0x450, 0);
	outp(0x452, (1024 - num) % 256);

	outp(0x450, 1);
	outp(0x452, 0x80 | (((1024 - num) / 256) & 0x03));
}

// スプライトポインタ
void set_sprite(int num, int posx, int posy) {
	spr_x[num] = posx;
	spr_y[num] = posy;
}

// スプライト設定
void set_sprite_all(void)
{
	int i, j;
	int pchr_data2;

/* スプライト表示 */
/* EGB/TBIOSは使わずスプライトRAMに直接書き込む */
/* 	SPR_display(2, 0); */
/* SPRAM先頭アドレスをスプライト表示最大数から算出 */
/* 最大数が可変の時はどうなる? */
//	_FP_OFF(spram) = (1024 - MAX_SPRITE) * 8;
//	spram = (1024 - MAX_SPRITE) * 8;

/* 表示数ぶん書き込む */
	if(spr_count > MAX_SPRITE){
		spram = (1024 - MAX_SPRITE) * 8;
		_disable();
		if(total_count & 1){
			pchr_data2 = (int)&chr_data[spr_count - 1];
			for(i = spr_count - 1, j = 0; j < (MAX_SPRITE - message_count); i--, j++){
//				PUT_SP(chr_data[i].x, chr_data[i].y, chr_data[i].pat_num, chr_data[i].atr);
				_memcpyfar( LSEG_CODE_READ_WRITE, pchr_data2, GSEG_SPRITE_PATTERN, spram, 8);
				pchr_data2 -= 8;
				spram += 8;
			}
			pchr_data2 = (int)&chr_data[message_count - 1];
			for(i = message_count - 1; i >= 0; i--){
//				PUT_SP(chr_data[i].x, chr_data[i].y, chr_data[i].pat_num, chr_data[i].atr);
				_memcpyfar( LSEG_CODE_READ_WRITE, pchr_data2, GSEG_SPRITE_PATTERN, spram, 8);
				pchr_data2 -= 8;
				spram += 8;
			}

		}else{
			pchr_data2 = (int)&chr_data[MAX_SPRITE - 1];
			for(i = MAX_SPRITE - 1; i >= 0; i--){
//				PUT_SP(chr_data[i].x, chr_data[i].y, chr_data[i].pat_num, chr_data[i].atr);
				_memcpyfar( LSEG_CODE_READ_WRITE, pchr_data2, GSEG_SPRITE_PATTERN, spram, 8);
				pchr_data2 -= 8;
				spram += 8;
			}
		}
		spr_on(MAX_SPRITE);
		_enable();
		old_count = MAX_SPRITE;
	}else{
		spram = (1024 - spr_count) * 8;
		pchr_data2 = (int)&chr_data[spr_count - 1];
		_disable();
		for(i = spr_count - 1; i >= 0; i--){
//			PUT_SP(chr_data[i].x, chr_data[i].y, chr_data[i].pat_num, chr_data[i].atr);
			_memcpyfar( LSEG_CODE_READ_WRITE, pchr_data2, GSEG_SPRITE_PATTERN, spram, 8);
			pchr_data2 -= 8;
			spram += 8;
		}
		spr_on(spr_count);
		_enable();

/* スプライトの表示数が減った場合､減った分を画面外に消去する */
/* 増える分には問題ない */
/*		if (old_count > spr_count){
			for(i = 0;i < (old_count - spr_count); i++){
				PUT_SP(0,(SCREEN_MAX_Y + 2),0,0x2000);
			}
		}*/
/* このフレ－ムで表示したスプライトの数を保存 */
		old_count = spr_count;
	}


	++total_count;

///	if(seflag){
//		if(soundflag == TRUE)
//			if(se_check())
//				se_stop();
//		S_IL_FUNC(se_play(sndtable[0], seflag - 1));	/* 効果音 */
///		SND_pcm_play_stop(71) ;
///		SND_pcm_play( 71, 60, 127, &SNDBUFF[seflag - 1][0] );

//		MSV_partstop(13);
//		MSV_partplay(13 , msv_se[seflag - 1]);
///		seflag = 0;
///	}
}

void set_sprite_pattern(int num, int no) {
	sprite_pattern_no[num] = no + CHR_TOP + FONTPARTS + TITLEPARTS;
}

void se(void)
{
//	seflag = 1;
	SND_pcm_play_stop(71) ;
	SND_pcm_play( 71, 60, 127, &SNDBUFF[seflag - 1][0] );
}

void seoff(void)
{
	SND_pcm_play_stop(71) ;
}

/* 画面設定 */
void grp_set(void){

	char para[64];

	EGB_init(egb_work, 1536);

/* 31kHz出力用 */
	EGB_resolution(egb_work, 0, 3);			/* ペ－ジ0は640x480/16 */
	EGB_resolution(egb_work, 1, 5);			/* ペ－ジ1は256x512/32768 */
	EGB_displayPage(egb_work, 1, 3);		/* 上にくるペ－ジは1で両方とも表示 */

/* もし15kHz出力したいならこうする */
/* 	EGB_resolution(egb_work, 0, 8);			/* ペ－ジ0は512x256/32768 */
/* 	EGB_resolution(egb_work, 1, 11);		/* ペ－ジ1は256x512/32768 */

	EGB_writePage(egb_work, 0);				/* ペ－ジ0(BG)の設定 */
	EGB_displayStart(egb_work, 2, 2, 2);		/* 表示拡大率(縦横2倍) */
	EGB_displayStart(egb_work, 3, 640, 480);	/* EGB画面の大きさ(640x480) */
	EGB_displayStart(egb_work, 0, 0, 0);		/* 表示開始位置 */
	EGB_displayStart(egb_work, 1, 0, 0);		/* 仮想画面中の移動 */

	EGB_writePage(egb_work, 1);				/* ペ－ジ1(スプライト)の設定 */
	EGB_displayStart(egb_work, 2, 2, 2);		/* */
	EGB_displayStart(egb_work, 3, 256, 240);	/* */
	EGB_displayStart(egb_work, 0, 32, 0);		/* */
	EGB_displayStart(egb_work, 1, 0, 2);		/* 下に2ドットずらす(仕様) */

	EGB_color(egb_work, 0, 0x8000);				/* ペ－ジ1をクリアスクリ－ン */
	EGB_color(egb_work, 2, 0x8000);				/* 透明色で埋める */
	EGB_writePage(egb_work, 1);
	EGB_clearScreen(egb_work);
}

/* 画面を戻す(コンソ－ルからの起動対策) */
void grp_term(void){
	EGB_color(egb_work, 0, 0x8000);				/* ペ－ジ1をクリアスクリ－ン */
	EGB_color(egb_work, 2, 0x8000);				/* 透明色で埋める */
	EGB_writePage(egb_work, 1);
	EGB_clearScreen(egb_work);

	EGB_resolution(egb_work, 0, 4);		/* ペ－ジ0は640x400/16 */
	EGB_resolution(egb_work, 1, 4);		/* ペ－ジ1は640x400/16 */
	EGB_displayPage(egb_work, 0, 3);

	EGB_writePage(egb_work, 0);			/* ペ－ジ0をクリアスクリ－ン */
	EGB_clearScreen(egb_work);
	EGB_displayStart(egb_work,0,0,0);
	EGB_displayStart(egb_work, 1, 0, 0);
	EGB_displayStart(egb_work,2,1,1);
	EGB_displayStart(egb_work, 3, 640, 400);
	EGB_writePage(egb_work, 1);			/* ペ－ジ1をクリアスクリ－ン */
	EGB_clearScreen(egb_work);
	EGB_displayStart(egb_work,0,0,0);
	EGB_displayStart(egb_work, 1, 0, 0);
	EGB_displayStart(egb_work,2,1,1);
	EGB_displayStart(egb_work, 3, 640, 400);
}

/* スプライト面の塗りつぶし設定 */
/* タイリングにすると疑似半透明にもなる */
void spr_fill(int spbackcolor){
	int i;

/* ダブルバッファなので2度行う */
/* バッファ0 */
//	_FP_OFF(vram) = 0x40000;
	vram = 0x40000;
	i = 256 * 2 * 2;
	while(i--){
		_poke_word(0x120, vram, spbackcolor);
		vram += 2;
	}
//		*vram++ = spbackcolor;

/* バッファ1 */
//	_FP_OFF(vram) = 0x60000;
	i = 256 * 2 * 2;
	while(i--){
		_poke_word(0x120, vram, spbackcolor);
		vram += 2;
	}
//		*vram++ = spbackcolor;
}

/* 疑似半透明 */
/* [100000 00000 00000][0001 11 00 111 0 0111] */
/* 0x642 0xc63 0x1ce7 */
/* [8000 7fff] */
void spr_fill2(int level){
	int i,j;

	j = ((((level << 5) | level) << 5) | level);

/* バッファ0 */
//	_FP_OFF(vram) = 0x40000;
	vram = 0x40000;
	i = 128;
	while(i--){
		_poke_word(0x120, vram++, 0x8000);
		_poke_word(0x120, vram++, j);
//		*vram++ = 0x8000;
//		*vram++ = j;
	}
	i = 128;
	while(i--){
		_poke_word(0x120, vram++, j);
		_poke_word(0x120, vram++, 0x8000);
//		*vram++ = j;
//		*vram++ = 0x8000;
	}
/* バッファ1 */
//	_FP_OFF(vram) = 0x60000;
	vram = 0x60000;
	i = 128;
	while(i--){
		_poke_word(0x120, vram++, j);
		_poke_word(0x120, vram++, 0x8000);
//		*vram++ = j;
//		*vram++ = 0x8000;
	}
	i = 128;
	while(i--){
		_poke_word(0x120, vram++, 0x8000);
		_poke_word(0x120, vram++, j);
//		*vram++ = 0x8000;
//		*vram++ = j;
	}
}

/* スプライトを全て画面外に移す */
void spr_clear(void){
	int i;
//	_FP_OFF(spram) = 0;
	spram = 0;
	for(i = 0; i < 1024; i++){
		_poke_word(0x130, spram, 0);
		spram += 2;
//		*(spram++) = 0;
		_poke_word(0x130, spram, SCREEN_MAX_Y + 2);
		spram += 2;
//		*(spram++) = SCREEN_MAX_Y + 2; ///240;
		_poke_word(0x130, spram, 0);
		spram += 2;
//		*(spram++) = 0x0; //3ff;
		_poke_word(0x130, spram, 0x2000);
		spram += 2;
//		*(spram++) = 0x2000; //2fff;
	}
}

void spr_off(void){
	outp(0x450, 0);
	outp(0x452, 0xff);
	outp(0x450, 1);
	outp(0x452, 0x7f);
//	SPR_display(0,1);
}

/* スプライトパタ－ン設定 */
void spr_set(int spbackcolor){
/* スプライト定義 */
	SPR_init();
	spr_clear();
 	spr_fill(spbackcolor);
/*	spr_fill2(7); */
/* 	spr_make(); */
}

/* パッドの読込 */
void pad_read(int port, int *a, int *b, int *pd){
	int data, ab;

/*	SND_joy_in_2(port, &data); */
 	data = inportb(0x4d0 + port * 2); 

/*	*s = ((data >> 6) & 0x03) ^ 0x03;*/
	ab = ((data >> 4) & 0x03) ^ 0x03;
	*a = ab & 0x01;
	*b = (ab >> 1) & 0x01;

	*pd = (data & 0x0f) ^ 0x0f;
}

#include "inkey.h"

unsigned char keyscan(void)
{
	unsigned char st, pd;
	unsigned char k5, k6, k7, k8, k9, ka;
	unsigned char paddata;
	static char matrix[16];
	unsigned char keycode = 0;

	_disable();
	KYB_matrix(matrix);
	_enable();

	k5 = matrix[5];
	k6 = matrix[6];
	k7 = matrix[7];
	k8 = matrix[8];
	k9 = matrix[9];
	ka = matrix[0xa];

	paddata = inportb(0x4d0 + 0 * 2); 
	st = (paddata & 0x0f);
	pd = (paddata >> 4) & 0x03;

	if((k5 & 0x04) || (k6 & 0x20) || !(pd & 0x01)) /* Z,SPACE */
		keycode |= KEY_A;
	if((k5 & 0x08) || !(pd & 0x02)) /* X */
		keycode |= KEY_B;
	if((k7 & 0x08) || (k9 & 0x20) || !(st & 0x01)) /* 8 */
		keycode |= KEY_UP1;
	if((k8 & 0x08) || (ka & 0x01) || !(st & 0x02)) /* 2 */
		keycode |= KEY_DOWN1;

	if(!(st & 0x0c)){ /* RL */
		keycode |= KEY_START;
	}else{
		if((k7 & 0x40) || (k9 & 0x80) || !(st & 0x04)) /* 4 */
			keycode |= KEY_LEFT1;
		if((k8 & 0x01) || (ka & 0x02) || !(st & 0x08)) /* 6 */
			keycode |= KEY_RIGHT1;
	}

	return keycode;
}


void wait_vsync(void)
{
	while(!vsync_flag);
	vsync_flag = 0;
	/* VSYNC(=1)待ち */
//*	do{
//		outportb(0x440, 30);
//	}while((inportb(0x0443) & 0x04)); /* 動作中 */
//	do{
//		outportb(0x440, 30);
//	}while(!(inportb(0x0443) & 0x04)); /* 動作中 */
}

void wait_vsync2(void)
{
	while(!vsync_flag);
	vsync_flag = 0;
	/* VSYNC(=1)待ち */
//	do{
//		outportb(0x440, 30);
//	}while((inportb(0x0443) & 0x04)); /* 動作中 */
//	do{
//		outportb(0x440, 30);
//	}while(!(inportb(0x0443) & 0x04)); /* 動作中 */
}

void wait_sprite(void)
{
/* スプライト動作チェック(BUSY=1) */
/* VSYNC割り込みしない場合は2回見ないと誤判断する可能性があった */
//	while(!(inportb(0x044c) & 0x02)); /* 動作中 */
	while((inportb(0x044c) & 0x02)); /* 動作中 */
//	while(!(inportb(0x044c) & 0x02)); /* 動作中 */

/*	_outportb(0x450, 1);	// スプライトコントローラーを切るテスト
	_outportb(0x452, 0x7f);*/
}

void put_sprite(void)
{
	int i;
	for(i = 0; i < 8; ++i){
		DEF_SP_SINGLE(spr_count, spr_x[i], spr_y[i], sprite_pattern_no[i], CHRPAL_NO, 0);
	}

	for(i = 0; i < 16; ++i){
		DEF_SP_SINGLE(spr_count, (i * 16 - SPR_OFS_X) << SHIFT_NUM, (21 * 8 - SPR_OFS_Y) << SHIFT_NUM, 	IMG_SET(8, 0),  CHRPAL_NO, 0);
	}
}

int	main(int argc,char **argv)
{
	short i, j;
	short errlv;
	unsigned char keycode;

/* 実行時引数が設定されているかどうか調べる */
/*	if (argc < 2 ) */
	/*	return 1;*/
/*	if (argv[1] == NULL)
		title_filename = "";
	else
		title_filename = argv[1];
*/

//	for(i = 0; i <  256; ++i)
//		sin_table[i] = sin_table[i + 256] = (16 * sin(2 * M_PI * i / 256));

//	load_fmdbgm("dummy.ob2");

	if ((SND_load("se1.snd", &SNDBUFF[0][0])) == NULL)
		return 1;
//	if ((SND_load("se2.snd", &SNDBUFF[1][0])) == NULL)
//		return ERROR;
//	if ((SND_load("se3.snd", &SNDBUFF[2][0])) == NULL)
//		return ERROR;
//	if ((SND_load("se4.snd", &SNDBUFF[3][0])) == NULL)
//		return ERROR;

/* サウンドライブラリの初期化 */
/* 	if (eup_init()) */
/* 		return ERROR; */
	SND_init(sndwork);
	SND_pcm_mode_set( 1 ); 
//	MSV_init(MSVwork,MSVWorkSize,EXPMODE);
//	SND_elevol_all_mute(-1);
	SND_elevol_mute(0x33);

//	seflag = 1;
//	se();

/* 画面初期化 */
	grp_set();
/*	grp_fill(BACKCOLOR); */
	spr_set(SPBACKCOLOR);

//	_FP_SEG(spram)=0x130;
//	_FP_OFF(spram) = 0x4000; // + 256*SPRSIZEX*SPRSIZEY;
	spram = 0x4000;
	font_load("FONTYOKO.SC5", FONTPARTS); //SPRPARTS);

	msxspconv("PLUSTAKE.SC5", SPRPARTS); //PCGPARTS / 4, 256 - PCGPARTS / 4);

	spr_on(MAX_SPRITE);	/* スプライト動作の開始 */

/* ゲ－ムを実行 */

	spr_clear();
	spr_count = old_count = 0;

	init_vsync();


	wait_vsync2();
	pal_allblack(BGPAL_NO);
	pal_allblack(CHRPAL_NO);
	paint(0x0);

	pal_all(CHRPAL_NO, org_pal);
	pal_all(BGPAL_NO, org_pal);
	pal_all(REVPAL_NO, rev_pal);

	old_count = MAX_SPRITE;



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
		Entity player = {160 - 8, 160-16 + 8*3};
		Entity bullets[3] = {{0}};
		Entity enemies[4] = {{0}};
		Entity pluses[8] = {{0}};  // Plus最大8個

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

		int b,e,p;
		spr_count = 0;

		score = 0;
		combo = 0;

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
						enemies[i].x = 16 + ((rand() & ((SCREEN_WIDTH - 16))));// * 4 / 5 + 1)) * 5 / 4);
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

			spr_count = 0;
//			if(score_disp_flag){
//				score_disp_flag = 0;
				score_display();
//			}
//			else if(combo_display_flag){
//				combo_display_flag = 0;
				if(combo){
					combo_display();
				}//else{
//					hiscore_display_clear();
//				}
//			}

			put_sprite();

			wait_sprite();
			set_sprite_all();
			wait_vsync();

//			put_strings(SCREEN2, 8, 22, "SCORE", CHRPAL_NO);
//	put_numd(score, 8);
			KYB_read( 1, &encode );
		}
//		seoff();
		for(;;){
			keycode = keyscan();
			if(!(keycode & KEY_A))
				break;
		}
		// ゲームオーバー画面
		/* ここにテキスト描画追加可能 */
		spr_count = 0;
		score_display();
		hiscore_display();
		put_sprite();
		wait_sprite();
		set_sprite_all();
		wait_vsync();
		for(;;){
			keycode = keyscan();
			if (keycode & KEY_A)
				break;
		}
//		hiscore_display_clear();
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
	reset_vsync();

	pal_allblack(BGPAL_NO);
	pal_allblack(CHRPAL_NO);

/* 画面を戻す(コンソ－ル対応) */

	spr_off();
	grp_term();

/* 音楽演奏停止 */
//	if(MSV_stat_flag());
//		MSV_fader(255,-100,1000/4);
//	while(MSV_stat_flag());

/* サウンドライブラリの開放 */
/* 	eup_stop(); */
/* 	eup_term(); */
	SND_elevol_mute(0x00);
//	MSV_end();
	SND_end();

//	reset_vsync();

	return (0);
}

