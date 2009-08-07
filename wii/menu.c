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
#include	<SDL/SDL.h>
#include	"SFont.h"
#include	<fat.h>
#include	<dirent.h>
#include	"log_console.h"
#include	<wiiuse/wpad.h>
#include	<wiiuse/wiiuse.h>

#ifdef DEBUG_NP2
extern FILE* logfp;
#endif

#define WIIMOTE_HORI
#define HEIGHT_BEGIN 	3
#define MAX_ENTRIES	20
#define	CONTROL_COUNT	11

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
} load_entry;

extern GXRModeObj* vmode;

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
	int cnt;
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
		res = dirnext(dirIter, filename, &filestat);
		
		if(res != 0)
			break;
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
				sizes[basetype]--; //sprintf(gamelist[i].name, "Up One Level");
			else if(strcmp(filename, ".") == 0)
				sizes[basetype]--;
			else {
				games[sizes[basetype]].name = calloc(strlen(games[sizes[basetype]].path) + 1, 1);
				strncpy(games[sizes[basetype]].name, games[sizes[basetype]].path, strlen(games[sizes[basetype]].path) + 1);
			}
		}else{
			games[sizes[basetype]].name = calloc(strlen(games[sizes[basetype]].path) + 1, 1);
			strncpy(games[sizes[basetype]].name, games[sizes[basetype]].path, strlen(games[sizes[basetype]].path) + 1);
		}
		sizes[basetype]++;
/*		gamelist[i].path = calloc(strlen(filename) + 1, 1);
		strncpy(gamelist[i].path, filename, strlen(filename));
		gamelist[i].isdir = (filestat.st_mode & _IFDIR) == 0 ? 0 : 1; // flag this as a dir
		MakeLowercase(gamelist[i].path);
		gamelist[i].type = GetFiletype(gamelist[i].path);
		gamelist[i].basetype = GetBasetype(gamelist[i].type);
		
		if(gamelist[i].isdir) {
			if(strcmp(filename, "..") == 0)
				i--; //sprintf(gamelist[i].name, "Up One Level");
			else if(strcmp(filename, ".") == 0)
				i--;
			else {
				gamelist[i].name = calloc(strlen(gamelist[i].path) + 1, 1);
				strncpy(gamelist[i].name, gamelist[i].path, strlen(gamelist[i].path) + 1);
			}
		}else{
			gamelist[i].name = calloc(strlen(gamelist[i].path) + 1, 1);
			strncpy(gamelist[i].name, gamelist[i].path, strlen(gamelist[i].path) + 1);
		}*/
	}
