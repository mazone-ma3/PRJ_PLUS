#include "inkey.h"

#ifndef DEBUG2
#include "PSGMSXD.H"
#endif


#define COUNT1S 60

#define width 256
#define height 192
#define CHRPAL_NO 0
#define REVPAL_NO 1
#define TRUE 1
#define FALSE 0
#define ERROR 1
#define NOERROR 0
#define SCREEN2 0
#define MAX_SPRITE 32			/* 最大許容スプライト表示数 */

#define SHIFT_NUM_Y	0
#define SHIFT_NUM	0		/* 座標系をシフトする回数(固定小数点演算用) */

enum {
	SPR_OFS_X = -16, //16,
	SPR_OFS_Y = -16, //-16,
};

typedef struct chr_para3{
	unsigned char y, x;
//	short x;
	unsigned char pat_num,atr;
//	unsigned char pal;
} CHR_PARA3;

typedef struct chr_para4{
	unsigned char pat_num,atr;
//	unsigned char pal;
} CHR_PARA4;

extern CHR_PARA3 spr_chr[MAX_SPRITE * 2];
extern CHR_PARA3 *psprchr;

//extern CHR_PARA3 chr_data[MAX_SPRITE * 2];
#define chr_data spr_chr
//EXTERN CHR_PARA3 chr_data2[MAX_SPRITE];
//extern CHR_PARA4 old_data[2][MAX_SPRITE];
extern CHR_PARA3 *pchr_data;

// V9968 SPMode3
/*#define DEF_SP_SINGLE(NO, X, Y, PAT, PAL, VS) {\
	pchr_data = &chr_data[tmp_spr_count++];\
	tx = (((X) >> SHIFT_NUM) + SPR_OFS_X); \
	pchr_data->xl = tx & 0xff; \
	pchr_data->xh = (tx >> 8) & 0x03; \
	ty = (((Y) >> SHIFT_NUM_Y) + SPR_OFS_Y - 1); \
	pchr_data->yl = ty & 0xff; \
	pchr_data->yh = (ty >> 8) & 0x03; \
	pchr_data->pat_num = (PAT); \
	pchr_data->mgx = 16; \
	pchr_data->mgy = 16; \
	pchr_data->ps = PAL; \
}*/
//	NO++;\
//}

inline void msx_set_sprite(unsigned char spr_count, unsigned char x, unsigned char y, unsigned char no, unsigned char color);

//#define sprite_set(NO, X, Y, PAT, PAL, VS) DEF_SP_SINGLE(NO, X, Y, PAT, 0, VS)

void init_star(void);
void bg_roll(void);
unsigned char get_key(unsigned char matrix) __sdcccall(1);
void se(void);

char checkbgm(void) __sdcccall(1);
void play_bgm(unsigned char mode) __sdcccall(1);
void playbgm(void) __sdcccall(1);
void stopbgm(void) __sdcccall(1);
/*
unsigned char play_fmdbgm(void) __sdcccall(1);
void stop_fmdbgm(void) __sdcccall(1);
*/

void wait_vsync(void);
void msx_print(unsigned char x, unsigned char y,  char *str);
//void put_numd(long j, unsigned char digit) __sdcccall(1);

//void set_sprite_all(void);
void set_sprite_all(unsigned char start, unsigned char end);
void cls(void);
unsigned char keyscan(void);

void key_flush(void);

void msx_sound(unsigned char no, unsigned char dummy) __sdcccall(1);
void msx_print_num(unsigned char x, unsigned char y, unsigned long number, unsigned char digits) __sdcccall(1);

void msx_cls(void);

#define msx_wait_vsync wait_vsync

//void msx_print_num_l(unsigned char x, unsigned char y, unsigned long number, unsigned char digits) __sdcccall(1);

#define msx_print_num_l msx_print_num

void reset_int(void);
void set_int(void);

void draw_bg(void);

extern unsigned char seflag;
extern unsigned char playmode;
extern unsigned char str_temp[11];
extern unsigned char total_count;
extern unsigned char spr_count;
extern volatile unsigned char tmp_spr_count;

extern short tx,ty;

extern unsigned char keycode;

extern unsigned char vdps0;
