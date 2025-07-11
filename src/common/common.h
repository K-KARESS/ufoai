/**
 * @file
 * @brief definitions common between client and server, but not game lib
 */

/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#pragma once

#include "../shared/ufotypes.h"
#include "../shared/byte.h"
#include "../shared/shared.h"
#include "../shared/mathlib.h"
#include "../shared/defines.h"
#include "cvar.h"
#include "mem.h"

#define UFO_VERSION "2.6-dev"
#define GAME_TITLE "UFO:AI"
#define GAME_TITLE_LONG "UFO:Alien Invasion"

#ifdef _WIN32
#  define BUILDSTRING_OS "Win32"
#  ifndef SO_EXT
#    define SO_EXT "dll"
#  endif

#elif defined __linux__
#  define BUILDSTRING_OS "Linux"
#  ifndef SO_EXT
#    define SO_EXT "so"
#  endif

#elif defined(__FreeBSD__)
#  define BUILDSTRING_OS "FreeBSD"
#  ifndef SO_EXT
#    define SO_EXT "so"
#  endif

#elif defined(__OpenBSD__)
#  define BUILDSTRING_OS "OpenBSD"
#  ifndef SO_EXT
#    define SO_EXT "so"
#  endif

#elif defined(__NetBSD__)
#  define BUILDSTRING_OS "NetBSD"
#  ifndef SO_EXT
#    define SO_EXT "so"
#  endif

#elif defined __sun
#  define BUILDSTRING_OS "Solaris"
#  ifndef SO_EXT
#    define SO_EXT "so"
#  endif

#elif defined (__APPLE__) || defined (MACOSX)
#  define BUILDSTRING_OS "MacOSX"
#  ifndef SO_EXT
#    define SO_EXT "dylib"
#  endif

#elif defined ANDROID
#  define BUILDSTRING_OS "Android"
#  ifndef SO_EXT
#    define SO_EXT "so"
#  endif

#else
#  define BUILDSTRING_OS "Unknown"
#endif

#if defined __alpha__ || defined __alpha || defined _M_ALPHA
#  define CPUSTRING "Alpha"
#elif defined __amd64__ || defined __amd64 || defined __x86_64__ || defined __x86_64 || defined _M_X64
#  define CPUSTRING "AMD64"
#elif defined __arm__ || defined __thumb__ || defined _ARM
#  define CPUSTRING "ARM"
#elif defined i386 || defined __i386__ || defined __i386 || defined _M_IX86 || defined __X86__ || defined _X86_ || defined __THW_INTEL__ || defined __I86__ || defined __INTEL__
#  define CPUSTRING "IA-32"
#elif defined __ia64__ || defined _IA64 || defined __IA64__ || defined __ia64 || defined _M_IA64
#  define CPUSTRING "IA-64"
#elif defined __mips__ || defined mips || defined __mips || defined __MIPS__
#  define CPUSTRING "MIPS"
#elif defined __powerpc || defined __powerpc__ || defined __POWERPC__ || defined __ppc__ || defined _M_PPC || defined _ARCH_PPC
#  define CPUSTRING "PowerPC"
#elif defined __sparc__ || defined __sparc
#  define CPUSTRING "SPARC"
#else
#  define CPUSTRING "Unknown"
#endif

#ifdef DEBUG
#  define BUILDSTRING_VARIANT "DEBUG"
#else
#  define BUILDSTRING_VARIANT "RELEASE"
#endif

#ifdef UFO_REVISION
#  define BUILDSTRING BUILDSTRING_OS " " BUILDSTRING_VARIANT " build " UFO_REVISION
#else
#  define BUILDSTRING BUILDSTRING_OS " " BUILDSTRING_VARIANT
#endif

#define MASTER_SERVER "http://ufoai.org/" /* sponsored by NineX */

/*
==============================================================
PROTOCOL
==============================================================
*/

/* protocol.h -- communications protocols */

#define	PROTOCOL_VERSION	18

#define	PORT_CLIENT	27901
#define	PORT_SERVER	27910

/**
 * @brief server to client
 * the svc_strings[] array in cl_parse.c should mirror this
 */
enum svc_ops_e {
	svc_bad,

	/** the rest are private to the client and server */
	svc_nop,
	svc_ping,
	svc_disconnect,
	svc_reconnect,
	svc_print,					/**< [byte] id [string] null terminated string */
	svc_stufftext,				/**< [string] stuffed into client's console buffer, should be \n terminated */
	svc_serverdata,				/**< [long] protocol, spawncount, playernum, mapname, ... */
	svc_configstring,			/**< [short] [string] */
	svc_event,					/**< event like move or inventory usage - see EV_* and @sa CL_ParseEvent */
	svc_oob = 0xff
};

