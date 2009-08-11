/*****************************************************************************/
/*  Neko Project II Wii                                                      */
/* ------------------------------------------------------------------------- */
/*  menu.c                                                                   */
/*  The Wii specific menu code. Should be launchable from any point in the   */
/*  emulator.                                                                */
/*****************************************************************************/

#include	"compiler.h"
#include	"np2.h"
// #include	<signal.h>
#include	"inputmng.h"
#include	"taskmng.h"
#include	"sdlkbd.h"
#include	"vramhdl.h"
#include	"menu.h"
#include	"sysmenu.h"
#include	"diskdrv.h"
#include	"scrnmng.h"
#include	"menubase.h"
#include	"pccore.h"
#include	"keystat.h"
#include	"sysmng.h"
#include	"pc9861k.h"
#include	"mpu98ii.h"
#include	"iocore.h"

#include	<SDL/SDL.h>
#include	"SFont.h"
#include	<fat.h>
#include	<dirent.h>
#include	"log_console.h"
#include	<wiiuse/wpad.h>
#include	<wiiuse/wiiuse.h>
#include	<math.h>

#ifdef DEBUG_NP2
extern FILE* logfp;
#endif

#define CONWIDTH	(64)
#define CONHEIGHT	(16)
#define HEIGHT_BEGIN 	( 6)
#define MAX_ENTRIES	(CONHEIGHT - HEIGHT_BEGIN)
#define	CONTROL_COUNT	(11)

#define FILETYPE_UNKNOWN	0
#define FILETYPE_HDD_HDI	1
#define FILETYPE_FDD_D88	2
#define FILETYPE_FDD_MTR	3
#define FILETYPE_FDD_XDF	4
#define FILETYPE_COUNT		5

#define BASETYPE_UNKNOWN	0
#define BASETYPE_HDD		1
#define BASETYPE_FDD		2
#define BASETYPE_COUNT		3

extern int base_control_map[];
extern int vert_wiimote_map[];
extern int hori_wiimote_map[];

extern void sys_cmd(MENUID id);

int started = 0;

extern int time_to_leave;

typedef struct {
	char	*name;
	char	*path;
	int	type;
	int	basetype;
	int	isdir;
} gamelistentry;

typedef struct {
	char	*hdd0;
	char	*hdd1;
	char	*fdd0;
	char	*fdd1;
	char	*hdd0n;
	char	*hdd1n;
	char	*fdd0n;
	char	*fdd1n;
} load_entry;

extern GXRModeObj* vmode;

load_entry selected = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

void MakeLowercase(char *path)
{
	int i;
	int len = strlen(path);
	for(i = 0; i < len; i++) {
		path[i] = tolower(path[i]);
	}
}

void MakeUppercase(char *path)
{
	int i;
	int len = strlen(path);
	for(i = 0; i < len; i++) {
		path[i] = toupper(path[i]);
	}
}

int GetBasetype(int type)
{
	int ret = BASETYPE_UNKNOWN;
	switch(type) {
		case FILETYPE_FDD_D88:
		case FILETYPE_FDD_MTR:
		case FILETYPE_FDD_XDF:
			ret = BASETYPE_FDD;
			break;
		case FILETYPE_HDD_HDI:
			ret = BASETYPE_HDD;
			break;
		default:
			ret = BASETYPE_UNKNOWN;
			break;
	}
	return ret;
}

int GetFiletype(char *path)
{
	int ret = FILETYPE_UNKNOWN;
	char *ptr = strrchr(path, '.');
	if(ptr == NULL) {
		ret = FILETYPE_UNKNOWN;
	}
	if(strcmp(ptr, ".hdi") == 0) {
		ret = FILETYPE_HDD_HDI;
	}else if(strcmp(ptr, ".d88") == 0) {
		ret = FILETYPE_FDD_D88;
	}else if(strcmp(ptr, ".mtr") == 0) {
		ret = FILETYPE_FDD_MTR;
	}else if(strcmp(ptr, ".xdf") == 0) {
		ret = FILETYPE_FDD_XDF;
	}
	return ret;
}

