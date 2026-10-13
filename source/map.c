#include "game.h"

// The Weeping Fields, south of Ashveil Keep. One character per 16x16 tile:
//   .  grass       ,  tall grass (foes lurk)   T  trees      ~  river
//   =  road        B  bridge                   R  ruins      #  keep wall
//   L  lantern of grace                        F  fog gate   C  chest
//   S  signpost    E  golden canopy            I  golden trunk
//   < > [ ]  rounded canopy corners (top-left, top-right, bottom-left, bottom-right)
const char *const worldMap[MAP_H] = {
	"TTTTTTTTT##############TTTTTTTTT",
	"TTTTTTTTT#RRRRRRRRRRRR#TTTTTTTTT",
	"TTTTTTTTT#RRRRRRRRRRRR#TTTTTTTTT",
	"TTTTTTTTT#RRRRRRRRRRRR#TTTTTTTTT",
	"TTTTTTTTT######FF######TTTTTTTTT",
	"TTTTTTTTT,,,,,,==,,,,,,TTTTTTTTT",
	"TTTTTTTT,,,,,,,==,,,,,,,TTTTTTTT",
	"TTTTTTT,,,,R,,.==L,,R,,,,TTTTTTT",
	"TTTTTT,,,,,,,,.==.,,,,,,,,TTTTTT",
	"TTTTTT,,R,,,,,.==.,,,,,R,,TTTTTT",
	"~~~~~~~~~~~~~~~BB~~~~~~~~~~~~~~~",
	"~~~~~~~~~~~~~~~BB~~~~~~~~~~~~~~~",
	"TT....,,,,,,...==...,,,,<E>..TTT",
	"TT..C.,,,,,,...==...,,,,[E]...TT",
	"TT...,,,,,,,...==....,,..I....TT",
	"TT..,,,,,,,,..L==.....,..I.,,,TT",
	"TT..,,,,,,,,...==......,,,,,,,TT",
	"TTT..,,,,,,....==.....,,,,,,,.TT",
	"TTTT...,,,,....==....,,,,,..TTTT",
	"TTTTT..........==.........TTTTTT",
	"TTTTTTT...TT...==...TT...TTTTTTT",
	"TTTTTT,,,,,TT..==..TT,,,,,TTTTTT",
	"TTTTT,,,,,,,,..==..,,,,,,,,TTTTT",
	"TTTT,,,,R,,,,..==..,,,,R,,,,TTTT",
	"TTTT,,,,,,,,...==...,,,,,,,CTTTT",
	"TTTT,,,,,,,,...==...,,,,,,,,TTTT",
	"TTTTT,,,,,,....==....,,,,,,TTTTT",
	"TTTTTT........S==.........TTTTTT",
	"TTTTTTT.......L==........TTTTTTT",
	"TTTTTTTT..........~~~...TTTTTTTT",
	"TTTTTTTTTT.......~~~~TTTTTTTTTTT",
	"TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
};

// Chest locations, matching Player.chestOpened.
const int chestPos[2][2] = { { 4, 13 }, { 27, 24 } };

int tileAt(int x, int y)
{
	if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H)
		return MT_TREE;

	switch (worldMap[y][x]) {
	case ',': return MT_TALLGRASS;
	case 'T': return MT_TREE;
	case '~': return MT_WATER;
	case '=': return MT_PATH;
	case 'B': return MT_BRIDGE;
	case 'R': return MT_RUIN;
	case '#': return MT_WALL;
	case 'L': return MT_GRACE;
	case 'F': return pl.bossDefeated ? MT_PATH : MT_FOG;
	case 'S': return MT_SIGN;
	case 'E': return MT_CANOPY;
	case 'I': return MT_TRUNK;
	case '<': return MT_CANOPY_TL;
	case '>': return MT_CANOPY_TR;
	case '[': return MT_CANOPY_BL;
	case ']': return MT_CANOPY_BR;
	case 'C':
		for (int i = 0; i < 2; i++)
			if (chestPos[i][0] == x && chestPos[i][1] == y && pl.chestOpened[i])
				return MT_CHEST_OPEN;
		return MT_CHEST;
	default: return MT_GRASS;
	}
}

bool tileSolid(int t)
{
	switch (t) {
	case MT_TREE: case MT_WATER: case MT_RUIN: case MT_WALL:
	case MT_CANOPY: case MT_TRUNK: case MT_SIGN:
	case MT_CANOPY_TL: case MT_CANOPY_TR: case MT_CANOPY_BL: case MT_CANOPY_BR:
	case MT_CHEST: case MT_CHEST_OPEN:
		return true;
	default:
		return false;
	}
}
