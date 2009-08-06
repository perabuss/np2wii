#include "compiler.h"
#include <SDL/SDL.h>
// #include <sys/time.h>
// #include <signal.h>
// #include <unistd.h>
#include	"strres.h"
#include	"np2.h"
#include	"dosio.h"
#include	"commng.h"
#include	"fontmng.h"
#include	"inputmng.h"
#include	"scrnmng.h"
#include	"soundmng.h"
#include	"sysmng.h"
#include	"taskmng.h"
#include	"sdlkbd.h"
#include	"ini.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"scrndraw.h"
#include	"s98.h"
#include	"diskdrv.h"
#include	"timing.h"
#include	"keystat.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"sysmenu.h"
#include	"menu.h"

#include	<ogc/pad.h>
#include	<wiiuse/wpad.h>
#include	<fat.h>
#include	<SDL/SDL.h>
#include	<SDL/SDL_ttf.h>



#ifdef DEBUG_NP2
FILE* logfp;
#endif //DEBUG_NP2
		NP2OSCFG	np2oscfg = {0, 0, 0, 0, 0};
static	UINT		framecnt;
static	UINT		waitcnt;
static	UINT		framemax = 1;
static	char		datadir[MAX_PATH] = "./";


extern void wiimenu_loadgame();

static void usage(const char *progname) {

	printf("Usage: %s [options]\n", progname);
	printf("\t--help   [-h]       : print this message\n");
}


// ---- resume

static void getstatfilename(char *path, const char *ext, int size) {

	file_cpyname(path, datadir, size);
	file_cutext(path);
	file_catname(path, "np2sdl.", size);
	file_catname(path, ext, size);
}

static int flagsave(const char *ext) {

	int		ret;
	char	path[MAX_PATH];

	getstatfilename(path, ext, sizeof(path));
	ret = statsave_save(path);
	if (ret) {
		file_delete(path);
	}
	return(ret);
}

static void flagdelete(const char *ext) {

	char	path[MAX_PATH];

	getstatfilename(path, ext, sizeof(path));
	file_delete(path);
}

static int flagload(const char *ext, const char *title, BOOL force) {

	int		ret;
	int		id;
	char	path[MAX_PATH];
	char	buf[1024];
	char	buf2[1024 + 256];

	getstatfilename(path, ext, sizeof(path));
	id = DID_YES;
	ret = statsave_check(path, buf, sizeof(buf));
	if (ret & (~STATFLAG_DISKCHG)) {
		menumbox("Couldn't restart", title, MBOX_OK | MBOX_ICONSTOP);
		id = DID_NO;
	}
	else if ((!force) && (ret & STATFLAG_DISKCHG)) {
		SPRINTF(buf2, "Conflict!\n\n%s\nContinue?", buf);
		id = menumbox(buf2, title, MBOX_YESNOCAN | MBOX_ICONQUESTION);
	}
	if (id == DID_YES) {
		statsave_load(path);
	}
	return(id);
}


// ---- proc

#define	framereset(cnt)		framecnt = 0

static void processwait(UINT cnt) {

	if (timing_getcount() >= cnt) {
		timing_setcount(0);
		framereset(cnt);
	}
	else {
		taskmng_sleep(1);
	}
}

static void wii_load_game()
{
	wiimenu_loadgame();
	//diskdrv_sethdd(0x00, "sd:/PC98/ROMS/test.hdi");		// Load the first SASI/IDE drive.
}

void wii_shutdown(s32 chan)
{
	WPAD_Disconnect(WPAD_CHAN_ALL);
	taskmng_exit();
}

static void wii_initialize()
{
	fatInitDefault();
	PAD_Init();
	WPAD_Init();
	WPAD_SetPowerButtonCallback(wii_shutdown);
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
}