void BorderOverlay()
{
	int i;
	printf("\x1b[1;0H");
	printf("\xDA");
	for(i = 0; i < (CONWIDTH - 2); i++) {
		printf("\xC4");
	}
	printf("\xBF");
	for(i = 0; i < (CONHEIGHT - 3); i++) {
		printf("\x1b[%d;2H\xB3", i + 1);
		printf("\x1b[%d;%dH\xB3", i + 1, CONWIDTH - 1);
	}
	printf("\x1b[%d;0H", CONHEIGHT);
	printf("\xC0");
	for(i = 0; i < (CONWIDTH - 2); i++) {
		printf("\xC4");
	}
	printf("\xD9");
}

int sortgames_cb(const void *f1, const void *f2)
{
        /* Special case for implicit directories */
        if(((((gamelistentry*)f1)->path[0] == '.') && (((gamelistentry*)f1)->path[1] != '_')) ||	\
	   ((((gamelistentry*)f2)->path[0] == '.') && (((gamelistentry*)f2)->path[1] != '_')))
        {
                if(strcmp(((gamelistentry*)f1)->path, ".") == 0) { return -1; }
                if(strcmp(((gamelistentry*)f2)->path, ".") == 0) { return 1; }
                if(strcmp(((gamelistentry*)f1)->path, "..") == 0) { return -1; }
                if(strcmp(((gamelistentry*)f2)->path, "..") == 0) { return 1; }
        }
	
        /* If one is a file and one is a directory the directory is first. */
        if(((gamelistentry*)f1)->isdir && !(((gamelistentry*)f2)->isdir)) return -1;
        if(!(((gamelistentry*)f1)->isdir) && ((gamelistentry*)f2)->isdir) return 1;
	
        return stricmp(((gamelistentry*)f1)->path, ((gamelistentry*)f2)->path);
}

int* wiimenu_buildgamelist(char* root, gamelistentry* gamelist, int max, \
			   gamelistentry *gamesunk, gamelistentry *gamesfdd, gamelistentry *gameshdd)
{
	int i = 0;
	int *sizes = malloc(sizeof(int) * BASETYPE_COUNT);
	gamelistentry *games;
	memset(sizes, 0, BASETYPE_COUNT * sizeof(int));
	DIR_ITER * dirIter = NULL;
	dirIter = diropen(root);
	if(dirIter == NULL) {
#ifdef DEBUG_NP2
		fprintf(logfp, "Couldn't open %s\n", root);
#endif //DEBUG_NP2
		return 0;
	}
	char filename[MAXPATHLEN];
        struct stat filestat;
	int res;
	int type, basetype;
        for(i = 0; i < max; i++) {
		if(time_to_leave)
			break;
		res = dirnext(dirIter, filename, &filestat);
		
		if(res != 0)
			break;
		char *origname = strdup(filename);
		MakeLowercase(filename);
		type = GetFiletype(filename);
		basetype = GetBasetype(type);
		
		switch(basetype) {
			case BASETYPE_UNKNOWN:
				games = gamesunk;
				break;
			case BASETYPE_FDD:
				games = gamesfdd;
				break;
			case BASETYPE_HDD:
				games = gameshdd;
				break;
		}
		games[sizes[basetype]].path = calloc(strlen(filename) + 1, 1);
		strncpy(games[sizes[basetype]].path, filename, strlen(filename));
		games[sizes[basetype]].isdir = (filestat.st_mode & _IFDIR) == 0 ? 0 : 1;
		games[sizes[basetype]].type = type;
		games[sizes[basetype]].basetype = basetype;
		
		if(games[sizes[basetype]].isdir) {
			if(strcmp(filename, "..") == 0)
				sizes[basetype]--;
			else if(strcmp(filename, ".") == 0)
				sizes[basetype]--;
			else {
				games[sizes[basetype]].name = calloc(strlen(origname) + 1, 1);
				strncpy(games[sizes[basetype]].name, origname, strlen(origname) + 1);
			}
		}else{
			games[sizes[basetype]].name = calloc(strlen(origname) + 1, 1);
			strncpy(games[sizes[basetype]].name, origname, strlen(origname) + 1);
		}
		sizes[basetype]++;
	}
	
	// Sort the file lists
        if(sizes[BASETYPE_UNKNOWN] >= 0)
		qsort(gamesunk, sizes[BASETYPE_UNKNOWN], sizeof(gamelistentry), sortgames_cb);
        if(sizes[BASETYPE_HDD] >= 0)
		qsort(gameshdd, sizes[BASETYPE_HDD], sizeof(gamelistentry), sortgames_cb);
        if(sizes[BASETYPE_FDD] >= 0)
		qsort(gamesfdd, sizes[BASETYPE_FDD], sizeof(gamelistentry), sortgames_cb);
	
	dirclose(dirIter);	// close directory
	return sizes;
}

