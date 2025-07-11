SO_EXT                    = dll
SO_LDFLAGS                = -shared
SO_CFLAGS                 = -fpic
SO_LIBS                  :=
EXE_EXT                  := .exe

CFLAGS                   += -DWINVER=0x501
LDFLAGS                  += -Wl,--stack,8388608

INTL_LIBS                ?= -lintl
OPENGL_LIBS              ?= -lopengl32

game_LDFLAGS             += $(SRCDIR)/game/game.def
testall_LDFLAGS          += -lws2_32 -lwinmm
ufo_LDFLAGS              += -lws2_32 -lwinmm -lssp
ufo2map_LDFLAGS          += -lwinmm -mconsole -lssp
ufoded_LDFLAGS           += -lws2_32 -lwinmm -lssp
ufomodel_LDFLAGS         += -lwinmm -mconsole
