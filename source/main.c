// Elden Kingdom: a small dark fantasy RPG for the Nintendo DS.
// Explore the Weeping Fields on the top screen, read menus on the bottom.

#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

Player pl;

const EnemyDef enemies[EN_COUNT] = {
	[EN_HUSK]   = { "Wandering Husk",           34, 14,  2,   45, ART_HUSK,   0 },
	[EN_HOUND]  = { "Grave Hound",              48, 18,  4,   70, ART_HOUND,  0 },
	[EN_ROTTED] = { "Rotted Husk",              80, 24,  7,  130, ART_HUSK,   1 },
	[EN_KNIGHT] = { "Keep Knight",             130, 30, 12,  240, ART_KNIGHT, 0 },
	[EN_BOSS]   = { "Ser Vael, Grafted Warden", 480, 42, 14, 5000, ART_BOSS,   2 },
};

int playerLevel(void) { return 1 + pl.vig + pl.str + pl.mnd + pl.end - 40; }
int maxHP(void)       { return 40 + pl.vig * 10; }
int maxFP(void)       { return 10 + pl.mnd * 3; }
int playerAtk(void)   { return 10 + pl.str * 2 + pl.weapon; }
int playerDef(void)   { return pl.end; }
int levelCost(void)   { return 30 + 25 * playerLevel(); }

// Hero position in tiles, facing (0 down, 1 up, 2 left, 3 right).
static int hx, hy, hdir;

static void newGame(void)
{
	memset(&pl, 0, sizeof(pl));
	pl.vig = pl.str = pl.mnd = pl.end = 10;
	pl.maxFlasks = 3;
	pl.graceX = 14;
	pl.graceY = 28;
	pl.lostEmbers = 0;
	hx = 15; hy = 28; hdir = 1;
#ifdef DEBUG_START
	// Dev shortcut (make DEBUG=1): start by the keep with embers to spend.
	hx = 16; hy = 8;
	pl.graceX = 17; pl.graceY = 7;
	pl.embers = 20000;
	pl.str = 40;
	pl.end = 30;
#endif
}

static void restoreAll(void)
{
	pl.hp = maxHP();
	pl.fp = maxFP();
	pl.flasks = pl.maxFlasks;
}

static void placeHero(int ox, int oy, int bob)
{
	int wx = hx * 16 + ox, wy = hy * 16 + oy;
	setCamera(wx - 120, wy - 88);
	heroSprite(hdir, wx, wy - bob, true);
	embersSprite(pl.lostX * 16, pl.lostY * 16, pl.lostEmbers > 0);
}

static void showWorld(void)
{
	drawWorld();
	enemySprite(0, 0, false, false);
	battleHeroSprite(false);
	placeHero(0, 0, 0);
	uiClear();
	uiStatus();
}

static void say(const char *msg)
{
	uiStatus();
	uiMessage(msg);
	uiWaitA();
	uiMessage("");
}

static void levelUp(void)
{
	for (;;) {
		char items[5][32];
		const char *ptrs[5];
		snprintf(items[0], 32, "Vigor     %2d  (HP)", pl.vig);
		snprintf(items[1], 32, "Strength  %2d  (attack)", pl.str);
		snprintf(items[2], 32, "Mind      %2d  (FP)", pl.mnd);
		snprintf(items[3], 32, "Endurance %2d  (defense)", pl.end);
		snprintf(items[4], 32, "Done");
		for (int i = 0; i < 5; i++) ptrs[i] = items[i];

		uiStatus();
		char head[96];
		snprintf(head, sizeof(head), "Offer %d embers to grow stronger.", levelCost());
		uiMessage(head);
		int c = uiMenu(10, ptrs, 5, true);
		if (c < 0 || c == 4) return;
		if (pl.embers < levelCost()) {
			say("You lack the embers.");
			continue;
		}
		pl.embers -= levelCost();
		int *stat[] = { &pl.vig, &pl.str, &pl.mnd, &pl.end };
		(*stat[c])++;
		restoreAll();
	}
}

