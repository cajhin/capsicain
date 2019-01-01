#define PROGRAM_NAME_AHK "autohotkey.exe"
#include "interception.h"
#include "utils.h"
#include "config.h"

enum KEYSTATE
{
	KEYSTATE_DOWN = 0,
	KEYSTATE_UP = 1,
	KEYSTATE_EXT_DOWN = 2,
	KEYSTATE_EXT_UP = 3,
};

void keySequenceAppendMakeKey(unsigned short scancode, std::vector<Stroke> &sequence);
void keySequenceAppendBreakKey(unsigned short scancode, std::vector<Stroke> &sequence);
void keySequenceAppendMakeBreakKey(unsigned short scancode, std::vector<Stroke> &sequence);

std::string getSymbolForStrokeState(unsigned short state);

void processCommand();
void processMapAlphaKeys(unsigned short & scancode);
void processBufferedScancode();
void processRemapModifiers();
void playStrokeSequence(std::vector<Stroke> strokeSequence);
void processModifierState();
void processLayoutIndependentAction();

void printHelloFeatures();

void sendStroke(Stroke stroke);

void sendResultingKeyOrSequence();

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

void resetAlphaMap();
