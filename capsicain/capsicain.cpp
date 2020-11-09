#pragma once;

#include "pch.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <string>
#include <Windows.h>  //for Sleep()

#include "capsicain.h"
#include "modifiers.h"
#include "scancodes.h"

using namespace std;

const string VERSION = "58";

string SCANCODE_LABELS[256]; // contains e.g. [01]="ESC" instead of SC_ESCAPE 

const int DEFAULT_ACTIVE_LAYER = 0;
const string DEFAULT_ACTIVE_LAYER_NAME = "no processing, forward everything";

const int DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS = 5;  //System may drop keys when they are sent too fast. Local host needs 0-1ms, Linux VM 5+ms for 100% reliable keystroke detection
const int MAX_MACRO_LENGTH = 200;  //stop recording at some point if it was forgotten.

const bool DEFAULT_START_AHK_ON_STARTUP = true;
const int DEFAULT_DELAY_FOR_AHK = 50;	    //autohotkey is slow
const unsigned short AHK_HOTKEY1 = SC_F14;  //this key triggers supporting AHK script
const unsigned short AHK_HOTKEY2 = SC_F15;

vector<string> sanitizedIniContent;  //loaded on startup and reset

struct Options
{
    string iniVersion = "unnamed version - add 'iniVersion xyz' to capsicain.ini";
    bool debug = false;
    int delayForKeySequenceMS = DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;
    bool flipZy = false;
    bool altAltToAlt = false;
    bool shiftShiftToCapsLock = false;
    unsigned char secondEscapeKey = SC_NOP;
    bool flipAltWinOnAppleKeyboards = false;
    bool LControlLWinBlocksAlphaMapping = false;
    bool processOnlyFirstKeyboard = false;
} option;

struct ModifierCombo
{
    unsigned short key = SC_NOP;
    unsigned short modAnd = 0;
    unsigned short modNot = 0;
    unsigned short modTap = 0;
    vector<KeyEvent> keyEventSequence;
};

struct RewireIftappedMapping
{
    unsigned char inkey = SC_NOP;
    unsigned char outkey = SC_NOP;
    unsigned char ifTapped = SC_NOP;
};

struct AllMaps
{
    vector<RewireIftappedMapping> rewireIftappedMapping;
    vector<ModifierCombo> modCombos;
    unsigned char alphamap[256] = { SC_NOP };
} allMaps;

struct GlobalState
{
    int  activeLayer = DEFAULT_ACTIVE_LAYER;
    string layerName = DEFAULT_ACTIVE_LAYER_NAME;

    InterceptionContext interceptionContext = NULL;
    InterceptionDevice interceptionDevice = NULL;
    InterceptionDevice previousInterceptionDevice = NULL;

    bool escapeIsDown = false;

    string deviceIdKeyboard = "";
    bool deviceIsAppleKeyboard = false;

    KeyEvent previousKeyEventIn = { SC_NOP, 0 };
    KeyEvent previous2KeyEventIn = { SC_NOP, 0 }; //2 keys ago
    KeyEvent previousKeyEventInStore = { SC_NOP, 0 }; //mostly useless but simplifies the handling
    bool keysDownSent[256] = { false };

    bool recordingMacro = false;
    vector<KeyEvent> recordedMacro;

    vector<KeyEvent> username;
    vector<KeyEvent> password;
    bool readingUsername = false;
    bool readingPassword = false;
} globalState;

struct ModifierState
{
    unsigned short modifierDown = 0;
    unsigned short modifierTapped = 0;
    bool lastModBrokeTapping = false;

    vector<KeyEvent> modsTempAltered;

} modifierState;

struct LoopState
{
    unsigned short scancode = 0;
    bool isDownstroke = false;
    bool isModifier = false;
    bool wasTapped = false;

    InterceptionKeyStroke originalIKstroke = { SC_NOP, 0 };
    KeyEvent originalKeyEvent = { SC_NOP, false };

    bool blockKey = false;  //true: do not send the current key
    vector<KeyEvent> resultingKeyEventSequence;

    chrono::steady_clock::time_point loopStartTimepoint;
} loopState;

string errorLog = "";
void error(string txt)
{
    cout << endl << "ERROR: " << txt << endl;
    errorLog += "\r\n" + txt;
}


void readGlobalsOnStartup()
{
    int initialLayer = DEFAULT_ACTIVE_LAYER;
    if(!getIntValueForTaggedKey(INI_TAG_GLOBAL, "ActiveLayerOnStartup", globalState.activeLayer, sanitizedIniContent))
    {
        globalState.activeLayer = DEFAULT_ACTIVE_LAYER;
        IFDEBUG cout << endl << "No ini setting for 'ActiveLayerOnStartup'. Setting default layer " << DEFAULT_ACTIVE_LAYER;
    }

    option.debug = configHasTaggedKey(INI_TAG_GLOBAL, "debugOnStartup", sanitizedIniContent);

    getStringValueForTaggedKey(INI_TAG_GLOBAL, "iniVersion", option.iniVersion, sanitizedIniContent);
    if (option.iniVersion == "")
        option.iniVersion = "GLOBAL iniVersion is undefined";
}