load_entry wiimenu_choosefromlist(gamelistentry *gamesunk, gamelistentry *gamesfdd, gamelistentry *gameshdd, \
				  int* listcnt, load_entry* ent)
{
	int go = 1;
	int position = 0;
	int cur_off = 0;
	int i = 0;
	int eol = 0; //End of list
	int redraw = 1;
	int devnum = 0;
	int devtype = BASETYPE_HDD;
	char *devices[] = { "HDD0", "HDD1", "FDD0", "FDD1" };
	int sel[] = { -1, -1, -1, -1 };
	VIDEO_Flush();
	VIDEO_WaitVSync();
	while(go) {
		if(time_to_leave)
			break;
		WPAD_ScanPads();
		u32 WPAD_Pressed = WPAD_ButtonsDown(0);
		WPAD_Pressed	|= WPAD_ButtonsDown(1);
		WPAD_Pressed	|= WPAD_ButtonsDown(2);
		WPAD_Pressed	|= WPAD_ButtonsDown(3);
		
		if(WPAD_Pressed & WPAD_BUTTON_B){
			go = 0;
		}
		if(WPAD_Pressed & WPAD_BUTTON_1) {
			devnum++;
			devnum %= 4;
			switch(devnum) {
				case 0:
				case 1:
					devtype = BASETYPE_HDD;
					break;
				case 2:
				case 3:
					devtype = BASETYPE_FDD;
					break;
			}
			position = 0;
			cur_off = 0;
			i = 0;
			eol = 0;
			redraw = 1;
		}
		if(WPAD_Pressed & WPAD_BUTTON_A) {
			switch(devnum) {
				case 0:
					if(ent->hdd0 == NULL) {
						ent->hdd0  = calloc(256, 1);
					}
					if(ent->hdd0n != NULL) {
						free(ent->hdd0n);
					}
					sprintf(ent->hdd0, "sd:/PC98/ROMS/%s", gameshdd[position].path);
					ent->hdd0n = strdup(gameshdd[position].name);
					break;
				case 1:
					if(ent->hdd1 == NULL) {
						ent->hdd1  = calloc(256, 1);
					}
					if(ent->hdd1n != NULL) {
						free(ent->hdd1n);
					}
					sprintf(ent->hdd1, "sd:/PC98/ROMS/%s", gameshdd[position].path);
					ent->hdd1n = strdup(gameshdd[position].name);
					break;
				case 2:
					if(ent->fdd0 == NULL) {
						ent->fdd0  = calloc(256, 1);
					}
					if(ent->fdd0n != NULL) {
						free(ent->fdd0n);
					}
					sprintf(ent->fdd0, "sd:/PC98/ROMS/%s", gamesfdd[position].path);
					ent->fdd0n = strdup(gamesfdd[position].name);
					break;
				case 3:
					if(ent->fdd1 == NULL) {
						ent->fdd1  = calloc(256, 1);
					}
					if(ent->fdd1n != NULL) {
						free(ent->fdd1n);
					}
					sprintf(ent->fdd1, "sd:/PC98/ROMS/%s", gamesfdd[position].path);
					ent->fdd1n = strdup(gamesfdd[position].name);
					break;
			}
			sel[devnum] = position;
			redraw = 1;
		}
		if(WPAD_Pressed & WPAD_BUTTON_2) {
			switch(devnum) {
				case 0:
					if(ent->hdd0 != NULL) {
						free(ent->hdd0);
						ent->hdd0 = NULL;
					}
					if(ent->hdd0n != NULL) {
						free(ent->hdd0n);
						ent->hdd0n = NULL;
					}
					break;
				case 1:
					if(ent->hdd1 != NULL) {
						free(ent->hdd1);
						ent->hdd1 = NULL;
					}
					if(ent->hdd1n != NULL) {
						free(ent->hdd1n);
						ent->hdd1n = NULL;
					}
					break;
				case 2:
					if(ent->fdd0 != NULL) {
						free(ent->fdd0);
						ent->fdd0 = NULL;
					}
					if(ent->fdd0n != NULL) {
						free(ent->fdd0n);
						ent->fdd0n = NULL;
					}
					break;
				case 3:
					if(ent->fdd1 != NULL) {
						free(ent->fdd1);
						ent->fdd1 = NULL;
					}
					if(ent->fdd1n != NULL) {
						free(ent->fdd1n);
						ent->fdd1n = NULL;
					}
					break;
			}
			sel[devnum] = -1;
			redraw = 1;
		}
		if(WPAD_Pressed & WPAD_BUTTON_DOWN) {
			printf("\x1b[%d;0H   ", HEIGHT_BEGIN + cur_off);
			if((sel[devnum] != -1) && (sel[devnum] == position)) {
				printf("\x1b[%d;0H*", HEIGHT_BEGIN + cur_off);
			}
			cur_off++;
			position++;
			
			redraw = 1;

			if(position >= listcnt[devtype]) {
				eol = 1;
				position = listcnt[devtype] - 1;
			}
			if(cur_off >= MAX_ENTRIES) {
				cur_off = MAX_ENTRIES - 1;
				if(!eol)
					redraw = 1;
			}
			if(cur_off >= listcnt[devtype]) {
				cur_off = listcnt[devtype] - 1;
				if(!eol)
					redraw = 1;
			}
			printf("\x1b[%d;0H  \x1A", HEIGHT_BEGIN + cur_off);
			if((sel[devnum] != -1) && (sel[devnum] == position)) {
				printf("\x1b[%d;0H*", HEIGHT_BEGIN + cur_off);
			}
		} else if(WPAD_Pressed & WPAD_BUTTON_UP) {
			printf("\x1b[%d;0H   ", HEIGHT_BEGIN + cur_off);
			if((sel[devnum] != -1) && (sel[devnum] == position)) {
				printf("\x1b[%d;0H*", HEIGHT_BEGIN + cur_off);
			}
			cur_off--;
			position--;

			redraw = 1;

			if(cur_off < 0) {
				cur_off = 0;
				redraw = 1;
			}
			if(position < 0)
				position = 0;
			if(position >= listcnt[devtype])
				eol = 1;
			else
				eol = 0;
			printf("\x1b[%d;0H  \x1A", HEIGHT_BEGIN + cur_off);
			if((sel[devnum] != -1) && (sel[devnum] == position)) {
				printf("\x1b[%d;0H*", HEIGHT_BEGIN + cur_off);
			}
		}
		
		if(redraw) {
			printf("\x1b[J");
			printf("\x1b[%d;0H  Load %s\n  Currently loaded: %s\n  Press 1 to change device. Press 2 to eject.\n\n", HEIGHT_BEGIN - 4, devices[devnum], \
			       (devnum == 0) ? ((ent->hdd0 != NULL) ? ent->hdd0n : "None") : (\
			       (devnum == 1) ? ((ent->hdd1 != NULL) ? ent->hdd1n : "None") : (\
			       (devnum == 2) ? ((ent->fdd0 != NULL) ? ent->fdd0n : "None") : (\
			       (devnum == 3) ? ((ent->fdd1 != NULL) ? ent->fdd1n : "None") : "None"))));
			i = 0;
			// position == real  location
			// cur_off  == arrow location
			i = position - (MAX_ENTRIES - 1);
			if(i < 0)
				i = 0;
			if(i > (MAX_ENTRIES - 1)) {
				while(i > (MAX_ENTRIES - 1)) {
					i -= MAX_ENTRIES;
				}
			}
			int x;
			int bak = -1;
			for(x = 0; (i < listcnt[devtype]) && (x < MAX_ENTRIES); i++, x++) {
				switch(devtype) {
					case BASETYPE_FDD:
						printf("\x1b[%d;0H   %s\n", HEIGHT_BEGIN + x, gamesfdd[i].name);
						break;
					case BASETYPE_HDD:
						printf("\x1b[%d;0H   %s\n", HEIGHT_BEGIN + x, gameshdd[i].name);
						break;
					default:
						break;
				}
				if((sel[devnum] != -1) && (sel[devnum] == i)) {
					printf("\x1b[%d;0H*", HEIGHT_BEGIN + x);
					bak = x;
				}
			}
			printf("\x1b[%d;0H  \x1A", HEIGHT_BEGIN + cur_off);
			if((bak == cur_off)) {
				printf("\x1b[%d;0H*", HEIGHT_BEGIN + cur_off);
			}
			redraw = 0;
			BorderOverlay();
		}
		VIDEO_Flush();
		VIDEO_WaitVSync();
	}
	return *ent;
}

