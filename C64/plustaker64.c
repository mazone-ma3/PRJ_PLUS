/* plustaker64.c By m@33 with Grok */

#include <stdio.h>
#include <stdlib.h>

/************************************************************************/
/*		BIT操作マクロ定義												*/
/************************************************************************/

/* BITデータ算出 */
#define BITDATA(n) (1 << (n))
//unsigned char bitdata[8] = { 1, 2, 4, 8, 16, 32, 64, 128};
//#define BITDATA(n) bitdata[n]

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

enum {
	COLOR_BLACK       = 0,
	COLOR_WHITE       = 1,
	COLOR_RED         = 2,
	COLOR_CYAN        = 3,
	COLOR_PURPLE      = 4,
	COLOR_GREEN       = 5,
	COLOR_BLUE        = 6,
	COLOR_YELLOW      = 7,
	COLOR_ORANGE      = 8,
	COLOR_BROWN       = 9,
	COLOR_LIGHT_RED   = 10,
	COLOR_GRAY_ONE    = 11,
	COLOR_DARK_GREY   = 11,
	COLOR_GRAY_TWO    = 12,
	COLOR_MEDIUM_GREY = 12,
	COLOR_LIGHT_GREEN = 13,
	COLOR_LIGHT_BLUE  = 14,
	COLOR_GRAY_THREE  = 15,
	COLOR_LIGHT_GREY  = 15,
};

enum {
	VICOFFSET = 0x8000,
	BGRAM = (0x400 + VICOFFSET),
	CRAM = (0xd800),
	CHARDATA = (0x800 + VICOFFSET)
};

#define SPRITE_POINTERS ((unsigned char *)0x07f8 + VICOFFSET)
#define SPRITE_ENABLE_REGISTER ((unsigned char *)0xd015)
#define SPRITE_0_COLOR_REGISTER ((unsigned char *)0xd027)
#define SPRITE_MULTICOLOR_REGISTERS ((unsigned char *)0xd01c)
#define SPMC0 ((unsigned char *)0xd025)
#define SPMC1 ((unsigned char *)0xd026)

#define MSGIGX ((unsigned char *)0xd010)
#define SP0X ((unsigned char *)0xd000)
#define SP0Y ((unsigned char *)0xd001)
#define SPXX ((unsigned char *)0xd010)

#define RASTER ((volatile unsigned char *)0xd012)

#define spr01ShapeData ((unsigned char *)(0xf * 64 + VICOFFSET))
#define spr02ShapeData ((unsigned char *)(0xe * 64 + VICOFFSET))
#define spr03ShapeData ((unsigned char *)(0xd * 64 + VICOFFSET))
#define spr04ShapeData ((unsigned char *)(0xc * 64 + VICOFFSET))
#define spr05ShapeData ((unsigned char *)(0xb * 64 + VICOFFSET))
#define spr06ShapeData ((unsigned char *)(0xa * 64 + VICOFFSET))
#define spr07ShapeData ((unsigned char *)(0x9 * 64 + VICOFFSET))
#define spr08ShapeData ((unsigned char *)(0x8 * 64 + VICOFFSET))

#define porta ((volatile unsigned char *)0xdc00)
#define portb ((volatile unsigned char *)0xdc01)
#define portadir ((volatile unsigned char *)0xdc02)
#define portbdir ((volatile unsigned char *)0xdc03)

#define BoaderColorRegister ((unsigned char *)0xd020)
#define BackgroundColor ((unsigned char *)0xd021)

#define screenarea0 ((unsigned char *)(BGRAM))
#define screenarea1 ((unsigned char *)(BGRAM + 0x100))
#define screenarea2 ((unsigned char *)(BGRAM + 0x200))
#define screenarea3 ((unsigned char *)(BGRAM + 0x2e8))

#define foregroundarea0 ((unsigned char *)(CRAM))
#define foregroundarea1 ((unsigned char *)(CRAM + 0x100))
#define foregroundarea2 ((unsigned char *)(CRAM + 0x200))
#define foregroundarea3 ((unsigned char *)(CRAM + 0x2e8))

#define interrupt ((unsigned char *)0xdc0e)
#define charset ((unsigned char *)0x001L)
#define charsetram ((unsigned char *)0xd018)
#define vicbank ((unsigned char *)0xdd00)
unsigned char *chardata, *chardata2, oldcharsetram, oldvicbank;

#define SCREENCONTROL1 ((unsigned char *)0xd011)
#define SCREENCONTROL2 ((unsigned char *)0xd016)