int main()
{
    initConsoleWindow();
    printHelloHeader();

    cout << endl << "Release all keys now..." << endl;
    Sleep(1000); //time to release shortcut keys that started capsicain

    getAllScancodeLabels(SCANCODE_LABELS);
    resetAllStatesToDefault();

    if (!readIniFile(sanitizedIniContent))
    {
        cout << endl << "No capsicain.ini - exiting..." << endl;
        Sleep(5000);
        return 0;
    }

    readGlobalsOnStartup();
    switchLayer(globalState.activeLayer);

    globalState.interceptionContext = interception_create_context();
    interception_set_filter(globalState.interceptionContext, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);

    printHelloFeatures();
    if (DEFAULT_START_AHK_ON_STARTUP)
    {
        string msg = startProgramSameFolder(PROGRAM_NAME_AHK);
        cout << endl << endl << "starting AHK... ";
        cout << (msg == "" ? "OK" : "Not. '" + msg + "'");
    }
    cout << endl << endl << "[ESC] + [X] to stop." << endl << "[ESC] + [H] for Help";
    cout << endl << endl << "capsicain running.... ";

    raise_process_priority(); //careful: if we spam key events, other processes get no timeslots to process them. Sleep a bit...

    while (interception_receive( globalState.interceptionContext,
                                 globalState.interceptionDevice = interception_wait(globalState.interceptionContext),
                                 (InterceptionStroke *)&loopState.originalIKstroke, 1)   > 0)
    {
         IFDEBUG cout << ". ";
        //ignore secondary keyboard?
        if (option.processOnlyFirstKeyboard 
            && (globalState.previousInterceptionDevice != NULL)
            && (globalState.previousInterceptionDevice != globalState.interceptionDevice))
        {
            IFDEBUG cout << endl << "Ignore 2nd board (" << globalState.interceptionDevice << ") scancode: " << loopState.originalIKstroke.code;
            interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&loopState.originalIKstroke, 1);
            continue;
        }

        //check for Apple Keyboard / device ID
        if (globalState.previousInterceptionDevice == NULL    //startup
            || globalState.previousInterceptionDevice != globalState.interceptionDevice)  //keyboard changed
        {
            getHardwareId();
            cout << endl << "new keyboard: " << (globalState.deviceIsAppleKeyboard ? "Apple keyboard" : "IBM keyboard");
            resetCapsNumScrollLock();
            globalState.previousInterceptionDevice = globalState.interceptionDevice;
        }

        loopState.loopStartTimepoint = timepointNow();
        resetLoopState();

        //copy InterceptionKeyStroke (unpleasant to use) to plain KeyEvent
        loopState.originalKeyEvent = ikstroke2keyEvent(loopState.originalIKstroke);

        //store last key and the one before
        globalState.previous2KeyEventIn = globalState.previousKeyEventIn;
        globalState.previousKeyEventIn = globalState.previousKeyEventInStore;
        globalState.previousKeyEventInStore = loopState.originalKeyEvent;
        
        loopState.scancode = loopState.originalKeyEvent.scancode;
        loopState.isDownstroke = loopState.originalKeyEvent.isDownstroke;
        loopState.wasTapped = !loopState.isDownstroke 
            && (loopState.originalKeyEvent.scancode == globalState.previousKeyEventIn.scancode)
            && ((loopState.originalKeyEvent.scancode != globalState.previous2KeyEventIn.scancode)
                || (!globalState.previous2KeyEventIn.isDownstroke));

        //sanity check
        if (loopState.originalIKstroke.code >= 0x80)
        {
            error("Received unexpected extended Interception Key Stroke code > 0x79: " + to_string(loopState.originalIKstroke.code));
            continue;
        }
        if (loopState.originalIKstroke.code == 0)
        {
            error("Received unexpected SC_NOP Key Stroke code 0. Ignoring this.");
            continue;
        }

        //ESC+X exits, unconditionally
        if (loopState.scancode == SC_X && loopState.isDownstroke
            && globalState.previousKeyEventIn.scancode == SC_ESCAPE && globalState.previousKeyEventIn.isDownstroke)
        {
            break;
        }

        //ESC+[0]..[9]  layer switch
        if (loopState.isDownstroke
            && globalState.previousKeyEventIn.scancode == SC_ESCAPE && globalState.previousKeyEventIn.isDownstroke)
        {
            if (loopState.scancode == SC_0)
            {
                cout << endl << "LAYER CHANGE: 0";
                switchLayer(DEFAULT_ACTIVE_LAYER);
                resetAlphaMap();
                continue;
            }
            else if (loopState.scancode >= SC_1 && loopState.scancode <= SC_9)
            {
                int layer = loopState.scancode - 1;
                cout << endl << "LAYER CHANGE: " << layer;
                switchLayer(layer);
                continue;
            }
        }

        //Layer 0: standard keyboard, just forward everything
        if (globalState.activeLayer == DEFAULT_ACTIVE_LAYER)
        {
            interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&loopState.originalIKstroke, 1);
            continue;
        }

        //hard rewire everything but esc-x and layer switches
        processRewire();
        if (loopState.scancode == SC_NOP)   //used the mapping to disable keys?
        {
            IFDEBUG cout << " (NOP)";
            continue;
        }

        //ESC tapped -> reset state, send ESC
        if (loopState.scancode == SC_ESCAPE)
        {
            globalState.escapeIsDown = loopState.isDownstroke;
            IFDEBUG cout << endl << " ESC" << (loopState.isDownstroke ? "v " : "^ ");
            if (!loopState.isDownstroke 
                && globalState.previousKeyEventIn.scancode == SC_ESCAPE)
            {
                sendKeyEvent({ SC_ESCAPE, true });
                sendKeyEvent({ SC_ESCAPE, false });
                    resetLoopState();
                resetModifierState();
                releaseAllRealModifiers();
            }
            continue;
        }

        // ESC + Y commands
        // NOTE: some major key shadowing here...
        // - cherry is good
        // - apple keyboard cannot do RCTRL+CAPS+ESC and ESC+Caps shadows the entire row a-s-d-f-g-....
        // - Dell cant do ctrl-caps-x
        // - Cypher has no RControl... :(
        // - HP shadows the 2-w-s-x and 3-e-d-c lines
        if(globalState.escapeIsDown)
        {
            if (loopState.isDownstroke)
            {
                if (processCommand())
                {
                    continue;
                }
                else
                    break;
            }
            else if (isModifier(loopState.scancode))  //suppress key up in Esc+modifier combos
            {
                continue;
            }
        }

        //Username
        if (globalState.readingUsername)
        {
            if (loopState.scancode == SC_RETURN)
            {
                globalState.readingUsername = false;
                cout << " done";
            }
            else if (loopState.isDownstroke || globalState.username.size() > 0)  //filter upstroke from the command key
                globalState.username.push_back(loopState.originalKeyEvent);
            continue;
        }
        //Password
        if (globalState.readingPassword)
        {
            if (loopState.scancode == SC_RETURN)
            {
                globalState.readingPassword = false;
                cout << " done";
            }
            else if (loopState.isDownstroke || globalState.password.size() > 0)  //filter upstroke from the command key
                globalState.password.push_back(loopState.originalKeyEvent);
            continue;
        }

        /////CONFIGURED RULES
        IFDEBUG cout << endl << " [" << SCANCODE_LABELS[loopState.scancode] << getSymbolForIKStrokeState(loopState.originalIKstroke.state)
            << " =" << hex << loopState.originalIKstroke.code << " " << loopState.originalIKstroke.state << "]";
        
        processModifierState();
    
        IFDEBUG cout << " [M:" << hex << modifierState.modifierDown;
        IFDEBUG if (modifierState.modifierTapped)  cout << " TAP:" << hex << modifierState.modifierTapped;
        IFDEBUG cout << "] ";

        //evaluate modified keys
        if (!loopState.isModifier && (modifierState.modifierDown > 0 || modifierState.modifierTapped > 0))
        {
            processModifiedKeys();
        }

        //basic character key layout. Don't remap the Ctrl combos?
        if (!loopState.isModifier && 
            !(option.LControlLWinBlocksAlphaMapping && (IS_LCTRL_DOWN || IS_LWIN_DOWN) ))
        {
            processMapAlphaKeys(loopState.scancode);
            if (option.flipZy)
            {
                switch (loopState.scancode)
                {
                case SC_Y:		loopState.scancode = SC_Z;		break;
                case SC_Z:		loopState.scancode = SC_Y;		break;
                }
            }
        }

        IFDEBUG cout << "\t (" << dec << millisecondsSinceTimepoint(loopState.loopStartTimepoint) << " ms)";
        IFDEBUG loopState.loopStartTimepoint = timepointNow();
        sendResultingKeyOrSequence();
        IFDEBUG cout << "\t (" << dec << millisecondsSinceTimepoint(loopState.loopStartTimepoint) << " ms)";
    }
    interception_destroy_context(globalState.interceptionContext);

    cout << endl << "bye" << endl;
    return 0;
}
////////////////////////////////////END MAIN//////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////


