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

const std::string INI_TAG_INCLUDE = "INCLUDE";
const std::string INI_TAG_GLOBAL = "GLOBAL";
const std::string INI_TAG_OPTIONS = "OPTION";
const std::string INI_TAG_MODIFIERS = "MODIFIER";
const std::string INI_TAG_COMBOS = "COMBO";
const std::string INI_TAG_ALPHA_FROM = "ALPHA_FROM";
const std::string INI_TAG_ALPHA_TO = "ALPHA_TO";
const std::string INI_TAG_ALPHA_END = "ALPHA_END";


void keySequenceAppendMakeKey(unsigned short scancode, std::vector<KeyEvent> &sequence);
void keySequenceAppendBreakKey(unsigned short scancode, std::vector<KeyEvent> &sequence);
void keySequenceAppendMakeBreakKey(unsigned short scancode, std::vector<KeyEvent> &sequence);

std::string getSymbolForIKStrokeState(unsigned short state);

bool processCommand();
void processMapAlphaKeys(unsigned short & scancode);
void processModifiedKeys();
void playKeyEventSequence(std::vector<KeyEvent> keyEventSequence);
void processModifierState();
void processKeyToModifierMapping();

void printHelloFeatures();

void sendKeyEvent(KeyEvent keyEvent);

void sendResultingKeyOrSequence();

KeyEvent ikstroke2keyEvent(InterceptionKeyStroke ikStroke);

void normalizeIKStroke(InterceptionKeyStroke &ikstroke);
InterceptionKeyStroke keyEvent2ikstroke(KeyEvent keyEvent);
void getHardwareId();

void initConsoleWindow();

void printHelloHeader();
void printStatus();
void printHelp();
void reset();
std::vector<std::string> assembleLayerConfig(int layer);
bool parseIni(int layer);
void switchLayer(int layer);
void resetCapsNumScrollLock();
void resetAlphaMap();

void resetLoopState();

void resetAllStatesToDefault();

#define IFDEBUG if(option.debug) 

