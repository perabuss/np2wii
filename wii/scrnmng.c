#include	"compiler.h"
// #include	<sys/time.h>
// #include	<signal.h>
// #include	<unistd.h>
#include	"scrnmng.h"
#include	"scrndraw.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"np2.h"

#ifdef DEBUG_NP2
extern FILE* logfp;
#endif //DEBUG_NP2

#define USE_SDL

typedef struct {
	BOOL		enable;
	int		width;
	int		height;
	int		bpp;
#if defined(USE_SDL)
	SDL_Surface	*surface;
#endif //USE_SDL
	VRAMHDL		vram;
} SCRNMNG;

typedef struct {
	int		width;
	int		height;
} SCRNSTAT;

static const char app_name[] = "Neko Project II";

static	SCRNMNG		scrnmng;
static	SCRNSTAT	scrnstat;
static	SCRNSURF	scrnsurf;

typedef struct {
	int		xalign;
	int		yalign;
	int		width;
	int		height;
	int		srcpos;
	int		dstpos;
} DRAWRECT;

// Centers the screen
#define SCREEN_OFFSET	40

#if defined(USE_SDL)
static BOOL calcdrawrect(SDL_Surface *surface, DRAWRECT *dr, VRAMHDL s, const RECT_T *rt)
#endif //USE_SDL
{
	int		pos;

#if defined(USE_SDL)
	dr->xalign = surface->format->BytesPerPixel;
	dr->yalign = surface->pitch;
#endif //USE_SDL
	dr->srcpos = 0;
	dr->dstpos = 0;
	dr->width = min(scrnmng.width, s->width);
	dr->height = min(scrnmng.height, s->height);
	if (rt) {
		pos = max(rt->left, 0);
		dr->srcpos += pos;
		dr->dstpos += pos * dr->xalign;
		dr->width = min(rt->right, dr->width) - pos;

		pos = max(rt->top, 0);
		dr->srcpos += pos * s->width;
		dr->dstpos += pos * dr->yalign;
		dr->height = min(rt->bottom, dr->height) - pos;
	}
	if ((dr->width <= 0) || (dr->height <= 0)) {
		return(FAILURE);
	}
	return(SUCCESS);
}


void scrnmng_initialize(void)
{
	scrnstat.width = 640;
	scrnstat.height = 400;
}

BOOL scrnmng_create(int width, int height)
{
#if defined(USE_SDL)
	char			s[256];
	const SDL_VideoInfo	*vinfo;
	SDL_Surface		*surface;
	SDL_PixelFormat		*fmt;
	BOOL			r;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
#ifdef DEBUG_NP2
		fprintf(logfp, "Error: SDL_Init: %s\n", SDL_GetError());
#endif //DEBUG_NP2
		return(FAILURE);
	}
	SDL_ShowCursor(SDL_DISABLE);
	vinfo = SDL_GetVideoInfo();
	if (vinfo == NULL) {
#ifdef DEBUG_NP2
		fprintf(logfp, "Error: SDL_GetVideoInfo: %s\n", SDL_GetError());
#endif //DEBUG_NP2
		return(FAILURE);
	}
	SDL_VideoDriverName(s, sizeof(s));

	surface = SDL_SetVideoMode(FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT, FULLSCREEN_BPP, 0);
	if (surface == NULL) {
#ifdef DEBUG_NP2
		fprintf(logfp, "Error: SDL_SetVideoMode: %s\n", SDL_GetError());
#endif //DEBUG_NP2
		return(FAILURE);
	}

	r = FALSE;
	fmt = surface->format;
#if defined(SUPPORT_8BPP)
	if (fmt->BitsPerPixel == 8) {
		r = TRUE;
	}
#endif
#if defined(SUPPORT_16BPP)
	if ((fmt->BitsPerPixel == 16) && (fmt->Rmask == 0xf800) &&
		(fmt->Gmask == 0x07e0) && (fmt->Bmask == 0x001f)) {
		r = TRUE;
	}
#endif
#if defined(SUPPORT_24BPP)
	if (fmt->BitsPerPixel == 24) {
		r = TRUE;
	}
#endif
#if defined(SUPPORT_32BPP)
	if (fmt->BitsPerPixel == 32) {
		r = TRUE;
	}
#endif
#if defined(SCREEN_BPP)
	if (fmt->BitsPerPixel != SCREEN_BPP) {
		r = FALSE;
	}
#endif
	if (r) {
		scrnmng.enable = TRUE;
		scrnmng.width = FULLSCREEN_WIDTH;
		scrnmng.height = FULLSCREEN_HEIGHT;
		scrnmng.bpp = FULLSCREEN_BPP;
		return(SUCCESS);
	}
	else {
#ifdef DEBUG_NP2
		fprintf(logfp, "Error: Bad screen mode %d\n", fmt->BitsPerPixel);
#endif //DEBUG_NP2
		return(FAILURE);
	}
