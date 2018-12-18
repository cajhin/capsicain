#define PROGRAM_NAME_AHK "autohotkey.exe"
#include "interception.h"

enum KEYSTATE
{
	KEYSTATE_DOWN = 0,
	KEYSTATE_UP = 1,
	KEYSTATE_EXT_DOWN = 2,
	KEYSTATE_EXT_UP = 3,
};

void makeKeyMacro(unsigned short scancode);
void breakKeyMacro(unsigned short scancode);
void makeBreakKeyMacro(unsigned short scancode);

void createMacroKeyCombo(int a, int b, int c, int d);
void createMacroKeyComboRemoveShift(int a, int b, int c, int d);
void createMacroKeyCombo10timesIfAltDown(int a, int b, int c, int d, unsigned short modifiers);
void createMacroKeyComboNtimes(int a, int b, int c, int d, int repeat);

void createMacroAltNumpad(unsigned short a, unsigned short b, unsigned short c, unsigned short d);
void processCapsTapped(unsigned short scancd, int charCrtMode);

std::string getSymbolForStrokeState(unsigned short state);

void playMacro(InterceptionKeyStroke macro[], int macroLength);

void processCommand();
void processAlphaMappingTable(unsigned short & scancode);
void processBufferedScancode();
void processRemapModifiers();
void processTrackModifierState();
void processLayoutIndependentAction();

void printHello();

void processCaps();

void sendStroke(InterceptionKeyStroke stroke);
void sendResultingKeyOrMacro();

void processLayoutDependentActions();

void scancode2stroke(unsigned short &scancode, InterceptionKeyStroke &istroke);
void stroke2scancode(InterceptionKeyStroke &stroke, unsigned short &scancode);

void normalizeKeyStroke(InterceptionKeyStroke &istroke);
void getHardwareId();

void printStatus();
void printHelp();
void reset();

#define IFDEBUG if(mode.debug) 

#define BITMASK_LSHIFT		0x001
#define BITMASK_RSHIFT		0x010
#define BITMASK_LCONTROL	0x002
#define BITMASK_RCONTROL	0x020
#define BITMASK_LALT		0x004
#define BITMASK_RALT		0x040
#define BITMASK_LWIN		0x008
#define BITMASK_RWIN		0x080
#define BITMASK_CAPS		0x100
#define BITMASK_TAB			0x200
#define BITMASK_ESC			0x400

#define IS_SHIFT_DOWN (state.modifiers & 0x01 || state.modifiers & 0x10)
#define IS_LSHIFT_DOWN (state.modifiers & 0x01)
#define IS_RSHIFT_DOWN (state.modifiers & 0x10)
#define IS_LCONTROL_DOWN (state.modifiers & 0x02)
#define IS_LALT_DOWN (state.modifiers & 0x04)
#define IS_LWIN_DOWN (state.modifiers & 0x08)

