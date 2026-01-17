// common.h 各機種共通ルーチン
//#define DEBUG

// 構造体
typedef struct {
	int x, y;
	unsigned char active;
} Entity;

#define vram_m(x, y) vram[(x) + (MAP_W+4) * (y)]

// max/min マクロ
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define SCREEN_WIDTH (640/8)
#define SCREEN_HEIGHT (200/4)
#define DEADLINE ((160-8)/4)

// 定数
#define MAP_W 640/8
#define MAP_H DEADLINE //200/4

// 仮想VRAM
//static char vram[MAP_W+4][MAP_H+4];
char *vram;

// グローバル変数
char battle_msg[40];
char *pbattle_msg;
char c;

long score = 0, hiscore = 5000;
int combo = 0;

// 関数
int quotient = 0;
int rem = 0;

int divideBy10(int n) {
	quotient = 0;
	rem = n;

	while (rem >= 10) {
		rem -= 10;
		quotient++;
	}
	return quotient;
}

int res;

int get_mod10(int n) {
//	int res;
	res = n - divideBy10(n) * 10;
	return (res >= 10) ? res - 10 : res;
}


int itoa2(int value, char *str) {
	char *p = str;
	int tmp = value;
	int size = 0;
	char *start;
	char t;

	if (value <= 0) { *p++ = '0'; *p = '\0'; return 0; }
	while (tmp) { *p++ = '0' + get_mod10(tmp); tmp = divideBy10(tmp); }
	*p-- = '\0';
	// 逆転
	start = str;
	while (start < p) {
		size++;
		t = *start; *start++ = *p; *p-- = t;
	}
	return size;
}

unsigned char simple_rnd(void) {
/*	static unsigned char r = 1;
	r = r * 37 + 41;  // 適当な定数
	return r;*/
	static unsigned char seed = 1;
	seed = (seed * 5) + 1;
	return seed;  // 0-255
}

int size;

int strcpy2(char *dst, char *src)
{
	size = 0;
	while(*src != '\0'){
		size++;
		*(dst++) = *(src++);
	}
	*dst = '\0';
	return size;
}

void put_strings(int scr, int x, int y,  char *str, char pal);
void put_numd(long j, char digit);
char str_temp[9];

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
	print_at(19, 22 , str_temp);
	if(score >= hiscore){
//		if((get_mod10(score)) == 0){
		if((score % 10) == 0){
			hiscore = score;
			print_at(12, 22, "HIGH ");
		}
	}
	else
		print_at(12, 22, "SCORE");
}

void combo_display(void)
{
	put_numd(combo, 8);
	print_at(19, 24 , str_temp);
		print_at(12, 24, "COMBO");
}

void score_displayall(void)
{
//	print_at(9, 22, "SCORE");
	score_display();
}

void hiscore_display(void)
{
/*	if(score > hiscore)
		if((score % 10) == 0)
			hiscore = score;
*/
	put_numd(hiscore, 8);

	print_at(12, 24, "HIGH ");
	print_at(19, 24, str_temp);
}

void hiscore_display_clear(void)
{
	print_at(12, 24, "     ");
	print_at(19, 24, "        ");
}

void init_status(void)
{
}

void set_map(int x, int y, unsigned char x_size, unsigned char y_size)
{
	unsigned char i,j;
	for(j=0;j<y_size;++j)
		for(i=0;i<x_size;++i)
			if((x+i) >= 0)
				if((y+j) >= 0)
//					if((x+i) <= MAP_W)
//						if((y+j) <= MAP_H)
//					if(!vram[x+i][y+j])
//							++vram[x+i][y+j];
							++vram_m(x+i,y+j);
}

void reset_map(int x,int y, unsigned char x_size, unsigned char y_size)
{
	unsigned char i,j;
	for(j=0;j<y_size;++j)
		for(i=0;i<x_size;++i)
			if((x+i) >= 0)
				if((y+j) >= 0)
//					if((x+i) <= MAP_W)
//						if((y+j) <= MAP_H)
//					if(vram[x+i][y+j])
//							if((vram[x+i][y+j] - 1)>=1){
							if((vram_m(x+i, y+j) - 1)>=1){
//								--vram[x+i][y+j];
								--vram_m(x+i,y+j);
							}else{
//								vram[x+i][y+j] = 0;
								vram_m(x+i,y+j) = 0;
								erase_chr8(x-16/8+i*1, (y-16/4)*2+2*j);

//							put_chr8(x-16/8+i*2, (y-16/4)*2+4*j, 16*2*1+3*2*0,0);
							}
}


char sprite_pattern_no[8]; //, sprite_color_no[8][2];
int spr_x[8], spr_y[8];
int spr_x_new[8], spr_y_new[8];
unsigned char spr_x_size[8], spr_y_size[8];

