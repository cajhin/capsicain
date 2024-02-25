#include <string>
#include <map>

#include "interception.h"
#include "utils.h"
#include "configUtils.h"
#include "traybar.h"

#define IFDEBUG if(options.debug && !globalState.secretSequenceRecording)
#define IFTRACE if(false)
#define IFPROF if(false) //measuring time takes some time

struct Executable
{
    std::string verb;
    std::string path;
    std::string args;
    std::string dir;
    int mode;
    HANDLE proc;
    DWORD pid;
    HWND hwnd;
};

struct Device {
    std::string id;
    bool keyboard;
    bool apple;
};

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

bool processOnOffKey();
void InterceptionSendCurrentKeystroke();
bool processCommand();
void processModifierState();
bool processMessyKeys();
void processRewireScancodeToVirtualcode();
void processCombos();
void processMapAlphaKeys();

void detectTapping();
void playKeyEventSequence(std::vector<VKeyEvent> keyEventSequence);

void printOptions();

void sendVKeyEvent(VKeyEvent keyEvent, bool hold = true);

void sendResultingKeyOrSequence();

VKeyEvent convertIkstroke2VKeyEvent(InterceptionKeyStroke ikStroke);

void normalizeIKStroke(InterceptionKeyStroke &ikstroke);
InterceptionKeyStroke convertVkeyEvent2ikstroke(VKeyEvent keyEvent);
std::map<uint8_t, Device>* getHardwareId(bool refresh = true);

bool initConsoleWindow();
void parseIniGlobals();

void printHelloHeader();
void printStatus();
void printKeylabels();
void printHelp();
void printIKStrokeState(InterceptionKeyStroke iks);
void printLoopState1Input();
void printLoopState2Modifier();
void printLoopStateMappingTime(long us);
void printLoopState4TapState();


void reset();
void reload();
void releaseAllSentKeys();
std::vector<std::string> assembleConfig(int config);
void switchConfig(int config, bool forceReloadSameLayer);
void resetCapsNumScrollLock();

int obfuscateVKey(int vk);
int deObfuscateVKey(int vk);

int getKeyHolding(int vcode);
bool runExecutable(Executable &exe);
void killExecutable(Executable &exe);
void killExecutableByPath(std::string path);

void loadAHK();
void unloadAHK();
void sendAHK(std::string msg);
