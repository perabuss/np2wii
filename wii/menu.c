/*****************************************************************************/
/*  Neko Project II Wii                                                      */
/* ------------------------------------------------------------------------- */
/*  menu.c                                                                   */
/*  The Wii specific menu code. Should be launchable from any point in the   */
/*  emulator.                                                                */
/*****************************************************************************/

#include	"compiler.h"
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
#include	<ogc/consol.h>

#define DEBUG_NP2
#ifdef DEBUG_NP2
extern FILE* logfp;
#endif

#define WIIMOTE_HORI

#define	CONTROL_COUNT	11

extern int base_control_map[];
extern int vert_wiimote_map[];
extern int hori_wiimote_map[];

typedef struct {
	char	name[256];
	char	path[256];
	int	isdir;
} gamelistentry;

void* con_bak;

int wiimenu_buildgamelist(char* root, gamelistentry* gamelist)
{
	int count = 0;
	DIR* pdir = opendir(root);
	if (pdir != NULL) {
		while(1) {
			struct dirent* pent = readdir(pdir);
			if(pent == NULL) break;
			
			if((strcmp(".", pent->d_name) != 0) && (strcmp("..", pent->d_name) != 0)) {
				strncpy(gamelist[count].name, pent->d_name, 256);
				char dnbuf[260];
				sprintf(dnbuf, "%s/%s", root, pent->d_name);
				strncpy(gamelist[count].path, dnbuf, 256);
				
				struct stat statbuf;
				stat(dnbuf, &statbuf);
				
				gamelist[count].isdir = 0;
				if(S_ISDIR(statbuf.st_mode))
					gamelist[count].isdir = 1;
				count++;
			}
		}
		closedir(pdir);
	}else
		printf("opendir() failure.\n");
	return count;
}

int wiimenu_choosefromlist(gamelistentry* gamelist, int listcnt)
{
	return 0;
}

void wiimenu_loadgame()
{
	gamelistentry list[256];
//	int listcnt = wiimenu_buildgamelist("sd:/PC98/ROMS/", list);
	int listcnt = 0;
	int selected = wiimenu_choosefromlist(list, listcnt);
	(void)selected;
	diskdrv_sethdd(0x00, "sd:/PC98/ROMS/test.hdi");		// Load the first SASI/IDE drive.
}


void wiimenu_initialize()
{
//	fprintf(logfp, "Initting!\n");
//	wiimenufont = SFont_InitFont(SDL_LoadBMP_RW(SDL_RWFromFile("sd:/PC98/DATA/menufont.bmp", "rb"), 1));
}