void init_sprite(char num, int posx, int posy, unsigned char x_size, unsigned char y_size)
{
	spr_x[num] = spr_x_new[num] = posx;// - 16;
	spr_y[num] = spr_y_new[num] = posy;// - 16 + 1;
	spr_x_size[num] = x_size;
	spr_y_size[num] = y_size;
//	set_map(spr_x_new[i], spr_y_new[i],x_size,y_size);
}

void end_sprite(char num, int posx, int posy)
{
//	reset_map(spr_x_new[num], spr_y_new[num],4,4);
	reset_map(spr_x[num], spr_y[num],spr_x_size[num]*2, spr_y_size[num]*2);
	spr_x[num] = spr_x_new[num] = -2;
	spr_y[num] = spr_x_new[num] = -2;
//	reset_map(posx, posy,spr_x_size[num]*2, spr_y_size[num]*2);
}

void set_sprite(char num, int posx, int posy)
{
	spr_x_new[num] = posx;// - 16;
	spr_y_new[num] = posy;// - 16 + 1;
}

void set_sprite_pattern(int num, int no) {
	sprite_pattern_no[num] = no; //patternno_table[no];
//	sprite_color_no[num][0] = patterncolor_table[no * 2];
//	sprite_color_no[num][1] = patterncolor_table[no * 2 + 1];
}

/*void put_sprite(void)
{
	char i;
	for(i = 0; i < 8; ++i){	
		spr_x[i] = spr_x_new[i];
		spr_y[i] = spr_y_new[i];
	}
}*/

void set_sprite_all(void) {
	char i, spr_count = 0;
	for(i = 0; i < 8; ++i){
//		VDP_put_sprite_16(spr_count++, spr_x[i],  spr_y[i], sprite_pattern_no[i], sprite_color_no[i][0]);
		if((spr_x[i] != spr_x_new[i]) || (spr_y[i] != spr_y_new[i])){
			set_map(spr_x_new[i], spr_y_new[i],spr_x_size[i]*2, spr_y_size[i]*2);
		}
	}
//			if((spr_x[i] >=0) && (spr_y[i] >= 0))
	for(i = 0; i < 8; ++i){
		if((spr_x[i] != spr_x_new[i]) || (spr_y[i] != spr_y_new[i])){
			reset_map(spr_x[i], spr_y[i],spr_x_size[i]*2, spr_y_size[i]*2);
		}
	}
//	for(i = 0; i < 8; ++i){
//	}
	for(i = 0; i < 8; ++i){
		spr_x[i] = spr_x_new[i];
		spr_y[i] = spr_y_new[i];
		if((spr_x[i] >= 0) && (spr_y[i] >= 0))
			put_chr16(spr_x[i]-16/8, spr_y[i]-16/4, sprite_pattern_no[i],spr_x_size[i], spr_y_size[i]);
	}
}

int spawn_timer = 0;
int combo_timer = 0;
int wave = 1;
int enemies_killed_this_wave = 0;
int game_over = 0;
int score_display_flag = 1;
int combo_display_flag = 0;
int enemy_speed = 2;

int count = 0;
char k;

unsigned char seflag;

Entity player = {160/8, (160-16)/4};
Entity old_player = {0,0};//160/8, (160-16)/8};
Entity bullets[3] = {{0}};
Entity enemies[4] = {{0}};
Entity pluses[8] = {{0}};  // Plus最大8個

int e,b,p;

void se(void)
{
	if(seflag == 1){
		set_se();
	}
	seflag = 0;
}

