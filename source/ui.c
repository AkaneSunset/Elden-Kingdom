// Bottom screen: status, messages and menus, drawn with the text console.

#include <nds.h>
#include <stdio.h>
#include <string.h>
#include "game.h"

#define GOLD  "\x1b[33;1m"
#define RED   "\x1b[31;1m"
#define BLUE  "\x1b[36;1m"
#define GREY  "\x1b[37;0m"
#define WHITE "\x1b[37;1m"

void uiClear(void)
{
	consoleClear();
}

static void bar(int value, int max, int width, const char *color)
{
	int filled = max > 0 ? (value * width + max - 1) / max : 0;
	for (int i = 0; i < width; i++) {
		if (i < filled)
			iprintf("%s=", color);
		else
			iprintf(GREY "-");
	}
	iprintf(GREY);
}

void uiStatus(void)
{
	iprintf("\x1b[0;0H" GOLD " ELDEN KINGDOM" GREY "          Lv %3d\n", playerLevel());
	iprintf(" HP %3d/%3d ", pl.hp, maxHP());
	bar(pl.hp, maxHP(), 16, RED);
	iprintf("\n FP %3d/%3d ", pl.fp, maxFP());
	bar(pl.fp, maxFP(), 16, BLUE);
	iprintf("\n Flasks %d/%d     Embers %7d\n", pl.flasks, pl.maxFlasks, pl.embers);
	iprintf(GREY "--------------------------------");
}

// Word-wraps a message into the area under the status block.
void uiMessage(const char *msg)
{
	for (int r = 6; r < 24; r++)
		iprintf("\x1b[%d;0H%31s", r, "");
	int row = 7, col = 1;
	iprintf("\x1b[%d;%dH" WHITE, row, col);
	const char *p = msg;
	while (*p) {
		if (*p == '\n') {
			row++; col = 1; p++;
			iprintf("\x1b[%d;%dH", row, col);
			continue;
		}
		int len = strcspn(p, " \n");
		if (col + len > 31) {
			row++; col = 1;
			iprintf("\x1b[%d;%dH", row, col);
		}
		iprintf("%.*s", len, p);
		col += len;
		p += len;
		if (*p == ' ') {
			if (col < 31) iprintf(" ");
			col++; p++;
		}
	}
	iprintf(GREY);
}

void uiWaitA(void)
{
	iprintf("\x1b[23;26H" GOLD "(A)");
	for (;;) {
		waitFrames(1);
		scanKeys();
		if (keysDown() & (KEY_A | KEY_B)) break;
	}
	iprintf("\x1b[23;26H    ");
}

// Vertical menu starting at `row`. Returns the chosen index, or -1 on B.
int uiMenu(int row, const char *const *items, int count, bool cancel)
{
	int sel = 0;
	for (;;) {
		for (int i = 0; i < count; i++)
			iprintf("\x1b[%d;1H%s%s %-28s" GREY, row + i,
				i == sel ? GOLD : GREY, i == sel ? ">" : " ", items[i]);
		waitFrames(1);
		scanKeys();
		int k = keysDown();
		if (k & KEY_UP) sel = (sel + count - 1) % count;
		if (k & KEY_DOWN) sel = (sel + 1) % count;
		if (k & KEY_A) return sel;
		if (cancel && (k & KEY_B)) return -1;
	}
}
