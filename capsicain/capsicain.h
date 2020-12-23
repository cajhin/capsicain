#define PROGRAM_NAME_AHK "autohotkey.exe"
#include "interception.h"
#include "utils.h"
#include "config.h"
#include "traybar.h"

#define IFDEBUG if(option.debug)

enum KEYSTATE
{
    KEYSTATE_DOWN = 0,
    KEYSTATE_UP = 1,
    KEYSTATE_EXT_DOWN = 2,
    KEYSTATE_EXT_UP = 3,
};

void keySequenceAppendMakeKey(unsigned short scancode, std::vector<VKeyEvent> &sequence);
void keySequenceAppendBreakKey(unsigned short scancode, std::vector<VKeyEvent> &sequence);
void keySequenceAppendMakeBreakKey(unsigned short scancode, std::vector<VKeyEvent> &sequence);

std::string getSymbolForIKStrokeState(unsigned short state);

bool processCommand();
void processMapAlphaKeys(int & scancode);
void processModifiedKeys();
void playKeyEventSequence(std::vector<VKeyEvent> keyEventSequence);
void processModifierState();
void processRewireScancodeToVirtualcode();

void printHelloFeatures();

void sendVKeyEvent(VKeyEvent keyEvent);

void sendResultingKeyOrSequence();

VKeyEvent ikstroke2VKeyEvent(InterceptionKeyStroke ikStroke);

void normalizeIKStroke(InterceptionKeyStroke &ikstroke);
InterceptionKeyStroke vkeyEvent2ikstroke(VKeyEvent keyEvent);
void getHardwareId();

void initConsoleWindow();

void printHelloHeader();
void printStatus();
void printKeylabels();
void printHelp();
void reset();
void releaseAllSentKeys();
std::vector<std::string> assembleLayerConfig(int layer);
bool parseIni(int layer);
void switchLayer(int layer);
void resetCapsNumScrollLock();
void resetAlphamap();
void resetRewiremap();

void resetLoopState();
void resetModifierState();
void resetAllStatesToDefault();

