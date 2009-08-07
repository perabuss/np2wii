#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	np2
BUILD		:=	build
DATA		:=	data  

BASE		= ..
WIISRC		= $(BASE)/wii
COMMON		= $(BASE)/common
I286C		= $(BASE)/i286c
IO		= $(BASE)/io
CBUS		= $(BASE)/cbus
BIOS		= $(BASE)/bios
SOUND		= $(BASE)/sound
VERMOUTH	= $(SOUND)/vermouth
GETSND		= $(SOUND)/getsnd
VRAM		= $(BASE)/vram
FDD		= $(BASE)/fdd
LIO		= $(BASE)/lio
FONT		= $(BASE)/font
GENERIC		= $(BASE)/generic
EMBED		= $(BASE)/embed
MENU		= $(EMBED)/menu
MENUBASE	= $(EMBED)/menubase
SLZAURUS	= $(BASE)/wii/slzaurus
CODECNV		= $(BASE)/codecnv
MEM		= $(BASE)/mem

SOURCES		=	$(WIISRC) $(BASE) $(COMMON) $(I286C) $(IO) $(CBUS) $(BIOS) $(SOUND)  \
			$(VERMOUTH) $(GETSND) $(VRAM) $(FDD) $(LIO) $(FONT)  \
			$(GENERIC) $(EMBED) $(MENU) $(MENUBASE) $(SLZAURUS) $(CODECNV) $(MEM)

INCLUDES	=	$(WIISRC) $(BASE) $(COMMON) $(I286C) $(IO) $(CBUS) $(BIOS) $(SOUND)  \
			$(VERMOUTH) $(GETSND) $(VRAM) $(FDD) $(LIO) $(FONT)  \
			$(GENERIC) $(EMBED) $(MENU) $(MENUBASE) $(SLZAURUS) $(CODECNV) $(MEM)

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

NORMFLAGS	=	-g -Wall $(MACHDEP) $(INCLUDE)
OPTIMALFLAGS	=	-O3 -g -Wall $(MACHDEP) $(INCLUDE) -falign-loops=16 -falign-jumps=16 \
			-falign-functions=16 -malign-natural -ffast-math -fstrict-aliasing \
			-funroll-loops -ftree-loop-linear -fsched-interblock -fgcse-sm
CFLAGS		=	$(OPTIMALFLAGS) -DDEBUG_NP2
CXXFLAGS	=	$(CFLAGS)

LDFLAGS		=	-g $(MACHDEP) -Wl,-Map,$(notdir $@).map

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS		:= -lmxml -lSDL_ttf -lfreetype -lSDL -lwiikeyboard -lwiiuse -lbte -lfat -logc -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS		:=

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
					$(sFILES:.s=.o) $(SFILES:.S=.o)

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES), -iquote $(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) \
					-I$(LIBOGC_INC) -I$(LIBOGC_INC)/SDL \
			-I$(WIISRC) -I$(BASE) -I$(COMMON) -I$(I286C) -I$(IO) -I$(CBUS) -I$(BIOS) \
			-I$(SOUND) -I$(VERMOUTH) -I$(GETSND) -I$(VRAM) -I$(FDD) -I$(LIO) \
			-I$(FONT) -I$(GENERIC) -I$(EMBED) -I$(MENU) -I$(MENUBASE) \
			-I$(SLZAURUS)

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					-L$(LIBOGC_LIB)

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
send:
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	@wiiload $(TARGET).dol


#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