void processModifiedKeys()
{
    if (loopState.isDownstroke)
    {
        for (ModifierCombo modcombo : allMaps.modCombos)
        {
            if (modcombo.key == loopState.scancode)
            {
                if (
                    (modifierState.modifierDown & modcombo.modAnd) == modcombo.modAnd &&
                    (modifierState.modifierDown & modcombo.modNot) == 0 &&
                    ((modifierState.modifierTapped & modcombo.modTap) == modcombo.modTap)
                    )
                {
                    loopState.resultingKeyEventSequence = modcombo.keyEventSequence;
                    break;
                }
            }
        }
    }
    modifierState.modifierTapped = 0;
}
void processMapAlphaKeys(unsigned short &scancode)
{
    if (scancode > 0xFF)
    {
        error("Unexpected scancode > 255 while mapping alphachars: " + std::to_string(scancode));
    }
    else
    {
        scancode = allMaps.alphamap[scancode];
    }
}

// [ESC]+x combos
// returns false if exit was requested
bool processCommand()
{
    bool continueLooping = true;
    cout << endl << endl << "::";

    switch (loopState.scancode)
    {
    case SC_Q:   // quit only if a debug build
#ifdef NDEBUG
        sendKeyEvent({ SC_ESCAPE, true });
        sendKeyEvent({ SC_Q, true });
        sendKeyEvent({ SC_Q, false });
        sendKeyEvent({ SC_ESCAPE, false });
#else
        continueLooping = false;
#endif
        break;
    case SC_W:
        option.flipAltWinOnAppleKeyboards = !option.flipAltWinOnAppleKeyboards;
        cout << "Flip ALT<>WIN for Apple boards: " << (option.flipAltWinOnAppleKeyboards ? "ON" : "OFF") << endl;
        break;
    case SC_E:
        cout << "ERROR LOG: " << endl << errorLog << endl;
        break;
    case SC_R:
        cout << "RESET";
        reset();
        getHardwareId();
        cout << endl << (globalState.deviceIsAppleKeyboard ? "APPLE keyboard (flipping Win<>Alt)" : "PC keyboard");
        break;
    case SC_Y:
        cout << "Stop AHK";
        closeOrKillProgram("autohotkey.exe");
        break;
    case SC_U:
    {
        if (globalState.username.size() == 0)
        {
            cout << "Type Username (Enter)";
            globalState.readingUsername = true;
        }
        else
        {
            cout << "Play user";
            playKeyEventSequence(globalState.username);
        }
        break;
    }
    case SC_I:
    {
        cout << "INI filtered for layer " << globalState.layerName;
        vector<string> assembledLayer = assembleLayerConfig(globalState.activeLayer);
        for (string line : assembledLayer)
            cout << endl << line;
        break;
    }
    case SC_P:
    {
        if (globalState.password.size() == 0)
        {
            cout << "Type Password (Enter)";
            globalState.readingPassword = true;
        }
        else
        {
            cout << "Play password";
            playKeyEventSequence(globalState.password);
        }
        break;
    }
    case SC_A:
    {
        cout << "Start AHK";
        string msg = startProgramSameFolder("autohotkey.exe");
        if (msg != "")
            cout << endl << "Cannot start: " << msg;
        break;
    }
    case SC_S:
        printStatus();
        break;
    case SC_D:
        option.debug = !option.debug;
        cout << "DEBUG mode: " << (option.debug ? "ON" : "OFF");
        break;
    case SC_H:
        printHelp();
        break;
    case SC_J:
        cout << "MACRO START RECORDING";
        globalState.recordingMacro = true;
        globalState.recordedMacro.clear();
        break;
    case SC_K:
        if (globalState.recordingMacro)
            cout << "MACRO STOP RECORDING (" << globalState.recordedMacro.size() << ")";
        else
            cout << "MACRO RECORDING ALREADY STOPPED";
        globalState.recordingMacro = false;
        break;
    case SC_L:
        cout << "MACRO PLAYBACK";
        playKeyEventSequence(globalState.recordedMacro);
        break;
    case SC_SEMI:
    {
        cout << "COPY MACRO TO CLIPBOARD";
        string macro = "";
        for (KeyEvent key : globalState.recordedMacro)
        {
            if (macro.size() > 0)
                macro += "_";
            if (key.isDownstroke)
                macro += "&";
            else
                macro += "^";
            macro += SCANCODE_LABELS[key.scancode];
        }
        copyToClipBoard(macro);
        break;
    }
    case SC_Z:
        option.flipZy = !option.flipZy;
        cout << "Flip Z<>Y mode: " << (option.flipZy ? "ON" : "OFF");
        break;
    case SC_C:
        cout << "List of all Key Labels for scancodes" << endl
             << "------------------------------------" << endl;
        printKeylabels();
        break;
    case SC_COMMA:
        if (option.delayForKeySequenceMS >= 1)
            option.delayForKeySequenceMS -= 1;
        cout << "delay between characters in key sequences (ms): " << dec << option.delayForKeySequenceMS;
        break;
    case SC_DOT:
        if (option.delayForKeySequenceMS <= 100)
            option.delayForKeySequenceMS += 1;
        cout << "delay between characters in key sequences (ms): " << dec << option.delayForKeySequenceMS;
        break;
    default: 
    {
        //[ESC]+modifier locks the modifier. Tap [ESC] to release.
        if (isModifier(loopState.scancode))
        {
            modifierState.modifierDown |= getBitmaskForModifier(loopState.scancode);
            modifierState.modifierTapped = 0;
            if (isRealModifier(loopState.scancode))
                sendKeyEvent(loopState.originalKeyEvent);
            IFDEBUG cout << endl << "Locking modifier: " << SCANCODE_LABELS[loopState.scancode];
        }
        else
            cout << "Unknown command";
        break;
    }
    }

    return continueLooping;
}