#define MAX_SVC_STUFFTEXT 1024
#define MAX_SVC_PRINT 1024
#define MAX_SVC_RECONNECT 256
#define MAX_SVC_DISCONNECT 256
#define MAX_CLC_STRINGCMD 512

typedef int32_t svc_ops_t;

/*============================================== */

/**
 * @brief client to server
 */
enum clc_ops_e {
	clc_bad,
	clc_nop,
	clc_ack,					/**< answer on a ping */
	clc_endround,
	clc_teaminfo,
	clc_initactorstates,
	clc_action,
	clc_userinfo,				/**< [[userinfo string] */
	clc_stringcmd,				/**< [string] message */
	clc_oob = svc_oob			/**< out of band - connectionless */
};

#define SOUND_ATTN_NONE 0.0f /**< full volume the entire level */
#define SOUND_ATTN_NORM	1.0f
#define SOUND_ATTN_IDLE 1.2f
#define SOUND_ATTN_STATIC 3.0f /**< dimish very rapidly with distance */
#define SOUND_ATTN_MAX SOUND_ATTN_STATIC

#include "../ports/system.h"
#include "cmd.h"
#include "cvar.h"
#include "cmodel.h"
#include "filesys.h"
#include "scripts.h"
#include "net.h"
#include "dbuffer.h"
#include "netpack.h"

/*
==============================================================
MISC
==============================================================
*/

#define ANGLE2SHORT(x)		((int)((x)*65536/360) & 65535)
#define SHORT2ANGLE(x)		((x)*(360.0/65536))

#define	ERR_FATAL	0			/* exit the entire game with a popup window */
#define	ERR_DROP	1			/* print to console and disconnect from game */
#define	ERR_QUIT	2			/* not an error, just a normal exit */

#define Q_COLOR_ESCAPE	'^'
#define Q_IsColorString(p)	((p) && *(p) == Q_COLOR_ESCAPE && *((p) + 1) && isalnum(*((p) + 1)))

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED	"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"

#define CON_COLOR_BLACK		0
#define CON_COLOR_RED		1
#define CON_COLOR_GREEN		2
#define CON_COLOR_YELLOW	3
#define CON_COLOR_BLUE		4
#define CON_COLOR_CYAN		5
#define CON_COLOR_MAGENTA	6
#define CON_COLOR_WHITE		7
#define MAX_COLORS			8

void Com_BeginRedirect(struct net_stream* stream, char* buffer, int buffersize);
void Com_EndRedirect(void);
void Com_vPrintf(const char* fmt, va_list);

typedef void (*exceptionCallback_t)(void);

/**
 * Thrown by Com_Drop().
 */
struct comDrop_t {};
class comRestart_t {
public:
	const char* gamedir;
	comRestart_t (const char* _gamedir) : gamedir(_gamedir ? strdup(_gamedir) : nullptr) {}
	virtual ~comRestart_t () { free(static_cast<void*>(const_cast<char*>(gamedir))); }
};

void* Com_LoadLibrary(const char* path, const char* prefix);

void Com_Drop(void) __attribute__((noreturn));
void Com_Quit(void);
void Com_BreakIntoDebugger(void);
void Com_WriteConfigToFile(const char* filename);
void Cvar_WriteVariables(qFILE* f);

int Com_ServerState(void);
void Com_SetServerState(int state);
void Com_SetRandomSeed(unsigned int seed);
const char* Com_UnsignedIntToBinary(uint32_t x);
const char* Com_ByteToBinary(byte x);

#include "md4.h"
const char* Com_MD5File(const char* fn, int length = 0);
const char* Com_MD5Buffer(const byte* buf, size_t len);

extern cvar_t* http_proxy;
extern cvar_t* http_timeout;
extern cvar_t* developer;
extern cvar_t* sv_dedicated;
extern cvar_t* sv_maxclients;
extern cvar_t* sv_gametype;
extern cvar_t* masterserver_url;
extern cvar_t* port;

extern cvar_t* sys_priority;
extern cvar_t* sys_affinity;
extern cvar_t* sys_os;
extern cvar_t* hwclass;

#define MAXCMDLINE 256

typedef void (*vPrintfPtr_t)(const char* fmt, va_list ap);
vPrintfPtr_t Qcommon_GetPrintFunction(void);
void Qcommon_SetPrintFunction(vPrintfPtr_t func);

