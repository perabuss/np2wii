
typedef struct {
	BYTE	NOWAIT;
	BYTE	DRAW_SKIP;
	BYTE	F12KEY;
	BYTE	resume;
	BYTE	jastsnd;
} NP2OSCFG;


enum {
	FULLSCREEN_WIDTH	= 640,
	FULLSCREEN_HEIGHT	= 480,
	FULLSCREEN_BPP		= 16
};

extern	NP2OSCFG	np2oscfg;