static void restAtGrace(void)
{
	pl.graceX = hx;
	pl.graceY = hy;
	restoreAll();
	say("You rest by the lantern of grace. Your wounds close and your flasks refill.");
	for (;;) {
		static const char *const opts[] = { "Level up", "Leave" };
		uiStatus();
		uiMessage("The lantern burns softly.");
		int c = uiMenu(10, opts, 2, true);
		if (c == 0) levelUp();
		else break;
	}
	uiClear();
	uiStatus();
}

static void fallen(void)
{
	heroSprite(0, 0, 0, false);
	enemySprite(0, 0, false, false);
	battleHeroSprite(false);
	uiClear();
	iprintf("\x1b[9;8H\x1b[31;1mYOU HAVE FALLEN\x1b[37;0m");
	if (pl.embers > 0) {
		iprintf("\x1b[12;3H%d embers left where you fell.", pl.embers);
		pl.lostEmbers = pl.embers;
		pl.lostX = hx;
		pl.lostY = hy;
		pl.embers = 0;
	}
	uiWaitA();
	hx = pl.graceX;
	hy = pl.graceY;
	hdir = 0;
	restoreAll();
	showWorld();
	say("You wake by the lantern of grace.");
}

static void ending(void)
{
	pl.bossDefeated = true;
	showWorld();
	heroSprite(0, 0, 0, false);
	uiClear();
	iprintf("\x1b[2;3H\x1b[33;1mThe warden of Ashveil Keep\x1b[37;0m");
	iprintf("\x1b[3;3H\x1b[33;1mis no more.\x1b[37;0m");
	iprintf("\x1b[6;1HThe fog lifts from the gate.");
	iprintf("\x1b[7;1HFar to the east, the golden");
	iprintf("\x1b[8;1Htree flares once, as if it");
	iprintf("\x1b[9;1Hnoticed.");
	iprintf("\x1b[11;1HYour road goes on, Faded One.");
	iprintf("\x1b[14;1HThanks for playing this first");
	iprintf("\x1b[15;1Hchapter of Elden Kingdom.");
	iprintf("\x1b[18;1HLevel %d   Embers %d", playerLevel(), pl.embers);
	uiWaitA();
}

// Returns true when the game is over and we should go back to the title.
static bool afterBattle(int result)
{
	if (result == BATTLE_LOST) {
		fallen();
		return false;
	}
	showWorld();
	return false;
}

static int pickFoe(void)
{
	int r = rand() % 100;
	if (hy >= 20) return r < 60 ? EN_HUSK : EN_HOUND;
	if (hy >= 12) return r < 55 ? EN_HOUND : EN_ROTTED;
	return r < 50 ? EN_ROTTED : EN_KNIGHT;
}

// Called each time the hero finishes a step. Returns true on game end.
static bool onStep(void)
{
	if (pl.lostEmbers > 0 && hx == pl.lostX && hy == pl.lostY) {
		pl.embers += pl.lostEmbers;
		char msg[64];
		snprintf(msg, sizeof(msg), "You recover %d embers.", pl.lostEmbers);
		pl.lostEmbers = 0;
		embersSprite(0, 0, false);
		say(msg);
	}

	switch (tileAt(hx, hy)) {
	case MT_GRACE:
		restAtGrace();
		break;
	case MT_FOG: {
		static const char *const opts[] = { "Pass through the fog", "Turn back" };
		uiStatus();
		uiMessage("A wall of fog. Something vast waits beyond.");
		int c = uiMenu(10, opts, 2, true);
		uiMessage("");
		if (c != 0) {
			hy++;
			hdir = 0;
			placeHero(0, 0, 0);
			break;
		}
		int result = battle(EN_BOSS);
		if (result == BATTLE_WON) {
			ending();
			return true;
		}
		fallen();
		break;
	}
	case MT_TALLGRASS:
		if (rand() % 8 == 0)
			return afterBattle(battle(pickFoe()));
		break;
	}
	return false;
}

