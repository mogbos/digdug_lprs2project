
///////////////////////////////////////////////////////////////////////////////
// Headers.

#include <stdint.h>
#include "system.h"
#include <stdio.h>

#include "sprites_rgb333.h"

///////////////////////////////////////////////////////////////////////////////
// HW stuff.

#define WAIT_UNITL_0(x) while(x != 0){}
#define WAIT_UNITL_1(x) while(x != 1){}

#define SCREEN_IDX1_W 640
#define SCREEN_IDX1_H 480
#define SCREEN_IDX4_W 320
#define SCREEN_IDX4_H 240
#define SCREEN_RGB333_W 160
#define SCREEN_RGB333_H 120

#define SCREEN_IDX4_W8 (SCREEN_IDX4_W/8)

#define gpu_p32 ((volatile uint32_t*)LPRS2_GPU_BASE)
#define palette_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x1000))
#define unpack_idx1_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x400000))
#define pack_idx1_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x600000))
#define unpack_idx4_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x800000))
#define pack_idx4_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0xa00000))
#define unpack_rgb333_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0xc00000))
#define joypad_p32 ((volatile uint32_t*)LPRS2_JOYPAD_BASE)

typedef struct {
	unsigned a : 1;
	unsigned b : 1;
	unsigned z : 1;
	unsigned start : 1;
	unsigned up : 1;
	unsigned down : 1;
	unsigned left : 1;
	unsigned right : 1;
} bf_joypad;
#define joypad (*((volatile bf_joypad*)LPRS2_JOYPAD_BASE))

typedef struct {
	uint32_t m[SCREEN_IDX1_H][SCREEN_IDX1_W];
} bf_unpack_idx1;
#define unpack_idx1 (*((volatile bf_unpack_idx1*)unpack_idx1_p32))

///////////////////////////////////////////////////////////////////////////////
// Game config.

#define STEP 1
#define PACMAN_ANIM_DELAY 3

///////////////////////////////////////////////////////////////////////////////
// Game data structures.

typedef struct {
	uint16_t x;
	uint16_t y;
} point_t;

typedef enum {
	PACMAN_IDLE,
	PACMAN_OPENING_MOUTH,
	PACMAN_WITH_OPEN_MOUTH,
	PACMAN_CLOSING_MOUTH,
	PACMAN_WITH_CLOSED_MOUTH
} pacman_anim_states_t;

typedef struct {
	pacman_anim_states_t state;
	uint8_t delay_cnt;
} pacman_anim_t;

typedef struct {
	point_t pos;
	pacman_anim_t anim;
} pacman_t;

typedef struct {
	pacman_t pacman;
} game_state_t;

void draw_sprite_from_atlas(
	uint16_t src_x,
	uint16_t src_y,
	uint16_t w,
	uint16_t h,
	uint16_t dst_x,
	uint16_t dst_y
) {


	for (uint16_t y = 0; y < h; y++) {
		for (uint16_t x = 0; x < w; x++) {
			uint32_t src_idx =
				(src_y + y) * Pacman_Sprite_Map__w +
				(src_x + x);
			uint32_t dst_idx =
				(((dst_y + y) % SCREEN_RGB333_H) * SCREEN_RGB333_W) +
				(dst_x + x) % SCREEN_RGB333_W;
			uint16_t pixel = Pacman_Sprite_Map__p[src_idx];
			unpack_rgb333_p32[dst_idx] = (pixel) ? pixel : unpack_rgb333_p32[dst_idx];
		}
	}


}

///////////////////////////////////////////////////////////////////////////////
// Game code.