#define EXTBGCOOLR1 ((unsigned char *)0xd022)
#define EXTBGCOOLR2 ((unsigned char *)0xd023)

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200 // PAL

// max/min マクロ
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

unsigned char spr0[64] = {
	 0x1a, 0x90, 0x00,
	 0x65, 0x64, 0x00,
	 0x9a, 0x98, 0x00,
	 0xaa, 0xa8, 0x00,
	 0x99, 0x98, 0x00,
	 0xb7, 0x78, 0x00,
	 0xb7, 0x78, 0x00,
	 0x2f, 0xe0, 0x00,
	 0x3f, 0xf0, 0x00,
	 0xe6, 0x6c, 0x00,
	 0xe5, 0x6c, 0x00,
	 0x35, 0x90, 0x00,
	 0x1d, 0xb0, 0x00,
	 0x1b, 0xe0, 0x00,
	 0x55, 0x54, 0x00,
	 0x69, 0xa4, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0,
};

unsigned char spr1[64] = {
	 0x07, 0xe0, 0x00,
	 0x07, 0xe0, 0x00,
	 0x07, 0xe0, 0x00,
	 0x07, 0xe0, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0,
};

unsigned char spr2[64] = {
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0xaa, 0xaa, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00,
	 0,
};

unsigned char font[] = {
	 0x1f,
	 0x3e,
	 0x3a,
	 0x00,
	 0xf1,
	 0xe3,
	 0xa3,
	 0x00,
};

char *bgram;
unsigned char *cram;

// 構造体
typedef struct {
	int x, y;
	int active;
} Entity;

static void put_strings(unsigned char x, unsigned char y,  char *str, unsigned char pal)
{
	char chr;
	bgram = (char *)BGRAM + (x + y * 40);
	cram = (unsigned char *)CRAM + (x + y * 40);

//	if(cram > (char *)0x0dbe7)
//		return;

	while((chr = *(str++)) != '\0'){
//		if((chr < 0x30)) //|| (chr > 0x5f))
//			chr = 0x20;
//		else
//			chr -= '0';
		chr &= 0x3f;
//		if((chr >= 0x40))
//			chr -= 0x40;
		*(bgram++) = chr;
		*(cram++) = pal;
	}
}

char str_temp[9];

static void put_numd(long j, char digit)
{
	char i = digit;

	while(i--){
		str_temp[i] = j % 10 + 0x30;
		j /= 10;
	}
	str_temp[digit] = '\0';
}


long score = 0, hiscore = 5000;
#define CHRPAL_NO COLOR_WHITE

void score_display(void)
{
	put_numd(score, 8);
	put_strings(19, 22 , str_temp, CHRPAL_NO);
	if(score >= hiscore){
		if((score % 10) == 0){
			hiscore = score;
			put_strings(12, 22, "HIGH ", CHRPAL_NO);
		}
	}else
		put_strings(12, 22, "SCORE", CHRPAL_NO);

}

int combo = 0;

void combo_display(void)
{
	put_numd(combo, 8);
	put_strings(19, 24 , str_temp, CHRPAL_NO);
		put_strings(12, 24, "COMBO", CHRPAL_NO);

}

void score_displayall(void)
{
//	put_strings(8, 22, "SCORE", CHRPAL_NO);
	score_display();
}

void hiscore_display(void)
{
	if(score > hiscore)
		if((score % 10) == 0)
			hiscore = score;

	put_numd(hiscore, 8);

	put_strings(12, 24, "HIGH ", CHRPAL_NO);
	put_strings(19, 24, str_temp, CHRPAL_NO);
}

void hiscore_display_clear(void)
{
	put_strings(12, 24, "     ", CHRPAL_NO);
	put_strings(19, 24, "        ", CHRPAL_NO);
}

#ifdef DEBUG
static void copy(void)
{
	register unsigned char ra asm("a");

	asm volatile(
//		"txa\n"
//		"pha\n"
//		"tya\n"
//		"pha\n"
		"lda #0\n"
		"ldy #$d0\n"
		"sta 95\n"
		"sty 96\n"
		"lda #0\n"
		"ldy #$e0\n"
		"sta 90\n"
		"sty 91\n"
		"lda #0\n"
		"ldy #$40\n"
		"sta 88\n"
		"sty 89\n"
		"jsr $a3bf\n"
//		"pla\n"
//		"tay\n"
//		"pla\n"
//		"tax\n"
		://"=a"(ra)	/* 値が返るレジスタ変数 */
		://"a"(ra)	/* 引数として使われるレジスタ変数 */
		:"%a","%x","%y"		/* 破壊されるレジスタ */
	);
}
#endif