int main(int argc, char **argv) {

	int		pos;
	char	*p;
	int		id;

	wii_initialize();
#ifdef DEBUG_NP2
	logfp = fopen("sd:/log.txt", "wb");
	if(logfp == NULL) {
		printf("Can't open logfile.\n");
		sleep(5);
		exit(1);
	}
#endif //DEBUG_NP2

	pos = 1;
	while(pos < argc) {
		p = argv[pos++];
		if ((!milstr_cmp(p, "-h")) || (!milstr_cmp(p, "--help"))) {
			usage(argv[0]);
			goto np2main_err1;
		}
		else {
			printf("error command: %s\n", p);
			goto np2main_err1;
		}
	}

	dosio_init();
	file_setcd(datadir);
	initload();
	
	TRACEINIT();
	
	if (fontmng_init() != SUCCESS) {
		goto np2main_err2;
	}
	sdlkbd_initialize();
	inputmng_init();
	keystat_initialize();

	scrnmng_initialize();
	if (scrnmng_create(FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT) != SUCCESS) {
		goto np2main_err4;
	}
	wiimenu_initialize();
	
	wii_load_game();
	soundmng_initialize();
	commng_initialize();
	sysmng_initialize();
	taskmng_initialize();
	strcpy(np2cfg.fontfile, "sd:/PC98/DATA/font.bmp");
//	strcpy(np2cfg.biospath, "sd:/PC98/BIOS/");
	pccore_init();
	S98_init();

	scrndraw_redraw();
	pccore_reset();

	if (np2oscfg.resume) {
		id = flagload(str_sav, str_resume, FALSE);
		if (id == DID_CANCEL) {
			goto np2main_err5;
		}
	}
//	np2oscfg.DRAW_SKIP = 4;
//	np2oscfg.NOWAIT = 1;
	np2oscfg.NOWAIT = 0;
	while(taskmng_isavail()) {
		taskmng_rol();
		if (np2oscfg.NOWAIT) {
			pccore_exec(framecnt == 0);
			if (np2oscfg.DRAW_SKIP) {			// nowait frame skip
				framecnt++;
				if (framecnt >= np2oscfg.DRAW_SKIP) {
					processwait(0);
				}
			}
			else {							// nowait auto skip
				framecnt = 1;
				if (timing_getcount()) {
					processwait(0);
				}
			}
		}
		else if (np2oscfg.DRAW_SKIP) {		// frame skip
			if (framecnt < np2oscfg.DRAW_SKIP) {
				pccore_exec(framecnt == 0);
				framecnt++;
			}
			else {
				processwait(np2oscfg.DRAW_SKIP);
			}
		}
		else {								// auto skip
			if (!waitcnt) {
				UINT cnt;
				pccore_exec(framecnt == 0);
				framecnt++;
				cnt = timing_getcount();
				if (framecnt > cnt) {
					waitcnt = framecnt;
					if (framemax > 1) {
						framemax--;
					}
				}
				else if (framecnt >= framemax) {
					if (framemax < 12) {
						framemax++;
					}
					if (cnt >= 12) {
						timing_reset();
					}
					else {
						timing_setcount(cnt - framecnt);
					}
					framereset(0);
				}
			}
			else {
				processwait(waitcnt);
				waitcnt = framecnt;
			}
		}
	}

	pccore_cfgupdate();
	if (np2oscfg.resume) {
		flagsave(str_sav);
	}
	else {
		flagdelete(str_sav);
	}
	pccore_term();
	S98_trash();
	soundmng_deinitialize();

	if (sys_updates	& (SYS_UPDATECFG | SYS_UPDATEOSCFG)) {
		initsave();
	}

	scrnmng_destroy();
	sysmenu_destroy();
	TRACETERM();
	SDL_Quit();
	dosio_term();
#ifdef DEBUG_NP2
	fprintf(logfp, "FINISHED!\n");
#endif //DEBUG_NP2
	return(SUCCESS);

np2main_err5:
#ifdef DEBUG_NP2
	fprintf(logfp, "Error 5!\n");
#endif //DEBUG_NP2
	pccore_term();
	S98_trash();
	soundmng_deinitialize();

np2main_err4:
#ifdef DEBUG_NP2
	fprintf(logfp, "Error 4!\n");
#endif //DEBUG_NP2
	scrnmng_destroy();

np2main_err2:
#ifdef DEBUG_NP2
	fprintf(logfp, "Error 2!\n");
#endif //DEBUG_NP2
	TRACETERM();
	SDL_Quit();
	dosio_term();

np2main_err1:
#ifdef DEBUG_NP2
	fprintf(logfp, "Error 1!\n");
#endif //DEBUG_NP2
	return(FAILURE);
}

