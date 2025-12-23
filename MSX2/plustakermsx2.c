/* plustakermsx2.c By m@3 3 with Grok 2025. */

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

#include <video/tms99x8.h>

#include <msx.h>
#include <msx\gfx.h>

#include "plusspr.h"

unsigned char *jiffy = (unsigned char *)0xfc9e, old_jiffy;

//#define SINGLEMODE

#define DI() {\
__asm\
	di\
__endasm;\
}

#define EI() {\
__asm\
	ei\
__endasm;\
}

void set_int(void);
void reset_int(void);

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

#define SHIFT_NUM_Y	0
#define SHIFT_NUM	0		/* 座標系をシフトする回数(固定小数点演算用) */

enum {
	SPR_OFS_X = 0, //-16, //16,
	SPR_OFS_Y = 0 //-16, //-16,
};

#define MAX_SPRITE 32			/* 最大許容スプライト表示数 */

typedef struct chr_para3{
	unsigned char y, x;
	unsigned char pat_num,atr;
} CHR_PARA3;

typedef struct chr_para4{
	unsigned char pat_num,atr;
} CHR_PARA4;

CHR_PARA3 *pchr_data;
CHR_PARA3 chr_data[MAX_SPRITE * 2];
CHR_PARA4 old_data[2][MAX_SPRITE];

volatile unsigned char tmp_spr_count = 0;
unsigned char old_count[2];
unsigned char total_count = 0;

#ifdef SINGLEMODE
unsigned char spr_page = 0;
#else
unsigned char spr_page = 1;
#endif
unsigned char spr_flag = 0, spr_next = 0;

unsigned char *vdp_value = 0xf3df;
unsigned char *oldscr = 0xfcb0;

int i, j;

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 212

// max/min マクロ
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

// 構造体
typedef struct {
	int x, y;
	int active;
} Entity;


#define HMMM 0xD0
#define LMMM 0x90

enum {
	VDP_READDATA = 0,
	VDP_READSTATUS = 1
};

enum {
	VDP_WRITEDATA = 0,
	VDP_WRITECONTROL = 1,
	VDP_WRITEPAL = 2,
	VDP_WRITEINDEX = 3
};

#define VDP_readport(no) (VDP_readadr + no)
#define VDP_writeport(no) (VDP_writeadr + no)

unsigned char VDP_readadr;
unsigned char VDP_writeadr;

#define MAXCOLOR 16