void sendResultingKeyOrSequence()
{
    if (loopState.resultingKeyEventSequence.size() > 0)
    {
        playKeyEventSequence(loopState.resultingKeyEventSequence);
    }
    else
    {
        IFDEBUG
        {
            if (loopState.blockKey)
                cout << "\t--> BLOCKED ";
            else if (loopState.originalKeyEvent.scancode != loopState.scancode)
                cout << "\t--> " << SCANCODE_LABELS[loopState.scancode] << " " << getSymbolForIKStrokeState(loopState.originalIKstroke.state);
            else
                cout << "\t--";
        }
        if (!loopState.blockKey && !loopState.scancode == SC_NOP)
        {
            sendKeyEvent({ loopState.scancode, loopState.isDownstroke });
        }
    }
}

void processModifierState()
{
    if (!loopState.isModifier)
        return; 

    unsigned short bitmask = getBitmaskForModifier(loopState.scancode);
    if ((bitmask & 0xFF00) > 0)
        loopState.blockKey = true;

    //a configured tapped mod?
    bool tappedModConfigFound = false;
    if (loopState.wasTapped)
    {
        for (RewireIftappedMapping map : allMaps.rewireIftappedMapping)
        {
            if (loopState.originalKeyEvent.scancode == map.inkey)
                {
                if (map.ifTapped != SC_NOP)
                {
                    if (!loopState.blockKey)
                        loopState.resultingKeyEventSequence.push_back({ loopState.scancode, false });
                    loopState.resultingKeyEventSequence.push_back({ map.ifTapped, true });
                    loopState.resultingKeyEventSequence.push_back({ map.ifTapped, false });
                    loopState.blockKey = false;
                    tappedModConfigFound = true;
                    modifierState.modifierTapped = 0;
                }
                break;
            }
        }
    }

    //set internal modifier state
    if (loopState.isDownstroke)
    {
        modifierState.lastModBrokeTapping = modifierState.modifierTapped & bitmask;
        modifierState.modifierDown |= bitmask;
    }
    else
        modifierState.modifierDown &= ~bitmask;

    //Set tapped bitmask. You can combine mod-taps (like tap-Ctrl then tap-Alt).
    //Double-tap clears all taps.
    //Long presses, release will not register as tapped.
    if (!tappedModConfigFound && !modifierState.lastModBrokeTapping)
    {
        bool sameKey = loopState.originalKeyEvent.scancode == globalState.previousKeyEventIn.scancode;
        if (loopState.wasTapped && modifierState.lastModBrokeTapping)
            modifierState.modifierTapped = 0;
        else if (loopState.wasTapped)
            modifierState.modifierTapped |= bitmask;
        else if (sameKey && loopState.isDownstroke)
        {
            if (modifierState.modifierTapped & bitmask)
            {
                modifierState.lastModBrokeTapping = true;
                modifierState.modifierTapped &= ~bitmask;
            }
            else
                modifierState.lastModBrokeTapping = false;
        }
        else
            modifierState.lastModBrokeTapping = false;
    }
}

void processRewire()
{
    if (option.flipAltWinOnAppleKeyboards && globalState.deviceIsAppleKeyboard)
    {
        switch (loopState.scancode)
        {
        case SC_LALT: loopState.scancode = SC_LWIN; break;
        case SC_LWIN: loopState.scancode = SC_LALT; break;
        case SC_RALT: loopState.scancode = SC_RWIN; break;
        case SC_RWIN: loopState.scancode = SC_RALT; break;
        }
    }

    bool finalScancode = false;

    if (option.altAltToAlt)
    {
        if (loopState.scancode == SC_LALT || loopState.scancode == SC_RALT)
        {
            if (loopState.isDownstroke)
            {
                if (globalState.previousKeyEventIn.scancode != loopState.originalKeyEvent.scancode)
                {
                    if (IS_MOD12_DOWN)
                    {
                        modifierState.modifierDown &= ~BITMASK_MOD12;
                        finalScancode = true;
                    }
                }
                else
                {
                    if (modifierState.modifierDown & (BITMASK_LALT | BITMASK_RALT))
                        finalScancode = true;
                    else if (modifierState.modifierTapped & BITMASK_MOD12)
                    {
                        modifierState.modifierTapped &= ~BITMASK_MOD12;
                        finalScancode = true;
                    }
                }
            }
            else
            {
                if(    loopState.scancode == SC_LALT && IS_LALT_DOWN
                    || loopState.scancode == SC_RALT && IS_RALT_DOWN)
                    finalScancode = true;
            }
        }
    }

    if (!finalScancode)
    {
        for (RewireIftappedMapping map : allMaps.rewireIftappedMapping)
        {
            if (loopState.scancode == map.inkey)
            {
                loopState.scancode = map.outkey;
                break;
            }
        }
    }

    if (!finalScancode && option.shiftShiftToCapsLock)
    {
        switch (loopState.scancode)
        {
        case SC_LSHIFT:  //handle LShift+RShift -> CapsLock
            if (loopState.isDownstroke
                && (modifierState.modifierDown == (BITMASK_RSHIFT))
                && (GetKeyState(VK_CAPITAL) & 0x0001)) //ask Win for Capslock state
            {
                keySequenceAppendMakeBreakKey(SC_CAPS, loopState.resultingKeyEventSequence);
                finalScancode = true;
            }
            break;
        case SC_RSHIFT:
            if (loopState.isDownstroke
                && (modifierState.modifierDown == (BITMASK_LSHIFT))
                && !(GetKeyState(VK_CAPITAL) & 0x0001))
            {
                keySequenceAppendMakeBreakKey(SC_CAPS, loopState.resultingKeyEventSequence);
                finalScancode = true;
            }
            break;
        }
    }

    loopState.isModifier = isModifier(loopState.scancode) ? true : false;
}

