#include	"compiler.h"
// #include	<signal.h>
#include	"inputmng.h"
#include	"taskmng.h"
#include	"sdlkbd.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"sysmenu.h"
#include	<SDL/SDL.h>
#include	<wiiuse/wiiuse.h>
#include	<wiiuse/wpad.h>

SDL_Joystick* wiimote1;

#define DEBUG_NP2
#ifdef DEBUG_NP2
extern FILE* logfp;
#endif

#define WIIMOTE_HORI

#define	CONTROL_COUNT	11

int base_control_map[] = {
	WPAD_BUTTON_UP,
	WPAD_BUTTON_DOWN,
	WPAD_BUTTON_LEFT,
	WPAD_BUTTON_RIGHT,
	WPAD_BUTTON_A,
	WPAD_BUTTON_B,
	WPAD_BUTTON_1,
	WPAD_BUTTON_2,
	WPAD_BUTTON_MINUS,
	WPAD_BUTTON_PLUS,
	WPAD_BUTTON_HOME
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
	task_avail = TRUE;
}

void taskmng_exit(void) {

	task_avail = FALSE;
}

UINT8 hatsdown = 0;
UINT32 buttonsdown = 0;

void taskmng_rol(void) {
	UINT32 buttons = 0;
	WPAD_ScanPads();
	u32 btn = WPAD_ButtonsDown(0) | WPAD_ButtonsHeld(0);
	buttons = btn;
/*	// UP button
	if(btn & WPAD_BUTTON_UP)	buttons |= base_control_map[0];
	// DOWN button
	if(btn & WPAD_BUTTON_DOWN)	buttons |= base_control_map[1];
	// LEFT button
	if(btn & WPAD_BUTTON_LEFT)	buttons |= base_control_map[2];
	// RIGHT button
	if(btn & WPAD_BUTTON_RIGHT)	buttons |= base_control_map[3];

	// A button
	if(btn & WPAD_BUTTON_A)		buttons |= base_control_map[4];
	// B button
	if(btn & WPAD_BUTTON_B)		buttons |= base_control_map[5];
	// 1 button
	if(btn & WPAD_BUTTON_1)		buttons |= base_control_map[6];
	// 2 button
	if(btn & WPAD_BUTTON_2)		buttons |= base_control_map[7];
	// MINUS button
	if(btn & WPAD_BUTTON_MINUS)	buttons |= base_control_map[8];
	// PLUS button
	if(btn & WPAD_BUTTON_PLUS)	buttons |= base_control_map[9];
	// HOME button
	if(btn & WPAD_BUTTON_HOME)	buttons |= base_control_map[10];*/
	int idx = 0;
	int* controlmap = vert_wiimote_map;
#ifdef WIIMOTE_HORI
	controlmap = hori_wiimote_map;
#endif
	for(idx = 0; idx < CONTROL_COUNT; idx++) {
		if((buttons & (base_control_map[idx])) && (!(buttonsdown & (base_control_map[idx])))) {
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