/*	cnt = i;
	for(i = 0; (i < cnt) && (i < max); i++) {
		games[gamelist[i].basetype][sizes[gamelist[i].basetype]++] = gamelist[i];
	}*/
	
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
				  int* listcnt)
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
	load_entry ent;
	ent.hdd0 = NULL;
	ent.hdd1 = NULL;
	ent.fdd0 = NULL;
	ent.fdd1 = NULL;
	VIDEO_Flush();
	VIDEO_WaitVSync();
	while(go) {
		WPAD_ScanPads();
		u32 WPAD_Pressed = WPAD_ButtonsDown(0);
		WPAD_Pressed |= WPAD_ButtonsDown(1);
		WPAD_Pressed |= WPAD_ButtonsDown(2);
		WPAD_Pressed |= WPAD_ButtonsDown(3);
		
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
					if(ent.hdd0 == NULL) {
						ent.hdd0 = calloc(256, 1);
					}
					sprintf(ent.hdd0, "sd:/PC98/ROMS/%s", gameshdd[position].path);
					break;
				case 1:
					if(ent.hdd1 == NULL) {
						ent.hdd1 = calloc(256, 1);
					}
					sprintf(ent.hdd1, "sd:/PC98/ROMS/%s", gameshdd[position].path);
					break;
				case 2:
					if(ent.fdd0 == NULL) {
						ent.fdd0 = calloc(256, 1);
					}
					sprintf(ent.fdd0, "sd:/PC98/ROMS/%s", gamesfdd[position].path);
					break;
				case 3:
					if(ent.fdd1 == NULL) {
						ent.fdd1 = calloc(256, 1);
					}
					sprintf(ent.fdd1, "sd:/PC98/ROMS/%s", gamesfdd[position].path);
					break;
			}
			sel[devnum] = position;
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
			printf("\x1b[%d;0H ->", HEIGHT_BEGIN + cur_off);
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
			printf("\x1b[%d;0H ->", HEIGHT_BEGIN + cur_off);
			if((sel[devnum] != -1) && (sel[devnum] == position)) {
				printf("\x1b[%d;0H*", HEIGHT_BEGIN + cur_off);
			}
		}
		
		if(redraw) {
			printf("\x1b[J");
			printf("\x1b[%d;0HLoad %s. Press 1 to change device.\n\n", HEIGHT_BEGIN - 1, devices[devnum]);
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
			printf("\x1b[%d;0H ->", HEIGHT_BEGIN + cur_off);
			if((bak == cur_off)) {
				printf("\x1b[%d;0H*", HEIGHT_BEGIN + cur_off);
			}
			redraw = 0;
		}
		VIDEO_Flush();
		VIDEO_WaitVSync();
	}
	return ent;
}

void wiimenu_loadgame()
{
	gamelistentry list[20];
	gamelistentry gamesunk[20];
	gamelistentry gamesfdd[20];
	gamelistentry gameshdd[20];
	int loaded = 0;
	log_console_enable_video(1);
	int* listcnt = wiimenu_buildgamelist("sd:/PC98/ROMS/", list, 20, gamesunk, gamesfdd, gameshdd);
	printf("\x1b[J\n\n");
	printf("Found:\n%d Unknown Files\n%d FDD Images\n%d HDD Images\n", listcnt[BASETYPE_UNKNOWN], listcnt[BASETYPE_FDD], listcnt[BASETYPE_HDD]);
	sleep(1);
	while(!loaded) {
		load_entry selected = wiimenu_choosefromlist(gamesunk, gamesfdd, gameshdd, listcnt);
		printf("\x1b[J\n\n");
		if(selected.hdd0 != NULL) {
			printf("Loading %s into HDD0.\n", selected.hdd0);
			diskdrv_sethdd(0x00, selected.hdd0);
			free(selected.hdd0);
			sleep(1);
			loaded = 1;
		}
		if(selected.hdd1 != NULL) {
			printf("Loading %s into HDD1.\n", selected.hdd1);
			diskdrv_sethdd(0x01, selected.hdd1);
			free(selected.hdd1);
			sleep(1);
			loaded = 1;
		}
		if(selected.fdd0 != NULL) {
			printf("Loading %s into FDD0.\n", selected.fdd0);
			diskdrv_setfdd(0x00, selected.fdd0, 0);
			free(selected.fdd0);
			sleep(1);
			loaded = 1;
		}
		if(selected.fdd0 != NULL) {
			printf("Loading %s into FDD1.\n", selected.fdd1);
			diskdrv_setfdd(0x01, selected.fdd1, 0);
			free(selected.fdd1);
			sleep(1);
			loaded = 1;
		}
		if(!loaded) {
			printf("ERROR: No disks loaded! Please choose a disk!\n");
			sleep(1);
		}
	}
	sleep(1);
	log_console_enable_video(0);
	free(listcnt);
}


void wiimenu_initialize()
{
	log_console_init(vmode, 9001);
	log_console_enable_video(0);
//	fprintf(logfp, "Initting!\n");
//	wiimenufont = SFont_InitFont(SDL_LoadBMP_RW(SDL_RWFromFile("sd:/PC98/DATA/menufont.bmp", "rb"), 1));
}