//May contain Capsicain Escape sequences, those are a bit hacky. 
//They are used to temporarily make/break modifiers (for example, ALT+Q -> Shift+1... but don't mess with shift state if it is actually pressed)
//Sequence starts with SC_CPS_ESC DOWN
//Scancodes inside are modifier bitmasks. State DOWN means "set these modifiers if they are up", UP means "clear those if they are down".
//Sequence ends with SC_CPS_ESC UP -> the modifier sequence is played.
//Second SC_CPS_ESC UP -> the previous changes to the modifiers are reverted.
void playKeyEventSequence(vector<KeyEvent> keyEventSequence)
{
    KeyEvent newKeyEvent;
    unsigned int delayBetweenKeyEventsMS = option.delayForKeySequenceMS;
    bool inCpsEscape = false;  //inside an escape sequence, read next keyEvent
    int escapeSequenceType = 0; // 1: escape sequence to temp release modifiers; 2: pause; 3... undefined

    IFDEBUG cout << "\t--> SEQUENCE (" << dec << keyEventSequence.size() << ")";
    for (KeyEvent keyEvent : keyEventSequence)
    {
        if (keyEvent.scancode == SC_CPS_ESC)
        {
            if (keyEvent.isDownstroke)
            {
                if(inCpsEscape)
                    error("Internal error: Received double SC_CPS_ESC down.");
                else
                {
                    if (modifierState.modsTempAltered.size() > 0)
                    {
                        error("Internal error: previous escape sequence for 'temp alter mods' was not undone.");
                        modifierState.modsTempAltered.clear();
                    }
                }
            }
            else if (escapeSequenceType == CPS_ESC_SEQUENCE_TYPE_TEMPALTERMODIFIERS)
            {
                if (inCpsEscape) //play the escape sequence
                {
                    for (KeyEvent strk : modifierState.modsTempAltered)
                    {
                        sendKeyEvent(strk);
                        Sleep(delayBetweenKeyEventsMS);
                    }
                }
                else //undo the previous temp modifier changes, in reverse order
                {
                    IFDEBUG cout << endl << "undo alter modifiers" << keyEvent.scancode;
                    KeyEvent strk;
                    for (size_t i = modifierState.modsTempAltered.size(); i>0; i--)
                    {
                        strk = modifierState.modsTempAltered[i - 1];
                        strk.isDownstroke = !strk.isDownstroke;
                        sendKeyEvent(strk);
                        Sleep(delayBetweenKeyEventsMS);
                    }
                    modifierState.modsTempAltered.clear();
                    escapeSequenceType = 0;
                }
            }
            else
            {
                error("Internal error: received a SC_CPS_ESC UP but we're not in any escape sequence.");
                escapeSequenceType = 0;
            }
            inCpsEscape = keyEvent.isDownstroke;
            continue;
        }
        if (inCpsEscape)  //first key after SC_CAP_ESC = escape function: 1=conditional break/make of modifiers, 2=Pause
        {
            if (escapeSequenceType == 0)
            {
                switch (keyEvent.scancode)
                {
                case 1:
                case 2:
                    escapeSequenceType = keyEvent.scancode;
                    IFDEBUG cout << endl << "CPS_ESC_SEQUENCE_TYPE: " << keyEvent.scancode;
                    break;
                default:
                    error("internal error: escapeSequenceType not defined: " + to_string(keyEvent.scancode));
                    return;
                }
            }
            else if (escapeSequenceType == CPS_ESC_SEQUENCE_TYPE_TEMPALTERMODIFIERS)
            {
                IFDEBUG cout << endl << "temp alter modifiers" << keyEvent.scancode;

                //the scancode is a modifier bitmask. Press/Release keys as necessary.
                //make modifier IF the system knows it is up
                unsigned short tempModChange = keyEvent.scancode;       //escaped scancode carries the modifier bitmask
                KeyEvent newKeyEvent;

                if (keyEvent.isDownstroke)  //make modifiers when they are up
                {
                    unsigned short exor = modifierState.modifierDown ^ tempModChange; //figure out which ones we must press
                    tempModChange &= exor;
                }
                else	//break mods if down
                    tempModChange &= modifierState.modifierDown;

                newKeyEvent.isDownstroke = keyEvent.isDownstroke;

                const int NUMBER_OF_MODIFIERS_ALTERED_IN_SEQUENCES = 8; //high modifiers are skipped because they are never sent anyways.
                for (int i = 0; i < NUMBER_OF_MODIFIERS_ALTERED_IN_SEQUENCES; i++)  //push keycodes for all mods to be altered
                {
                    unsigned short modBitmask = tempModChange & (1 << i);
                    if (!modBitmask)
                        continue;
                    unsigned short sc = getModifierForBitmask(modBitmask);
                    newKeyEvent.scancode = sc;
                    modifierState.modsTempAltered.push_back(newKeyEvent);
                }
                continue;
            }
            else if (escapeSequenceType == CPS_ESC_SEQUENCE_TYPE_SLEEP)   //PAUSE
            {
                IFDEBUG cout << endl << "sleep: " << keyEvent.scancode;
                Sleep(keyEvent.scancode * 100);
                inCpsEscape = false;  //type 2 does not need an "escape up" key to finish the sequence
                escapeSequenceType = 0;
            }
            else
            {
                error("Internal error: unknown CPS_ESC_SEQUENCE_TYPE: " + to_string(escapeSequenceType));
                return;
            }
        }
        else //regular non-escaped keyEvent
        {
            sendKeyEvent(keyEvent);
            if (keyEvent.scancode == AHK_HOTKEY1 || keyEvent.scancode == AHK_HOTKEY2)
                delayBetweenKeyEventsMS = DEFAULT_DELAY_FOR_AHK;
            else
                Sleep(delayBetweenKeyEventsMS);
        }
    }
    if (inCpsEscape)
        error("SC_CPS escape sequence was not finished properly. Check your config.");
}

void getHardwareId()
{
    {
        wchar_t  hardware_id[500] = { 0 };
        string id;
        size_t length = interception_get_hardware_id(globalState.interceptionContext, globalState.interceptionDevice, hardware_id, sizeof(hardware_id));
        if (length > 0 && length < sizeof(hardware_id))
        {
            wstring wid(hardware_id);
            string sid(wid.begin(), wid.end());
            id = sid;
        } 
        else
            id = "UNKNOWN_ID";

        globalState.deviceIdKeyboard = id;
        globalState.deviceIsAppleKeyboard = (id.find("VID_05AC") != string::npos) || (id.find("VID&000205ac") != string::npos);

        IFDEBUG cout << endl << "getHardwareId:" << id << " / Apple keyboard: " << globalState.deviceIsAppleKeyboard;
    }
}