#endif //USE_SDL
}

void scrnmng_destroy(void)
{
	scrnmng.enable = FALSE;
}

RGB16 scrnmng_makepal16(RGB32 pal32)
{
	RGB16	ret;

	ret = (pal32.p.r & 0xf8) << 8;
#if defined(SIZE_QVGA)
	ret += (pal32.p.g & 0xfc) << (3 + 16);
#else
	ret += (pal32.p.g & 0xfc) << 3;
#endif
	ret += pal32.p.b >> 3;
	return(ret);
}

void scrnmng_setwidth(int posx, int width)
{
	scrnstat.width = width;
}

void scrnmng_setheight(int posy, int height)
{
	scrnstat.height = height;
}

SCRNSURF *scrnmng_surflock(void)
{
#if defined(USE_SDL)
	SDL_Surface	*surface;
#endif //USE_SDL

	if (scrnmng.vram == NULL) {
#if defined(USE_SDL)
		surface = SDL_GetVideoSurface();
#endif //USE_SDL
		if (surface == NULL) {
			return(NULL);
		}
#if defined(USE_SDL)
		SDL_LockSurface(surface);
		scrnmng.surface = surface;
		scrnsurf.ptr = ((BYTE*)surface->pixels) + (SCREEN_OFFSET * surface->pitch);
		scrnsurf.xalign = surface->format->BytesPerPixel;
		scrnsurf.yalign = surface->pitch;
		scrnsurf.bpp = surface->format->BitsPerPixel;
#endif //USE_SDL
	}
	else {
		scrnsurf.ptr = scrnmng.vram->ptr;
		scrnsurf.xalign = scrnmng.vram->xalign;
		scrnsurf.yalign = scrnmng.vram->yalign;
		scrnsurf.bpp = scrnmng.vram->bpp;
	}
	scrnsurf.width = min(scrnstat.width, 640);
	scrnsurf.height = min(scrnstat.height, 400);
	scrnsurf.extend = 0;
	return(&scrnsurf);
}

static void draw_onmenu(void)
{
	RECT_T		rt;
#if defined(USE_SDL)
	SDL_Surface	*surface;
#endif //USE_SDL)

	DRAWRECT	dr;
	const BYTE	*p;
	BYTE		*q;
	const BYTE	*a;
	int		salign;
	int		dalign;
	int		x;
	BYTE		*pixs;

	rt.left = 0;
	rt.top = 0;
	rt.right = min(scrnstat.width, 640);
	rt.bottom = min(scrnstat.height, 400);
#if defined(SIZE_QVGA)
	rt.right >>= 1;
	rt.bottom >>= 1;
#endif

#if defined(USE_SDL)
	surface = SDL_GetVideoSurface();
	if (surface == NULL) {
		return;
	}
	SDL_LockSurface(surface);
#endif //USE_SDL
	if (calcdrawrect(surface, &dr, menuvram, &rt) == SUCCESS) {
#if defined(USE_SDL)
		pixs = ((BYTE*)surface->pixels) + (SCREEN_OFFSET * surface->pitch);
#endif //USE_SDL
		switch(scrnmng.bpp) {
#if defined(SUPPORT_16BPP)
			case 16:
				p = scrnmng.vram->ptr + (dr.srcpos * 2);
				q = pixs + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if (a[x] == 0) {
							*(UINT16 *)q = *(UINT16 *)(p + (x * 2));
						}
						q += dr.xalign;
					} while(++x < dr.width);
					p += salign * 2;
					q += dalign;
					a += salign;
				} while(--dr.height);
				break;
#endif
#if defined(SUPPORT_24BPP)
			case 24:
				p = scrnmng.vram->ptr + (dr.srcpos * 3);
				q = pixs + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if (a[x] == 0) {
							q[0] = p[x*3+0];
							q[1] = p[x*3+1];
							q[2] = p[x*3+2];
						}
						q += dr.xalign;
					} while(++x < dr.width);
					p += salign * 3;
					q += dalign;
					a += salign;
				} while(--dr.height);
				break;
#endif
#if defined(SUPPORT_32BPP)
			case 32:
				p = scrnmng.vram->ptr + (dr.srcpos * 4);
				q = pixs + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if (a[x] == 0) {
							*(UINT32 *)q = *(UINT32 *)(p + (x * 4));
						}
						q += dr.xalign;
					} while(++x < dr.width);
					p += salign * 4;
					q += dalign;
					a += salign;
				} while(--dr.height);
				break;
#endif
		}
	}
#if defined(USE_SDL)
	SDL_UnlockSurface(surface);
	SDL_Flip(surface);
#endif //USE_SDL
}

