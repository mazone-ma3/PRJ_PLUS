/* plustakermsx.c By m@3 3 with Grok 2025. */

#include <stdio.h>
#include <conio.h>

#include <video/tms99x8.h>

#include <msx.h>
#include <msx\gfx.h>


unsigned char *jiffy = (unsigned char *)0xfc9e, old_jiffy;
unsigned char *clicksw = 0xf3db;

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

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192

// max/min マクロ
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))


// White (Code=15)
unsigned char spr0[] = {
	0x00, 0x00, 0x03, 0x04, 0x00, 0x00, 0x05, 0x05,
	0x03, 0x0f, 0x30, 0x30, 0x08, 0x07, 0x00, 0x00,
	0x00, 0x00, 0xc0, 0x20, 0x00, 0x00, 0xa0, 0xa0,
	0xc0, 0xf0, 0x0c, 0x0c, 0x30, 0xe0, 0x00, 0x00,
};

// Yellow(Code=10 or 11)
unsigned char spr1[] = {
	0x00, 0x03, 0x04, 0x0b, 0x0e, 0x0a, 0x08, 0x08,
	0x00, 0x00, 0x0f, 0x0f, 0x07, 0x00, 0x0e, 0x0e,
	0x00, 0xc0, 0x20, 0xd0, 0xf0, 0xd0, 0x10, 0x10,
	0x00, 0x00, 0xf0, 0xf0, 0xc0, 0x00, 0x70, 0x70,
};