void wiimenu_loadgame()
{
	gamelistentry list[20];
	gamelistentry gamesunk[20];
	gamelistentry gamesfdd[20];
	gamelistentry gameshdd[20];
	int loaded = 0;
	int* listcnt = wiimenu_buildgamelist("sd:/PC98/ROMS/", list, 20, gamesunk, gamesfdd, gameshdd);
	printf("\x1b[J\n\n");
	printf("  Found:\n  %d Unknown Files\n  %d FDD Images\n  %d HDD Images\n", listcnt[BASETYPE_UNKNOWN], listcnt[BASETYPE_FDD], listcnt[BASETYPE_HDD]);
	sleep(1);
	while(!loaded) {
		if(time_to_leave)
			break;
		wiimenu_choosefromlist(gamesunk, gamesfdd, gameshdd, listcnt, &selected);
		printf("\x1b[J\n\n");
		if(selected.hdd0 != NULL) {
			printf("  Loading %s into HDD0.\n", selected.hdd0);
			diskdrv_sethdd(0x00, selected.hdd0);
			free(selected.hdd0);
			sleep(1);
			loaded = 1;
		}
		if(selected.hdd1 != NULL) {
			printf("  Loading %s into HDD1.\n", selected.hdd1);
			diskdrv_sethdd(0x01, selected.hdd1);
			free(selected.hdd1);
			sleep(1);
			loaded = 1;
		}
		if(selected.fdd0 != NULL) {
			printf("  Loading %s into FDD0.\n", selected.fdd0);
			diskdrv_setfdd(0x00, selected.fdd0, 0);
			free(selected.fdd0);
			sleep(1);
			loaded = 1;
		}
		if(selected.fdd0 != NULL) {
			printf("  Loading %s into FDD1.\n", selected.fdd1);
			diskdrv_setfdd(0x01, selected.fdd1, 0);
			free(selected.fdd1);
			sleep(1);
			loaded = 1;
		}
		if(!loaded) {
			printf("  ERROR: No disks loaded! Please choose a disk!\n");
			sleep(1);
		}
		BorderOverlay();
	}
	started = 1;
	sleep(1);
	diskdrv_hddbind();
	free(listcnt);
}