/* R G B */
unsigned char org_pal[MAXCOLOR][3] = {
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


/* mainromの指定番地の値を得る */
unsigned char  read_mainrom(unsigned short adr) __sdcccall(1)
{
__asm
	ld	a,(#0xfcc1)	; exptbl
	call	#0x000c	; RDSLT
__endasm;
}

/* screenのBIOS切り替え */
void set_screenmode(unsigned char mode) __sdcccall(1)
{
__asm
;	ld	 hl, 2
;	add	hl, sp

	push	ix
	ld	b,a

	ld	a,(#0xfcc1)	; exptbl
	ld	d,a
	ld	e,0
	push	de
	pop	iy
	ld ix,#0x005f	; CHGMOD(MAINROM)

;	ld	a, (hl)	; a = mode
	ld	a,b

	call	#0x001c	; CALSLT
	pop	ix
__endasm;
}

void set_screencolor(void)
{
__asm
	push	ix
	ld	a,(#0xfcc1)	; exptbl
	ld	d,a
	ld	e,0
	push	de
	pop	iy
	ld ix,#0x0062	; CHGCLR(MAINROM)

	call	#0x001c	; CALSLT
	pop	ix
__endasm;
}

void write_VDP(unsigned char regno, unsigned char data) __sdcccall(1)
{
//	outp(VDP_writeport(VDP_WRITECONTROL), data);
//	outp(VDP_writeport(VDP_WRITECONTROL), 0x80 | regno);
__asm
	ld	h,a
	ld	a,(_VDP_writeadr)
	inc	a
	ld	c,a
	ld	a,l
	out	(c),a
	ld	a,h
	set 7,a
	out	(c),a
__endasm;
}

void write_vram_adr(unsigned char highadr, int lowadr) __sdcccall(1)
{
__asm
	push	de
__endasm;
	write_VDP(14, (((highadr  << 2) & 0x04) | (lowadr >> 14) & 0x03));
__asm
	pop	de
	ld	a,(_VDP_writeadr)
	inc	a
	ld	c,a
	out	(c),e
	ld	a,d
	and	a,0x3f
	set	6,a
	out	(c),a
__endasm;
//	outp(VDP_writeport(VDP_WRITECONTROL), (lowadr & 0xff));
//	outp(VDP_writeport(VDP_WRITECONTROL), 0x40 | ((lowadr >> 8) & 0x3f));
}

void write_vram_data(unsigned char data) __sdcccall(1)
{
__asm
//	outp(VDP_writeport(VDP_WRITEDATA), data);
	ld	b,a
	ld	a,(_VDP_writeadr)
	ld	c,a
	out	(c),b
__endasm;
}

void read_vram_adr(unsigned char highadr, int lowadr) __sdcccall(1)
{
__asm
	push	de
__endasm;
	write_VDP(14, (((highadr  << 2) & 0x04) | (lowadr >> 14) & 0x03));
__asm
	pop	de
	ld	a,(_VDP_writeadr)
	inc	a
	ld	c,a
	out	(c),e
	ld	a,d
	and	a,0x3f
	out	(c),a
__endasm;
//	outp(VDP_writeport(VDP_WRITECONTROL), (lowadr & 0xff));
//	outp(VDP_writeport(VDP_WRITECONTROL), 0x00 | ((lowadr >> 8) & 0x3f));
}

unsigned char read_vram_data(void) __sdcccall(1)
{
__asm
	ld	a,(_VDP_readadr)
	ld	c,a
	in	a,(c)
__endasm;
//	return inp(VDP_readport(VDP_READDATA));
}


//#define read_vram_data() inp(VDP_readport(VDP_READDATA))


void set_displaypage(int page) __sdcccall(1)
{
	DI();
	write_VDP(2, (page << 5) & 0x60 | 0x1f);
	EI();
}

unsigned char read_VDPstatus(unsigned char no) __sdcccall(1)
{
	unsigned char data;
	DI();
	write_VDP(15, no);
//	data = inp(VDP_readport(VDP_READSTATUS));
__asm
	ld	a,(_VDP_readadr)
	inc	a
	ld	c,a
	in a,(c)
	push	af
__endasm;
	write_VDP(15, 0);
__asm
	pop	af
	ei
__endasm;
//	return data;
}

unsigned char port,port2;

void wait_VDP(void) {
//	unsigned char data;
	port = VDP_writeport(VDP_WRITECONTROL);
	port2 = VDP_readport(VDP_READSTATUS);

/*	do{
__asm
	EI
__endasm;
__asm
	DI
__endasm;
		outp(port, 2);
		outp(port, 0x80 + 15);

		data = inp(port2);

		outp(port, 0);
		outp(port, 0x80 + 15);
	}while((data & 0x01));
*/
__asm
waitloop:
	ei
	nop
	di
	ld	a,(_port)
	ld	c,a
	ld	a,2
	out	(c),a
	ld	a,#0x80 + 15
	out	(c),a
	ld	b,c

	ld	a,(_port2)
	ld	c,a
	in a,(c)
	ld	c,b
	ld	b,a

	xor	a,a
	out	(c),a
	ld	a,#0x80 + 15
	out	(c),a

	ld	a,b
	and	a,#0x01
	jr	nz,waitloop
__endasm;
}

void boxfill(int dx, int dy, int nx, int ny, unsigned char dix, unsigned char diy, unsigned char data)
{
	unsigned char port = VDP_writeport(VDP_WRITEINDEX);
	unsigned char port2 = VDP_writeport(VDP_WRITECONTROL);

	wait_VDP();

//	write_vdp(17, 36);
	outp(port2, 36);
	outp(port2, 0x80 | 17);

	outp(port, dx & 0xff);
	outp(port, (dx >> 8) & 0x01);
	outp(port, dy & 0xff);
	outp(port, (dy >> 8) & 0x03);
	outp(port, nx & 0xff);
	outp(port, (nx >> 8) & 0x01);
	outp(port, ny & 0xff);
	outp(port, (ny >> 8) & 0x03);
	outp(port, data);
	outp(port, ((diy << 3) & 0x80) | ((diy << 2) & 0x40));
	outp(port, 0xc0);

	wait_VDP();

	EI();
}

unsigned char port3, port4;

unsigned char sx, sy, dx, dy; //, nc, ny, dix, diy, 
unsigned char VDPcommand;
unsigned char APAGE,VPAGE,XSIZE,XSIZA,YSIZE;

void VDPsetAREA2(void)
/*unsigned short sx, unsigned short sy, unsigned short dx, unsigned short dy, unsigned short nx, unsigned short ny, unsigned char dix, unsigned char diy, unsigned char command)*/
{
	port3 = VDP_writeport(VDP_WRITEINDEX);
	port4 = VDP_writeport(VDP_WRITECONTROL);

__asm
	ld	a,(_sx)	;SX
	ld	h,a
	ld	a,(_sy)	;SY
	ld	l,a
	ld	a,(_dx)	;DX
	ld	d,a
	ld	a,(_dy)	;DY
	ld	e,a

__endasm;
	wait_VDP();
__asm
	ld	a,(_port4)
	ld	c,a
	ld	a,32
	out	(c),a
	ld	a,#0x80 | 17
	out	(c),a

	ld	b,0x0f
	ld	a,(_port3)
	ld	c,a

	XOR	A
	OUT	(C),H	;SX
	OUT	(C),A	
	LD	A,(_APAGE)
	OUT	(C),L	;SY
	OUT	(C),A	

	XOR	A
	OUT	(C),D	;DX
	OUT	(C),A	
	LD	A,(_VPAGE)
	OUT	(C),E	;DY
	OUT	(C),A
	LD	A,(_XSIZE)
	LD	B,A
	LD	A,(_XSIZA)
	OUT	(C),B
	OUT	(C),A
	LD	A,(_YSIZE)
	LD	B,A
	XOR	A
	OUT	(C),B
	OUT	(C),A
	OUT	(C),A	;DUMMY

	LD	A,H
	SUB	D
	LD	A,0
	JR	C,DQ
DQ:	OR	2

	OUT	(C),A	;DIX and DIY

	ld	a,(_VDPcommand)
	out	(C),a	/* com */
	ei
__endasm;
}

void spr_on(void)
{
	DI();
	write_VDP(8, 0x08);
	EI();
}

void spr_off(void)
{
	DI();
	write_VDP(8, 0x0a);
	EI();
}

void set_spr_atr_adr(unsigned char highadr) __sdcccall(1) //, int lowadr)
{
	write_VDP(11, ((highadr << 1) & 0x02));
}


char chr;

void put_strings(unsigned char x, unsigned char y,  char *str, unsigned char pal)
{
	XSIZE = 8;
	XSIZA = 0;
	YSIZE = 8;
	APAGE = 3; //map_page;
	VPAGE = 0;
	VDPcommand = HMMM;

	while(1){
		chr = *(str++);
		if(chr == '\0')
			break;
		if((chr < 0x30))
			chr = 0x40;
		chr -= '0';
		sx = (chr & 0x0f) * 8;
		sy = (chr / 16) * 8;
		dx = x * 8;
		dy = y * 8;
		VDPsetAREA2();
		++x;
	}
}

char str_temp[11];

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
#define CHRPAL_NO 0 //COLOR_WHITE

void score_display(void)
{
	put_numd(score, 8);
	put_strings(15, 21 , str_temp, CHRPAL_NO);
	if(score >= hiscore){
		if((score % 10) == 0){
			hiscore = score;
			put_strings(8, 21, "HIGH ", CHRPAL_NO);
		}
	}else
		put_strings(8, 21, "SCORE", CHRPAL_NO);

}

int combo = 0;

void combo_display(void)
{
	put_numd(combo, 8);
	put_strings(15, 23 , str_temp, CHRPAL_NO);
		put_strings(8, 23, "COMBO", CHRPAL_NO);

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

	put_strings(8, 23, "HIGH ", CHRPAL_NO);
	put_strings(15, 23, str_temp, CHRPAL_NO);
}

void hiscore_display_clear(void)
{
	put_strings(8, 23, "     ", CHRPAL_NO);
	put_strings(15, 23, "        ", CHRPAL_NO);
}


#ifdef DEBUG3
unsigned char jiffy_flag = 0;
unsigned char old_jiffy2 = 0;
#endif

void wait_vsync(void)
{
	while(*jiffy == old_jiffy);
	old_jiffy = *jiffy;

//	while((read_VDPstatus(2) & 0x40));
//	while(!(read_VDPstatus(2) & 0x40)); /* WAIT VSYNC */

	++total_count;
#ifdef DEBUG3
	if((unsigned char)(*jiffy - old_jiffy2) >= 60){
		old_jiffy2 = *jiffy;
		put_numd((long)(total_count), 2);
		put_strings(SCREEN2, 27, 22, str_temp, CHRPAL_NO);
		total_count = 0;
	}
#endif
}

void init_sys_wait(void)
{
__asm
	ld	a,(#0xfc9e)
	ld	(_old_jiffy),a
__endasm;
}

void sys_wait(unsigned char wait) __sdcccall(1)
{
__asm
	ld	l,a
	ld	a,(_old_jiffy)
;	add	a,l
	ld	b,a

jiffyloop2:
	ei
	nop
	di
	ld	a,(#0xfc9e)
	sub	a,b
;	cp	b
	cp	l
	jr	c,jiffyloop2	; a<b
__endasm;

	++total_count;
#ifdef DEBUG
	if((unsigned char)(*jiffy - old_jiffy2) >= 60){
//	if(*jiffy >= 60){
		old_jiffy2 = *jiffy;
		EI();
		put_numd((long)(total_count), 2);
		put_strings(SCREEN2, 27, 22, str_temp, CHRPAL_NO);
		total_count = 0;
//		*jiffy = 0;
	}
	EI();
#endif
}

void clr_sp(void) //unsigned char num) __sdcccall(1)
{
__asm
	ld	a,(_VDP_writeadr)
	ld	c,a
clrloop:
	ld	a,216 ;0xd4
	out	(c),a
__endasm;
}

/* スプライトを全て画面外に移す */
void spr_clear(void){
	DI();
	write_vram_adr(spr_page, 0x7600);
//	for(i = 0; i < MAX_SPRITE; i++){
//		PUT_SP(0, 212, 255, 0);
		clr_sp(); //MAX_SPRITE);
//	}
	EI();
	DI();
	wait_vsync();
//	spr_flag = 0;
	set_spr_atr_adr(spr_page); //, SPR_ATR_ADR); /* color table : atr-512 (0x7400) */
//	spr_next = spr_page;
#ifndef SINGLEMODE
	spr_page ^= 0x01;
#endif
	EI();
}


char sprite_pattern_no[MAX_SPRITE]; //, sprite_color_no[MAX_SPRITE][2];
int spr_x[MAX_SPRITE], spr_y[MAX_SPRITE];
//int spr_x_new[8], spr_y_new[8];

void set_sprite(char num, int posx, int posy)
{
	spr_x[num] = posx - 16;
	spr_y[num] = posy - 16 + 1;
}


char patternno_table[4] = {0, 13*4, 12*4, 0};
//char patterncolor_table[6] = {15, 10,  7,  7,  10, 10};

void set_sprite_pattern(int num, int no) {
	sprite_pattern_no[num] = patternno_table[no];
//	sprite_color_no[num][0] = patterncolor_table[no * 2];
//	sprite_color_no[num][1] = patterncolor_table[no * 2 + 1];
}

short xxx;
unsigned char yyy, sprpal_no; // spratr, patno,

short X, Y;
unsigned char PAT, PAL;

void DEF_SP_SINGLE(void) { //short X, short Y, unsigned char PAT, unsigned char PAL) __sdcccall(1) {
//	patno = PAT;
//	pchr_data = &chr_data[tmp_spr_count];
	xxx = ((X >> SHIFT_NUM) + SPR_OFS_X);
	yyy = ((Y >> SHIFT_NUM_Y) + SPR_OFS_Y - 1);

//	spratr = PAL;
/*	if((xxx >= 256) || (xxx <= -16)){
		yyy = 212;
	}
	else if(xxx >= 0){
		spratr = PAL;
	}else{
		xxx += 32;
		spratr = 0x80 | PAL;
	}*/
__asm
	ld	a,(_PAL)
;	ld	(_spratr),a
	ld	b,a

	ld	a,(_yyy)
	ld	c,a

	ld	a,(_xxx+0)
	ld	d,a

	ld	a,(_xxx+1)
	or	a
	jr	z,spdefend2

	cp	#0xff
	jr	nz,spdef02
	ld	a,d ;(_xxx+0)
	cp	#0xf0
	jr	nc,spdef12
spdef02:
:	ld	a,212
;	ld	(_yyy),a
	ld	c,212
	jr	spdefend2
spdef12:
;	ld	a,32
;	ld	c,32
	ld	a,d ;(_xxx+0)
	add	a,32
	ld	d,a ;(_xxx+0),a
	ld	a,b ;(_spratr)
	or	a,#0x80
	ld	b,a	;(_spratr),a
spdefend2:

	ld	hl,(_pchr_data)
;	ld	a,c ;(_yyy)
	ld	(hl),c
	inc	hl
;	ld	a,d ;(_xxx)
	ld	(hl),d
;	inc	hl		; (*)
;	inc	de
;	ld	(hl),d
	inc	hl
;	ld	a,(_patno)
	ld	a,(_PAT)
	ld	(hl),a
	inc	hl
;	ld	a,b	;(_spratr)
	ld	(hl),b
	ld	hl,_tmp_spr_count
	inc	(hl)
__endasm;
/*
__asm
	ld	hl,_tmp_spr_count
	ld	a,(hl)
	cp	32
	jr	nc,defspend
	inc	(hl)

	ld	a,(_VDP_writeadr)
	ld	c,a

	ld	a,(_yyy)
	out	(c),a
	ld	a,(_xxx)
	out	(c),a
	ld	a,(_patno)
	out	(c),a
	out	(c),a
	ld	hl,(_pchr_data)
	ld	(hl),a
	inc	hl
	ld	a,(_spratr)
	ld	(hl),a
defspend:
__endasm;*/

	++pchr_data;
}

void DEF_SP_DOUBLE(void) { //short X, short Y, unsigned char PAT, unsigned char PAL) __sdcccall(1) {
//	patno = PAT * 8;
//	pchr_data = &chr_data[tmp_spr_count];
	xxx = ((X >> SHIFT_NUM) + SPR_OFS_X);
	yyy = ((Y >> SHIFT_NUM_Y) + SPR_OFS_Y - 1);

//	spratr = PAL;
/*	if((xxx >= 256) || (xxx <= -16)){
		yyy = 212;
	}
	else if(xxx >= 0){
		spratr = PAL;
	}else{
		xxx += 32;
		spratr = 0x80 | PAL;
	}*/
__asm
	ld	a,(_PAL)
;	ld	(_spratr),a
	ld	b,a

	ld	a,(_yyy)
	ld	c,a

	ld	a,(_xxx+0)
	ld	d,a

	ld	a,(_xxx+1)
	or	a
	jr	z,spdefend

	cp	#0xff
	jr	nz,spdef0
	ld	a,d
	cp	#0xf0
	jr	nc,spdef1
spdef0:
	ld	a,212
	ld	c,a ;(_yyy),a
	jr	spdefend
spdef1:
;	ld	a,32
;	ld	c,32
	ld	a,d ;(_xxx+0)
	add	a,32
	ld	(_xxx+0),a
	ld	a,b	;(_spratr)
	or	a,#0x80
;	ld	(_spratr),a
	ld	b,a
spdefend:
	ld	hl,4
	ld	de,(_pchr_data)
	add	hl,de
;	ld	a,c ;(_yyy)
	ld	(de),c
	ld	(hl),c
	inc	hl
	inc	de
	ld	a,(_xxx)
	ld	(de),a
	ld	(hl),a
;	inc	hl		; (*)
;	inc	de		; (*)
	inc	hl
	inc	de
;	ld	a,(_patno)
	ld	a,(_PAT)
	add	a,a
	add	a,a
	add	a,a
	ld	(de),a
;	ld	c,4
	add	a,4
	ld	(hl),a
	inc	hl
	inc	de
;	ld	a,b	;(_spratr)
	ld	(de),b
	ld	(hl),b

	ld	hl,_tmp_spr_count
	inc	(hl)
	inc	(hl)
__endasm;
/*
__asm
	ld	hl,_tmp_spr_count
	ld	a,(hl)
	cp	31
	jr	nc,defspend2
	ld	b,2
	add	a,b
	ld	(hl),a

;	ld	hl,_chr_data
;	ld	a,(_tmp_spr_count)
;	add	a,a
;	ld	e,a
;	ld	d,0
;	add	hl,de
;	ld	(_pchr_data),hl

	ld	a,(_VDP_writeadr)
	ld	c,a

	ld	a,(_yyy)
	out	(c),a
	ld	a,(_xxx)
	out	(c),a
	ld	a,(_patno)
	out	(c),a
	out	(c),a

	ld	hl,(_pchr_data)
	ld	(hl),a
	inc	hl
	ld	a,(_spratr)
	ld	(hl),a

	ld	a,(_yyy)
	out	(c),a
	ld	a,(_xxx)
	out	(c),a
	ld	a,(_patno)
	ld	b,4
	add	a,b
	out	(c),a
	out	(c),a
	ld	hl,(_pchr_data)
	ld	de,2
	add	hl,de
	ld	(hl),a
	inc	hl
	ld	a,(_spratr)
	ld	(hl),a
defspend2:
__endasm;*/

	pchr_data += 2;
}


unsigned char pat_num, atr, atr2, *patr;

inline void set_spr(void)
{
__asm
;	push	af
;	push	bc
;	push	de
;	push	hl

;	ld	hl, (_pchr_data)
	ld	hl,_chr_data
;	ld	de,2

	ld	a,(_VDP_writeadr)
	ld	c,a
	ld	a,(_tmp_spr_count)
	or	a
	jr	z,sprend
;	ld	b,a
	ld	d,a
	xor	a,a
sprloop2:
;	ld	a,(hl)
;	out	(c),a
;	inc	hl
	outi
;	ld	a,(hl)
;	out	(c),a
;	inc	hl
	outi
;	inc	hl		;(*)
;	ld	a,(hl)
;	out	(c),a
	outi
;	xor	a,a
;	out	(c),a
	outi
;	inc	hl
;	add	hl,de
	dec	d
	jr	nz,sprloop2
;	djnz	sprloop2
sprend:
;	pop	hl
;	pop	de
;	pop	bc
;	pop	af
__endasm;
}

CHR_PARA4 *pold_data;

void set_sprite_all(void)
{
	unsigned char i, j;

//	spr_page ^= 0x01;

//	tmp_spr_count = spr_count[spr_page];

/* スプライト表示 */
//	spr_count = 2;

/* 表示数ぶん書き込む */
	if(tmp_spr_count > MAX_SPRITE){
/*		if(total_count & 1){
			for(i = tmp_spr_count - MAX_SPRITE, j = 0; j < MAX_SPRITE; i++, j++){
				chr_data2[j] = chr_data[i][spr_page];
			}
			for(i = 0; i < MAX_SPRITE; i++){
				chr_data[i][spr_page] = chr_data2[i];
			}
		}*/
		tmp_spr_count = MAX_SPRITE;
	}
/*	if(tmp_spr_count < MAX_SPRITE){
		clr_sp();
	}*/

/*	for(i = 0; i < tmp_spr_count; i++){
		CHR_PARA4 *pold_data = &old_data[spr_page][i];
		pchr_data = &chr_data[i];
		if((pold_data->pat_num != pchr_data->pat_num) || (pold_data->atr != pchr_data->atr) || (pold_data->pal != pchr_data->pal)){
			color_flag[i] = 1;
			pold_data->pat_num = pchr_data->pat_num;
			pold_data->atr = pchr_data->atr;
			pold_data->pal = pchr_data->pal;
		}
	}
*/

//	goto spr_end;

	/* 色情報の処理 */
//	wait_vsync();
//	DI();

//	write_vram_adr(spr_page, 0x7600);

	pold_data = &old_data[spr_page][0];
	pchr_data = &chr_data[0];

	for(i = 0; i < tmp_spr_count; i++){
/*__asm
	DI
__endasm;
*/
		if((pold_data->pat_num != pchr_data->pat_num) || (pold_data->atr != pchr_data->atr)){
//		if((pold_data->pat_num == pchr_data->pat_num))
//			if((pold_data->atr == pchr_data->atr))
//				if((pold_data->pal == pchr_data->pal))
//					continue;
			pat_num = pold_data->pat_num = pchr_data->pat_num;
			atr = pold_data->atr = pchr_data->atr;
//			pold_data->pal = pchr_data->pal;
//		if(color_flag[i]){
//			pchr_data = &chr_data[i];
//			color_flag[i] = 0;
//			pat_num = pchr_data->pat_num / 4;
//			atr = pchr_data->atr & 0xf0;

			if((atr & 0x0f)){
//				atr2 = 13 | atr;
//				for(j = 0; j < 16; ++j){
//					write_vram_data(13 | atr);
				DI();
				write_vram_adr(spr_page, 0x7600 - 512 + i * 16);
//				EI();
__asm
	push	bc
	ld	b,16
	ld	a,(_VDP_writeadr)
	ld	c,a
	ld	a,(_atr)
	or	13
c_loop:
	out	(c),a
	djnz	c_loop
	pop	bc
__endasm;
///				}
				EI();
			}else{
//				for(j = 0; j < 16; ++j){
//					write_vram_data(spr_col[pat_num][j] | atr);
//					atr2 = spr_col[pat_num][j]; // | atr;
					patr = (unsigned char *)&spr_col[pat_num/4][0]; // | atr;
					DI();
					write_vram_adr(spr_page, 0x7600 - 512 + i * 16);
//					EI();
__asm
	push	bc
	push	de
	push	hl
;	ld	b,16
	ld	a,(_VDP_writeadr)
	ld	c,a
	ld	hl,(_patr)
	ld	a,(_atr)
	and	a,0xf0
	ld	d,a
palloop:
;	ld	a,(_atr2)
	ld	a,(hl)
;	ld	b,a
	or	d
	out	(c),a	;1
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;2
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;3
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;4
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;5
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;6
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;7
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;8
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;9
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;10
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;11
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;12
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;13
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;14
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;15
	inc	hl

	ld	a,(hl)
	or	d
	out	(c),a	;16
;	inc	hl

;	djnz	palloop
	pop	hl
	pop	de
	pop	bc
__endasm;
					EI();
//				}
			}
//			write_vram_adr(spr_page, 0x7600  + (i) * 4);
		}
//		PUT_SP(pchr_data->x, pchr_data->y, pchr_data->pat_num, 0);

		++pold_data;
		++pchr_data;

	}

	DI();
	write_vram_adr(spr_page, 0x7600);
	set_spr();

	if(tmp_spr_count < MAX_SPRITE){
		clr_sp();
	}
	EI();

spr_end:
/*
	for(i = 0; i < tmp_spr_count; ++i){
		put_numd(chr_data[i].x, 3);
		put_strings(0, i , str_temp, CHRPAL_NO);
		put_numd(chr_data[i].y, 3);
		put_strings(4, i , str_temp, CHRPAL_NO);
		put_numd(chr_data[i].pat_num, 3);
		put_strings(8, i , str_temp, CHRPAL_NO);
	}
*/
	old_count[spr_page] = tmp_spr_count;
	tmp_spr_count = 0;

//	wait_vsync();

	DI();
//	if((read_VDPstatus(2) & 0x40)){ /* WAIT VSYNC */
//		set_spr_atr_adr(spr_page); 
//		spr_flag = 0;
//	}else 
//	if(spr_flag == 2){		/* END */
		spr_flag = 1;
		spr_next = spr_page;
//	}else if(spr_flag == 0){	/* IDLE */
//		spr_flag = 1;
//		spr_next = spr_page;
#ifndef SINGLEMODE
		spr_page ^= 0x01;
#endif
//	}
	EI();

	sys_wait(1);
	init_sys_wait();
//	EI();
}

void set_se(void)
{
	DI();
	set_psg(6,127);
	set_psg(11,0);
	set_psg(12,15);
	set_psg(7,0x9c);  // 10011100
	set_psg(13,9);
	set_psg(10,0x10);
	EI();
}


void key_flush(void)
{
__asm
	push	ix
	ld	a,(#0xfcc1)	; exptbl
	ld	d,a
	ld	e,0
	push	de
	pop	iy
	ld ix,#0x0156	; KILBUF(MAINROM)

	call	#0x001c	; CALSLT
	pop	ix
__endasm;
}

unsigned char get_key(unsigned char matrix) __sdcccall(1)
{
	outp(0xaa, ((inp(0xaa) & 0xf0) | matrix));
	return inp(0xa9);
/*
__asm
	ld	a,(#0xfcc1)	; exptbl
	ld	d,a
	ld	e,#0
	push	de
	pop	iy
	ld ix,#0x0141	; SNSMAT(MAINROM)

	ld	 hl, #2
	add	hl, sp
	ld	a, (hl)	; a = mode

	call	#0x001c	; CALSLT

	ld	l,a
	ld	h,#0
__endasm;
*/
}

unsigned char get_stick1(unsigned char trigno) __sdcccall(1)
{
__asm
;	ld	 hl, #2
;	add	hl, sp
	ld	l,a

	push	ix

	ld	a,(#0xfcc1)	; exptbl
	ld	d,a
	ld	e,#0
	push	de
	pop	iy
	ld ix,#0x00d5	; GTSTCK(MAINROM)

;	ld	a, (hl)	; a = mode
	ld	a,l

	call	#0x001c	; CALSLT
;	ld	l,a
;	ld	h,#0

	pop	ix
__endasm;
}


unsigned char get_trigger1(unsigned char trigno) __sdcccall(1)
{
__asm
;	ld	 hl, #2
;	add	hl, sp
	ld	l,a

	push	ix

	ld	a,(#0xfcc1)	; exptbl
	ld	d,a
	ld	e,#0
	push	de
	pop	iy
	ld ix,#0x00d8	; GTTRIG(MAINROM)

;	ld	a, (hl)	; a = mode
	ld	a,l

	call	#0x001c	; CALSLT
;	ld	l,a
;	ld	h,#0

	pop	ix
__endasm;
}

#include "inkey.h"

unsigned char st0, st1, pd0, pd1, pd2, k3, k5, k7, k9, k10;
unsigned char keycode = 0;

void keyscan(void)
{
	DI();
	keycode = 0;

	k3 = get_key(3);

	k9 = get_key(9);
	k10 = get_key(10);
	k5 = get_key(5);

	st0 = get_stick1(0);
	st1 = get_stick1(1);

	pd0 = get_trigger1(0);
	pd1 = get_trigger1(1);
	pd2 = get_trigger1(3);

	EI();

	if((pd0) || (pd1) || !(k5 & 0x20)) /* X,SPACE */
		keycode |= KEY_A;
	if((pd2) || !(k3 & 0x01)) /* C */
		keycode |= KEY_B;
	if((st0 >= 1 && st0 <=2) || (st0 == 8) || (st1 >= 1 && st1 <=2) || (st1 ==8) || !(k10 & 0x08)) /* 8 */
		keycode |= KEY_UP1;
	if((st0 >= 4 && st0 <=6) || (st1 >= 4 && st1 <=6) || !(k9 & 0x20)) /* 2 */
		keycode |= KEY_DOWN1;

//	if(!(st & 0x0c)){ /* RL */
//		keycode |= KEY_START;
//	}else{
	if((st0 >= 6 && st0 <=8) || (st1 >= 6 && st1 <=8) || !(k9 & 0x80)) /* 4 */
		keycode |= KEY_LEFT1;
	if((st0 >= 2 && st0 <=4) || (st1 >= 2 && st1 <=4) || !(k10 & 0x02)) /* 6 */
		keycode |= KEY_RIGHT1;
//	}

	return; // keycode;
}

void put_sprite(void)
{
//	int i;
	tmp_spr_count = 0;
	for(i = 0; i < 8; ++i){
		X = spr_x[i];
		Y = spr_y[i];
		PAT = sprite_pattern_no[i];
		PAL = CHRPAL_NO;
		pchr_data = &chr_data[tmp_spr_count];
		if(PAT == 0)
			DEF_SP_DOUBLE(); //tmp_spr_count, spr_x[i], spr_y[i], sprite_pattern_no[i], CHRPAL_NO, 0);
		else
			DEF_SP_SINGLE();
/*		put_numd(X, 3);
		put_strings(0, i , str_temp, CHRPAL_NO);
		put_numd(Y, 3);
		put_strings(4, i , str_temp, CHRPAL_NO);
		put_numd(PAT, 3);
		put_strings(8, i , str_temp, CHRPAL_NO);
*/	}
/*	for(i = 0; i < tmp_spr_count; ++i){
		put_numd(chr_data[i].x, 3);
		put_strings(0, i , str_temp, CHRPAL_NO);
		put_numd(chr_data[i].y, 3);
		put_strings(4, i , str_temp, CHRPAL_NO);
		put_numd(chr_data[i].pat_num, 3);
		put_strings(8, i , str_temp, CHRPAL_NO);
	}*/
}

/*
		Entity player = {160 - 8, 160 - 16};
		Entity bullets[3] = {{0}};
		Entity enemies[4] = {{0}};
		Entity pluses[8] = {{0}};  // Plus最大8個
*/
//		score = 0;
//		combo = 0;
int spawn_timer = 0;
int combo_timer = 0;
int wave = 1;
int enemies_killed_this_wave = 0;
int game_over = 0;
int score_display_flag = 0;
int combo_display_flag = 0;
int enemy_speed = 2;

int count = 0;

Entity player = {160 - 8, 160 - 16};
Entity bullets[3] = {{0}};
Entity enemies[4] = {{0}};
Entity pluses[8] = {{0}};  // Plus最大8個

int e,b,p;

unsigned char *forclr = 0xf3e9;
unsigned char *bakclr = 0xf3ea;
unsigned char *bdrclr = 0xf3eb;
unsigned char *clicksw = 0xf3db;
unsigned char forclr_old, bakclr_old, bdrclr_old, clicksw_old;

int main(void)
{
//	int i, j;
	VDP_readadr = read_mainrom(0x0006);
	VDP_writeadr = read_mainrom(0x0007);

	forclr_old = *forclr;
	bakclr_old = *bakclr;
	bdrclr_old = *bdrclr;

	*forclr = 15;
	*bakclr = 0;
	*bdrclr = 0;
	set_screencolor();

	clicksw_old = *clicksw;
	*clicksw = 0;

	set_screenmode(5);
	set_displaypage(0);
	DI();
	write_VDP(1, vdp_value[1] | 0x02);
	EI();
	spr_on();
	boxfill(0, 256, 256, 212, 0, 0, 0x00);

	tmp_spr_count = 0;
	old_count[0] = old_count[1] = 0; //MAX_SPRITE;
	set_int();

	for(i = 0; i < 8; ++i){
		char patno = 0;
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
		set_sprite(i,i*16,i*16);
		set_sprite_pattern(i, patno);
	}
	for(i = 0; i < MAX_SPRITE * 2; ++i){
		chr_data[i].pat_num = 255;
		chr_data[i].atr = 0x80;
	}
	for(i = 0; i < MAX_SPRITE; ++i){
//		color_flag[i] = 0;
		for(j = 0; j < 2; ++j){
			old_data[j][i].pat_num = 255;
			old_data[j][i].atr = 0;
		}
	}
	put_sprite();
//	wait_vsync();
//	set_sprite_all();
//	set_int();

	sx = 8 * 16;
	sy = 0 * 16;
	dy = 18 * 8;
	XSIZE = 16;
	XSIZA = 0;
	YSIZE = 16;
	APAGE = 2;
	VPAGE = 0;
	VDPcommand = HMMM;
	for(i = 0; i < 16; ++i){
		dx = i * 16;
		VDPsetAREA2();
	}
	wait_VDP();

	spr_clear();


	DI();
	write_VDP(23, 0);
	EI();

	// Main Loop

	for(;;){

		player.x = 160 - 8;
		player.y = 160 - 16;
		for(i = 0; i < 4; ++i)
			bullets[i].active = 0;
		for(i = 0; i < 4; ++i)
			enemies[i].active = 0;
		for(i = 0; i < 8; ++i)
			pluses[i].active = 0;  // Plus最大8個

		score = 0;
		combo = 0;
		spawn_timer = 0;
		combo_timer = 0;
		wave = 1;
		enemies_killed_this_wave = 0;
		game_over = 0;
		score_display_flag = 0;
		combo_display_flag = 0;
		enemy_speed = 2;

		count = 0;
		old_jiffy = *jiffy;

		score_displayall();
//		score_display_flag = 1;
//		set_int();

//		combo_display();
		while (!game_over) {
			++count;
			if(combo)
				++combo_timer;

//			++score;
//			score_display_flag = 1;

			keycode = 0;

			keyscan();

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
						enemies[i].x = 16 + ((rand() & ((SCREEN_WIDTH - 16))));
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

					if (abs(bullets[b].x - enemies[e].x) > 64)
						continue;
					if( abs(bullets[b].y - enemies[e].y) > 64)
						continue;

					if (abs(bullets[b].x - enemies[e].x) < 16 && abs(bullets[b].y - enemies[e].y) < 16) {
						bullets[b].active = 0;
//						enemies[e].active = 0;
						set_sprite(b+1, 0, 0);
						set_sprite(e+4, 0, 0);
						set_se();

						// Plus生成
//						for (ip = 0; p < 1; p++) {
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
			for (p = 0; p < 4; p++) {
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
			for (e = 0; e < 4; e++) {
				if (enemies[e].active && abs(enemies[e].x - player.x) < 16 && abs(enemies[e].y - player.y) < 16) {
					if(!pluses[e].active)
						game_over = 1;
				}
			}

			put_sprite();
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
			spr_flag = 0;
			EI();
			DI();
			spr_flag = 1;
			EI();
		}
//		reset_int();
		put_sprite();
		set_sprite_all();
		for(;;){
//			if(!((get_trigger(0) | get_trigger(1)) & 0x01))	/* SHOT */
			keyscan();
			if(!(keycode & KEY_A))
				break;
		}
		// ゲームオーバー画面
		/* ここにテキスト描画追加可能 */
		hiscore_display();
		for(;;){
//			if((get_trigger(0) | get_trigger(1)) & 0x01)	/* SHOT */
			keyscan();
			if((keycode & KEY_A))
				break;
		}
		hiscore_display_clear();
		// (スプライト全消し)
		for (i = 0; i < 8; i++){
			set_sprite(i, 0, 0);
		}
		put_sprite();
		set_sprite_all();
		for(;;){
//			if(!((get_trigger(0) | get_trigger(1)) & 0x01))	/* SHOT */
			keyscan();
			if(!(keycode & KEY_A))
				break;
		}
	}
end:
	reset_int();

	*forclr = forclr_old;
	*bakclr = bakclr_old;
	*bdrclr = bdrclr_old;
	set_screencolor();

	set_screenmode(*oldscr);
	*clicksw = clicksw_old;

	key_flush();

	return 0;
}

unsigned char vdps0;

unsigned char INTWORK[5];

void intvsync(void)
{
__asm
;intvsync:
	ld	(_vdps0),a
	push	af
;	push	ix
__endasm;

	if(spr_flag == 1){
		spr_flag = 0;
//		set_spr_atr_adr(spr_next); 
//		write_VDP(11, ((spr_next << 1) & 0x02));
__asm
	ld	a,(_VDP_writeadr)
	inc	a
	ld	c,a
	ld	a,(_spr_next)
	add	a,a
	and	a,#0x02
	out	(c),a
	ld	a,11 | 0x80
	out	(c),a
__endasm;
	}
__asm
;	pop	ix
	pop	af
INTWORK:
	DB	0,0,0,0,0
__endasm;
}

void set_int(void)
{
#ifndef SINGLEMODE
__asm
	DI
;	PUSH	IY
;	PUSH	HL
;	PUSH	DE
;	PUSH	BC
	LD	IY,-609
	PUSH	IY
	POP	HL
	LD	DE,INTWORK
	LD	BC,5
	LDIR
	LD	HL,_intvsync
	LD	(IY+2),L
	LD	(IY+3),H
	LD	(IY+0),0F7H
	LD	(IY+4),0C9H

	ld	a,h
	ld	hl,#0xf341	;slot0
	cp	#0x40
	jr	c,slotset
	inc	hl			;slot1
	cp	#0x80
	jr	c,slotset
	inc	hl			;slot2
	cp	#0xc0
	jr	c,slotset
	inc	hl			;slot3
slotset:
	LD	A,(HL)
	LD	(IY+1),A

;	POP	BC
;	POP	DE
;	POP	HL
;	POP	IY
	EI
__endasm;
#endif
}

void reset_int(void)
{
#ifndef SINGLEMODE
__asm
	DI
	LD	HL,INTWORK
	LD	DE,-609
	LD	BC,5
	LDIR
	EI
__endasm;
#endif
}
