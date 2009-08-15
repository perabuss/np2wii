#include	"compiler.h"
// #include	<signal.h>
#include	"inputmng.h"
#include	"taskmng.h"
#include	"sdlkbd.h"
#include	"vramhdl.h"
#include	"sysmenu.h"
#include	"dosio.h"
#include	"log_console.h"
#include	"menu.h"

#include	<SDL/SDL.h>
#include	<wiiuse/wiiuse.h>
#include	<wiiuse/wpad.h>
#include	<mxml.h>

SDL_Joystick* wiimote1;

#ifdef DEBUG_NP2
extern FILE* logfp;
#endif

extern void MakeLowercase(char *path);
extern void MakeUppercase(char *path);

typedef struct {
	int	loc;
	char	*name;
} control_entries;

#define	CONTROL_COUNT	28

#define MENU_BTN	(9001)

#define SDLMAPPING(z)	{ SDLK_##z , #z }

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
	WPAD_BUTTON_HOME,
	
	WPAD_NUNCHUK_BUTTON_Z,
	WPAD_NUNCHUK_BUTTON_C,

	WPAD_CLASSIC_BUTTON_UP,
	WPAD_CLASSIC_BUTTON_DOWN,
	WPAD_CLASSIC_BUTTON_LEFT,
	WPAD_CLASSIC_BUTTON_RIGHT,
	WPAD_CLASSIC_BUTTON_A,
	WPAD_CLASSIC_BUTTON_B,
	WPAD_CLASSIC_BUTTON_X,
	WPAD_CLASSIC_BUTTON_Y,
	WPAD_CLASSIC_BUTTON_ZL,
	WPAD_CLASSIC_BUTTON_ZR,
	WPAD_CLASSIC_BUTTON_FULL_L,
	WPAD_CLASSIC_BUTTON_FULL_R,
	WPAD_CLASSIC_BUTTON_MINUS,
	WPAD_CLASSIC_BUTTON_PLUS,
	WPAD_CLASSIC_BUTTON_HOME,
};

const control_entries entrs[] = {
	/* Wiimote */
	{  0, "up"	},
	{  1, "down"	},
	{  2, "left"	},
	{  3, "right"	},
	{  4, "a"	},
	{  5, "b"	},
	{  6, "one"	},
	{  7, "two"	},
	{  8, "minus"	},
	{  9, "plus"	},
	{ 10, "home"	},
	/* Nunchuk */
	{ 11, "z"	},
	{ 12, "c"	},
	/* Classic Controller */
	{ 13, "up"	},
	{ 14, "down"	},
	{ 15, "left"	},
	{ 16, "right"	},
	{ 17, "a"	},
	{ 18, "b"	},
	{ 19, "x"	},
	{ 20, "y"	},
	{ 21, "zl"	},
	{ 22, "zr"	},
	{ 23, "l"	},
	{ 24, "r"	},
	{ 25, "minus"	},
	{ 26, "plus"	},
	{ 27, "home"	},
};

const control_entries sdlmap[] = {
	SDLMAPPING(z),
	SDLMAPPING(x),
	SDLMAPPING(c),
	SDLMAPPING(v),
	SDLMAPPING(b),
	SDLMAPPING(n),
	SDLMAPPING(m),
	SDLMAPPING(a),
	SDLMAPPING(s),
	SDLMAPPING(d),
	SDLMAPPING(f),
	SDLMAPPING(g),
	SDLMAPPING(h),
	SDLMAPPING(j),
	SDLMAPPING(k),
	SDLMAPPING(l),
	SDLMAPPING(q),
	SDLMAPPING(w),
	SDLMAPPING(e),
	SDLMAPPING(r),
	SDLMAPPING(t),
	SDLMAPPING(y),
	SDLMAPPING(u),
	SDLMAPPING(i),
	SDLMAPPING(o),
	SDLMAPPING(p),
	SDLMAPPING(LEFT),
	SDLMAPPING(RIGHT),
	SDLMAPPING(DOWN),
	SDLMAPPING(UP),
	SDLMAPPING(RETURN),
	SDLMAPPING(SPACE),
	SDLMAPPING(ESCAPE),
	SDLMAPPING(LSHIFT),
	SDLMAPPING(RSHIFT)
};

int default_wiimote_map[3][CONTROL_COUNT] = {
	/* Wiimote (No extensions) */
	{ 
	/* Wiimote */
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
	  MENU_BTN,
	/* Nunchuk */
	  0, 0,
	/* Classic Controller */
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  MENU_BTN },
	/* Wiimote (+Nunchuk) */
	{
	/* Wiimote */
	  SDLK_UP,
	  SDLK_DOWN,
	  SDLK_LEFT,
	  SDLK_RIGHT,
	  SDLK_RETURN,
	  SDLK_SPACE,
	  SDLK_z,
	  SDLK_x,
	  SDLK_ESCAPE,
	  SDLK_ESCAPE,
	  MENU_BTN,
	/* Nunchuk */
	  SDLK_x,
	  SDLK_z,
	/* Classic Controller */
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  MENU_BTN },
	/* Wiimote (+Classic Controller) */
	{ 
	/* Wiimote */
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  MENU_BTN,
	/* Nunchuk */
	  0, 0,
	/* Classic Controller */
	  SDLK_UP,
	  SDLK_DOWN,
	  SDLK_LEFT, 
	  SDLK_RIGHT,
	  SDLK_RETURN, 
	  SDLK_x, 
	  SDLK_z, 
	  SDLK_SPACE, 
	  SDLK_LSHIFT,
	  SDLK_RSHIFT, 
	  SDLK_LSHIFT,
	  SDLK_RSHIFT, 
	  SDLK_ESCAPE, 
	  SDLK_RETURN,
	  MENU_BTN },
};