void CenterPrint(char* string)
{
	int len = strlen(string);
	int x = floor((CONWIDTH - len) / 2);
	int i;
	for(i = 0; i < x; i++) {
		printf(" ");
	}
	printf(string);
}

void wiimenu_controlconf()
{
	
}

void wiimenu_sound_cfg()
{

}

void PrintSwitches(unsigned char bits, int col)
{
	int i;
	for(i = 0; i < 8; i++) {
		if(col == i)
			printf("\x1A");
		else
			printf(" ");
		printf("%s ", (bits & (1 << i)) ? "#" : " ");
	}
}

void PrintMenuHead()
{
	printf("\x1B[J\n\n");
	if(started)
		CenterPrint("PAUSED");
	else
		CenterPrint("MENU");
	printf("\n\n");
}

/* Array 1:
 * ?
 * ?
 * Plasma Display
 * Boot Order (default: HDD -> Floppy)
 * Serial Mode
 * ''       ''
 * ?
 * Graphic Mode
 */
/* Array 2:
 * ?
 * Terminal Mode
 * ?
 * Line Count in Text Mode
 * Memory Switch
 * ? Disk?
 * ?
 * GDC 5MHz
 */
/* Array 3:
 * ?
 * ?
 * ?
 * Floppy Motor ?
 * DMA Clock
 * ?
 * ?
 * CPU Mode
 */

