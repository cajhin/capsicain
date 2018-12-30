#define PROGRAM_NAME_AHK "autohotkey.exe"
#include "interception.h"
#include "utils.h"

enum KEYSTATE
{
	KEYSTATE_DOWN = 0,
	KEYSTATE_UP = 1,
	KEYSTATE_EXT_DOWN = 2,
	KEYSTATE_EXT_UP = 3,
};

void macroMakeKey(unsigned short scancode);
void macroBreakKey(unsigned short scancode);
void macroMakeBreakKey(unsigned short scancode);

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
void playStrokeSequence(std::vector<Stroke> strokeSequence);
void processModifierState();
void processLayoutIndependentAction();

void printHelloHelp();

void processCaps();

void sendStroke(Stroke stroke);

void sendResultingKeyOrMacro();

void processLayoutDependentActions();

void scancode2ikstroke(unsigned short &scancode, InterceptionKeyStroke &istroke);

Stroke ikstroke2stroke(InterceptionKeyStroke ikStroke);

void normalizeKeyStroke(InterceptionKeyStroke &istroke);
InterceptionKeyStroke stroke2ikstroke(Stroke stroke);
void getHardwareId();

void printHelloHeader();

void printStatus();
void printHelp();
void reset();

#define IFDEBUG if(mode.debug) 


