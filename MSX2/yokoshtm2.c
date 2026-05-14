/* yokoshtm2.c ゲ－ム本体 for MSX2 */

#define MAIN
#define MSX2

//#define DEBUG

//#define SINGLEMODE

#include <stdio.h>
#include <stdlib.h>

#include "yokoshtm2.h"
#include "spr_col.h"

#include "mode.h"

enum {
	BGMMAX = 2,
	SEMAX = 4
};

extern void main2(void);
extern void reset(void);

extern unsigned long high_score;

void wait_vsync(void);
void msx_print(unsigned char x, unsigned char y,  char *str);
void msx_print_num(unsigned char x, unsigned char y,unsigned long j, unsigned char digit) __sdcccall(1);

#ifdef SINGLEMODE
unsigned char spr_page = 0;
#else
unsigned char spr_page = 1;
#endif

void write_vram_adr(unsigned char highadr, int lowadr) __sdcccall(1);

void set_int(void);
void reset_int(void);

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

unsigned char old_count[2];
unsigned char total_count = 0;
unsigned char str_temp[11];
volatile unsigned char tmp_spr_count = 0;

CHR_PARA3 *pchr_data;
//CHR_PARA3 chr_data[MAX_SPRITE * 2];
//CHR_PARA4
unsigned char old_data[2][MAX_SPRITE];
//CHR_PARA4 *pold_data;

unsigned char wake_count = 0;
//unsigned char muki = PAT_JIKI_C;

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


CHR_PARA3 spr_chr[MAX_SPRITE * 2];
CHR_PARA3 *pspr_chr;

inline void msx_set_sprite(unsigned char spr_count, unsigned char x, unsigned char y, unsigned char no, unsigned char color)
{
	if(spr_count < 32){
//	y-=1;
/*		spr_x[spr_count] = x;
		spr_y[spr_count] = y-1;
		spr_no[spr_count] = no * 4;
		spr_color[spr_count] = color;
*/
		spr_chr[spr_count].x = x;
		spr_chr[spr_count].y = y-1;
		spr_chr[spr_count].pat_num = no; // * 4;
		spr_chr[spr_count].atr = 0; //no/4; //color;
	}
}

volatile FILE *stream[2];

/* MSX BLOADデータをファイルからメモリに読み込む */
short msxload(char *loadfil, unsigned short offset)
{
	unsigned short size;
	unsigned char *address;
	unsigned char buffer[2];

	if ((stream[0] = fopen( loadfil, "rb")) == NULL) {
//		printf("Can\'t open file %s.", loadfil);
		return ERROR;
	}

	fread( buffer, 1, 1, stream[0]);
	fread( buffer, 1, 2, stream[0]);
	address = (unsigned short *)(buffer[0] + buffer[1] * 256);
	fread( buffer, 1, 2, stream[0]);
	size = (buffer[0] + buffer[1] * 256) - (unsigned short)address;
	fread( buffer, 1, 2, stream[0]);
	address -= offset;
	printf("Load file %s. Address %x Size %x End %x\n", loadfil, address, size, (unsigned short)address + size);

	fread( address , 1, size, stream[0]);
	fclose(stream[0]);
	return NOERROR;
}

unsigned char bgmmode = 0;

#ifdef DEBUG2

char checkbgm(void) __sdcccall(1)
{
__asm
	ld	hl,_checkbgm
	ld	a,0x80
	cp	h
	jr	c,bgmerr1

	ld	hl,#0xbc00
	ld	a,(hl)
	cp	#0xc3
	jr	nz,bgmon
	inc	hl
	ld	a,(hl)
	cp	#0x0d
	jr	nz,bgmon
	inc	hl
	ld	a,(hl)
	cp	#0xbc
	jr	nz,bgmon
	ld	a,1
	ret

bgmerr1:
	ld	hl,#0xdc00
	ld	a,(hl)
	cp	#0x2a
	jr	nz,bgmerr2
	inc	hl
	ld	a,(hl)
	cp	#0xf8
	jr	nz,bgmerr2
	inc	hl
	ld	a,(hl)
	cp	#0xf7
	jr	nz,bgmerr2
	ld	a,2
	ret
bgmerr2:
	xor	a
	ret

bgmon:
__endasm;
	if(msxload("psgtone.dat", 0x2000*1) == ERROR)
		return 0;

	if(msxload("C_6_2.PDT", 0x2000*1) == ERROR)
		return 0;

	if(msxload("PSGMSXD.MSX", 0) == ERROR)
		return 0;

	return 1;

__asm
__endasm;
	return 0;
}