int *loaded_wiimote_map[3];

BOOL	task_avail;

char xmlbuff[4096];

char *                    /* O - Buffer */
get_value(mxml_node_t *node,        /* I - Node to get */
	  void        *buffer,        /* I - Buffer */
	  int         buflen)        /* I - Size of buffer */
{
	char        *ptr,            /* Pointer into buffer */
	*end;            /* End of buffer */
	int        len;            /* Length of node */
	mxml_node_t    *current;        /* Current node */
	
	
	ptr = (char*)buffer;
	end = (char*)buffer + buflen - 1;
	char tempbuf[4092];
	current = node->child;
	for (current = node->child; current && ptr < end; current = current->next)
	{
		if (current->type == MXML_TEXT)
		{
			if (current->value.text.whitespace)
				*ptr++ = ' ';
			
			len = (int)strlen(current->value.text.string);
			if (len > (int)(end - ptr))
				len = (int)(end - ptr);
			
			memcpy(ptr, current->value.text.string, len);
			ptr += len;
		}
		else if (current->type == MXML_OPAQUE)
		{
			len = (int)strlen(current->value.opaque);
			if (len > (int)(end - ptr))
				len = (int)(end - ptr);
			
			memcpy(ptr, current->value.opaque, len);
			ptr += len;
		}
		else if (current->type == MXML_INTEGER)
		{
			sprintf(tempbuf, "%d", current->value.integer);
			len = (int)strlen(tempbuf);
			if (len > (int)(end - ptr))
				len = (int)(end - ptr);
			
			memcpy(ptr, tempbuf, len);
			ptr += len;
		}
		else if (current->type == MXML_REAL)
		{
			sprintf(tempbuf, "%f", current->value.real);
			len = (int)strlen(tempbuf);
			if (len > (int)(end - ptr))
				len = (int)(end - ptr);
			
			memcpy(ptr, tempbuf, len);
			ptr += len;
		}
	}
	*ptr = 0;
	return buffer;
}

const char *whitespace_cb(mxml_node_t *node, int where)
{
	/* code by Matt_P */
	const char *name = node->value.element.name;
	mxml_node_t * temp = node;
	int depth = 0;
	while(temp){
		depth++;
		temp = temp->parent;
	}
	if (where == 3 || where == 1){
		return "";
	}else if(((node->prev && where == 0) || node->parent) && !(where == 2 && strncmp(name, "ua", 2) && strcmp(name, "flip") && strcmp(name, "zoom") && strcmp(name, "coords") && strcmp(name, "entries") && strcmp(name, "xmlyt") && strcmp(name, "xmlan") && strcmp(name, "entries") && strcmp(name, "triplet") && strcmp(name, "pair") && strcmp(name, "entry") && strcmp(name, "pane") && (strcmp(name, "tag") && strcmp(name, "size") && strcmp(name, "material") && strcmp(name, "colors") && strcmp(name, "subs") && strcmp(name, "font") && strcmp(name, "wnd4") && strcmp(name, "wnd4mat") && strcmp(name, "set") && strcmp(name, "coordinates"))) ){
		sprintf(xmlbuff, "\n%*s",  (depth-1)*4, "");
    }else{
        return "";
    }
    return xmlbuff;
}

int FIND_SDL_ENTRY(char *x)
{
	int y;
	for(y = 0; y < 35; y++) {
		if(strcmp(sdlmap[y].name, x) == 0) {
			return sdlmap[y].loc;
		}
	}
	return 0;
}

void LOAD_WIIMOTE_FROM_XML(control_entries x, int y, mxml_node_t* z)
{
	char a[256];
	mxml_node_t *tempnode = mxmlFindElement(z, z, x.name, NULL, NULL, MXML_DESCEND);
	if(tempnode == NULL) {
		loaded_wiimote_map[y][x.loc] = default_wiimote_map[y][x.loc];
                return;
	}
	get_value(tempnode, a, 256);
	
	MakeUppercase(a);
	if((isalpha(a[0])) && (a[1] == '\0')){
		MakeLowercase(a);
	}
	loaded_wiimote_map[y][x.loc] = FIND_SDL_ENTRY(a);
}