void initConsoleWindow()
{
    //disable quick edit; blocking the console window means the keyboard is dead
    HANDLE Handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(Handle, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    mode &= ~ENABLE_MOUSE_INPUT;
    SetConsoleMode(Handle, mode);

    system("color 8E");  //byte1=background, byte2=text
    string title = ("Capsicain v" + VERSION);
    SetConsoleTitle(title.c_str());
}

// Parses the OPTIONS section.
// Returns false if section does not exist.
bool parseIniOptions(std::vector<std::string> assembledIni)
{
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_OPTIONS, assembledIni);
    if (sectLines.size() == 0)
        return false;

    if (!getIntValueForKey("delayForKeySequenceMS", option.delayForKeySequenceMS, sectLines))
    {
        option.delayForKeySequenceMS = DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;
        IFDEBUG cout << endl << "No ini setting for 'option delayForKeySequenceMS'. Using default " << DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;
    }

    option.debug = configHasKey("debug", sectLines);
    option.flipZy = configHasKey("flipZy", sectLines);
    option.shiftShiftToCapsLock = configHasKey("shiftShiftToCapsLock", sectLines);
    option.altAltToAlt = configHasKey("altAltToAlt", sectLines);
    option.flipAltWinOnAppleKeyboards = configHasKey("flipAltWinOnAppleKeyboards", sectLines);
    option.LControlLWinBlocksAlphaMapping = configHasKey("LControlLWinBlocksAlphaMapping", sectLines);
    option.processOnlyFirstKeyboard = configHasKey("processOnlyFirstKeyboard", sectLines);
    return true;
}

bool parseIniRewire(std::vector<std::string> assembledIni)
{
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_REWIRE, assembledIni);
    allMaps.rewireIftappedMapping.clear();
    option.secondEscapeKey = 0;
    if (sectLines.size() == 0)
        return false;

    unsigned char keyIn, keyOut, keyIfTapped;
    for (string line : sectLines)
    {
        if (lexScancodeMapping(line, keyIn, keyOut, keyIfTapped, SCANCODE_LABELS))
        {
            if (keyOut == SC_ESCAPE)
                option.secondEscapeKey = keyIn;

            bool isDuplicate = false;
            for (RewireIftappedMapping test : allMaps.rewireIftappedMapping)
            {
                if (test.inkey == keyIn) 
                {
                    if( test.outkey != keyOut || test.ifTapped != keyIfTapped)
                        IFDEBUG cout << endl << "WARNING: ignoring redefinition of " << INI_TAG_REWIRE << " "
                            << SCANCODE_LABELS[keyIn] << " " << SCANCODE_LABELS[keyOut] << " " << SCANCODE_LABELS[keyIfTapped];
                    isDuplicate = true;
                    break;
                }
            }
            if(!isDuplicate)
                allMaps.rewireIftappedMapping.push_back({ keyIn, keyOut, keyIfTapped });

            if (!isModifier(keyOut) && keyIfTapped != SC_NOP)
                IFDEBUG cout << endl << "WARNING: 'If-Tapped' definition is ignored for non-modifier: " << INI_TAG_REWIRE << " " << line;
        }
        else
            error("Bad Rewire / key mapping: " + line);
    }
    return true;
}

bool parseIniCombos(std::vector<std::string> assembledIni)
{
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_COMBOS, assembledIni);
    if (sectLines.size() == 0)
        return false;

    allMaps.modCombos.clear();
    unsigned short mods[3] = { 0 }; //and, not, tap (nop, for)
    vector<KeyEvent> keyEventSequence;

    for (string line : sectLines)
    {
        unsigned short key;
        if (lexRule(line, key, mods, keyEventSequence, SCANCODE_LABELS))
        {
            bool isDuplicate = false;
            for (ModifierCombo testcombo : allMaps.modCombos)
            {
                if (key == testcombo.key && mods[0] == testcombo.modAnd
                    && mods[1] == testcombo.modNot && mods[2] == testcombo.modTap)
                {
                    //warn only if the combos are different
                    bool redefined = false;
                    if (testcombo.keyEventSequence.size() == keyEventSequence.size())
                    {
                        for (int i = 0; i < keyEventSequence.size(); i++)
                        {
                            if (keyEventSequence[i].scancode != testcombo.keyEventSequence[i].scancode
                                || keyEventSequence[i].isDownstroke != testcombo.keyEventSequence[i].isDownstroke)
                            {
                                redefined = true;
                                break;
                            }
                        }
                    }
                    else
                        redefined = true;

                    if(redefined)
                        IFDEBUG cout << endl << "WARNING: Ignoring redefinition of Combo: " <<
                            SCANCODE_LABELS[key] << " [ " << hex << mods[0] << ", " << mods[1] << ", " << mods[2] << "] > ...";

                    isDuplicate = true;
                    break;
                }
            }
            if(!isDuplicate)
                allMaps.modCombos.push_back({ key, mods[0], mods[1], mods[2], keyEventSequence });
        }
        else
            error("Cannot parse combo: " + line);
    }
    return true;
}

bool parseIniAlphaLayout(std::vector<std::string> assembledIni)
{
    string tagFrom = stringToLower(INI_TAG_ALPHA_FROM);
    string tagEnd = stringToLower(INI_TAG_ALPHA_END);

    resetAlphaMap();
    string mapFromTo = "";
    bool inMapFromTo = false;
    for (string line : assembledIni)
    {
        string firstToken = stringGetFirstToken(line);
        if (firstToken == tagFrom)
        {
            if (inMapFromTo)
            {
                error("Bad " + INI_TAG_ALPHA_FROM + ".." + INI_TAG_ALPHA_TO + "definition - received second "+ INI_TAG_ALPHA_FROM +". Forgot the "+INI_TAG_ALPHA_END+"?");
                return false;
            }
            inMapFromTo = true;
            mapFromTo = stringGetRestBehindFirstToken(line) + " ";
        }
        else if (firstToken == tagEnd)
        {
            inMapFromTo = false;
            if (!lexAlphaFromTo(mapFromTo, allMaps.alphamap, SCANCODE_LABELS))
                error("Cannot parse the " + INI_TAG_ALPHA_FROM + ".." + INI_TAG_ALPHA_TO + " alpha definition");
        }
        else if (inMapFromTo)
        {
            mapFromTo += line + " ";
        }
    }
    return true;
}