void main2(void) {
	int	i,j;

	vram = (char *)0x5400;

	for(i = 0; i < MAP_H; ++i)
		for(j = 0; j < MAP_W; ++j)
//			vram[i][j]=0;
			vram_m(i,j) = 0;

	init_sprite(0,player.x, player.y,2,2);

	for(i = 0; i < 640/8/2 ;++i)
		put_chr16(i*2, (DEADLINE),1,2,2);
	for (;;) {
		player.x = 320/8;
		player.y = (160 - 16 + 8)/4;
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

		score_displayall();

		while (!game_over) {
			++count;
			if(combo)
				++combo_timer;

			keycode = keyscan();

//			old_player.x = player.x;
//			old_player.y = player.y;

			// 移動 (キーボードor joy direct)
			if (keycode & KEY_UP1) player.y -= 2;  // up
			if (keycode & KEY_DOWN1) player.y += 2;  // down
			if (keycode & KEY_LEFT1) player.x -= 2;  // left
			if (keycode & KEY_RIGHT1) player.x += 2;  // right

			player.x = MAX(16/8, MIN(((SCREEN_WIDTH-16/8)), player.x));
			player.y = MAX(16/4, MIN(((DEADLINE)), player.y));

			set_sprite(0,player.x, player.y);
			set_sprite_pattern(0, 0);

			// 射撃 (ボタン押したら弾発射)
			if (keycode & KEY_A) {  // adjust bit
				for (i = 0; i < 3; i++) {
					if (!bullets[i].active) {
						bullets[i].active = 1;
						bullets[i].x = player.x+2;
						bullets[i].y = player.y;// - 16;
						init_sprite(i+1,bullets[i].x, bullets[i].y-2,1,1);
						break;
					}
				}
			}

			// 弾更新
			for (i = 0; i < 3; i++) {
				if (bullets[i].active) {

					if ((bullets[i].y-16/4-2) < 0){
						end_sprite(i+1,bullets[i].x, bullets[i].y);
						bullets[i].active = 0;
					}else{
						bullets[i].y -= 2;
						set_sprite(i+1,bullets[i].x, bullets[i].y);
						set_sprite_pattern(i+1, 2);
					}
				}
			}


		// 敵スポーン&更新
			if (++spawn_timer > 10) {
				spawn_timer = 0;
				for (i = 0; i < 4; i++) {
					if (!enemies[i].active) {
						enemies[i].active = 1;
						enemies[i].x = 16/8 + ((rand() % ((SCREEN_WIDTH - 16/8))));
						enemies[i].y = 0;
						init_sprite(i+4, enemies[i].x, enemies[i].y+enemy_speed,2,2);
						break;
					}
				}
			}

			for (i = 0; i < 4; i++) {
				if (enemies[i].active) {
					if (!pluses[i].active) {
//						set_sprite_pattern(i+4, 2);
						set_sprite_pattern(i+4, 3);
						enemies[i].y += enemy_speed;

						if (enemies[i].y > (DEADLINE)){//SCREEN_HEIGHT + 16/4)){
							enemies[i].active = 0;
							end_sprite(i+4, enemies[i].x, enemies[i].y);
						}else
							set_sprite(i+4, enemies[i].x, enemies[i].y);
					} else {
//						end_sprite(i+4, enemies[i].x, enemies[i].y);
					}
				}else{
//					end_sprite(i+4, enemies[i].x, enemies[i].y);
				}
			}

			// 衝突判定: 弾 vs 敵
			for (b = 0; b < 3; b++) {
				if (!bullets[b].active) continue;
				for (e = 0; e < 4; e++) {
					if (pluses[e].active) continue;
					if (!enemies[e].active) continue;

					if (abs((bullets[b].x-1) - enemies[e].x) > (64/8))
						continue;
					if( abs(bullets[b].y - enemies[e].y) > (64/4))
						continue;

					if (abs((bullets[b].x-1) - enemies[e].x) < ((16*2+8)/8) && abs(bullets[b].y - enemies[e].y) < ((16)/4)) {
						bullets[b].active = 0;
//						enemies[e].active = 0;
						end_sprite(b+1, bullets[b].x, bullets[b].y);

						end_sprite(e+4, enemies[e].x, enemies[e].y);
//						set_se();
						seflag = 1;

						// Plus生成
//						for (p = 0; p < 1; p++) {
//							if (!pluses[e].active) {
								pluses[e].active = 1;
								pluses[e].x = enemies[e].x;
								pluses[e].y = enemies[e].y;
								
//								init_sprite(e+4, pluses[e].x, pluses[e].y+1,2,2);
//								set_sprite(e+4, pluses[e].x, pluses[e].y+1);
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
//					set_sprite(p+4, pluses[p].x, pluses[p].y);
//					pluses[p].y += 1;
					if (abs(pluses[p].x - player.x) < ((16*2+8)/8) && abs(pluses[p].y - player.y) < (16/4)) {
						pluses[p].active = 0;
						enemies[p].active = 0;
						end_sprite(p+4, pluses[p].x, pluses[p].y);
//						end_sprite(p+4, enemies[p].x, enemies[p].y);
						score += 50 * combo;
						score_display_flag = 1;
						combo_timer = 0;
						combo_display_flag = 1;

					}else{
						if (pluses[p].y >  (DEADLINE)){//(SCREEN_HEIGHT + 16/4)){
							pluses[p].active = 0;
							enemies[p].active = 0;
							end_sprite(p+4, pluses[p].x, pluses[p].y);
						}else{
							set_sprite(p+4, pluses[p].x, pluses[p].y);
							pluses[p].y += 1;
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
				if (enemies[e].active && abs(enemies[e].x - player.x) < (16*2/8) && abs(enemies[e].y - player.y) < (16/4)) {
					if(!pluses[e].active)
						game_over = 1;
				}
			}

			wait(1);
			set_sprite_all();

			if(seflag){
				se();
				seflag = 0;
//				secounter = 8;
			}
//			if(secounter){
//				--secounter;
//				if(!secounter)
//					seoff();
//			}

			if(score_display_flag){
				score_display_flag = 0;
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
end:;
}