void scrnmng_surfunlock(const SCRNSURF *surf)
{
#if defined(USE_SDL)
	SDL_Surface	*surface;
#endif //USE_SDL

	if (surf) {
		if (scrnmng.vram == NULL) {
			if (scrnmng.surface != NULL) {
#if defined(USE_SDL)
				surface = scrnmng.surface;
				scrnmng.surface = NULL;
				SDL_UnlockSurface(surface);
				SDL_Flip(surface);
#endif //USE_SDL
			}
		}
		else {
			if (menuvram) {
				draw_onmenu();
			}
		}
	}
}


// ----

BOOL scrnmng_entermenu(SCRNMENU *smenu)
{
	if (smenu == NULL) {
		goto smem_err;
	}
	vram_destroy(scrnmng.vram);
	scrnmng.vram = vram_create(scrnmng.width, scrnmng.height, FALSE, scrnmng.bpp);
	if (scrnmng.vram == NULL) {
		goto smem_err;
	}
	scrndraw_redraw();
	smenu->width = scrnmng.width;
	smenu->height = scrnmng.height;
	smenu->bpp = (scrnmng.bpp == 32)?24:scrnmng.bpp;
	return(SUCCESS);

smem_err:
	return(FAILURE);
}

void scrnmng_leavemenu(void)
{
	VRAM_RELEASE(scrnmng.vram);
}

void scrnmng_menudraw(const RECT_T *rct)
{
#if defined(USE_SDL)
	SDL_Surface	*surface;
#endif //USE_SDL
	DRAWRECT	dr;
	const BYTE	*p;
	const BYTE	*q;
	BYTE		*r;
	BYTE		*a;
	int		salign;
	int		dalign;
	int		x;
	BYTE		*pixs;

	if ((!scrnmng.enable) && (menuvram == NULL)) {
		return;
	}
#if defined(USE_SDL)
	surface = SDL_GetVideoSurface();
	if(surface == NULL) {
		return;
	}
	SDL_LockSurface(surface);
#endif //USE_SDL
	if(calcdrawrect(surface, &dr, menuvram, rct) == SUCCESS) {
#if defined(USE_SDL)
		pixs = ((BYTE*)surface->pixels) + (SCREEN_OFFSET * surface->pitch);
#endif //USE_SDL
		switch(scrnmng.bpp) {
#if defined(SUPPORT_16BPP)
			case 16:
				p = scrnmng.vram->ptr + (dr.srcpos * 2);
				q = menuvram->ptr + (dr.srcpos * 2);
				r = pixs + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if(a[x]) {
							if(a[x] & 2) {
								*(UINT16 *)r = *(UINT16 *)(q + (x * 2));
							}
							else {
								a[x] = 0;
								*(UINT16 *)r = *(UINT16 *)(p + (x * 2));
							}
						}
						r += dr.xalign;
					} while(++x < dr.width);
					p += salign * 2;
					q += salign * 2;
					r += dalign;
					a += salign;
				} while(--dr.height);
				break;
#endif
#if defined(SUPPORT_24BPP)
			case 24:
				p = scrnmng.vram->ptr + (dr.srcpos * 3);
				q = menuvram->ptr + (dr.srcpos * 3);
				r = pixs + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if(a[x]) {
							if(a[x] & 2) {
								r[RGB24_B] = q[x*3+0];
								r[RGB24_G] = q[x*3+1];
								r[RGB24_R] = q[x*3+2];
							}
							else {
								a[x] = 0;
								r[0] = p[x*3+0];
								r[1] = p[x*3+1];
								r[2] = p[x*3+2];
							}
						}
						r += dr.xalign;
					} while(++x < dr.width);
					p += salign * 3;
					q += salign * 3;
					r += dalign;
					a += salign;
				} while(--dr.height);
				break;
#endif
#if defined(SUPPORT_32BPP)
			case 32:
				p = scrnmng.vram->ptr + (dr.srcpos * 4);
				q = menuvram->ptr + (dr.srcpos * 3);
				r = pixs + dr.dstpos;
				a = menuvram->alpha + dr.srcpos;
				salign = menuvram->width;
				dalign = dr.yalign - (dr.width * dr.xalign);
				do {
					x = 0;
					do {
						if(a[x]) {
							if(a[x] & 2) {
								((RGB32 *)r)->p.b = q[x*3+0];
								((RGB32 *)r)->p.g = q[x*3+1];
								((RGB32 *)r)->p.r = q[x*3+2];
							//	((RGB32 *)r)->p.e = 0;
							}
							else {
								a[x] = 0;
								*(UINT32 *)r = *(UINT32 *)(p + (x * 4));
							}
						}
						r += dr.xalign;
					} while(++x < dr.width);
					p += salign * 4;
					q += salign * 3;
					r += dalign;
					a += salign;
				} while(--dr.height);
				break;
#endif
		}
	}
#if defined(USE_SDL)
	SDL_UnlockSurface(surface);
	SDL_Flip(surface);
#endif //USE_SDL
}