/*static void put_sprite(unsigned char no, short x, unsigned char y, unsigned char color, unsigned char pat_no)
{
	BITSET(no, *SPRITE_ENABLE_REGISTER);
	SP0Y[no * 2] = y;
	SP0X[no * 2] = x & 0xff;
	if(x & 256){
		BITSET(no, *SPXX);
	}else{
		BITCLR(no, *SPXX);
	}
	SPRITE_0_COLOR_REGISTER[no] = color;
	SPRITE_POINTERS[no] = pat_no;
}*/

#define put_sprite(no, x, y, color, pat_no) {\
	BITSET(no, *SPRITE_ENABLE_REGISTER);\
	SP0Y[no * 2] = y;\
	SP0X[no * 2] = x & 0xff;\
	if(x & 256){\
		BITSET(no, *SPXX);\
	}else{\
		BITCLR(no, *SPXX);\
	}\
	SPRITE_0_COLOR_REGISTER[no] = color;\
	SPRITE_POINTERS[no] = pat_no;\
}

char sprite_pattern_no[8], sprite_color_no[8];
int spr_x[8], spr_y[8];

void set_sprite(char num, int posx, int posy)
{
	spr_x[num] = posx + 8;
	spr_y[num] = posy + 16 + 16 + 2;
//	put_sprite(i, x - 16 * i, y - 16 * i, color, 0x0d);
}

char patternno_table[] = {0x0c, 0x0d, 0x0e};
char patterncolor_table[] = {COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW};

void set_sprite_pattern(int num, int no) {
	sprite_pattern_no[num] = patternno_table[no];
	sprite_color_no[num] = patterncolor_table[no];
}

void set_sprite_all(void) {
	char i;
	for(i = 0; i < 8; ++i){
		put_sprite(i, spr_x[i], spr_y[i], sprite_color_no[i], sprite_pattern_no[i]);
	}
}


unsigned char keyscan(void)
{
	unsigned char keycode = 0;
			/* PAD */
			*portadir = 224;
			*porta = 0xff;
			if(!BITTST(0, *portb)){	/* UP */
				BITSET(0, keycode);
			}
			if(!BITTST(1, *portb)){	/* DOWN */
				BITSET(1, keycode);
			}
			if(!BITTST(2, *portb)){	/* LEFT */
				BITSET(2, keycode);
			}
			if(!BITTST(3, *portb)){	/* RIGHT */
				BITSET(3, keycode);
			}
			if(!BITTST(4, *portb)){	/* SHOT */
				BITSET(4, keycode);
			}

			if(keycode)
				goto skip;

			/* KEYBOARD */
			*portadir = 255;
			*porta = 0xef;

			if(!BITTST(1, *portb)){	/* I */
				BITSET(0, keycode);
			}
			if(!BITTST(4, *portb)){	/* M */
				BITSET(1, keycode);
			}
			if(!BITTST(2, *portb)){	/* J */
				BITSET(2, keycode);
			}
			if(!BITTST(5, *portb)){	/* K */
				BITSET(3, keycode);
			}
			if(!BITTST(7, *portb)){	/* N */
				BITSET(4, keycode);
			}
			if(!BITTST(3, *portb)){	/* 0 */
				BITSET(5, keycode);
			}

			*porta = 0x7f;
			if(!BITTST(4, *portb)){	/* ' ' */
				BITSET(4, keycode);
			}
			if(!BITTST(1, *portb)){	/* <- */
				BITSET(5, keycode);
			}
			if(!BITTST(7, *portb)){	/* STOP */
				BITSET(5, keycode);
			}

			*porta = 0xfd;
			if(!BITTST(4, *portb)){	/* Z */
				BITSET(4, keycode);
			}

skip:
			return keycode;
}


short x;
unsigned char y, color;

unsigned char i = 0, keycode, oldcharset, data = 0;
short j;

//unsigned char multi16[8] = { 0, 16, 32, 48, 64, 96, 128, 192};

