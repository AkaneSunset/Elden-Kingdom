// Top screen: the tiled world, battle backdrop and all sprites.
// Art is converted from the text in art.c into 4bpp tiles at boot.

#include <nds.h>
#include <string.h>
#include "game.h"

#define PALETTE_KEY ".ksSgGtbdwWYyrpL"

// OAM slots.
enum { OAM_HERO, OAM_EMBERS, OAM_ENEMY, OAM_BATTLE_HERO };

static int bg;
static int camX, camY;
static u16 *heroGfx[3];
static u16 *embersGfx;
static u16 *enemyGfx[ART_COUNT];

static int colorIndex(char c)
{
	const char *p = strchr(PALETTE_KEY, c);
	return p ? p - PALETTE_KEY : 0;
}

// Packs a w*h block of art (scaled by `scale`) into 1D-mapped 4bpp tiles.
static void packTiles(u32 *dst, const char *const *rows, int w, int h, int scale)
{
	int tw = w * scale / 8, th = h * scale / 8;
	for (int ty = 0; ty < th; ty++)
		for (int tx = 0; tx < tw; tx++)
			for (int y = 0; y < 8; y++) {
				u32 line = 0;
				for (int x = 0; x < 8; x++) {
					int px = (tx * 8 + x) / scale, py = (ty * 8 + y) / scale;
					line |= (u32)colorIndex(rows[py][px]) << (x * 4);
				}
				*dst++ = line;
			}
}

// Fills `px` with a metatile's 16x16 art. Canopy corners are the canopy
// art clipped to a quarter circle, with grass showing outside it.
static void metatilePixels(int mt, char px[16][16])
{
	if (mt < MT_CANOPY_TL) {
		for (int y = 0; y < 16; y++)
			for (int x = 0; x < 16; x++)
				px[y][x] = metatileArt[mt][y][x];
		return;
	}
	int corner = (mt - MT_CANOPY_TL) & 3;
	int outside = mt >= MT_ORB_TL ? MT_SKY : MT_GRASS;
	int cx = (corner & 1) ? 0 : 16, cy = (corner & 2) ? 0 : 16;
	for (int y = 0; y < 16; y++)
		for (int x = 0; x < 16; x++) {
			int dx = 2 * x + 1 - 2 * cx, dy = 2 * y + 1 - 2 * cy;
			int d2 = dx * dx + dy * dy;   // in half-pixels
			if (d2 > 32 * 32)
				px[y][x] = metatileArt[outside][y][x];
			else if (d2 > 29 * 29)
				px[y][x] = 'y';
			else
				px[y][x] = metatileArt[MT_CANOPY][y][x];
		}
}

// The map wants each quarter of a metatile as its own 8x8 tile.
static void packMetatile(u32 *dst, int mt)
{
	char px[16][16];
	metatilePixels(mt, px);
	for (int q = 0; q < 4; q++) {
		int ox = (q & 1) * 8, oy = (q >> 1) * 8;
		for (int y = 0; y < 8; y++) {
			u32 line = 0;
			for (int x = 0; x < 8; x++)
				line |= (u32)colorIndex(px[oy + y][ox + x]) << (x * 4);
			*dst++ = line;
		}
	}
}

void gfxInit(void)
{
	videoSetMode(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankB(VRAM_B_MAIN_SPRITE);

	bg = bgInit(0, BgType_Text4bpp, BgSize_T_512x512, 0, 1);
	dmaCopy(palette, BG_PALETTE, sizeof(palette));

	u32 *tiles = (u32 *)bgGetGfxPtr(bg);
	for (int i = 0; i < MT_COUNT; i++)
		packMetatile(tiles + i * 4 * 8, i);

	oamInit(&oamMain, SpriteMapping_1D_128, false);
	dmaCopy(palette, SPRITE_PALETTE, sizeof(palette));
	dmaCopy(paletteRot, SPRITE_PALETTE + 16, sizeof(paletteRot));
	dmaCopy(paletteBoss, SPRITE_PALETTE + 32, sizeof(paletteBoss));

	static u32 buf[64 * 64 / 8];
	for (int i = 0; i < 3; i++) {
		heroGfx[i] = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
		packTiles(buf, spriteArt[SPR_HERO_DOWN + i], 16, 16, 1);
		dmaCopy(buf, heroGfx[i], 16 * 16 / 2);
	}
	embersGfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
	packTiles(buf, spriteArt[SPR_EMBERS], 16, 16, 1);
	dmaCopy(buf, embersGfx, 16 * 16 / 2);

	for (int i = 0; i < ART_COUNT; i++) {
		enemyGfx[i] = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_16Color);
		packTiles(buf, enemyArt[i], 32, 32, 2);
		dmaCopy(buf, enemyGfx[i], 64 * 64 / 2);
	}
	// Affine matrix 0 doubles the boss to 128x128. Matrix 1 doubles the
	// hero in battle and mirrors him to face left.
	oamAffineTransformation(&oamMain, 0, 128, 0, 0, 128);
	oamAffineTransformation(&oamMain, 1, -128, 0, 0, 128);
}