void wiimenu_dip_cfg()
{
	int refresh = 1;
	int col = 0;
	int row = 0;
	int go = 1;
	while(go) {
		if(time_to_leave)
			break;
		VIDEO_WaitVSync();
		WPAD_ScanPads();
		u32 WPAD_Pressed = WPAD_ButtonsDown(0);
		WPAD_Pressed	|= WPAD_ButtonsDown(1);
		WPAD_Pressed	|= WPAD_ButtonsDown(2);
		WPAD_Pressed	|= WPAD_ButtonsDown(3);
		if(WPAD_Pressed & WPAD_BUTTON_DOWN) {
			row += 1;
			row %= 3;
			refresh = 1;
		}
		if(WPAD_Pressed & WPAD_BUTTON_UP) {
			row -= 1;
			if(row < 0)
				row = 2;
			refresh = 1;
		}
		if(WPAD_Pressed & WPAD_BUTTON_RIGHT) {
			col += 1;
			col %= 8;
			refresh = 1;
		}
		if(WPAD_Pressed & WPAD_BUTTON_LEFT) {
			col -= 1;
			if(col < 0)
				col = 7;
			refresh = 1;
		}
		if(WPAD_Pressed & WPAD_BUTTON_A) {
			np2cfg.dipsw[row] ^= 1 << col;
			refresh = 1;
		}
		if(WPAD_Pressed & WPAD_BUTTON_B) {
			go = 0;
		}
		if(refresh) {
			PrintMenuHead();
			printf("   DIP Switches:\n");
			printf("                      1  2  3  4  5  6  7  8  \n");
			printf("   Switch array 1:   ");
			PrintSwitches(np2cfg.dipsw[0], (row == 0) ? col : 9001);
			printf("\n   Switch array 2:   ");
			PrintSwitches(np2cfg.dipsw[1], (row == 1) ? col : 9001);
			printf("\n   Switch array 3:   ");
			PrintSwitches(np2cfg.dipsw[2], (row == 2) ? col : 9001);
			BorderOverlay();
		}
	}
}

int wiimenu_do_option_selection(int opt)
{
	UINT32 update = 0;
	switch(opt) {
		case 0:
			np2oscfg.DRAW_SKIP++;
			np2oscfg.DRAW_SKIP %= 5;
			return 1;
		case 1:
			np2oscfg.NOWAIT ^= 1;
			return 2;
		case 2:
			np2cfg.KEY_MODE++;
			np2cfg.KEY_MODE %= 4;
			keystat_resetjoykey();
			update |= SYS_UPDATECFG;
			sysmng_update(update);
			return 3;
		case 3:
			wiimenu_sound_cfg();
			return 4;
		case 4:
			switch(np2cfg.EXTMEM) {
				case 7:
					np2cfg.EXTMEM = 0;
					break;
				case 3:
					np2cfg.EXTMEM += 2;
				case 1:
					np2cfg.EXTMEM++;
				case 0:
					np2cfg.EXTMEM++;
					break;
			}
			return 5;
		case 5:
			rs232c_midipanic();
			mpu98ii_midipanic();
			pc9861k_midipanic();
			return 6;
		case 6:
			wiimenu_dip_cfg();
			return 0;
		default:
			return -1;
	}
}

void wiimenu_options()
{
	int go = 1;
	int selected = 0;
	int oldselected = 0;
	int refresh = 1;
	int reload = 1;
	char blnkstr[1] = "\0";
	while(go) {
		if(time_to_leave)
			break;
		VIDEO_WaitVSync();
		WPAD_ScanPads();
		u32 WPAD_Pressed = WPAD_ButtonsDown(0);
		WPAD_Pressed	|= WPAD_ButtonsDown(1);
		WPAD_Pressed	|= WPAD_ButtonsDown(2);
		WPAD_Pressed	|= WPAD_ButtonsDown(3);
		if(WPAD_Pressed & WPAD_BUTTON_DOWN) {
			selected++;
			selected %= 9;
			refresh = 1;
		}
		
		if(WPAD_Pressed & WPAD_BUTTON_UP) {
			refresh = 1;
			selected--;
			if(selected < 0)
				selected = 8;
		}
		
		if((WPAD_Pressed & WPAD_BUTTON_A) || (WPAD_Pressed & WPAD_BUTTON_PLUS) || (WPAD_Pressed & WPAD_BUTTON_RIGHT)) {
			if(wiimenu_do_option_selection(selected) == -1)
				break;
			refresh = 1;
			reload = 1;
		}

		if((WPAD_Pressed & WPAD_BUTTON_B) || (WPAD_Pressed & WPAD_BUTTON_HOME)) {
			break;
		}
		
		if(reload) {
			PrintMenuHead();
			
			printf("   Frameskip:       %37s", blnkstr);
			if(np2oscfg.DRAW_SKIP != 0) {
				printf("   %d\n", np2oscfg.DRAW_SKIP - 1);
			}else printf("AUTO\n");
			printf("   Framerate Limit: %41s\n", (np2oscfg.NOWAIT == 0) ? "ON" : \
										     "OFF");
			printf("   Key Mode:        %41s\n", (np2cfg.KEY_MODE == 0) ? "KEYS" : (\
							     (np2cfg.KEY_MODE == 1) ? "JOY1" : (\
							     (np2cfg.KEY_MODE == 2) ? "JOY2" : (\
							     (np2cfg.KEY_MODE == 3) ? "MOUSE" : \
										      "UNK"))));
			printf("   Sound Config     <NOUSE>%34s >\n", blnkstr);
			printf("   Memory Size:     %41s\n", (np2cfg.EXTMEM   == 0) ? "640K" : (\
							     (np2cfg.EXTMEM   == 1) ? "640K + 1MB" : (\
							     (np2cfg.EXTMEM   == 3) ? "640K + 3MB" : (\
							     (np2cfg.EXTMEM   == 7) ? "640K + 7MB" : \
										      "UNK"))));
			printf("   MIDI Panic\n");
			printf("   Raw DIP Switches %41s >\n", blnkstr);
			printf("   BIOS Settings    <NOUSE>%34s >\n", blnkstr);
			printf("   Return           %41s >\n", blnkstr);
			BorderOverlay();
		}

		if(refresh) {
			printf("\x1B[%d;0H   ", oldselected + 3);
			printf("\x1B[%d;0H  \x1A", selected + 3);
			oldselected = selected;
		}
	}
}