void LoadWiimoteMapping()
{
	FILE* fp = fopen(file_getcd("control.xml"), "rb");
	if(fp == NULL) {
		printf("  Unable to open config, using defaults\n");
		/* We don't have a Config file available then! */
		return;
	}
	mxml_node_t *hightree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
	if(hightree == NULL) {
		/* Couldn't open hightree! */
		return;
	}
	mxml_node_t *tree = mxmlFindElement(hightree, hightree, "controls", NULL, NULL, MXML_DESCEND);
	if(hightree == NULL) {
		/* Couldn't get tree! */
		return;
	}
	mxml_node_t *node;
	mxml_node_t *subnode;
	int i;
	/* Wiimote portion */
	node = mxmlFindElement(tree, tree, "wiimote", NULL, NULL, MXML_DESCEND);
	if(node != NULL) {
		for(i = 0; i < 10; i++) {
			LOAD_WIIMOTE_FROM_XML(entrs[i], 0, node);
		}
	}
	/* Wiimote+Nunchuk portion */
	node = mxmlFindElement(tree, tree, "nunchuk", NULL, NULL, MXML_DESCEND);
	if(node != NULL) {
		subnode = mxmlFindElement(node, node, "wiimote", NULL, NULL, MXML_DESCEND);
		if(subnode != NULL) {
			for(i = 0; i < 10; i++) {
				LOAD_WIIMOTE_FROM_XML(entrs[i], 1, subnode);
			}
		}
		for(i = 11; i < 13; i++) {
			LOAD_WIIMOTE_FROM_XML(entrs[i], 1, node);
		}
	}
	/* Wiimote+Classic portion */
	node = mxmlFindElement(tree, tree, "classic", NULL, NULL, MXML_DESCEND);
	if(node != NULL) {
		subnode = mxmlFindElement(node, node, "wiimote", NULL, NULL, MXML_DESCEND);
		if(subnode == NULL) {
			memcpy(loaded_wiimote_map[2], default_wiimote_map[2], 10 * sizeof(int));
		}else{
			for(i = 0; i < 10; i++) {
				LOAD_WIIMOTE_FROM_XML(entrs[i], 2, subnode);
			}
		}
		for(i = 13; i < (CONTROL_COUNT - 1); i++) {
			LOAD_WIIMOTE_FROM_XML(entrs[i], 2, node);
		}
		loaded_wiimote_map[2][11] = loaded_wiimote_map[2][13];
		loaded_wiimote_map[2][12] = loaded_wiimote_map[2][15];
	}
}

void sighandler(int signo)
{
	(void)signo;
	task_avail = FALSE;
}

void taskmng_initialize()
{
	task_avail = TRUE;
	printf("\x1B[J\n\n");
	printf("  Loading controller config from %s...\n", file_getcd("control.xml"));
	loaded_wiimote_map[0] = calloc(CONTROL_COUNT, sizeof(int));
	loaded_wiimote_map[1] = calloc(CONTROL_COUNT, sizeof(int));
	loaded_wiimote_map[2] = calloc(CONTROL_COUNT, sizeof(int));
	memcpy(loaded_wiimote_map[0], default_wiimote_map[0], CONTROL_COUNT * sizeof(int));
	memcpy(loaded_wiimote_map[1], default_wiimote_map[1], CONTROL_COUNT * sizeof(int));
	memcpy(loaded_wiimote_map[2], default_wiimote_map[2], CONTROL_COUNT * sizeof(int));
	LoadWiimoteMapping();
	printf("  Loaded config...\n");
	BorderOverlay();
}

void taskmng_exit()
{
	task_avail = FALSE;
}

UINT32 buttonsdown = 0;

void taskmng_rol()
{
	UINT32 buttons = 0;
	struct expansion_t ext;
	WPAD_ScanPads();
	WPAD_Expansion(WPAD_CHAN_0, &ext);
	u32 btn = WPAD_ButtonsDown(WPAD_CHAN_0) | WPAD_ButtonsHeld(WPAD_CHAN_0);
	buttons = btn;
	int idx;
	int type;
	switch(ext.type) {
		case WPAD_EXP_NUNCHUK:
			type = 1;
			break;
		case WPAD_EXP_CLASSIC:
			type = 2;
			break;
		default:
			type = 0;
			break;
	}
	
	for(idx = 0; idx < CONTROL_COUNT; idx++) {
		if((buttons & (base_control_map[idx])) && (!(buttonsdown & (base_control_map[idx])))) {
			if(loaded_wiimote_map[type][idx] == MENU_BTN) {
				wiimenu_menu();
			}else{
				sdlkbd_keydown(loaded_wiimote_map[type][idx]);
				buttonsdown |= base_control_map[idx];
			}
		}else if((!(buttons & (base_control_map[idx]))) && (buttonsdown & (base_control_map[idx]))) {
			if(loaded_wiimote_map[type][idx] != MENU_BTN) {
				sdlkbd_keyup(loaded_wiimote_map[type][idx]);
				buttonsdown &= ~(base_control_map[idx]);
			}
		}
	}
}

BOOL taskmng_sleep(UINT32 tick)
{
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

