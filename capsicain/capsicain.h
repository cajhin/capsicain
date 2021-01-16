#define PROGRAM_NAME_AHK "autohotkey.exe"
#include "interception.h"
#include "utils.h"
#include "configUtils.h"
#include "traybar.h"

#define IFDEBUG if(options.debug)

enum KEYSTATE
{
    KEYSTATE_DOWN = 0,
    KEYSTATE_UP = 1,
    KEYSTATE_E0_DOWN = 2,
    KEYSTATE_E0_UP = 3,
    KEYSTATE_E1_DOWN = 4,
    KEYSTATE_E1_UP = 5
};

void keySequenceAppendMakeKey(unsigned short scancode, std::vector<VKeyEvent> &sequence);
void keySequenceAppendBreakKey(unsigned short scancode, std::vector<VKeyEvent> &sequence);
void keySequenceAppendMakeBreakKey(unsigned short scancode, std::vector<VKeyEvent> &sequence);

std::string getSymbolForIKStrokeState(unsigned short state);

bool processCommand();
void processMapAlphaKeys();
void processCombos();
void DetectTapping(const VKeyEvent &originalVKeyEvent);
void playKeyEventSequence(std::vector<VKeyEvent> keyEventSequence);
void processModifierState();
void processRewireScancodeToVirtualcode();

void printOptions();

void sendVKeyEvent(VKeyEvent keyEvent);

void sendResultingKeyOrSequence();

VKeyEvent ikstroke2VKeyEvent(InterceptionKeyStroke ikStroke);

void normalizeIKStroke(InterceptionKeyStroke &ikstroke);
InterceptionKeyStroke vkeyEvent2ikstroke(VKeyEvent keyEvent);
void getHardwareId();

bool initConsoleWindow();

void printHelloHeader();
void printStatus();
void printKeylabels();
void printHelp();
void printIKStrokeState(InterceptionKeyStroke iks);
void printLoopState1Incoming();
void printLoopState2Modifier();
void printLoopState3Timing();
void printLoopState4TapState();


void reset();
void reload();
void releaseAllSentKeys();
std::vector<std::string> assembleConfig(int config);
void switchConfig(int config, bool forceReloadSameLayer);
void resetCapsNumScrollLock();

int obfuscateVKey(int vk);
int deObfuscateVKey(int vk);