void play_bgm(unsigned char mode) __sdcccall(1)
{
__asm
	ld	(#0xf7f8),a
	call	#0xdc00
__endasm;
}

void playbgm(void) __sdcccall(1)
{
//	unsigned char a;
//	a = checkbgm();
	if(!bgmmode)
		return;
	if(bgmmode == 1){
__asm
	call #0xbc00
__endasm;
	}else if(bgmmode == 2){
		play_bgm(0);
	}
}

void stopbgm(void) __sdcccall(1)
{
//	unsigned char a;
//	a = checkbgm();
	if(!bgmmode)
		return;
	if(bgmmode == 1){
__asm
	call #0xbc03
__endasm;
	}else if(bgmmode == 2){
		play_bgm(-1);
	}
}
#else
//void play_bgm(unsigned char no, unsigned char noise)
void play_bgm(unsigned char mode) __sdcccall(1)
{
}

void playbgm(void) __sdcccall(1)
{
//	unsigned char *mem=(unsigned char *)INIT;
//	mem[0] = 0
//	mem[3] = noise;


	PARA = 0;

	INIT();
}

void stopbgm(void) __sdcccall(1)
{
	STOP();
}

char checkbgm(void) __sdcccall(1)
{
	return 1;
}


#endif

void set_vol(unsigned char vol) __sdcccall(1)
{
}


void set_psg(unsigned char reg, unsigned char tone) __sdcccall(1)
{
__asm
	push	ix

;	ld	h,a

;	ld	a,(#0xfcc1)	; exptbl
;	ld	b,a
;	ld	c,0
;	push	bc
;	pop	iy

;	ld	a,h
	ld	e,l

;	ld ix,#0x0093	; WRTPSG(MAINROM)
;	call	#0x001c	; CALSLT
	call	#0x0093
	pop	ix
__endasm;
}

/* screenのBIOS切り替え */
void set_screenmode(unsigned char mode) __sdcccall(1)
{
__asm
	push	ix
;	ld	b,a

;	ld	a,(#0xfcc1)	; exptbl
;	ld	d,a
;	ld	e,0
;	push	de
;	pop	iy
;	ld ix,#0x005f	; CHGMOD(MAINROM)

;	ld	a,b

;	call	#0x001c	; CALSLT
	call	#0x005f
	pop	ix
__endasm;
}

void set_screencolor(void)
{
__asm
	push	ix
;	ld	a,(#0xfcc1)	; exptbl
;	ld	d,a
;	ld	e,0
;	push	de
;	pop	iy
;	ld ix,#0x0062	; CHGCLR(MAINROM)

;	call	#0x001c	; CALSLT
	call	#0x0062
	pop	ix
__endasm;
}

void key_flush(void)
{
__asm
	push	ix
;	ld	a,(#0xfcc1)	; exptbl
;	ld	d,a
;	ld	e,0
;	push	de
;	pop	iy
;	ld ix,#0x0156	; KILBUF(MAINROM)

;	call	#0x001c	; CALSLT
	call	#0x0156
	pop	ix
__endasm;
}

void cls(void)
{
__asm
	push	ix
;	ld	a,(#0xfcc1)	; exptbl
;	ld	d,a
;	ld	e,0
;	push	de
;	pop	iy
;	ld ix,#0x00c3	; CLS(MAINROM)

;	call	#0x001c	; CALSLT
	call	#0x00c3
	pop	ix
__endasm;
}

/* mainromの指定番地の値を得る */
unsigned char  read_mainrom(unsigned short adr) __sdcccall(1)
{
__asm
	ld	a,(#0xfcc1)	; exptbl
	call	#0x000c	; RDSLT
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

void boxfill(int vdp_dx, int vdp_dy, int nx, int ny, unsigned char dix, unsigned char diy, unsigned char data)
{
	unsigned char port = VDP_writeport(VDP_WRITEINDEX);
	unsigned char port2 = VDP_writeport(VDP_WRITECONTROL);

	wait_VDP();

//	write_vdp(17, 36);
	outp(port2, 36);
	outp(port2, 0x80 | 17);

	outp(port, vdp_dx & 0xff);
	outp(port, (vdp_dx >> 8) & 0x01);
	outp(port, vdp_dy & 0xff);
	outp(port, (vdp_dy >> 8) & 0x03);
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

void msx_cls(void)
{
	boxfill(0, 16, 256, 212-16, 0, 0, 0x00);
}

unsigned char port3, port4;

unsigned char sx, sy, vdp_dx, vdp_dy; //, nc, ny, dix, diy, 
unsigned char VDPcommand;
unsigned char APAGE,VPAGE,XSIZE,XSIZA,YSIZE;

void VDPsetAREA2(void)
/*unsigned short sx, unsigned short sy, unsigned short vdp_dx, unsigned short vdp_dy, unsigned short nx, unsigned short ny, unsigned char dix, unsigned char diy, unsigned char command)*/
{
	port3 = VDP_writeport(VDP_WRITEINDEX);
	port4 = VDP_writeport(VDP_WRITECONTROL);

//	vdpdata[0] = (sx & 0xff);		/* 32 */
//	vdpdata[1] = ((sx >> 8) & 0x01);	/* 33 */
//	vdpdata[2] = (sy & 0xff);		/* 34 */
//	vdpdata[3] = ((sy >> 8) & 0x03);	/* 35 */
//	vdpdata[4] = (vdp_dx & 0xff);		/* 36 */
//	vdpdata[5] = ((vdp_dx >> 8) & 0x01);	/* 37 */
//	vdpdata[6] = (vdp_dy & 0xff);		/* 38 */
//	vdpdata[7] = ((vdp_dy >> 8) & 0x03);	/* 39 */
//	vdpdata[8] = (nx & 0xff);		/* 40 */
//	vdpdata[9] = ((nx >> 8) & 0x01);	/* 41 */
//	vdpdata[0xa] = (ny & 0xff);		/* 42 */
//	vdpdata[0xb] = ((ny >> 8) & 0x03);	/* 43 */
//	vdpdata[0xc] = 0;
//	vdpdata[0xd] = ((diy << 3) & 0x08) | ((dix << 2) & 0x04);	/* 45 */
//	vdpdata[0xe] = VDPcommand;
__asm
	ld	a,(_sx)	;SX
	ld	h,a
	ld	a,(_sy)	;SY
	ld	l,a
;	ld	a,(_vdpdata+3)
;	ld	(_APAGE),a
	ld	a,(_vdp_dx)	;DX
	ld	d,a
	ld	a,(_vdp_dy)	;DY
	ld	e,a
;	ld	a,(_vdpdata+7)
;	ld	(_VPAGE),a
;	ld	a,(_vdpdata+8)
;	ld	(_XSIZE),a
;	ld	a,(_vdpdata+9)
;	ld	(_XSIZA),a
;	ld	a,(_vdpdata+0xa)
;	ld	(_YSIZE),a
;	exx

__endasm;
	wait_VDP();
/*
	outp(port4, 32);
	outp(port4, 0x80 | 17);
*/
//__asm
//__endasm;

//	outp(port3, data0);			/* 32 */
//	outp(port3, data1);			/* 33 */
//	outp(port3, data2);			/* 34 */
//	outp(port3, data3);			/* 35 */
//	outp(port3, data4);			/* 36 */
//	outp(port3, data5);			/* 37 */
//	outp(port3, data6);			/* 38 */
//	outp(port3, data7);			/* 39 */
//	outp(port3, data8);			/* 40 */
//	outp(port3, data9);			/* 41 */
//	outp(port3, dataa);			/* 42 */
//	outp(port3, datab);			/* 43 */
//	outp(port3, 0);				/* 44 */

//	outp(port3, datad);	/* 45 */

//	outp(port3, VDPcommand);
__asm
;	exx
	ld	a,(_port4)
	ld	c,a
	ld	a,32
	out	(c),a
	ld	a,#0x80 | 17
	out	(c),a

	ld	b,0x0f
	ld	a,(_port3)
	ld	c,a
;	ld	hl,_vdpdata

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
//	DI();
//	write_VDP(5, (lowadr >> (2 + 5)) & 0xf8 | 0x07);
//	write_VDP(11, ((highadr << 1) & 0x02) | ((lowadr >> 15) & 0x01));
//	write_VDP(5, (0xe8));
	write_VDP(11, ((highadr << 1) & 0x02));
//	EI();
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

unsigned char get_stick(unsigned char trigno) __sdcccall(1)
{
__asm
;	ld	l,a

	push	ix

;	ld	a,(#0xfcc1)	; exptbl
;	ld	d,a
;	ld	e,#0
;	push	de
;	pop	iy
;	ld ix,#0x00d5	; GTSTCK(MAINROM)

;	ld	a,l

;	call	#0x001c	; CALSLT
	call	#0x00d5

	pop	ix
__endasm;
}

unsigned char get_pad(unsigned char trigno) __sdcccall(1)
{
__asm
;	ld	l,a

	push	ix

;	ld	a,(#0xfcc1)	; exptbl
;	ld	d,a
;	ld	e,#0
;	push	de
;	pop	iy
;	ld ix,#0x00d8	; GTTRIG(MAINROM)

;	ld	a,l

;	call	#0x001c	; CALSLT
	call	#0x00d8
	pop	ix
__endasm;
}

/* DISK BASIC only. */
/*volatile void Set_RAM_MODE(void){
__asm
	push	ix
	ld	a,(#0xf342)
	ld	hl,#0x4000
	call	#0x0024
	pop	ix
__endasm;
}

volatile void Set_ROM_MODE(void){
__asm
	push	ix
	ld	a,(#0xfcc1)
	ld	hl,#0x4000
	call	#0x0024
	pop	ix
__endasm;
}*/

unsigned char *jiffy = (unsigned char *)0xfc9e;
unsigned char jiffy_flag = 0;
unsigned char old_jiffy;
unsigned char old_jiffy2 = 0;

void wait_vsync(void)
{

__asm
	ld	a,(#0xfc9e)
	ld	b,a
jiffyloop:
	ld	a,(#0xfc9e)
	cp	b
	jr	z,jiffyloop
__endasm;

//	return;

//	while((read_VDPstatus(2) & 0x40));
//	while(!(read_VDPstatus(2) & 0x40)); /* WAIT VSYNC */

/*	++total_count;
	if(*jiffy >= 60){
		put_numd((long)(total_count), 2);
		put_strings(SCREEN2, 28, 22, str_temp, CHRPAL_NO);
		total_count = 0;
		*jiffy = 0;
	}*/


	++total_count;
#ifdef DEBUG3
	if((unsigned char)(*jiffy - old_jiffy2) >= 60){
//	if(*jiffy >= 60){
		old_jiffy2 = *jiffy;
		msx_print_num(27, 22, (long)(total_count), 2);
		total_count = 0;
//		*jiffy = 0;
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
#ifdef DEBUG_FPS
	if((unsigned char)(*jiffy - old_jiffy2) >= 60){
//	if(*jiffy >= 60){
		old_jiffy2 = *jiffy;
		EI();
		msx_print_num(27, 22, (long)(total_count), 2);
		total_count = 0;
//		*jiffy = 0;
	}
	EI();
#endif
}

/*void sys_wait(unsigned char wait)
{
__asm
	ld	l,a
	ld	a,(#0xfc9e)
	add	a,l
	ld	b,a
jiffyloop2:
	ld	a,(#0xfc9e)
	cp	b
	jr	c,jiffyloop2	; a<b
__endasm;

	return;

	unsigned char i;
	for(i = 0; i < wait; ++i)
		wait_vsync();
}
*/
/*パレット・セット*/
void pal_set(unsigned char pal_no, unsigned char color, unsigned char red, unsigned char green,
	unsigned char blue)
{
	unsigned char port = VDP_writeport(VDP_WRITEPAL);
	write_VDP(16, color);
	outp(port, red * 16 | blue);
	outp(port, green);
}

void pal_all(unsigned char pal_no, unsigned char color[MAXCOLOR][3])
{
	unsigned short i;
	for(i = 0; i < MAXCOLOR; i++)
		pal_set(pal_no, i, color[i][0]/2, color[i][1]/2, color[i][2]/2);
}

//value < 0 黒に近づける。
//value = 0 設定した色
//value > 0 白に近づける。
void set_constrast(int value, unsigned char org_pal[MAXCOLOR][3], int pal_no)
{
	int j, k;
	int pal[3];


	for(j = 0; j < MAXCOLOR; j++){
		for(k = 0; k < 3; k++){
			if(value > 0)
				pal[k] = org_pal[j][k] + value;
			else if(value < 0)
				pal[k] = org_pal[j][k] * (15 + value) / 15;
			else
				pal[k] = org_pal[j][k];
			if(pal[k] < 0)
				pal[k] = 0;
			else if(pal[k] > 15)
				pal[k] = 15;
		}
		pal_set(pal_no, j, pal[0]/2, pal[1]/2, pal[2]/2);
	}
}

//wait値の速度で黒からフェードインする。
/*void fadeinblack(unsigned char org_pal[MAXCOLOR][3], int pal_no, int wait)
{
	int j;

	init_sys_wait();
	for(j = -15; j <= 0; j++){
		sys_wait(wait);
		set_constrast(j, org_pal, pal_no);
		init_sys_wait();
	}
}*/

//wait値の速度で黒にフェードアウトする。
/*void fadeoutblack(unsigned char org_pal[MAXCOLOR][3], int pal_no, int wait)
{
	int j;

	init_sys_wait();
	for(j = 0; j != -16; j--){
		sys_wait(wait);
		set_constrast(j, org_pal, pal_no);
		init_sys_wait();
	}
}*/

//wait値の速度で白にフェードアウトする。
void fadeoutwhite(unsigned char org_pal[MAXCOLOR][3], int pal_no, int wait)
{
	int j;

	init_sys_wait();
	for(j = 0; j < 16; j++){
		sys_wait(wait);
		set_constrast(j, org_pal, pal_no);
		init_sys_wait();
	}
}

//パレットを暗転する。
void pal_allblack(int pal_no)
{
	unsigned char j;
	for(j = 0; j < MAXCOLOR; j++)
		pal_set(pal_no, j, 0, 0, 0);
}

unsigned char spr_flag = 0, spr_next = 0;
unsigned char vdps0;


short test_h_f = TRUE;
short soundflag = FALSE;

unsigned char chr;

void msx_print(unsigned char x, unsigned char y,  char *str)
{
//	y = 28-y;

	XSIZE = 8;
	XSIZA = 0;
	YSIZE = 8;
	APAGE = 3; //map_page;
	VPAGE = 0;
	VDPcommand = HMMM;

	while(1){
		chr = (unsigned char)*(str++);
		if(chr == '\0')
			break;
		if((chr < 0x30))
			chr = 0x40;
		chr -= '0';
		sx = (chr & 0x0f) * 8;
		sy = ((unsigned char)chr / 16) * 8;
		vdp_dx = x * 8;
		vdp_dy = y * 8;
		VDPsetAREA2();
		++x;
	}
}


const unsigned long pow10[7] = {
	1UL,			// 1桁目
	10UL,
	100UL,
	1000UL,
	10000UL,
	100000UL,	// 6桁目
	1000000UL,	// 7桁目
	10000000UL
};

void msx_print_num(unsigned char x, unsigned char y, unsigned long number, unsigned char digits) __sdcccall(1)
{
	unsigned char i = digits;
	unsigned long p;
	unsigned char digit, j = 0;

/*	while(i--){
		str_temp[i] = number % 10 + 0x30;
		number /= 10;
	}*/

//	for (int i = 0; i < 7; i++) {
	while(i--){
		digit = 0;

		p = pow10[i];

		while (number >= p) {	// 引き算ループ
			number -= p;
			digit++;
		}
		str_temp[j++] = digit + 0x30;
	}

	str_temp[digits] = '\0';

	msx_print(x, y, str_temp);
}


#include "inkey.h"

unsigned char st0, st1, pd0, pd1, pd2, k3, k5, k7, k9, k10;
unsigned char keycode = 0;

void keyscan1(void)
{
	keycode = 0;

	return;
}

unsigned char keyscan(void)
{
//	DI();
	keycode = 0;

	k3 = get_key(3);

	k9 = get_key(9);
	k10 = get_key(10);
	k5 = get_key(5);

	st0 = get_stick(0);
	st1 = get_stick(1);

	pd0 = get_pad(0);
	pd1 = get_pad(1);
	pd2 = get_pad(3);
//	EI();

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

	return keycode;
}

#define STAR_NUM	192/24-1						/* スタ－の数 */

unsigned short star_baseadr[STAR_NUM];
unsigned char star_offset[STAR_NUM];
unsigned char star_speed[STAR_NUM];
unsigned char star_color[STAR_NUM];

unsigned short star_adr_tmp;

void init_star(void)
{
	unsigned char i;
/* スタ－の座標系を初期化 */
	for(i = 0;i < STAR_NUM; i++){
		star_baseadr[i] = (24 * (i + 1) + 7) * 128 - 128 * 8;
		star_offset[i] = rand() % 128;
		star_speed[i] = (rand() % 2) + 1;

		star_color[i] = rand() % 14 + 2;
	}

}

void draw_bg(void)
{
//	unsigned char i;

/* スクロ－ルするスタ－の計算 */

//	if(total_count & 1){
//		return;
//	}
/*	i = STAR_NUM;
	while(i--){
		DI();
		write_vram_adr(0, star_baseadr[i] + star_offset[i]);
		write_vram_data(0);
		EI();
		star_offset[i] -= star_speed[i];
		star_offset[i] %= 128;
		DI();
		write_vram_adr(0, star_baseadr[i] + star_offset[i]);
		write_vram_data(star_color[i]);
		EI();
	}*/
__asm
	ld	a,(_total_count)
	and	1
	ret	nz

	ld	b,#0
	ld	c,#STAR_NUM
starloop:
	push	bc
;	write_vram_adr(0, star_baseadr[i] + star_offset[i]);

	ld	hl,_star_baseadr-2
	add	hl,bc
	add	hl,bc
	ld	e,(hl)
	inc	hl
	ld	d,(hl)	; vram baseaddress

	ld	hl,_star_offset-1
	add	hl,bc
;	ld	(_star_adr_tmp),hl	;offset

	ld	a,(hl)
	ld	l,a
	ld	h,0
	add	hl,de	;vram address

	push	de	;vram baseaddress
	push	bc	; counter
	push	hl	;vram address

	ld	a,h
	rlca
	rlca
	and	3
	ld	e,a		; A15,A14

	ld	a,(_VDP_writeadr)
	inc	a
	ld	c,a
	ld	a,e

	di
	out	(c),a
	ld	a,14
	set 7,a
	out	(c),a

	pop	de		;vram address
;	ld	a,(_VDP_writeadr)
;	inc	a
;	ld	c,a
	out	(c),e
	ld	a,d
	and	a,0x3f
	set	6,a
	out	(c),a

;	write_vram_data(0);

	dec	c
	xor	a
;	ld	a,15
	out	(c),a
	ei

	pop	bc		; counter

;	star_offset[i] -= star_speed[i];
;	star_offset[i] %= 128;

	ld	hl,_star_speed-1
	add	hl,bc
	ld	a,(hl)
	ld	e,a
	ld	hl,_star_offset-1
	add	hl,bc
;	ld	hl,(_star_adr_tmp)	;offset
	ld	a,(hl)
	sub	a,e
	and	127
	ld	(hl),a

;	write_vram_adr(0, star_baseadr[i] + star_offset[i]);

	pop	hl			;vram baseaddress
	ld	e,a
	ld	d,0
	add	hl,de		;vramaddress

	push	bc		; counter
	push	hl		;vramaddress
	ld	a,h
	rlca
	rlca
	and	3
	ld	e,a		; A15,A14

	ld	a,(_VDP_writeadr)
	inc	a
	ld	c,a
	ld	a,e

	di
	out	(c),a
	ld	a,14
	set 7,a
	out	(c),a

	pop	de			;vramaddress
;	ld	a,(_VDP_writeadr)
;	inc	a
;	ld	c,a
	out	(c),e
	ld	a,d
	and	a,0x3f
	set	6,a
	out	(c),a

;	write_vram_data(star_color[i]);

	dec	c
	pop	de		; counter

	ld	hl,_star_color-1
	add	hl,de
	ld	a,(hl)
	out	(c),a
	ei

	pop	bc
	dec	c
	jp	nz,starloop
__endasm;
}

void clr_sp(void)
{
__asm
	ld	a,(_VDP_writeadr)
	ld	c,a
clrloop:
	ld	a,216 ;0xd4
	out	(c),a

__endasm;
}

unsigned char pat_num, atr, atr2, *patr; //, pal;
unsigned char palskip_flag = 0;

inline void set_spr(void)
{
__asm
	ld	hl,(_pspr_chr) ;chr_data

	ld	a,(_VDP_writeadr)
	ld	c,a
	ld	a,(_tmp_spr_count)
	or	a
	jr	z,sprend
	ld	d,a
	xor	a,a
sprloop2:
	outi
	outi
	outi
	outi

	dec	d
	jr	nz,sprloop2
sprend:
__endasm;
}


void set_sprite_all(unsigned char start, unsigned char end)
{
	unsigned char i, j;

	pspr_chr = &spr_chr[start];
	tmp_spr_count = end;

/* 表示数ぶん書き込む */
	if(tmp_spr_count > MAX_SPRITE){
		tmp_spr_count = MAX_SPRITE;
	}

	/* 色情報の処理 */
//	pold_data = &old_data[spr_page][0];
//	pchr_data = &chr_data[0];

/*	for(i = 0; i < tmp_spr_count*0+1; i++){
		if((pold_data->pat_num != pchr_data->pat_num)){
			pat_num = pold_data->pat_num = pchr_data->pat_num;
			atr = pold_data->atr = pchr_data->atr;
					patr = (unsigned char *)&spr_col[pat_num/4][0];
					DI();
					write_vram_adr(spr_page, 0x7600 - 512 + i * 16);*/
__asm

	push	hl
	push	de
	push	bc

	xor	a
	ld	(_palskip_flag),a

	ld	a,(_spr_page)
	ld	l,a
	ld	h,0

	add	hl,hl	; * 32
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	ld	bc,_old_data
	add	hl,bc
	ex	de,hl

	ld	hl,(_pspr_chr)
	ld	bc,2
	add	hl,bc ;chr_data

	ld	c,0

palloop:
	ld	a,(de)	; old_data
	ld	b,a
	ld	a,(hl)	; chr_data
	cp	b
	jp	z,palskip
	ld	(_pat_num),a
	ld	(de),a


	push	bc
	push	de

	ld	a,(_palskip_flag)
	or	a
	jr	nz,palskip2
	ld	a,1
	ld	(_palskip_flag),a

	push	hl

;	write_VDP(14, (((highadr  << 2) & 0x04) | (lowadr >> 14) & 0x03));

;	A15-A0 0x7400+i*16

	ld	h,0
	ld	l,c
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	ld	de,7400h
	add	hl,de

	push	hl

	ld	a,h
	rlca
	rlca
	and	3
	ld	b,a		; A15,A14
	ld	a,(_spr_page)	;high A16
	sla	a
	sla	a
	or	b
	ld	e,a

	ld	a,(_VDP_writeadr)
	inc	a
	ld	c,a
	ld	a,e

	di
	out	(c),a
	ld	a,14
	set 7,a
	out	(c),a

	pop	de
;	ld	a,(_VDP_writeadr)
;	inc	a
;	ld	c,a
	out	(c),e
	ld	a,d
	and	a,0x3f
	set	6,a
	out	(c),a
	ei

	pop	hl

palskip2:

	push	hl

	ld	a,(_VDP_writeadr)
	ld	c,a

;					patr = (unsigned char *)&spr_col[pat_num/4][0];

	ld	a,(_pat_num)	; x4

	ld	l,a
	ld	h,0
	add	hl,hl			; 4x4 = x16
	add	hl,hl
	ld	de,_spr_col
	add	hl,de

	outi
	outi
	outi
	outi
	outi
	outi
	outi
	outi
	outi
	outi
	outi
	outi
	outi
	outi
	outi
	outi

	pop	hl
	pop	de
	pop	bc

	jr	palskip3

palskip:
	xor	a
	ld	(_palskip_flag),a

palskip3:
	inc	hl
	inc	hl
	inc	hl
	inc	hl
	inc	de

	inc	c
	ld	a,(_tmp_spr_count)
	cp	c
	jp	nz,palloop

palend:
	pop	bc
	pop	de
	pop	hl
__endasm;
/*					EI();
			}
		}

		++pold_data;
		++pchr_data;

	}
*/
	DI();
//	write_vram_adr(spr_page, 0x7600);

__asm
	ld	hl,7600h
	ld	a,h
	rlca
	rlca
	and	3
	ld	b,a		; A15,A14
	ld	a,(_spr_page)	;high A16
	sla	a
	sla	a
	or	b
	ld	e,a

	ld	a,(_VDP_writeadr)
	inc	a
	ld	c,a
	ld	a,e

	di
	out	(c),a
	ld	a,14
	set 7,a
	out	(c),a

	ld	de,7600h
;	ld	a,(_VDP_writeadr)
;	inc	a
;	ld	c,a
	out	(c),e
	ld	a,d
	and	a,0x3f
	set	6,a
	out	(c),a
	ei
__endasm;
	set_spr();



	if(tmp_spr_count < MAX_SPRITE){
		clr_sp();
	}
	EI();

spr_end:

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

void term(void)
{
__asm
	xor	a
	ld	c,0
	call	0005h
__endasm;
}

 // 適当に音を鳴らす（後で調整）
void msx_sound(unsigned char no, unsigned char dummy) __sdcccall(1)
{
	DI();
	if(no == 1){
		set_psg(6,127);
		set_psg(11,0);
		set_psg(12,15);
		set_psg(7,0x9c);  // 10011100
		set_psg(13,9);
		set_psg(10,0x10);
	}else if(no == 0){
			set_psg(4, 55);
			set_psg(5, 0);
			set_psg(6,127);
			set_psg(11,0);
			set_psg(12,15);
			set_psg(7,0xb8);  // 10111000
			set_psg(13,9);
			set_psg(10,0x10);

	}
	EI();
}


/* ゲ－ム本体の処理 */
short errlv = 0;

#define vdp_value ((volatile unsigned char *)0xf3df)
#define forclr ((volatile unsigned char *)0xf3e9)
#define bakclr ((volatile unsigned char *)0xf3ea)
#define bdrclr ((volatile unsigned char *)0xf3eb)
#define clicksw ((volatile unsigned char *)0xf3db)
#define oldscr ((volatile unsigned char *)0xfcb0)

unsigned char forclr_old, bakclr_old, bdrclr_old, clicksw_old;

void main(void)
{
	unsigned char i;

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

	bgmmode = checkbgm();
//	getchar();

	set_screenmode(5);
	set_displaypage(0);
	DI();
	write_VDP(1, vdp_value[1] | 0x02);
	EI();
	spr_on();
	boxfill(0, 256, 256, 212, 0, 0, 0x00);

//	spr_page = 0;
//	spr_count[0] = spr_count[1] = 0;
//	tmp_spr_count = 0;
	old_count[0] = old_count[1] = 0; //MAX_SPRITE;

	for(i = 0; i < MAX_SPRITE; ++i){
		old_data[0][i] = 255;
		old_data[1][i] = 255;
	}

//	set_int();

	init_star();

	high_score = 5000;
	reset();

#ifdef DEBUG
	do {
#else
	for(;;){
#endif
	main2();
#ifdef DEBUG
	}while((get_key(7) & 0x04));
#else
	}
#endif

/* 終了処理 */
	reset_int();
	stopbgm();
	set_vol(0);

//	reset_int();

	DI();
	pal_all(CHRPAL_NO, org_pal);
	EI();

	*forclr = forclr_old;
	*bakclr = bakclr_old;
	*bdrclr = bdrclr_old;
	set_screencolor();

	set_screenmode(*oldscr);

	*clicksw = clicksw_old;

	key_flush();
//	term();
//	exit(0);
}

unsigned char INTWORK[5] = {0,0,0,0,0};

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
;INTWORK:
;	DB	0,0,0,0,0
	jp _INTWORK
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
	LD	DE,_INTWORK
	LD	BC,5
	LDIR
	LD	HL,_intvsync

	LD	(IY+1),L
	LD	(IY+2),H
	LD	(IY+0),0C3H

;	LD	(IY+2),L
;	LD	(IY+3),H
;	LD	(IY+0),0F7H
;	LD	(IY+4),0C9H

;	ld	a,h
;	ld	hl,#0xf341	;slot0
;	cp	#0x40
;	jr	c,slotset
;	inc	hl			;slot1
;	cp	#0x80
;	jr	c,slotset
;	inc	hl			;slot2
;	cp	#0xc0
;	jr	c,slotset
;	inc	hl			;slot3
;slotset:
;	LD	A,(HL)
;	LD	(IY+1),A

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
	LD	HL,_INTWORK
	LD	A,(HL)
	OR	A
	JR	Z,reset_int_end
	LD	DE,-609
	LD	BC,5
	PUSH	HL
	LDIR
	POP	HL
	XOR	A
	LD	(HL),A
reset_int_end:
	EI
__endasm;
#endif
}