//insert all the INCLUDEd sub-sections into the base layer section
std::vector<std::string> assembleLayerConfig(int layer)
{
    string sectionName = "layer_" + to_string(layer);
    vector<string> assembledIni = getSectionFromIni(sectionName, sanitizedIniContent);

    while (true)
    {
        bool foundInclude = false;
        for (int i = 0; i < assembledIni.size(); i++)
        {
            string line = assembledIni.at(i);
            if (stringStartsWith(line, "include "))
            {
                assembledIni.erase(assembledIni.begin() + i);
                string subSectionName = stringGetRestBehindFirstToken(line);
                vector<string> subsection = getSectionFromIni(subSectionName, sanitizedIniContent);
                if (subsection.size() == 0)
                {
                    error("Subsection [" + subSectionName + "] does not exist or is empty)");
                }
                else
                {
                    IFDEBUG cout << endl << "inserting sub-section: " + subSectionName << " (" << subsection.size() << " lines)";
                    assembledIni.insert(assembledIni.begin() + i, subsection.begin(), subsection.end());
                }
                foundInclude = true;
                break;
            }
        }
        if (!foundInclude)
            break;
    }

    return assembledIni;
}

bool parseIni(int layer, std::string &newLayerName)
{
    if (sanitizedIniContent.size() == 0)
    {
        cout << endl << "Capsicain.ini is missing or empty.";
        return false;
    }

    option.debug = false;
    vector<string> assembledLayer = assembleLayerConfig(layer);
    if (assembledLayer.size() == 0)
    {
        cout << endl << "No valid configuration for Layer " << layer;
        return false;
    }
    if (configHasTaggedKey(INI_TAG_OPTIONS, "debug", assembledLayer))
    {
        option.debug = true;
        assembledLayer = assembleLayerConfig(layer);  //repeat just for the debug output
    }
    IFDEBUG cout << endl << "Assembled config for Layer " << layer << " : " << dec << assembledLayer.size() << " lines";

    parseIniOptions(assembledLayer);
    parseIniRewire(assembledLayer);
    IFDEBUG cout << endl << "Rewire Definitions: " << dec << allMaps.rewireIftappedMapping.size();
    parseIniCombos(assembledLayer);
    IFDEBUG cout << endl << "Combo  Definitions: " << dec << allMaps.modCombos.size();
    parseIniAlphaLayout(assembledLayer);
    IFDEBUG
    {
        int remapped = 0;
        for (int i = 0; i < 256; i++)
            if (i != allMaps.alphamap[i])
                remapped++;
        cout << endl << "Alpha  Definitions: " << dec << remapped;
    }

    if (!getStringValueForTaggedKey(INI_TAG_OPTIONS, "layerName", newLayerName, assembledLayer))
        newLayerName = "OPTION_layerName_undefined";

    return true;
}

void switchLayer(int layer)
{
    string newLayerName;

    if (layer == 0)
    {
        globalState.activeLayer = DEFAULT_ACTIVE_LAYER;
        globalState.layerName = to_string(DEFAULT_ACTIVE_LAYER) + " (" + DEFAULT_ACTIVE_LAYER_NAME + ")";
    }
    else if (parseIni(layer, newLayerName))
    {
        globalState.activeLayer = layer;
        globalState.layerName = to_string(layer) + " (" + newLayerName + ")";
    }
    else
    {
        switchLayer(DEFAULT_ACTIVE_LAYER);
        return;
    }

    cout << endl << "ACTIVE LAYER: " << globalState.layerName;
}

void resetCapsNumScrollLock()
{
    //set NumLock, release CapsLock+Scrolllock
    vector<KeyEvent> sequence;
    if (!(GetKeyState(VK_NUMLOCK) & 0x0001))
        keySequenceAppendMakeBreakKey(SC_NUMLOCK, sequence);
    if (GetKeyState(VK_CAPITAL) & 0x0001)
        keySequenceAppendMakeBreakKey(SC_CAPS, sequence);
    if (GetKeyState(VK_SCROLL) & 0x0001)
        keySequenceAppendMakeBreakKey(SC_SCRLOCK, sequence);
    playKeyEventSequence(sequence);
}

void resetAlphaMap()
{
    for (int i = 0; i < 256; i++)
        allMaps.alphamap[i] = i;
}

void resetLoopState()
{
    loopState.blockKey = false;
    loopState.isDownstroke = false;
    loopState.scancode = 0;
    loopState.isModifier = false;
    loopState.wasTapped = false;

    loopState.originalKeyEvent = { SC_NOP, false };
    loopState.resultingKeyEventSequence.clear();
}

void resetModifierState()
{
    modifierState.modifierDown = 0;
    modifierState.modifierTapped = 0;
    modifierState.lastModBrokeTapping = false;
    modifierState.modsTempAltered.clear();

    resetCapsNumScrollLock();
}
void resetAllStatesToDefault()
{
    //	globalState.interceptionDevice = NULL;
    globalState.deviceIdKeyboard = "";
    globalState.deviceIsAppleKeyboard = false;
    option.delayForKeySequenceMS = DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;

    resetModifierState();
    resetLoopState();
}

void printHelloHeader()
{
    string line1 = "Capsicain v" + VERSION;
#ifdef NDEBUG
    line1 += " (Release build)";
#else
    line1 += " (DEBUG build)";
#endif
    size_t linelen = line1.length();

    cout << endl;
    for (int i = 0; i < linelen; i++)
        cout << "-";
    cout << endl << line1 << endl;
    for (int i = 0; i < linelen; i++)
        cout << "-";
    cout << endl;
}

void printHelloFeatures()
{
    cout
        << endl << "ini version: " << option.iniVersion
        << endl << endl << "FEATURES"
        << endl << (option.flipZy ? "ON:" : "OFF:") << " Z <-> Y"
        << endl << (option.shiftShiftToCapsLock ? "ON:" : "OFF:") << " LShift + RShift -> ShiftLock"
        << endl << (option.altAltToAlt ? "ON:" : "OFF:") << " LAlt + RAlt -> Alt"
        << endl << (option.flipAltWinOnAppleKeyboards ? "ON:" : "OFF:") << " Alt <-> Win for Apple keyboards"
        << endl << (option.LControlLWinBlocksAlphaMapping ? "ON:" : "OFF:") << " Left Control and Win block alpha key mapping ('Ctrl + C is never changed')"
        << endl << (option.processOnlyFirstKeyboard ? "ON:" : "OFF:") << " Process only the keyboard that sent the first key"
        ;
}

