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

const std::string INI_SECTNAME_STARTUP = "STARTUP";
const std::string INI_SECTNAME_OPTIONS = "OPTIONS";
const std::string INI_SECTNAME_MODMAP = "MAP_KEY_TOMODIFIER_TOIFTAPPED";
const std::string INI_SECTNAME_COMBOMAP = "MAP_ALPHA_LAYER";
const std::string INI_SECTNAME_ALPHA = "MAP_ALPHA_LAYER";


void keySequenceAppendMakeKey(unsigned short scancode, std::vector<KeyEvent> &sequence);
void keySequenceAppendBreakKey(unsigned short scancode, std::vector<KeyEvent> &sequence);
void keySequenceAppendMakeBreakKey(unsigned short scancode, std::vector<KeyEvent> &sequence);

std::string getSymbolForIKStrokeState(unsigned short state);

bool processCommand();
void processMapAlphaKeys(unsigned short & scancode);
void processModifiedKeys();
void processModifierTapped();
void playKeyEventSequence(std::vector<KeyEvent> keyEventSequence);
void processModifierState();
void processKeyToModifierMapping();
void processLayoutIndependentAction();

void printHelloFeatures();

void sendKeyEvent(KeyEvent keyEvent);

void sendResultingKeyOrSequence();

void scancode2ikstroke(unsigned short &scancode, InterceptionKeyStroke &ikstroke);

KeyEvent ikstroke2keyEvent(InterceptionKeyStroke ikStroke);

void normalizeIKStroke(InterceptionKeyStroke &ikstroke);
InterceptionKeyStroke keyEvent2ikstroke(KeyEvent keyEvent);
void getHardwareId();

void initConsoleWindow();

bool readIniAlphaMappingLayer(int layer);

void printHelloHeader();

void printStatus();
void printHelp();
void reset();
bool readIni();
void resetCapsNumScrollLock();
void resetAlphaMap();

void resetLoopState();

void resetAllStatesToDefault();

#define IFDEBUG if(feature.debug) 

