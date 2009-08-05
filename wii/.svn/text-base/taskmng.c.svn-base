#include	"compiler.h"
// #include	<signal.h>
#include	"inputmng.h"
#include	"taskmng.h"
#include	"sdlkbd.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"sysmenu.h"
#include	<SDL/SDL.h>

SDL_Joystick* wiimote1;

#define DEBUG_NP2
#ifdef DEBUG_NP2
extern FILE* logfp;
#endif

#define WIIMOTE_HORI

#define	CONTROL_COUNT	11

int base_control_map[] = {
	SDL_HAT_UP,
	SDL_HAT_DOWN,
	SDL_HAT_LEFT,
	SDL_HAT_RIGHT,
	1 << 0,
	1 << 1,
	1 << 2,
	1 << 3,
	1 << 4,
	1 << 5,
	1 << 6
};

int vert_wiimote_map[] = {
	SDLK_UP,
	SDLK_DOWN,
	SDLK_LEFT,
	SDLK_RIGHT,
	SDLK_z,
	SDLK_x,
	SDLK_RETURN,
	SDLK_SPACE,
	SDLK_ESCAPE,
	SDLK_ESCAPE,
	SDLK_ESCAPE
};

int hori_wiimote_map[] = {
	SDLK_LEFT,
	SDLK_RIGHT,
	SDLK_DOWN,
	SDLK_UP,
	SDLK_RETURN,
	SDLK_SPACE,
	SDLK_z,
	SDLK_x,
	SDLK_ESCAPE,
	SDLK_ESCAPE,
	SDLK_ESCAPE
};


BOOL	task_avail;


void sighandler(int signo) {

	(void)signo;
	task_avail = FALSE;
}


void taskmng_initialize(void) {
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	wiimote1 = SDL_JoystickOpen(0);
	if(wiimote1 == NULL) {
		printf("Couldn't open wiimote =(\n");
		sleep(10);
		return;
	}
	task_avail = TRUE;
}

void taskmng_exit(void) {

	task_avail = FALSE;
}

UINT8 hatsdown = 0;
UINT32 buttonsdown = 0;

void taskmng_rol(void) {
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
		if((wiimote_hat & base_control_map[idx]) && !(hatsdown & base_control_map[idx])) { 
			hatsdown |= base_control_map[idx];
			sdlkbd_keydown(controlmap[idx]);
		}else if(hatsdown & base_control_map[idx]) { hatsdown &= ~base_control_map[idx];
			sdlkbd_keyup(controlmap[idx]);
		}
	}
	for(idx = 4; idx < CONTROL_COUNT; idx++) {
		if((buttons & (base_control_map[idx])) && !(buttonsdown & (base_control_map[idx]))) { 
			sdlkbd_keydown(controlmap[idx]);
			buttonsdown |= base_control_map[idx];
		}else if(buttonsdown & (base_control_map[idx])) {
			sdlkbd_keyup(controlmap[idx]);
			buttonsdown &= ~(base_control_map[idx]);
		}
	}
}

BOOL taskmng_sleep(UINT32 tick) {

	UINT32	base;

	base = GETTICK();
	while((task_avail) && ((GETTICK() - base) < tick)) {
		taskmng_rol();
#ifndef WIN32
		usleep(960);
#else
		Sleep(1);
#endif
	}
	return(task_avail);
}