void printStatus()
{
    int numMakeSent = 0;
    for (int i = 0; i < 255; i++)
    {
        if (globalState.keysDownSent[i])
            numMakeSent++;
    }
    cout << "STATUS" << endl << endl
        << "Capsicain version: " << VERSION << endl
        << "ini version: " << option.iniVersion << endl
        << "hardware id:" << globalState.deviceIdKeyboard << endl
        << "Apple keyboard: " << globalState.deviceIsAppleKeyboard << endl
        << "active layer: " << globalState.layerName << endl
        << "delay between keys in sequences (ms): " << option.delayForKeySequenceMS << endl
        << "number of keys-down sent: " << dec <<   numMakeSent << endl
        << (errorLog.length() > 1 ? "ERROR LOG contains entries" : "clean error log") << " (" << dec << errorLog.length() << " chars)" << endl
        ;
}

void printKeylabels()
{
    for (int i = 0; i <= 255; i++)
        cout << "sc " << uppercase << hex << i << " = " << SCANCODE_LABELS[i] << endl;
}

void printHelp()
{
    cout << "HELP" << endl << endl
        << "Press [ESC] + [key] for core commands" << endl << endl
        << "[H] Help" << endl
        << "[X] Exit" << endl
        << "[0]..[9] switch layers. [0] is the 'do nothing but listen for commands' layer" << endl
        << "[W] flip ALT <-> WIN on Apple keyboards" << endl
        << "[Z] (labeled [Y] on GER keyboard): flip Y <-> Z keys" << endl
        << "[S] Status" << endl
        << "[D] Debug mode output" << endl
        << "[E] Error log" << endl
        << "[C] Print list of key labels for all scancodes" << endl
        << "[R] Reset and reload the .ini" << endl
        << "[U] Username Enter/Playback" << endl
        << "[P] Password Enter/Playback. DO NOT USE if strangers can access your local machine." << endl
        << "[I] Show processed Ini for the active layer" << endl
        << "[A] Autohotkey start" << endl
        << "[Y] autohotkeY stop" << endl
        << "[J][K][L][;] Macro Recording: Start,Stop,Playback,Copy macro definition to clipboard." << endl
        << "[,] and [.]: pause between keys in sequences -/+ 1ms " << endl
        << "[Q] (dev feature) Stop the debug build if both release and debug are running" << endl
        << endl << "These commands work anywhere, Capsicain does not have to be the active window."
        ;
}


void normalizeIKStroke(InterceptionKeyStroke &ikstroke) {
    if (ikstroke.code > 0x7F) {
        ikstroke.code &= 0x7F;
        ikstroke.state |= 2;
    }
}

InterceptionKeyStroke keyEvent2ikstroke(KeyEvent ikstroke)
{
    InterceptionKeyStroke iks = { ikstroke.scancode, 0 };
    if (ikstroke.scancode >= 0x80)
    {
        iks.code = static_cast<unsigned short>(ikstroke.scancode & 0x7F);
        iks.state |= 2;
    }
    if (!ikstroke.isDownstroke)
        iks.state |= 1;

    return iks;
}

KeyEvent ikstroke2keyEvent(InterceptionKeyStroke ikStroke)
{	
    KeyEvent strk;
    strk.scancode = ikStroke.code;
    if ((ikStroke.state & 2) == 2)
        strk.scancode |= 0x80;
    strk.isDownstroke = ikStroke.state & 1 ? false : true;
    return strk;
}

void sendKeyEvent(KeyEvent keyEvent)
{
    if (keyEvent.scancode == 0xE4)
        IFDEBUG cout << " {sending E4} ";
    if (keyEvent.scancode > 0xFF)
    {
        error("Unexpected scancode > 255: " + to_string(keyEvent.scancode));
        return;
    }
    if (!keyEvent.isDownstroke &&  !globalState.keysDownSent[(unsigned char)keyEvent.scancode])  //ignore up when key is already up
    {
        IFDEBUG cout << " >(blocked " << SCANCODE_LABELS[keyEvent.scancode] << " UP: was not down)>";
        return;
    }
    globalState.keysDownSent[(unsigned char)keyEvent.scancode] = keyEvent.isDownstroke;

    if (globalState.recordingMacro)
    {
        if (globalState.recordedMacro.size() < MAX_MACRO_LENGTH)
            globalState.recordedMacro.push_back(keyEvent);
        else
        {
            globalState.recordingMacro = false;
            cout << endl << "Macro Length > " << MAX_MACRO_LENGTH << ". Forgotten Macro? Stopping recording.";
        }
    }
    
    InterceptionKeyStroke iks = keyEvent2ikstroke(keyEvent);
    IFDEBUG cout << " {" << SCANCODE_LABELS[keyEvent.scancode] << (keyEvent.isDownstroke ? "v" : "^") << "}";
    interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&iks, 1);
}

void reset()
{
    resetAllStatesToDefault();

    releaseAllRealModifiers();

    readIniFile(sanitizedIniContent);
    readGlobalsOnStartup();
    switchLayer(globalState.activeLayer);
}

void releaseAllRealModifiers()
{
    IFDEBUG cout << endl << "Resetting all modifiers to UP" << endl;
    for (int i = 0; i < 255; i++)	//Send() suppresses key UP if it thinks it is already up.
        globalState.keysDownSent[i] = true;

    vector<KeyEvent> keyEventSequence;
    keyEventSequence.push_back({ SC_LSHIFT, false });
    keyEventSequence.push_back({ SC_RSHIFT, false });
    keyEventSequence.push_back({ SC_LCTRL, false });
    keyEventSequence.push_back({ SC_RCTRL, false });
    keyEventSequence.push_back({ SC_LWIN, false });
    keyEventSequence.push_back({ SC_RWIN, false });
    keyEventSequence.push_back({ SC_LALT, false });
    keyEventSequence.push_back({ SC_RALT, false });
    keyEventSequence.push_back({ SC_CAPS, false });
    keyEventSequence.push_back({ AHK_HOTKEY1, false });
    keyEventSequence.push_back({ AHK_HOTKEY2, false });
    playKeyEventSequence(keyEventSequence);

    for (int i = 0; i < 255; i++)
        globalState.keysDownSent[i] = false;
}


void keySequenceAppendMakeKey(unsigned short scancode, vector<KeyEvent> &sequence)
{
    sequence.push_back({ scancode, true });
}
void keySequenceAppendBreakKey(unsigned short scancode, vector<KeyEvent> &sequence)
{
    sequence.push_back({ scancode, false });
}
void keySequenceAppendMakeBreakKey(unsigned short scancode, vector<KeyEvent> &sequence)
{
    sequence.push_back({ scancode, true });
    sequence.push_back({ scancode, false });
}

string getSymbolForIKStrokeState(unsigned short state)
{
    switch (state)
    {
    case 0: return "v";
    case 1: return "^";
    case 2: return "*v";
    case 3: return "*^";
    }
    return "???" + state;
}