void Qcommon_Init(int argc, char** argv);
void Qcommon_Frame(void);
void Qcommon_Shutdown(void);
void Com_SetGameType(void);
void Com_ReadFromPipe(void);
float Com_GrenadeTarget(const vec3_t from, const vec3_t at, float speed, bool launched, bool rolled, vec3_t v0);
bool Com_CheckConfigStringIndex(int index);
void Con_Print(const char* txt);

/* Event timing */

typedef void event_func(int now, void* data);
typedef bool event_check_func(int now, void* data);
typedef bool event_delay_func(int now, void* data);
typedef void event_notify_delay_func(int now, void* data, int delay);
/**
 * @return @c true to keep the event, @c false to remove it from the queue
 */
typedef bool event_filter(int when, event_func* func, event_check_func* check, void* data);
typedef void event_clean_func(void* data);

struct scheduleEvent_t {
	int when;
	int delayFollowing;
	event_func* func;
	event_check_func* check;
	/**
	 * @brief Called when the check failed and we have to delay events in the queue
	 * @return @c false if there should be no delay for this event, @c true if it should
	 * get delayed
	 * @param[in] now The current time
	 * @param[in] data The event userdata
	 */
	event_delay_func* delay;
	event_notify_delay_func* notifyDelay;
	void* notifyDelayUserData;
	event_clean_func* clean;
	void* data;
};

typedef SharedPtr<scheduleEvent_t> ScheduleEventPtr;

ScheduleEventPtr Schedule_Event(int when, event_func* func, event_check_func* check, event_clean_func* clean, void* data);
int CL_FilterEventQueue(event_filter* filter);

/*
==============================================================
CLIENT / SERVER SYSTEMS
==============================================================
*/

void CL_Init(void);
void CL_Drop(void);
void CL_Shutdown(void);
int CL_Milliseconds(void);
void CL_Frame(int now, void* data);
void CL_SlowFrame(int now, void* data);
bool CL_ParseClientData(const char* type, const char* name, const char** text);
void SCR_BeginLoadingPlaque(void);
void SCR_EndLoadingPlaque(void);
void CL_InitAfter(void);

void SV_Init(void);
void SV_Clear(void);
void SV_Shutdown(const char* finalmsg, bool reconnect);
void SV_ShutdownWhenEmpty(void);
void SV_Frame(int now, void*);
mapData_t* SV_GetMapData(void);
mapTiles_t* SV_GetMapTiles(void);

/*============================================================================ */

extern memPool_t* com_aliasSysPool;
extern memPool_t* com_cmdSysPool;
extern memPool_t* com_cmodelSysPool;
extern memPool_t* com_cvarSysPool;
extern memPool_t* com_fileSysPool;
extern memPool_t* com_genericPool;
extern memPool_t* com_networkPool;

/*============================================================================ */

int Com_Argc(void);
const char* Com_Argv(int arg);		/* range and null checked */
void Com_ClearArgv(int arg);
const char* Com_MacroExpandString(const char* text);

bool Com_ConsoleCompleteCommand(const char* s, char* target, size_t bufSize, uint32_t* pos, uint32_t offset);

void Key_Init(void);


/** Remove element at index from array of size n.  n gets adjusted to the new
 * array size, so must be an lvalue */
#define REMOVE_ELEM(array, index, n)                                               \
do {                                                                               \
	size_t idx__ = (index);                                                          \
	size_t n__   = --(n);                                                            \
	assert(idx__ <= n__);                                                            \
	memmove((array) + idx__, (array) + idx__ + 1, (n__ - idx__) * sizeof(*(array))); \
	OBJZERO((array)[n__]);                                                           \
} while (0)

/** Same as REMOVE_ELEM() and also updates the idx attribute of every moved
 * element */
#define REMOVE_ELEM_ADJUST_IDX(array, index, n) \
do {                                            \
	size_t idx__ = (index);                       \
	size_t n__;                                   \
	size_t i__;                                   \
	REMOVE_ELEM(array, index, n);                 \
	n__ = (n);                                    \
	for (i__ = idx__; i__ < n__; ++i__)           \
		--(array)[i__].idx;                         \
} while (0)

#define HASH_Add(hash, elem, index) \
do { \
	const int hashIndex = (index); \
	assert(hashIndex >= 0); \
	assert(hashIndex < lengthof(hash)); \
	(elem)->hash_next = (hash)[hashIndex]; \
	(hash)[hashIndex] = (elem); \
} while (0)

#define HASH_Delete(anchor) (*(anchor) = (*(anchor))->hash_next)
