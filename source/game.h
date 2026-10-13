#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#define MAP_W 32
#define MAP_H 32

// Metatile ids. Each metatile is 16x16 pixels (four 8x8 hardware tiles).
enum {
	MT_GRASS, MT_TALLGRASS, MT_TREE, MT_WATER, MT_PATH, MT_RUIN, MT_WALL,
	MT_GRACE, MT_FOG, MT_CHEST, MT_CHEST_OPEN, MT_SIGN, MT_CANOPY, MT_TRUNK,
	MT_BRIDGE, MT_SKY, MT_GROUND,
	// Rounded corners of the golden canopy, cut from MT_CANOPY at boot.
	MT_CANOPY_TL, MT_CANOPY_TR, MT_CANOPY_BL, MT_CANOPY_BR,
	// The same corners against the night sky, for the battle backdrop.
	MT_ORB_TL, MT_ORB_TR, MT_ORB_BL, MT_ORB_BR, MT_COUNT
};

// Sprite art ids.
enum { SPR_HERO_DOWN, SPR_HERO_UP, SPR_HERO_SIDE, SPR_EMBERS, SPR_COUNT };

// Enemy art ids (32x32 art, shown at 64x64).
enum { ART_HUSK, ART_HOUND, ART_KNIGHT, ART_BOSS, ART_COUNT };

typedef struct {
	const char *name;
	int hp, atk, def, embers;
	int art, pal;
} EnemyDef;

enum { EN_HUSK, EN_HOUND, EN_ROTTED, EN_KNIGHT, EN_BOSS, EN_COUNT };

typedef struct {
	int vig, str, mnd, end;
	int hp, fp;
	int flasks, maxFlasks;
	int embers;
	int weapon;            // bonus attack from the equipped weapon
	int graceX, graceY;    // where you wake after falling
	int lostEmbers, lostX, lostY;
	bool chestOpened[2];
	bool bossDefeated;
} Player;

extern Player pl;
extern const EnemyDef enemies[EN_COUNT];

// art.c
extern const unsigned short palette[16];
extern const unsigned short paletteRot[16];
extern const unsigned short paletteBoss[16];
extern const char *const metatileArt[MT_COUNT][16];
extern const char *const spriteArt[SPR_COUNT][16];
extern const char *const enemyArt[ART_COUNT][32];

// map.c
extern const char *const worldMap[MAP_H];
extern const int chestPos[2][2];
int tileAt(int x, int y);
bool tileSolid(int t);

// stats
int playerLevel(void);
int maxHP(void);
int maxFP(void);
int playerAtk(void);
int playerDef(void);
int levelCost(void);

// gfx.c
void gfxInit(void);
void drawWorld(void);
void drawBattleScene(void);
void setCamera(int x, int y);
void shakeScreen(int frames);
void heroSprite(int dir, int x, int y, bool show);
void embersSprite(int x, int y, bool show);
void enemySprite(int art, int pal, bool boss, bool show);
void battleHeroSprite(bool show);
void waitFrames(int n);

// ui.c
void uiClear(void);
void uiStatus(void);
void uiMessage(const char *msg);
void uiWaitA(void);
int uiMenu(int row, const char *const *items, int count, bool cancel);

// battle.c
enum { BATTLE_LOST, BATTLE_WON, BATTLE_FLED };
int battle(int enemyId);

#endif