static void interact(void)
{
	static const int dx[] = { 0, 0, -1, 1 }, dy[] = { 1, -1, 0, 0 };
	int tx = hx + dx[hdir], ty = hy + dy[hdir];
	int t = tileAt(tx, ty);

	if (t == MT_SIGN) {
		say("\"Tall grass hides the hungry dead. "
			"Rest at a lantern of grace to heal and to grow stronger. "
			"Ashveil Keep lies north, past the river.\"");
	} else if (t == MT_CHEST) {
		for (int i = 0; i < 2; i++) {
			if (chestPos[i][0] != tx || chestPos[i][1] != ty) continue;
			pl.chestOpened[i] = true;
			drawWorld();
			if (i == 0) {
				pl.maxFlasks++;
				pl.flasks++;
				say("Found an Amber Seed. Your flask holds one more draught.");
			} else {
				pl.weapon += 10;
				say("Found the Knight's Greatsword. Attack +10.");
			}
		}
	} else if (t == MT_CHEST_OPEN) {
		say("The chest is empty.");
	}
}

static void title(void)
{
	drawWorld();
	heroSprite(0, 0, 0, false);
	embersSprite(0, 0, false);
	enemySprite(0, 0, false, false);
	battleHeroSprite(false);
	setCamera(25 * 16 - 128, 13 * 16 - 96);
	uiClear();
	iprintf("\x1b[4;3H\x1b[33;0m. - * - . - * - . - * - .");
	iprintf("\x1b[6;10H\x1b[33;1mE L D E N");
	iprintf("\x1b[8;8HK I N G D O M");
	iprintf("\x1b[10;3H\x1b[33;0m. - * - . - * - . - * - .\x1b[37;0m");
	iprintf("\x1b[15;8HPress START\n");
	iprintf("\x1b[22;1HA homebrew RPG for Nintendo DS");

	unsigned seed = 0;
	for (;;) {
		waitFrames(1);
		seed++;
		scanKeys();
		if (keysDown() & (KEY_START | KEY_A)) break;
	}
	srand(seed);
}

static void prologue(void)
{
	showWorld();
	say("The Elden Kingdom has shattered. Its golden tree withers, and its lords "
		"have grafted strength from the fallen until nothing human remains.");
	say("You are one of the Faded, called back from death by a light you cannot name. "
		"North, behind the river, Ashveil Keep holds the road to the tree.");
	say("D-pad: walk.  A: talk, open, confirm.  B: back.  "
		"Lanterns of grace heal you and let you level up.");
}

int main(void)
{
	gfxInit();
	consoleDemoInit();

	for (;;) {
		title();
		newGame();
		restoreAll();
		prologue();

		bool over = false;
		while (!over) {
			placeHero(0, 0, 0);
			waitFrames(1);
			scanKeys();
			int held = keysHeld(), down = keysDown();

			if (down & KEY_A) {
				interact();
				continue;
			}

			int dir = -1;
			if (held & KEY_DOWN) dir = 0;
			else if (held & KEY_UP) dir = 1;
			else if (held & KEY_LEFT) dir = 2;
			else if (held & KEY_RIGHT) dir = 3;
			if (dir < 0) continue;

			hdir = dir;
			static const int dx[] = { 0, 0, -1, 1 }, dy[] = { 1, -1, 0, 0 };
			if (tileSolid(tileAt(hx + dx[dir], hy + dy[dir]))) continue;

			// Glide one tile over 8 frames with a little bob.
			for (int f = 1; f <= 8; f++) {
				placeHero(dx[dir] * f * 2, dy[dir] * f * 2, (f >= 3 && f <= 6) ? 1 : 0);
				waitFrames(1);
			}
			hx += dx[dir];
			hy += dy[dir];
			placeHero(0, 0, 0);
			over = onStep();
			uiStatus();
		}
	}
}
