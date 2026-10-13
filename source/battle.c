// Turn-based battles in the classic style: pick a command, trade blows.

#include <nds.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "game.h"

#define ART_COST 12
#define FLASK_PCT 60

static const EnemyDef *foe;
static int foeHP;
static char logLines[3][32];

static void drawBattle(void)
{
	iprintf("\x1b[0;0H\x1b[33;1m %-30s\x1b[37;0m\n", foe->name);
	iprintf(" HP ");
	int width = 26, filled = (foeHP * width + foe->hp - 1) / foe->hp;
	for (int i = 0; i < width; i++)
		iprintf(i < filled ? "\x1b[31;1m#" : "\x1b[37;0m.");
	iprintf("\x1b[37;0m\n--------------------------------\n");
	iprintf(" The Faded            Lv %3d\n", playerLevel());
	iprintf(" HP %3d/%3d   FP %3d/%3d      \n", pl.hp, maxHP(), pl.fp, maxFP());
	iprintf(" Flasks %d/%d                    \n", pl.flasks, pl.maxFlasks);
	iprintf("--------------------------------");
	iprintf("\x1b[14;0H--------------------------------");
	for (int i = 0; i < 3; i++)
		iprintf("\x1b[%d;0H %-31s", 15 + i, logLines[i]);
}

static void say(const char *fmt, ...)
{
	for (int i = 0; i < 2; i++)
		sprintf(logLines[i], "%s", logLines[i + 1]);
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(logLines[2], sizeof(logLines[2]), fmt, ap);
	va_end(ap);
	drawBattle();
	// Short pause; A skips it.
	for (int i = 0; i < 40; i++) {
		waitFrames(1);
		scanKeys();
		if (keysDown() & KEY_A) break;
	}
}

static void flicker(bool enemy)
{
	for (int i = 0; i < 10; i++) {
		if (enemy)
			enemySprite(foe->art, foe->pal, foe == &enemies[EN_BOSS], i & 1);
		else
			battleHeroSprite(i & 1);
		waitFrames(2);
	}
	enemySprite(foe->art, foe->pal, foe == &enemies[EN_BOSS], foeHP > 0);
	battleHeroSprite(true);
}

static int rollDamage(int atk, int def)
{
	int dmg = atk + rand() % (atk / 4 + 1) - def;
	return dmg < 1 ? 1 : dmg;
}

static void hurtPlayer(int dmg)
{
	shakeScreen(10);
	flicker(false);
	pl.hp -= dmg;
	if (pl.hp < 0) pl.hp = 0;
}

int battle(int enemyId)
{
	foe = &enemies[enemyId];
	foeHP = foe->hp;
	bool boss = enemyId == EN_BOSS;
	for (int i = 0; i < 3; i++) logLines[i][0] = 0;

	heroSprite(0, 0, 0, false);
	embersSprite(0, 0, false);
	drawBattleScene();
	enemySprite(foe->art, foe->pal, boss, true);
	battleHeroSprite(true);
	uiClear();
	drawBattle();
	if (boss)
		say("The warden bars the way!");
	else
		say("A %s draws near!", foe->name);

	int turn = 0;
	bool charging = false;
	for (;;) {
		static const char *const cmds[] = {
			"Attack", "Storm Art  (12 FP)", "Guard", "Flask", "Flee",
		};
		drawBattle();
		int cmd = uiMenu(8, cmds, 5, false);
		for (int r = 8; r < 13; r++) iprintf("\x1b[%d;0H%32s", r, "");
		bool guarding = false;

		switch (cmd) {
		case 0: {
			int dmg = rollDamage(playerAtk(), foe->def);
			flicker(true);
			foeHP -= dmg;
			say("You strike for %d.", dmg);
			break;
		}
		case 1:
			if (pl.fp < ART_COST) {
				say("Not enough FP.");
				continue;
			}
			pl.fp -= ART_COST;
			{
				// The storm cuts through armour and hits hard.
				int dmg = rollDamage(playerAtk() * 2, 0);
				flicker(true);
				foeHP -= dmg;
				say("Storm Art! %d damage.", dmg);
			}
			break;
		case 2:
			guarding = true;
			pl.fp += 6;
			if (pl.fp > maxFP()) pl.fp = maxFP();
			say("You raise your guard. +6 FP");
			break;
		case 3:
			if (pl.flasks == 0) {
				say("Your flask is empty.");
				continue;
			}
			pl.flasks--;
			{
				int heal = maxHP() * FLASK_PCT / 100;
				if (pl.hp + heal > maxHP()) heal = maxHP() - pl.hp;
				pl.hp += heal;
				say("You drink. +%d HP", heal);
			}
			break;
		case 4:
			if (boss) {
				say("There is no escape.");
				break;
			}
			if (rand() % 100 < 65) {
				say("You slip away.");
				return BATTLE_FLED;
			}
			say("You could not escape!");
			break;
		}

		if (foeHP <= 0) {
			foeHP = 0;
			enemySprite(foe->art, foe->pal, boss, false);
			say(boss ? "The warden falls." : "The %s falls.", foe->name);
			pl.embers += foe->embers;
			say("Gained %d embers.", foe->embers);
			return BATTLE_WON;
		}

		// Foe's turn.
		turn++;
		int def = playerDef() * (guarding ? 3 : 1);
		if (boss && charging) {
			charging = false;
			say("Grafted Flurry!");
			for (int hit = 0; hit < 3 && pl.hp > 0; hit++) {
				int dmg = rollDamage(foe->atk * 2 / 3, def);
				if (guarding) dmg /= 2;
				hurtPlayer(dmg);
				say("Hit %d: %d damage.", hit + 1, dmg);
			}
		} else if (boss && turn % 3 == 0) {
			charging = true;
			say("The warden raises many arms...");
		} else {
			int dmg = rollDamage(foe->atk, def);
			hurtPlayer(dmg);
			say("%s hits you for %d.", boss ? "The warden" : "It", dmg);
		}

		if (pl.hp <= 0)
			return BATTLE_LOST;
	}
}