unsigned char spr2[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

unsigned char spr3[] = {
	0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xc0, 0xc0, 0xc0, 0xc0, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

unsigned char block[] = {
	0x80,0x80,0x80,0xff,0x08,0x08,0x08,0xff,
};

// 構造体
typedef struct {
	int x, y;
	int active;
} Entity;

static void put_strings(unsigned char x, unsigned char y,  char *str, unsigned char pal)
{
	char chr;
	unsigned short vramadr = 0x1800 + x + y * 32;

	while((chr = *(str++)) != '\0'){
		if((chr < 0x30)) //|| (chr > 0x5f))
			chr = 0x20;
		vpoke(vramadr++, chr);
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
#define CHRPAL_NO 15 //COLOR_WHITE

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


char sprite_pattern_no[8], sprite_color_no[8][2];
int spr_x[8], spr_y[8];
int spr_x_new[8], spr_y_new[8];

void set_sprite(char num, int posx, int posy)
{
	spr_x_new[num] = posx - 16;
	spr_y_new[num] = posy - 16 + 1;
}

void put_sprite(void)
{
	char i;
	for(i = 0; i < 8; ++i){	
		spr_x[i] = spr_x_new[i];
		spr_y[i] = spr_y_new[i];
	}
}

char patternno_table[3] = {0, 12, 8};
char patterncolor_table[6] = {15, 10, 15, 15,  10, 10};

void set_sprite_pattern(int num, int no) {
	sprite_pattern_no[num] = patternno_table[no];
	sprite_color_no[num][0] = patterncolor_table[no * 2];
	sprite_color_no[num][1] = patterncolor_table[no * 2 + 1];
}

void set_sprite_all(void) {
	char i, spr_count = 0;
//	DI();
	for(i = 0; i < 8; ++i){
//		put_sprite(i, spr_x[i], spr_y[i], sprite_color_no[i], sprite_pattern_no[i]);
		vdp_put_sprite_16(spr_count++, spr_x[i],  spr_y[i], sprite_pattern_no[i], sprite_color_no[i][0]);
		if(sprite_pattern_no[i] == 0)
			vdp_put_sprite_16(spr_count++, spr_x[i],  spr_y[i], sprite_pattern_no[i] + 4, sprite_color_no[i][1]);
	}
	vdp_put_sprite_16(spr_count++, 0,  208, 0, 0);
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

short x;
unsigned char y, color;

unsigned char i = 0, keycode, oldcharset, data = 0;
short j;

volatile char spr_flag = 1;

int main(void)
{
	*clicksw = 0;

	vdp_color(15, 1, 1);
	vdp_set_mode(mode_1);
//#define set_mangled_mode() msx_set_mangled_mode()

	vdp_set_sprite_mode(sprite_large);
	vdp_set_sprite_16(0, spr0);
	vdp_set_sprite_16(4, spr1);
	vdp_set_sprite_16(8, spr2);
	vdp_set_sprite_16(12, spr3);

	vdp_put_sprite_16(0, 0,0, 0,15);
	vdp_put_sprite_16(1, 0,0, 4,10);

	for(j = '0'; j < 'Z'; ++j){
		for(i = 0; i < 8; ++i){
			char k =vpeek(i + 8  * j);
			vpoke(i + 8 * j, k | k << 1);
		}
	}

	for(i = 0; i < 8; ++i)
		vpoke(i + 8 * 'a', block[i]);

	vpoke(0x2000 + 'a' / 8, 8 * 16 + 9);
	for(i = 0; i < 32; ++i){
		put_strings(i, 18, "a", 0);
		put_strings(i, 19, "a", 0);
	}

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
		set_sprite_pattern(i, patno);
	}
	set_sprite_all();
//	set_int();

	// Main Loop

	for(;;){
		Entity player = {160 - 8, 160 - 16};
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
		old_jiffy = *jiffy;

		score_displayall();
		set_int();

//		combo_display();
		while (!game_over) {
			++count;
			if(combo)
				++combo_timer;
			keycode = get_stick(0) | get_stick(1);

			if((keycode == 1) || (keycode == 2) || (keycode == 8)){	/* UP */
				player.y -= 3;
			}
			if((keycode == 4) || (keycode == 5) || (keycode == 6)){	/* DOWN */
				player.y += 3;
			}
			if((keycode == 6) || (keycode == 7) || (keycode == 8)){	/* LEFT */
				player.x -= 3; //(*SP0X);
			}
			if((keycode == 2) || (keycode == 3) || (keycode == 4)){	/* RIGHT */
				player.x += 3; //(*SP0X);
			}
//			if((get_trigger(0) | get_trigger(1)) & 0x02){	/* END */
//				goto end;
//			}


			player.x = MAX(16, MIN(((SCREEN_WIDTH)), player.x));
			player.y = MAX(16, MIN(((SCREEN_HEIGHT)), player.y));

			set_sprite(0, player.x, player.y);

			// 射撃 (ボタン押したら弾発射)
			if((get_trigger(0) | get_trigger(1)) & 0x01){	/* SHOT */
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
						enemies[i].x = 16 + ((rand() & ((SCREEN_WIDTH - 16))));
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
						set_se();

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

//			old_jiffy = *jiffy;
			while(*jiffy == old_jiffy);
			old_jiffy = *jiffy;
			DI();
//			set_sprite_all();
			put_sprite();
			spr_flag = 1;

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
			EI();

		}
		reset_int();
		set_sprite_all();
		for(;;){
			if(!((get_trigger(0) | get_trigger(1)) & 0x01))	/* SHOT */
				break;
		}
		// ゲームオーバー画面
		/* ここにテキスト描画追加可能 */
		hiscore_display();
		for(;;){
			if((get_trigger(0) | get_trigger(1)) & 0x01)	/* SHOT */
				break;
		}
		hiscore_display_clear();
		// (スプライト全消し)
		for (int i = 0; i < 8; i++){
			set_sprite(i, 0, 0);
			set_sprite_all();
		}
		for(;;){
			if(!((get_trigger(0) | get_trigger(1)) & 0x01))	/* SHOT */
				break;
		}
	}
end:
//	reset_int();

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
		set_sprite_all();
/*
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
*/
	}
__asm
;	pop	ix
	pop	af
	jp	_INTWORK
;INTWORK:
;	DB	0,0,0,0,0
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

	LD	(IY+0),0C3H
	LD	(IY+1),L
	LD	(IY+2),H

	EI
	ret

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
	LD	HL,_INTWORK
	LD	DE,-609
	LD	BC,5
	LDIR
	EI
__endasm;
#endif
}