static void setMetatile(u16 *map, int mx, int my, int mt)
{
	for (int q = 0; q < 4; q++) {
		int tx = mx * 2 + (q & 1), ty = my * 2 + (q >> 1);
		// A 512x512 map is four 32x32 screen blocks laid out left-right, top-bottom.
		int block = (tx >> 5) + (ty >> 5) * 2;
		map[block * 1024 + (ty & 31) * 32 + (tx & 31)] = mt * 4 + q;
	}
}

void drawWorld(void)
{
	u16 *map = bgGetMapPtr(bg);
	for (int y = 0; y < MAP_H; y++)
		for (int x = 0; x < MAP_W; x++)
			setMetatile(map, x, y, tileAt(x, y));
}

void drawBattleScene(void)
{
	u16 *map = bgGetMapPtr(bg);
	for (int y = 0; y < 12; y++)
		for (int x = 0; x < 16; x++) {
			int mt = y < 7 ? MT_SKY : MT_GROUND;
			// A golden tree glows on the horizon.
			if (y == 4 && x == 11) mt = MT_ORB_TL;
			if (y == 4 && x == 12) mt = MT_ORB_TR;
			if (y == 5 && x == 11) mt = MT_ORB_BL;
			if (y == 5 && x == 12) mt = MT_ORB_BR;
			setMetatile(map, x, y, mt);
		}
	bgSetScroll(bg, 0, 0);
	bgUpdate();
}

void setCamera(int x, int y)
{
	camX = x < 0 ? 0 : x > 512 - 256 ? 512 - 256 : x;
	camY = y < 0 ? 0 : y > 512 - 192 ? 512 - 192 : y;
	bgSetScroll(bg, camX, camY);
	bgUpdate();
}

void shakeScreen(int frames)
{
	for (int i = 0; i < frames; i++) {
		bgSetScroll(bg, (i & 1) ? 3 : -3, (i & 2) ? 2 : 0);
		bgUpdate();
		swiWaitForVBlank();
	}
	bgSetScroll(bg, 0, 0);
	bgUpdate();
}

// dir: 0 down, 1 up, 2 left, 3 right. x/y are world pixels.
void heroSprite(int dir, int x, int y, bool show)
{
	int frame = dir < 2 ? dir : 2;
	oamSet(&oamMain, OAM_HERO, x - camX, y - camY, 0, 0, SpriteSize_16x16,
		SpriteColorFormat_16Color, heroGfx[frame], -1, false, !show, dir == 2, false, false);
}

void embersSprite(int x, int y, bool show)
{
	int sx = x - camX, sy = y - camY;
	bool onScreen = sx > -16 && sx < 256 && sy > -16 && sy < 192;
	oamSet(&oamMain, OAM_EMBERS, sx & 511, sy & 255, 0, 0, SpriteSize_16x16,
		SpriteColorFormat_16Color, embersGfx, -1, false, !(show && onScreen), false, false, false);
}

void enemySprite(int art, int pal, bool boss, bool show)
{
	if (boss)
		oamSet(&oamMain, OAM_ENEMY, 8, 8, 0, pal, SpriteSize_64x64,
			SpriteColorFormat_16Color, enemyGfx[art], 0, true, !show, false, false, false);
	else
		oamSet(&oamMain, OAM_ENEMY, 40, 66, 0, pal, SpriteSize_64x64,
			SpriteColorFormat_16Color, enemyGfx[art], -1, false, !show, false, false, false);
}

void battleHeroSprite(bool show)
{
	// Party stands on the right facing left, like the classics.
	oamSet(&oamMain, OAM_BATTLE_HERO, 184, 100, 0, 0, SpriteSize_16x16,
		SpriteColorFormat_16Color, heroGfx[2], 1, true, !show, false, false, false);
}

void waitFrames(int n)
{
	while (n--) {
		swiWaitForVBlank();
		oamUpdate(&oamMain);
	}
}