int main(void) {

	// Setup.
	gpu_p32[0] = 3; // RGB333 mode.
	gpu_p32[0x800] = 0x00ff00ff; // Magenta for HUD.


	// Game state.
	game_state_t gs;
	gs.pacman.pos.x = 32;
	gs.pacman.pos.y = 16;
	gs.pacman.anim.state = PACMAN_IDLE;
	gs.pacman.anim.delay_cnt = 0;

	typedef enum { RIGHT, LEFT, UP, DOWN } direction;
	direction dir = RIGHT;


	//matrica za kopanje i za fiziku kod kamena
	int matrix[SCREEN_RGB333_W][SCREEN_RGB333_H];
	for (int w = 0;w < SCREEN_RGB333_W;w++) {
		for (int h = 0;h < SCREEN_RGB333_H;h++) {
			matrix[w][h] = 0;
		}
	}
	//matrica koja pokazuje gde se igrac nalazi u svakom trenutku
	int matrixAction[SCREEN_RGB333_W][SCREEN_RGB333_H];
	for (int w = 0;w < SCREEN_RGB333_W;w++) {
		for (int h = 0;h < SCREEN_RGB333_H;h++) {
			matrixAction[w][h] = 0;
		}
	}

	uint16_t d1 = 17;  //zmaj
	uint16_t d2 = 100;   //zmaj
	int k1 = 1, k2 = -1;
	uint16_t h1 = 1, h2 = 1; //hamburgeri
	uint16_t pobeda = 0;
	uint16_t end = 0;


	while (1) {
		for (int w = 0;w < SCREEN_RGB333_W;w++) {
			for (int h = 0;h < SCREEN_RGB333_H;h++) {
				matrixAction[w][h] = 0;
			}
		}

		/////////////////////////////////////
		// Poll controls.
		int mov_x = 0;
		int mov_y = 0;
		if (joypad.right) {
			mov_x = +1;
			dir = RIGHT;
		}
		else if (joypad.left) {
			mov_x = -1;
			dir = LEFT;

		}
		else if (joypad.up) {
			mov_y = -1;
			dir = UP;

		}
		else if (joypad.down) {
			mov_y = +1;
			dir = DOWN;

		}

		/////////////////////////////////////
		// Gameplay.

		int x = gs.pacman.pos.x + mov_x * STEP;
		int y = gs.pacman.pos.y + mov_y * STEP;

		if (matrix[x][y] != 2 && matrix[x + 15][y] != 2 && matrix[x + 15][y + 15] != 2 && matrix[x][y + 15] != 2) {
			if (x >= 0 && x <= 144) gs.pacman.pos.x = x;
			if (y >= 0 && y <= 104) gs.pacman.pos.y = y;
		}

		if (dir == DOWN) {
			for (int m = 0;m < 16;m++) {
				matrix[gs.pacman.pos.x + m][gs.pacman.pos.y + 15] = 1;
			}
		}
		if (dir == LEFT) {
			for (int m = 0;m < 16;m++) {
				matrix[gs.pacman.pos.x][gs.pacman.pos.y + m] = 1;
			}
		}
		if (dir == RIGHT) {
			for (int m = 0;m < 16;m++) {
				matrix[gs.pacman.pos.x + 15][gs.pacman.pos.y + m] = 1;
			}
		}
		if (dir == UP) {
			for (int m = 0;m < 16;m++) {
				matrix[gs.pacman.pos.x + m][gs.pacman.pos.y] = 1;
			}
		}

		switch (gs.pacman.anim.state) {
			case PACMAN_IDLE:
				if (mov_x != 0 || mov_y != 0) {
					gs.pacman.anim.delay_cnt = PACMAN_ANIM_DELAY;
					gs.pacman.anim.state = PACMAN_WITH_OPEN_MOUTH;
				}
				break;
			case PACMAN_OPENING_MOUTH:
				if (gs.pacman.anim.delay_cnt != 0) {
					gs.pacman.anim.delay_cnt--;
				}
				else {
					gs.pacman.anim.delay_cnt = PACMAN_ANIM_DELAY;
					gs.pacman.anim.state = PACMAN_WITH_OPEN_MOUTH;
				}
				break;
			case PACMAN_WITH_OPEN_MOUTH:
				if (gs.pacman.anim.delay_cnt != 0) {
					gs.pacman.anim.delay_cnt--;
				}
				else {
					if (mov_x != 0 || mov_y != 0) {
						gs.pacman.anim.delay_cnt = PACMAN_ANIM_DELAY;
						gs.pacman.anim.state = PACMAN_CLOSING_MOUTH;
					}
					else {
						gs.pacman.anim.state = PACMAN_IDLE;
					}
				}
				break;
			case PACMAN_CLOSING_MOUTH:
				if (gs.pacman.anim.delay_cnt != 0) {
					gs.pacman.anim.delay_cnt--;
				}
				else {
					gs.pacman.anim.delay_cnt = PACMAN_ANIM_DELAY;
					gs.pacman.anim.state = PACMAN_WITH_CLOSED_MOUTH;
				}
				break;
			case PACMAN_WITH_CLOSED_MOUTH:
				if (gs.pacman.anim.delay_cnt != 0) {
					gs.pacman.anim.delay_cnt--;
				}
				else {
					if (mov_x != 0 || mov_y != 0) {
						gs.pacman.anim.delay_cnt = PACMAN_ANIM_DELAY;
						gs.pacman.anim.state = PACMAN_OPENING_MOUTH;
					}
					else {
						gs.pacman.anim.state = PACMAN_IDLE;
					}
				}
				break;
		}

		/////////////////////////////////////
		// Drawing.

		// Detecting rising edge of VSync.
		WAIT_UNITL_0(gpu_p32[2]);
		WAIT_UNITL_1(gpu_p32[2]);
		// Draw in buffer while it is in VSync.

		// Black background.
		for (uint16_t r = 0; r < SCREEN_RGB333_H; r++) {
			for (uint16_t c = 0; c < SCREEN_RGB333_W; c++) {
				unpack_rgb333_p32[r * SCREEN_RGB333_W + c] = 0000;

			}
		}
		//CRTANJE TERENA//
		//nebo
		for (uint16_t i = 0; i < SCREEN_RGB333_W;i += 16) {
			draw_sprite_from_atlas(66, 65, 16, 16, i, 0);
		}
		for (uint16_t i = 0; i < SCREEN_RGB333_W;i += 16) {
			draw_sprite_from_atlas(66, 65, 16, 16, i, 16);
		}
		//prvi nivo zemlje
		for (uint16_t i = 0; i < SCREEN_RGB333_W;i += 16) {
			draw_sprite_from_atlas(34, 65, 16, 16, i, 32); 
		}
		for (uint16_t i = 0; i < SCREEN_RGB333_W;i += 16) {
			draw_sprite_from_atlas(34, 65, 16, 16, i, 48);
		}
		//drugi nivo zemlje
		for (uint16_t i = 0; i < SCREEN_RGB333_W;i += 16) {
			draw_sprite_from_atlas(18, 65, 16, 16, i, 64);
		}
		for (uint16_t i = 0; i < SCREEN_RGB333_W;i += 16) {
			draw_sprite_from_atlas(18, 65, 16, 16, i, 80);
		}
		//treci nivo zemlje
		for (uint16_t i = 0; i < SCREEN_RGB333_W;i += 16) {
			draw_sprite_from_atlas(2, 65, 16, 16, i, 96);
		}
		for (uint16_t i = 0; i < SCREEN_RGB333_W;i += 16) {
			draw_sprite_from_atlas(2, 65, 16, 8, i, 112);
		}


		//GDE SE NALAZI IGRAC
		for (int pacw = gs.pacman.pos.x + 3;pacw < gs.pacman.pos.x + 11;pacw++) {
			for (int pach = gs.pacman.pos.y + 3;pach < gs.pacman.pos.y + 11;pach++) {
				matrixAction[pacw][pach] = 1;
			}
		}

		//tuneli i hamburger
		for (uint16_t i = 0; i < 48;i += 16) {
			draw_sprite_from_atlas(66, 65, 16, 16, 16 + i, 96);
		}
		//JEDENJE HAMBURGERA//
		for (int hamw = 48;hamw < 48 + 15;hamw++) {
			for (int hamh = 96;hamh < 96 + 15;hamh++) {
				if (matrixAction[hamw][hamh] == 1) {
					h1 = 0;
				}
			}
		}
		if (h1) {
			draw_sprite_from_atlas(17, 207, 16, 16, 48, 96);
		}

		for (uint16_t i = 0; i < 48;i += 16) {
			draw_sprite_from_atlas(66, 65, 16, 16, 266 - i, 64);
		}

		for (int hamw = 74;hamw < 74 + 15;hamw++) {
			for (int hamh = 64;hamh < 64 + 15;hamh++) {
				if (matrixAction[hamw][hamh] == 1) {
					h2 = 0;
				}
			}
		}

		if (h2) {
			draw_sprite_from_atlas(17, 207, 16, 16, 74, 64);
		}
		if (h1 == 0 && h2 == 0) {
			pobeda = 1;
			end = 1;
		}
		//cvet
		draw_sprite_from_atlas(98, 65, 16, 16, 0, 16);
		draw_sprite_from_atlas(98, 65, 16, 16, 284, 16);
		draw_sprite_from_atlas(98, 65, 16, 16, 300, 16);

		//kamen
		draw_sprite_from_atlas(114, 65, 16, 16, 48, 48);
		draw_sprite_from_atlas(114, 65, 16, 16, 140, 80);
		for (int kamenw = 48;kamenw < 48 + 15;kamenw++) {
			for (int kamenh = 48;kamenh < 48 + 15;kamenh++) {
				matrix[kamenw][kamenh] = 2;
			}
		}
		for (int kamenw = 140;kamenw < 140 + 15;kamenw++) {
			for (int kamenh = 80;kamenh < 80 + 15;kamenh++) {
				matrix[kamenw][kamenh] = 2;
			}
		}
		// Draw pacman.

		//KOPA TUNELE//
		for (int w = 0;w < SCREEN_RGB333_W;w++) {
			for (int h = 0;h < SCREEN_RGB333_H;h++) {
				if (matrix[w][h] == 1) {
					draw_sprite_from_atlas(70, 73, 1, 1, w, h);
				}
			}
		}


		switch (gs.pacman.anim.state) {
			case PACMAN_IDLE:
			case PACMAN_OPENING_MOUTH:
			case PACMAN_CLOSING_MOUTH:
				// Half open mouth.
				draw_sprite_from_atlas(
					16, 16 * dir, 16, 16, gs.pacman.pos.x, gs.pacman.pos.y
				);
				break;
			case PACMAN_WITH_OPEN_MOUTH:
				// Full open mouth.
				draw_sprite_from_atlas(
					0, 16 * dir, 16, 16, gs.pacman.pos.x, gs.pacman.pos.y
				);
				break;
			case PACMAN_WITH_CLOSED_MOUTH:
				// Close mouth.
				draw_sprite_from_atlas(
					32, 16 * dir, 16, 16, gs.pacman.pos.x, gs.pacman.pos.y
				);
				break;
		}

		//zmajevi
		if (d1 <= 16 || d1 > 46)k1 = k1 * (-1);
		if (k1 == 1)
			draw_sprite_from_atlas(130, 65, 16, 16, d1, 96);
		else
			draw_sprite_from_atlas(146, 65, 16, 16, d1, 96);
		d1 = d1 + k1;

		for (int zmajw = d1;zmajw < d1 + 15;zmajw++) {
			for (int zmajh = 96;zmajh < 96 + 15;zmajh++) {
				if (matrixAction[zmajw][zmajh] == 1) {
					pobeda = 0;
					end = 1;
				}
			}
		}

		if (d2 < 74 || d2>106)k2 = k2 * (-1);
		if (k2 == -1)
			draw_sprite_from_atlas(146, 65, 16, 16, d2, 64);
		else
			draw_sprite_from_atlas(130, 65, 16, 16, d2, 64);
		d2 = d2 + k2;

		for (int zmajw = d2;zmajw < d2 + 15;zmajw++) {
			for (int zmajh = 64;zmajh < 64 + 15;zmajh++) {
				if (matrixAction[zmajw][zmajh] == 1) {
					pobeda = 0;
					end = 1;
				}
			}
		}

		if (end == 1) {
			if (pobeda) {
				draw_sprite_from_atlas(55, 8, 111, 25, 30, 50);
			}
			else {
				draw_sprite_from_atlas(55, 38, 140, 25, 10, 50);
			}
			while (1) {
				if (joypad.b) {
					end = 0;
					h1 = 1;
					h2 = 1;
					gs.pacman.pos.x = 32;
					gs.pacman.pos.y = 16;
					dir = RIGHT;
					for (int w = 0;w < SCREEN_RGB333_W;w++) {
						for (int h = 0;h < SCREEN_RGB333_H;h++) {
							matrix[w][h] = 0;
						}
					}
					break;
				}
			}
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