int wiimenu_do_selection(int opt)
{
	if(!started) opt += 2;
	switch(opt) {
		case 0:
			return -1;
		case 1:
			pccore_cfgupdate();
			pccore_reset();
			return -1;
		case 2:
			wiimenu_controlconf();
			return 3;
		case 3:
			wiimenu_loadgame();
			return -1;
		case 4:
			wiimenu_options();
			return 5;
		case 5:
			wii_shutdown(0);
			return 0;
		case 6:
			wii_shutdown(9001);
			return 0;
		default:
			return 0;
	}
}

void wiimenu_menu()
{
	int go = 1;
	int selected = 0;
	int oldselected = 0;
	int refresh = 1;
	int reload = 1;
	/* The In-emulation menus */
	log_console_enable_video(1);
	
	while(go) {
		if(time_to_leave)
			break;
		VIDEO_WaitVSync();
		WPAD_ScanPads();
		u32 WPAD_Pressed = WPAD_ButtonsDown(0);
		WPAD_Pressed	|= WPAD_ButtonsDown(1);
		WPAD_Pressed	|= WPAD_ButtonsDown(2);
		WPAD_Pressed	|= WPAD_ButtonsDown(3);

		if(WPAD_Pressed & WPAD_BUTTON_DOWN) {
			selected++;
			if(started)
				selected %= 7;
			else
				selected %= 5;
			refresh = 1;
		}

		if(WPAD_Pressed & WPAD_BUTTON_UP) {
			selected--;
			if(selected < 0) {
				if(started)
					selected = 6;
				else
					selected = 4;
			}
			refresh = 1;
		}

		if((WPAD_Pressed & WPAD_BUTTON_A) || (WPAD_Pressed & WPAD_BUTTON_PLUS)) {
			if(wiimenu_do_selection(selected) == -1)
				break;
			refresh = 1;
			reload = 1;
		}

		if((WPAD_Pressed & WPAD_BUTTON_B) || (WPAD_Pressed & WPAD_BUTTON_HOME)) {
			if(started) {
				break;
			}
		}
		
		if(reload) {
			PrintMenuHead();
			
			if(started) {
				printf("   Return to Emulator                               \n");
				printf("   Reset Emulator                                   \n");
			}
			printf("   Controller Config                                    <NOUSE>\n");
			printf("   Switch Disks                                               >\n");
			printf("   Emulation Options   ");
			if(!started) {
				printf("                                       >\n");
			}else{
				printf("(May require emulator reset)           >\n");
			}
			printf("   Return to Loader                                            \n");
			printf("   Return to Wii Menu                                          \n");			
			BorderOverlay();
		}
		
		if(refresh) {
			printf("\x1B[%d;0H   ", oldselected + 3);
			printf("\x1B[%d;0H  \x1A", selected + 3);
			oldselected = selected;
		}
	}
	if(started)
		log_console_enable_video(0);
}

void wiimenu_initialize()
{
	log_console_init(vmode, 9001, (640 - (8 * CONWIDTH)) / 2, (480 - (16 * CONHEIGHT)) / 2, \
			8 * CONWIDTH, 16 * CONHEIGHT);
	log_console_enable_video(1);
}

