/*****************************************************************************/
/*  Neko Project ii                                                          */
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

extern SDL_Joystick* wiimote1;

SFont_Font* wiimenufont;
SDL_Surface* screensurf;

#define DEBUG_NP2
#ifdef DEBUG_NP2
extern FILE* logfp;
#endif

#define WIIMOTE_HORI

#define	CONTROL_COUNT	11

extern int base_control_map[];
extern int vert_wiimote_map[];
extern int hori_wiimote_map[];

UINT8 xhatsdown = 0;
UINT32 xbuttonsdown = 0;

void wiimenu_controllerhandle()
{
	SDL_JoystickUpdate();
	UINT8 wiimote_hat = SDL_JoystickGetHat(wiimote1, 0);
	UINT32 buttons = 0;
	// A button
	if(SDL_JoystickGetButton(wiimote1, 0)) buttons |= 1 << 0;
	// B button
	if(SDL_JoystickGetButton(wiimote1, 1)) buttons |= 1 << 1;
	// 1 button
	if(SDL_JoystickGetButton(wiimote1, 2)) buttons |= 1 << 2;
	// 2 button
	if(SDL_JoystickGetButton(wiimote1, 3)) buttons |= 1 << 3;
	// MINUS button
	if(SDL_JoystickGetButton(wiimote1, 4)) buttons |= 1 << 4;
	// PLUS button
	if(SDL_JoystickGetButton(wiimote1, 5)) buttons |= 1 << 5;
	// HOME button
	if(SDL_JoystickGetButton(wiimote1, 6)) buttons |= 1 << 6;
	int idx = 0;
	int* controlmap;
#ifdef WIIMOTE_VERT
	controlmap = vert_wiimote_map;
#endif
#ifdef WIIMOTE_HORI
	controlmap = hori_wiimote_map;
#endif
	for(idx = 0; idx < 4; idx++) {
		if((wiimote_hat & base_control_map[idx]) && !(xhatsdown & base_control_map[idx])) { 
			xhatsdown |= base_control_map[idx];
			sdlkbd_keydown(controlmap[idx]);
		}else if(xhatsdown & base_control_map[idx]) { xhatsdown &= ~base_control_map[idx];
			sdlkbd_keyup(controlmap[idx]);
		}
	}
	for(idx = 4; idx < CONTROL_COUNT; idx++) {
		if((buttons & (base_control_map[idx])) && !(xbuttonsdown & (base_control_map[idx]))) { 
			sdlkbd_keydown(controlmap[idx]);
			xbuttonsdown |= base_control_map[idx];
		}else if(xbuttonsdown & (base_control_map[idx])) {
			sdlkbd_keyup(controlmap[idx]);
			xbuttonsdown &= ~(base_control_map[idx]);
		}
	}
	
}

typedef struct {
	char	name[256];
	char	path[256];
	int	isdir;
} gamelistentry;

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
	fprintf(logfp, "getsurf!\n");
	screensurf = SDL_GetVideoSurface();
	fprintf(logfp, "locksurf!\n");
	SDL_LockSurface(screensurf);
	fprintf(logfp, "writesurf!\n");
	SFont_WriteCenter(screensurf, wiimenufont, 20, "Test");
	fprintf(logfp, "unlocksurf!\n");
	SDL_UnlockSurface(screensurf);
	fprintf(logfp, "Mission accomplished!\n");
	sleep(10);
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
	fprintf(logfp, "Initting!\n");
	wiimenufont = SFont_InitFont(SDL_LoadBMP_RW(SDL_RWFromFile("sd:/PC98/DATA/menufont.bmp", "rb"), 1));
}