int main(void)
{
/*	bgram = (char *)BGRAM;
	cram = (char *)0xCRAM;
	for(i = 0x0; i < 0xff; ++i){
//		putchar(i);
		*bgram++ = i; // 0x1f;
		*cram++ = COLOR_WHITE; //LIGHT_RED;
	}*/
	//Copy font ROM -> RAM
	*interrupt = *interrupt & 0xfe;
	oldcharset = *charset;
	*charset = *charset & 0xfb;
	*charset |= 0x03;	// BASIC ROM

//	copy();
	for(j = 0, chardata = (unsigned char *)(CHARDATA), chardata2 = (unsigned char *)0xd000; j < 4096; ++j, ++chardata, ++chardata2){
		*chardata = *chardata2;
	}
/*	for(chardata = (unsigned char *)53248; chardata <= (unsigned char *)57343; ++chardata){
			*(chardata - 40960) = *chardata;
	}*/

//	*charset = *charset | 0x04;
	*charset = oldcharset;
	*interrupt = *interrupt | 0x01;
	oldcharsetram = *charsetram;
	*charsetram = *charsetram & 0xf1 | 0x02;	/* Pointer of CHARDATA */
	oldvicbank = *vicbank;
	*vicbank = *vicbank & 0xfc | 0x01;	/* VICBANK OFFSET */

//	register unsigned char j;
	for(chardata = (unsigned char *)((CHARDATA + 0x1b * 8)), j = 0; j < 8; ++chardata, ++j){
		data = font[j];
		*chardata = data;
		*(chardata + 128 * 8) = 255 - data;
	}

/*	asm(
		"txa\n"
		"pha\n"
		"tya\n"
		"pha\n"
		"jsr $e544\n"
		"pla\n"
		"tay\n"
		"pla\n"
		"tax\n"
	);*/

	// Clear Screen Black
	*BoaderColorRegister = 0;
	*BackgroundColor = 0;

	*SCREENCONTROL1 = *SCREENCONTROL1 & 0x9f;
	*SCREENCONTROL2 = *SCREENCONTROL2 | 0x10;

	*EXTBGCOOLR1 = COLOR_WHITE;
	*EXTBGCOOLR2 = COLOR_YELLOW;

	for(;;){
		screenarea0[i] = screenarea1[i] = screenarea2[i] = screenarea3[i] = 0x20;

		foregroundarea0[i] = foregroundarea1[i] = foregroundarea2[i] = foregroundarea3[i] = 0x01;
		++i;
		if(i == 0)
			break;
	}

	// Put Strings
//	char *bgram, *cram;
//	put_strings(0, 0, "01234567890", COLOR_WHITE );
//	put_strings(0, 1, "HELLO WORLD\x5f", COLOR_WHITE );


	for(i = 0; i < 40; ++i){
		put_strings(i, 18, "\x5b", COLOR_LIGHT_RED);
		put_strings(i, 19, "\x5b", COLOR_LIGHT_RED);
	}
//	put_strings(12, 22, "SCORE 000000000", COLOR_WHITE );
//	put_strings(12, 24, "LIFE", COLOR_WHITE );


	// Set SPRITE Data
//	*SPRITE_ENABLE_REGISTER = 0x01;
	BITSET(0, *SPRITE_MULTICOLOR_REGISTERS);
	*SPMC0 = COLOR_LIGHT_RED; //YELLOW; //WHITE;
	*SPMC1 = COLOR_WHITE; //LIGHT_RED;
//	SPRITE_POINTERS[0] = 0x0d;	/* 13 */

	for(i = 0; i < 63; i++){
		spr04ShapeData[i] = spr0[i];
	}
	for(i = 0; i < 63; i++){
		spr03ShapeData[i] = spr1[i];
	}
	for(i = 0; i < 63; i++){
		spr02ShapeData[i] = spr2[i];
	}

	color = COLOR_YELLOW; //LIGHT_RED; //COLOR_YELLOW;
	x = 176;
//	*MSGIGX = 0;
//	*SP0X = x;
	y = 178;

	for(i = 0; i < 8; ++i){
		char patno;
		BITSET(i, *SPRITE_MULTICOLOR_REGISTERS);
		set_sprite(i, x - 16 * i, y - 16 * i); //, color, 0x0d);
		switch(i){
			case 0:
				patno = 0;
				break;
			case 1:
			case 2:
			case 3:
				patno = 1;
				break;
			case 5:
			case 6:
			case 7:
			case 8:
				patno = 2;
				break;
		}
		set_sprite_pattern(i, patno);
//		put_sprite(i, x - multi16[i], y - multi16[i], color, 0x0d);
	}
	set_sprite_all();

/*	BITSET(1, *SPRITE_MULTICOLOR_REGISTERS);
	put_sprite(1, x - 16 * 1, y - 16 * 1, color, 0x0d);
	BITSET(2, *SPRITE_MULTICOLOR_REGISTERS);
	put_sprite(2, x - 16 * 2, y - 16 * 2, color, 0x0d);
	BITSET(3, *SPRITE_MULTICOLOR_REGISTERS);
	put_sprite(3, x - 16 * 3, y - 16 * 3, color, 0x0d);
	BITSET(4, *SPRITE_MULTICOLOR_REGISTERS);
	put_sprite(4, x - 16 * 4, y - 16 * 4, color, 0x0d);
	BITSET(5, *SPRITE_MULTICOLOR_REGISTERS);
	put_sprite(5, x - 16 * 5, y - 16 * 5, color, 0x0d);
	BITSET(6, *SPRITE_MULTICOLOR_REGISTERS);
	put_sprite(6, x - 16 * 6, y - 16 * 6, color, 0x0d);
	BITSET(7, *SPRITE_MULTICOLOR_REGISTERS);
	put_sprite(7, x - 16 * 7, y - 16 * 7, color, 0x0d);
*/
	// Main Loop

	for(;;){
		Entity player = {160, 160 - 16};
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
		int score_display_flag = 0;
		int combo_display_flag = 0;
		int enemy_speed = 2;

		int count = 0;

		score_displayall();
//		combo_display();
		while (!game_over) {
			++count;
			if(combo)
				++combo_timer;
			keycode = keyscan();


			if(BITTST(0, keycode)){	/* UP */
				player.y -= 3;
			}
			if(BITTST(1, keycode)){	/* DOWN */
				player.y += 3;
			}
			if(BITTST(2, keycode)){	/* LEFT */
				player.x -= 3; //(*SP0X);
			}
			if(BITTST(3, keycode)){	/* RIGHT */
				player.x += 3; //(*SP0X);
			}
//			if(BITTST(4, keycode)){	/* SHOT */
//				++color;
//			}
			if(BITTST(5, keycode)){	/* END */
				goto end;
			}

//			*SP0X = x & 0xff;
//			*SPXX = (x >> 8)  & 0x01;
			put_sprite(0, x, y, color, 0x0d);

			player.x = MAX(16, MIN(((SCREEN_WIDTH)), player.x));
			player.y = MAX(16, MIN(((SCREEN_HEIGHT)), player.y));

			set_sprite(0, player.x, player.y);

			// 射撃 (ボタン押したら弾発射)
			if(BITTST(4, keycode)){	/* SHOT */
				for (int i = 0; i < 3; i++) {
					if (!bullets[i].active) {
						bullets[i].active = 1;
						bullets[i].x = player.x;
						bullets[i].y = player.y;// - 16;
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
						enemies[i].x = 16 + ((rand() & ((SCREEN_WIDTH - 16) * 4 / 5 + 1)) * 5 / 4);
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
						score_display_flag = 1;
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
						score_display_flag = 1;
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
				combo_display_flag = 1;
				combo_timer = 0;
			}

			// ゲームオーバー (敵接触)
			for (int e = 0; e < 4; e++) {
				if (enemies[e].active && abs(enemies[e].x - player.x) < 16 && abs(enemies[e].y - player.y) < 16) {
					game_over = 1;
				}
			}
			set_sprite_all();
			if(score_display_flag){
				score_display_flag = 0;
				score_display();
			}
			else if(combo_display_flag){
				combo_display_flag = 0;
				if(combo)
					combo_display();
				else
					hiscore_display_clear();
			}

			while(*RASTER != 249);

		}
end:
		for(;;){
			if(!BITTST(4, keyscan()))	/* SHOT */
				break;
		}
		// ゲームオーバー画面
		/* ここにテキスト描画追加可能 */
		hiscore_display();
		for(;;){
			if(BITTST(4, keyscan()))	/* SHOT */
				break;
		}
		hiscore_display_clear();
		// (スプライト全消し)
		for (int i = 0; i < 8; i++){
			set_sprite(i, 0, 0);
			set_sprite_all();
		}
		for(;;){
			if(!BITTST(4, keyscan()))	/* SHOT */
				break;
		}
	}

	*SPRITE_ENABLE_REGISTER = 0x0;
	*SCREENCONTROL2 = *SCREENCONTROL2 & 0xef;
//	*interrupt = *interrupt & 0xfe;
//	*charset = *charset | 0x04;
//	*interrupt = *interrupt | 0x01;
	*charsetram = oldcharsetram;
	*vicbank = oldvicbank;
	*((unsigned char *)0x2b) = 1;
	*((unsigned char *)0x2c) = 8;

	return 0;
}
