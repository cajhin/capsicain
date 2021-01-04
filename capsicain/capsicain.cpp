#pragma once;

#include "pch.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <algorithm>
#include <string>
#include <Windows.h>  //for Sleep()

#include "capsicain.h"
#include "constants.h"
#include "modifiers.h"
#include "scancodes.h"
#include "resource.h"

using namespace std;


string PRETTY_VK_LABELS[MAX_VCODES]; // contains e.g. [01]="ESC" instead of SC_ESCAPE, and all VKs > 0xFF

vector<string> sanitizedIniContent;  //loaded on startup and reset

//only written on ini load
struct Globals
{
    string iniVersion = "unnamed version - add 'iniVersion xyz123' to capsicain.ini";
    int activeConfigOnStartup = DEFAULT_ACTIVE_CONFIG;
    bool startMinimized = false;
    bool startInTraybar = false;
    bool startAHK = false;
    int capsicainOnOffKey = -1;
} globals;
static const struct Globals defaultGlobals;

//can be toggled with ESC commands
struct Options
{
    bool debug = false;
    int delayForKeySequenceMS = DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;
    bool flipZy = false;
    bool flipAltWinOnAppleKeyboards = false;
    bool LControlLWinBlocksAlphaMapping = false;
    bool processOnlyFirstKeyboard = false;
} options;
static const struct Options defaultOptions;

struct ModifierCombo
{
    int vkey = SC_NOP;
    unsigned char deadkey = 0;
    unsigned short modAnd = 0;
    unsigned short modOr = 0;
    unsigned short modNot = 0;
    unsigned short modTap = 0;
    vector<VKeyEvent> keyEventSequence;
};

struct AllMaps
{
    int alphamap[MAX_VCODES] = { }; //MUST initialize this manually to 1 1, 2 2, 3 3, ...
    vector<ModifierCombo> modCombos;// = new vector<ModifierCombo>();

    //inkey outkey (tapped)
    //-1 = undefined key
    int rewiremap[REWIRE_ROWS][REWIRE_COLS] = { }; //MUST initialize this manually to -1 !!
} allMaps;

struct InterceptionState
{
    InterceptionContext interceptionContext = NULL;
    InterceptionDevice interceptionDevice = NULL;
    InterceptionDevice previousInterceptionDevice = NULL;
    InterceptionKeyStroke lastIKstroke = { SC_NOP, 0 };

} interceptionState;

struct GlobalState
{
    bool capsicainOn = true;

    int  activeConfig = 0;
    string activeConfigName = DEFAULT_ACTIVE_CONFIG_NAME;
    int previousConfig = 1; // switch to this on func(CONFIGPREVIOUS)

    bool realEscapeIsDown = false;

    string deviceIdKeyboard = "";
    bool deviceIsAppleKeyboard = false;

    int tapAndHoldKey = -1; //remember the tap-and-hold key as long as it is down
    VKeyEvent previousKey2EventIn = { SC_NOP, 0 };
    VKeyEvent previousKey3EventIn = { SC_NOP, 0 }; //2 keys ago
    VKeyEvent previousKey1EventIn = { SC_NOP, 0 }; //mostly useless but simplifies the handling

    int keysDownSentCounter = 0;  //tracks how many keys are actually down that Windows knows about
    bool keysDownSent[256] = { false };  //Remember all forwarded to Windows. Sent keys must be 8 bit
    bool keysDownTempReleased[256] = { false };  //Remember all keys that were temporarily released, e.g. to send an Alt-Numpad combo

    bool recordingMacro = false; //currently recordingq
    vector<VKeyEvent> recordedMacro;

    vector<VKeyEvent> username;
    vector<VKeyEvent> password;
    bool recordingUsername = false;
    bool recordingPassword = false;

    chrono::steady_clock::time_point timepointPreviousKeyEvent;

} globalState;
static const struct GlobalState defaultGlobalState;

struct ModifierState
{
    unsigned char activeDeadkey = 0;  //it's not really a modifier though...
    unsigned short modifierDown = 0;
    unsigned short modifierTapped = 0;
    vector<VKeyEvent> modsTempAltered;
} modifierState;
static const struct ModifierState defaultModifierState;

struct LoopState
{
    unsigned char scancode = SC_NOP; //hardware code sent by Interception
    int vcode = -1; //key code used internally; equals scancode or a Virtual code > FF
    bool isDownstroke = false;
    bool isModifier = false;
    bool tapped = false;
    bool tappedSlow = false;  //autorepeat set in before key release
    bool tapHoldMake = false;  //tap-and-hold action (like LAlt > mod12 // LAlt)

    vector<VKeyEvent> resultingVKeyEventSequence;

    chrono::steady_clock::time_point timepointLoopStart;
    chrono::steady_clock::time_point timepointStopwatch;
    long timeSinceLastKeyEventMS = 0;
} loopState;
static const struct LoopState defaultLoopState;

string errorLog = "";
void error(string txt)
{
    cout << endl << "ERROR: " << txt << endl;
    errorLog += "\r\n" + txt;
}

string getPrettyVKLabelPadded(int vcode, int resultLength)
{
    string label = PRETTY_VK_LABELS[vcode];
    if (resultLength > label.size())
        label.insert(0, resultLength - label.size(), ' ');
    return label;
}
string getPrettyVKLabel(int vcode)
{
    return PRETTY_VK_LABELS[vcode];
}

void parseIniGlobals()
{
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_GLOBAL, sanitizedIniContent);

    for (string line : sectLines)
    {
        string token = stringGetFirstToken(line);
        if (token == "debugonstartup")
            options.debug = true;
        else if (token == "capsicainonoffkey")
        {
            string s = stringGetRestBehindFirstToken(line);
            int key = getVcode(s, PRETTY_VK_LABELS);
            if (key < 0)
                cout << "ERROR: unknown key label: " << line << endl;
            else if (key > 255)
                cout << "ERROR: virtual key makes no sense: " << line << endl;
            else 
                globals.capsicainOnOffKey = key;
        }
        else if (token == "iniversion")
            globals.iniVersion = stringGetRestBehindFirstToken(line);
        else if (token == "startminimized")
            globals.startMinimized = true;
        else if (token == "startintraybar")
            globals.startInTraybar = true;
        else if (token == "startahk")
            globals.startAHK = true;
        else if ((token == "activeconfigonstartup") || (token == "activelayeronstartup"))
            cout << endl;
        else
            cout << endl << "WARNING: unknown GLOBAL " << token;
    }

    if (!getIntValueForTaggedKey(INI_TAG_GLOBAL, "ActiveConfigOnStartup", globals.activeConfigOnStartup, sanitizedIniContent))
    {
        //backward compat for "layer"
        if (getIntValueForTaggedKey(INI_TAG_GLOBAL, "ActiveLayerOnStartup", globals.activeConfigOnStartup, sanitizedIniContent))
        {
            cout << endl << "INFO: Use 'GLOBAL activeConfigOnStartup' instead of 'GLOBAL activeLayerOnStartup'";
        }
        else
        {
            cout << endl << "No ini setting for 'GLOBAL activeConfigOnStartup'. Setting default config " << globals.activeConfigOnStartup;
        }
    }
}

void DetectTapping()
{
    //Tapped key?
    loopState.tapped = !loopState.isDownstroke
        && (loopState.vcode == globalState.previousKey1EventIn.vcode)
        && (globalState.previousKey1EventIn.isDownstroke);

    //Slow tap?
    loopState.tappedSlow = loopState.tapped
        && (globalState.previousKey2EventIn.vcode == loopState.vcode)
        && (globalState.previousKey2EventIn.isDownstroke);

    if (loopState.tappedSlow)
        loopState.tapped = false;

    //Tap and hold Make?
    if (loopState.isDownstroke)
    {
        if (
            loopState.vcode == globalState.previousKey1EventIn.vcode
            && loopState.vcode == globalState.previousKey2EventIn.vcode
            && !globalState.previousKey1EventIn.isDownstroke
            && globalState.previousKey2EventIn.isDownstroke
            )
        {
            loopState.tapHoldMake = true;
        }
    }
    
    //cannot detect tapHold Break here. This is done by ProcessRewire()

    //remember last three keys
    globalState.previousKey3EventIn = globalState.previousKey2EventIn;
    globalState.previousKey2EventIn = globalState.previousKey1EventIn;
    globalState.previousKey1EventIn = { loopState.vcode, loopState.isDownstroke };
}

int main()
{
    if (!initConsoleWindow())
    {
        std::cout << endl << "Capsicain already running - exiting..." << endl;
        Sleep(5000);
        return 0;
    }

    printHelloHeader();

    defineAllPrettyVKLabels(PRETTY_VK_LABELS);
    loopState.timepointLoopStart = timeSetTimepointNow();

    if (!readSanitizeIniFile(sanitizedIniContent))
    {
        std::cout << endl << "No capsicain.ini - exiting..." << endl;
        Sleep(5000);
        return 0;
    }

    parseIniGlobals();
    switchConfig(globals.activeConfigOnStartup, true);

    interceptionState.interceptionContext = interception_create_context();
    interception_set_filter(interceptionState.interceptionContext, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);

    if (globals.startAHK)
    {
        string msg = startProgramSameFolder(PROGRAM_NAME_AHK);
        cout << endl << endl << "starting AHK... ";
        cout << (msg == "" ? "OK" : "Not. '" + msg + "'");
    }
    cout << endl << endl << "[ESC] + [X] to stop." << endl << "[ESC] + [H] for Help";
    cout << endl << endl << "capsicain running.... ";

    if (globals.startInTraybar)
        ShowInTraybar(globalState.activeConfig != 0, globalState.activeConfig);
    else if (globals.startMinimized)
        ShowInTaskbarMinimized();

    resetCapsNumScrollLock();

    raise_process_priority(); //careful: if we spam key events, other processes get no timeslots to process them. Sleep a bit...

    //CORE LOOP
    //wait for the next key from Interception
    while (interception_receive(interceptionState.interceptionContext,
        interceptionState.interceptionDevice = interception_wait(interceptionState.interceptionContext),
                                 (InterceptionStroke *)&interceptionState.lastIKstroke, 1)   > 0)
    {
        //clear loop state
        loopState = defaultLoopState;

        //copy InterceptionKeyStroke (unpleasant to use) to plain VKeyEvent
        VKeyEvent originalVKeyEvent = ikstroke2VKeyEvent(interceptionState.lastIKstroke);
        loopState.scancode = originalVKeyEvent.vcode;  //scancode is write-once (except for the AppleWinAlt option)
        loopState.vcode = loopState.scancode;          //vcode may be altered below
        loopState.isDownstroke = originalVKeyEvent.isDownstroke;

        //if GLOBAL capsicainEnableDisable is configured, it toggles the state and still forwards the (scrLock?) key
        if (loopState.scancode == globals.capsicainOnOffKey)
        {
            if (loopState.isDownstroke)
            {
                globalState.capsicainOn = !globalState.capsicainOn;
                updateTrayIcon(globalState.capsicainOn, globalState.activeConfig);
                if (globalState.capsicainOn)
                {
                    reset();
                    cout << endl << endl << "[" << getPrettyVKLabel(globals.capsicainOnOffKey) << "] -> Capsicain ON";
                    cout << endl << "active config: " << globalState.activeConfig << " = " << globalState.activeConfigName;
                }
                else
                    cout << endl << endl << "[" << getPrettyVKLabel(globals.capsicainOnOffKey) << "] -> Capsicain OFF";
            }
            interception_send(interceptionState.interceptionContext, interceptionState.interceptionDevice, (InterceptionStroke*)&interceptionState.lastIKstroke, 1);
            continue;
        }
        //if disabled, just forward
        if (!globalState.capsicainOn)
        {
            interception_send(interceptionState.interceptionContext, interceptionState.interceptionDevice, (InterceptionStroke*)&interceptionState.lastIKstroke, 1);
            continue;
        }

        IFDEBUG cout << ". ";
        //ignore secondary keyboard?
        if (options.processOnlyFirstKeyboard 
            && (interceptionState.previousInterceptionDevice != NULL)
            && (interceptionState.previousInterceptionDevice != interceptionState.interceptionDevice))
        {
            IFDEBUG cout << endl << "Ignore 2nd board (" << interceptionState.interceptionDevice << ") scancode: " << interceptionState.lastIKstroke.code;
            interception_send(interceptionState.interceptionContext, interceptionState.interceptionDevice, (InterceptionStroke *)&interceptionState.lastIKstroke, 1);
            continue;
        }

        //check for Apple Keyboard / device ID
        if (interceptionState.previousInterceptionDevice == NULL    //startup
            || interceptionState.previousInterceptionDevice != interceptionState.interceptionDevice)  //keyboard changed
        {
            getHardwareId();
            cout << endl << "new keyboard: " << (globalState.deviceIsAppleKeyboard ? "Apple keyboard" : "IBM keyboard");
            resetCapsNumScrollLock();
            interceptionState.previousInterceptionDevice = interceptionState.interceptionDevice;
        }

        //Timing. Timer is not precise, sleep() even less; just a rough outline. Expect occasional 30ms sleeps from thread scheduling.
        loopState.timepointLoopStart = timeSetTimepointNow();
        loopState.timeSinceLastKeyEventMS = timeBetweenTimepointsMS(globalState.timepointPreviousKeyEvent, loopState.timepointLoopStart);
        globalState.timepointPreviousKeyEvent = loopState.timepointLoopStart;

        //sanity check
        if (interceptionState.lastIKstroke.code >= 0x80)
        {
            error("Received unexpected extended Interception Key Stroke code > 0x79: " + to_string(interceptionState.lastIKstroke.code));
            continue;
        }
        if (interceptionState.lastIKstroke.code == 0)
        {
            error("Received unexpected SC_NOP Key Stroke code 0. Ignoring this.");
            continue;
        }

        //ESC Commands
        if (loopState.scancode == SC_ESCAPE)
        {
            IFDEBUG cout << endl << "(Hard ESC" << (loopState.isDownstroke ? "v " : "^ ") << ")";
            globalState.realEscapeIsDown = loopState.isDownstroke;
        }
        else if (globalState.realEscapeIsDown && loopState.isDownstroke)
        {
            if (processCommand())
                continue;
            else
            {
                ShowInTaskbar();
                break;
            }
            
        }

        //Config 0: standard keyboard, no further processing, just forward everything
        if (globalState.activeConfig == DISABLED_CONFIG_NUMBER)
        {
            interception_send(interceptionState.interceptionContext, interceptionState.interceptionDevice, (InterceptionStroke *)&interceptionState.lastIKstroke, 1);
            continue;
        }

        //flip Win+Alt only for Apple keyboards.
        if (options.flipAltWinOnAppleKeyboards && globalState.deviceIsAppleKeyboard)
        {
            switch (loopState.vcode)
            {
            case SC_LALT: loopState.vcode = SC_LWIN; break;
            case SC_LWIN: loopState.vcode = SC_LALT; break;
            case SC_RALT: loopState.vcode = SC_RWIN; break;
            case SC_RWIN: loopState.vcode = SC_RALT; break;
            }

            loopState.scancode = loopState.vcode;       //only time where scancode is rewritten. Simplifies tapping and rewiring
        }

        //Tapdance
        DetectTapping();
        //slow tap breaks tapping
        if (loopState.tappedSlow)
            modifierState.modifierTapped = 0;

        //hard rewire all REWIREd keys
         processRewireScancodeToVirtualcode();
        if (loopState.vcode == SC_NOP)   //rewired to NOP to disable keys
        {
            IFDEBUG cout << " (REW2NOP)";
            continue;
        }

        //Username
        if (globalState.recordingUsername)
        {
            if (loopState.vcode == SC_RETURN)
            {
                globalState.recordingUsername = false;
                cout << " done";
            }
            else if (loopState.isDownstroke || globalState.username.size() > 0)  //filter upstroke from the command key
                globalState.username.push_back({ loopState.vcode,loopState.isDownstroke });
            continue;
        }
        //Password
        if (globalState.recordingPassword)
        {
            if (loopState.scancode == SC_RETURN)
            {
                globalState.recordingPassword = false;
                cout << " done";
            }
            else if (loopState.isDownstroke || globalState.password.size() > 0)  //filter upstroke from the command key
                globalState.password.push_back({ loopState.vcode, loopState.isDownstroke });
            continue;
        }

         IFDEBUG printLoopState1Incoming();

        //evaluate modifiers
        processModifierState();
    
        IFDEBUG printLoopState2Modifier();

        //evaluate modified keys
        processCombos();

        //alphakeys: basic character key layout. Don't remap the Ctrl combos?
        processMapAlphaKeys();

        //break tapped state?
        if (!isModifier(loopState.vcode))
            modifierState.modifierTapped = 0;

        IFDEBUG printLoopState3Timing();

        sendResultingKeyOrSequence();

        IFDEBUG printLoopState4TapState();
    }
    interception_destroy_context(interceptionState.interceptionContext);

    cout << endl << "bye" << endl;
    return 0;
}
////////////////////////////////////END MAIN//////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

void processModifierState()
{
    unsigned short modBitmask = getBitmaskForModifier(loopState.vcode);

    //set internal modifier state
    if (loopState.isDownstroke)
        modifierState.modifierDown |= modBitmask;
    else
        modifierState.modifierDown &= ~modBitmask;

    //Default tapping logic without specific rules
    //Tapped mod key sets tapped bitmask. You can combine mod-taps (like tap-Ctrl then tap-Alt).
    if (loopState.tapped)
        modifierState.modifierTapped |= modBitmask;
}

//handle all REWIRE configs
void processRewireScancodeToVirtualcode()
{
    //Rewire to new vcode; check for Tapped rule
    //ignore auto-repeating tapHold key
    if (loopState.scancode == globalState.tapAndHoldKey && loopState.isDownstroke)
    {
        loopState.vcode = SC_NOP;
        return;
    }

    int rewoutkey = allMaps.rewiremap[loopState.scancode][REWIRE_OUT];
    if (rewoutkey >= 0)
    {
        //Rewire
        loopState.vcode = rewoutkey;

        //tapped?
        int rewtapkey = allMaps.rewiremap[loopState.scancode][REWIRE_TAP];
        if (loopState.tapped && rewtapkey >= 0)  //ifTapped definition applies
        {
            //rewired tap (like TAB to TAB) clears all previous modifier taps. Good?
            modifierState.modifierTapped = 0;

            //release the original rewired result for hardware keys (e.g. Shift may be down)
            loopState.resultingVKeyEventSequence.push_back({ rewoutkey, false });
            if (isModifier(loopState.vcode))
            {
                unsigned short modBitmask = getBitmaskForModifier(loopState.vcode);
                if (modBitmask != 0)
                    modifierState.modifierDown &= ~modBitmask; //undo previous key down, e.g. clear internal 'MOD10 is down'
            }
            //send tap key
            loopState.vcode = rewtapkey;
            loopState.resultingVKeyEventSequence.push_back({ rewtapkey, true });
            loopState.resultingVKeyEventSequence.push_back({ rewtapkey, false });
        }

        //tapHold Make?
        if (loopState.tapHoldMake)
        {
            int rewtapholdkey = allMaps.rewiremap[loopState.scancode][REWIRE_TAPHOLD];
            if (rewtapholdkey >= 0)
            {
                if (globalState.tapAndHoldKey < 0)
                {
                    globalState.tapAndHoldKey = loopState.scancode;  //remember the original scancode
                    if(rewtapholdkey < 255) //send make only for real keys
                        loopState.resultingVKeyEventSequence.push_back({ rewtapholdkey, true });
                    loopState.vcode = rewtapholdkey;
                    IFDEBUG cout << endl << "Make taphold rewired: " << hex << rewtapholdkey;
                }
                else
                    error("Ignoring second tap-and-hold event; only one can be active.");
            }
        }
        //tapHold Break?
        if (!loopState.isDownstroke && loopState.scancode == globalState.tapAndHoldKey)
        {
            int rewtapholdkey = allMaps.rewiremap[loopState.scancode][REWIRE_TAPHOLD];
            if (rewtapholdkey >= 0)
            {
                globalState.tapAndHoldKey = -1;
                if (rewtapholdkey < 255) //send break only for real keys
                    loopState.resultingVKeyEventSequence.push_back({ rewtapholdkey, false });
                loopState.vcode = rewtapholdkey;
                IFDEBUG cout << endl << "Break taphold rewired: " << hex << rewtapholdkey;
            }
            else
            {
                error("BUG: undefined tapHold should never have been stored");
            }
        }
    }

    //update the internal modifier state
    loopState.isModifier = isModifier(loopState.vcode) ? true : false;
}


void processCombos()
{
    if (!loopState.isDownstroke || (modifierState.modifierDown == 0 && modifierState.modifierTapped == 0 && modifierState.activeDeadkey == 0))
        return;

    for (ModifierCombo modcombo : allMaps.modCombos)
    {
        if (modcombo.vkey == loopState.vcode)
        {
            if (
                modifierState.activeDeadkey == modcombo.deadkey &&
                (modifierState.modifierDown & modcombo.modAnd) == modcombo.modAnd &&
                (modcombo.modOr == 0 || (modifierState.modifierDown & modcombo.modOr) > 0) &&
                (modifierState.modifierDown & modcombo.modNot) == 0 &&
                ((modifierState.modifierTapped & modcombo.modTap) == modcombo.modTap)
                )
            {
                loopState.resultingVKeyEventSequence = modcombo.keyEventSequence;
                modifierState.modifierTapped = 0;
                break;
            }
        }
    }
    if(!loopState.isModifier)
        modifierState.activeDeadkey = 0;
}

void processMapAlphaKeys()
{
    if (loopState.isModifier ||
        (options.LControlLWinBlocksAlphaMapping && (IS_LCTRL_DOWN || IS_LWIN_DOWN)))
    {
        return;
    }

    loopState.vcode = allMaps.alphamap[loopState.vcode];

    if (options.flipZy)
    {
        switch (loopState.vcode)
        {
        case SC_Y:		loopState.vcode = SC_Z;		break;
        case SC_Z:		loopState.vcode = SC_Y;		break;
        }
    }
}

void testBeta()
{
    //flip icon
    options.debug = !options.debug;
    bool res = ShowInTraybar(options.debug, globalState.activeConfig);
    if (!res)
        cout << endl << "not flipped";
}

// [ESC]+x combos
// returns false if exit was requested
// uses the unwired keys for regular keys, and wired modifiers
bool processCommand()
{
    bool continueLooping = true;
    bool popupConsole = false;
    cout << endl << endl << "::";
    
    switch (loopState.scancode)
    {

    case SC_X:
    {
        cout << endl << endl << "ESC+X :: EXIT";
        return false;
    }
    case SC_0:
    {
        cout << endl << "CONFIG CHANGE: " << DISABLED_CONFIG_NUMBER;
        switchConfig(DISABLED_CONFIG_NUMBER, 0);
        break;
    }
    case SC_1:
    case SC_2:
    case SC_3:
    case SC_4:
    case SC_5:
    case SC_6:
    case SC_7:
    case SC_8:
    case SC_9:
    {
        int config = loopState.scancode - 1;
        cout << endl << "CONFIG CHANGE: " << config;
        switchConfig(config, false);
        break;
    }
    case SC_BACK:
    {
        cout << endl << endl << "::RESET STATE";
        reset();
        break;
    }
    case SC_T:
    {
        if (IsCapsicainInTray())
        {
            cout << "Show in taskbar";
            ShowInTaskbar();
        }
        else
        {
            cout << "Show traybar";
            ShowInTraybar(globalState.activeConfig != 0 , globalState.activeConfig);
        }
        break;
    }
    case SC_Q:   // quit only if a debug build
#ifdef NDEBUG
        sendVKeyEvent({ SC_ESCAPE, true });
        sendVKeyEvent({ SC_Q, true });
        sendVKeyEvent({ SC_Q, false });
        sendVKeyEvent({ SC_ESCAPE, false });
#else
        continueLooping = false;
#endif
        break;
    case SC_W:
        options.flipAltWinOnAppleKeyboards = !options.flipAltWinOnAppleKeyboards;
        cout << "Flip ALT<>WIN for Apple boards: " << (options.flipAltWinOnAppleKeyboards ? "ON" : "OFF") << endl;
        break;
    case SC_E:
        cout << "ERROR LOG: " << endl << errorLog << endl;
        popupConsole = true;
        break;
    case SC_R:
        cout << "RELOAD INI";
        reload();
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
            globalState.recordingUsername = true;
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
        cout << "INI filtered for config " << globalState.activeConfigName;
        vector<string> tmpAssembledConfig = assembleConfig(globalState.activeConfig);
        for (string line : tmpAssembledConfig)
            cout << endl << line;
        break;
    }
    case SC_P:
    {
        if (globalState.password.size() == 0)
        {
            cout << "Type Password (Enter)";
            globalState.recordingPassword = true;
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
        popupConsole = true;
        break;
    case SC_D:
        options.debug = !options.debug;
        cout << "DEBUG mode: " << (options.debug ? "ON" : "OFF");
        popupConsole = options.debug;
        break;
    case SC_H:
        printHelp();
        popupConsole = true;
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
        for (VKeyEvent key : globalState.recordedMacro)
        {
            if (macro.size() > 0)
                macro += "_";
            if (key.isDownstroke)
                macro += "&";
            else
                macro += "^";
            macro += PRETTY_VK_LABELS[key.vcode];
        }
        copyToClipBoard(macro);
        break;
    }
    case SC_Z:
        options.flipZy = !options.flipZy;
        cout << "Flip Z<>Y mode: " << (options.flipZy ? "ON" : "OFF");
        break;
    case SC_C:
        cout << "List of all Key Labels for scancodes" << endl
             << "------------------------------------" << endl;
        printKeylabels();
        popupConsole = true;
        break;
    case SC_COMMA:
        if (options.delayForKeySequenceMS >= 1)
            options.delayForKeySequenceMS -= 1;
        cout << "delay between characters in key sequences (ms): " << dec << options.delayForKeySequenceMS;
        break;
    case SC_DOT:
        if (options.delayForKeySequenceMS <= 100)
            options.delayForKeySequenceMS += 1;
        cout << "delay between characters in key sequences (ms): " << dec << options.delayForKeySequenceMS;
        break;
    case SC_B:
        testBeta();
        break;
    default: 
    {
        cout << "Unknown command";
        break;
    }
    }

    if(popupConsole)
    {
        ShowInTaskbar();
    }
    return continueLooping;
}



void getHardwareId()
{
    {
        wchar_t  hardware_id[500] = { 0 };
        string id;
        size_t length = interception_get_hardware_id(interceptionState.interceptionContext, interceptionState.interceptionDevice, hardware_id, sizeof(hardware_id));
        if (length > 0 && length < sizeof(hardware_id))
        {
            //forced conversion will replace special characters > 127 with "?"
            for (wchar_t c : hardware_id)
            {
                if (c > 127)
                    id += '?';
                else if (c == 0)
                    break;
                else
                    id += (char)c;
            }
        } 
        else
            id = "UNKNOWN_ID";

        globalState.deviceIdKeyboard = id;
        globalState.deviceIsAppleKeyboard = (id.find("VID_05AC") != string::npos) || (id.find("VID&000205ac") != string::npos);

        IFDEBUG cout << endl << "getHardwareId:" << id << " / Apple keyboard: " << globalState.deviceIsAppleKeyboard;
    }
}


bool initConsoleWindow()
{
    //check if already running
    //allow release and debug build at the same time
#ifdef NDEBUG
    CreateMutexA(0, FALSE, "capsicain_release"); // try to create a named mutex
#else
    CreateMutexA(0, FALSE, "capsicain_debug"); // try to create a named mutex
#endif
    if (GetLastError() == ERROR_ALREADY_EXISTS) // did the mutex already exist?
        return false; // quit; mutex is released automatically


    //disable CTRL-C
    SetConsoleCtrlHandler(NULL, TRUE);

    //disable quick edit; blocking the console window means the keyboard is dead
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(handle, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    mode &= ~ENABLE_MOUSE_INPUT;
    SetConsoleMode(handle, mode);

    //colors
    system("color 8E");  //byte1=background, byte2=text
    string title = "Capsicain v" VERSION;
    SetConsoleTitle(title.c_str());
    
    //resize to 800x600
    HWND console = GetConsoleWindow();
    RECT initialRect;
    GetWindowRect(console, &initialRect);
    MoveWindow(console, initialRect.left, initialRect.top, 800, 600, TRUE);

    return true;
}

// Parses the OPTIONS in the given section.
// Returns false if section does not exist.
bool parseIniOptions(std::vector<std::string> assembledIni)
{
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_OPTIONS, assembledIni);
    globalState.activeConfigName = INI_TAG_OPTIONS+" configName is undefined";

    for (string line : sectLines)
    {
        string token = stringGetFirstToken(line);
        if (token == "configname")
        {
            globalState.activeConfigName = stringGetRestBehindFirstToken(line);
        }
        else if (token == "layername")  //back compat, deprecated
        {
            globalState.activeConfigName = stringGetRestBehindFirstToken(line);
            cout << endl << "INFO: Option LAYERname is deprecated. Use Option CONFIGname instead.";
        }
        else if (token == "debug")
        {
            options.debug = true;
        }
        else if (token == "flipzy")
        {
            options.flipZy = true;
        }
        else if (token == "altalttoalt")
        {
            cout << endl << INI_TAG_OPTIONS+" AltAltToAlt is obsolete. You can do this now with 'REWIRE LALT MOD12 // LALT'";
        }
        else if (token == "flipaltwinonapplekeyboards")
        {
            options.flipAltWinOnAppleKeyboards = true;
        }
        else if (token == "lcontrollwinblocksalphamapping")
        {
            options.LControlLWinBlocksAlphaMapping = true;
        }
        else if (token == "processonlyfirstkeyboard")
        {
            options.processOnlyFirstKeyboard = true;
        }
        else if (token == "delayforkeysequencems")
        {
            getIntValueForKey("delayForKeySequenceMS", options.delayForKeySequenceMS, sectLines);
        }
        else if (token == "shiftshifttoshiftlock")
        {
            cout << endl << ("WARNING: this is obsolete: OPTION shiftShiftToShiftLock");
            cout << endl << "  Put this into your .ini instead: "
                << endl << "    COMBO  LSHF   [& ....] > key(CAPSOFF)"
                << endl << "    COMBO  RSHF[.&] > key(CAPSON)" << endl;
        }
        else
        {
            cout << endl << "WARNING: ignoring unknown OPTION " << line << endl;
        }
    }

    return true;
}

//fill the rewiremap array
//return # of valid rewires
void parseIniRewires(std::vector<std::string> assembledIni)
{
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_REWIRE, assembledIni);

    int tagCounter = 0;
    unsigned char keyIn;
    int keyOut, keyTap, keyTapHold;
    for (string line : sectLines)
    {
        keyTap = -1;
        keyTapHold = -1;
        if (parseKeywordRewire(line, keyIn, keyOut, keyTap, keyTapHold, PRETTY_VK_LABELS))
        {
            //duplicate?
            if (allMaps.rewiremap[keyIn][REWIRE_OUT] >= 0)
            {
                IFDEBUG cout << endl << "WARNING: ignoring redefinition of " << INI_TAG_REWIRE << " "
                    << PRETTY_VK_LABELS[keyIn] << " " << PRETTY_VK_LABELS[keyOut] << " " << PRETTY_VK_LABELS[keyTap];
                continue;
            }

            if (!isModifier(keyOut) && keyTap > 0)
                IFDEBUG cout << endl << "WARNING: 'If-Tapped' definition only makes sense for modifiers: " << INI_TAG_REWIRE << " " << line;

            tagCounter++;
            allMaps.rewiremap[keyIn][REWIRE_OUT] = keyOut;
            allMaps.rewiremap[keyIn][REWIRE_TAP] = keyTap;
            allMaps.rewiremap[keyIn][REWIRE_TAPHOLD] = keyTapHold;
        }
        else
            error("Bad Rewire / key mapping: " + line);
    }
    IFDEBUG cout << endl << "Rewire Definitions: " << dec << tagCounter;
}

bool parseIniCombos(std::vector<std::string> assembledIni)
{
    allMaps.modCombos.clear();
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_COMBOS, assembledIni);
    if (sectLines.size() == 0)
        return false;

    unsigned short mods[5] = { 0 }; //deadkey, and, or, not, tap
    vector<VKeyEvent> keyEventSequence;

    for (string line : sectLines)
    {
        int key;
        if (parseKeywordCombo(line, key, mods, keyEventSequence, PRETTY_VK_LABELS))
        {
            bool isDuplicate = false;
            for (ModifierCombo testcombo : allMaps.modCombos)
            {
                if (key == testcombo.vkey && mods[0] == testcombo.deadkey && mods[1] == testcombo.modAnd
                    && mods[2] == testcombo.modOr && mods[3] == testcombo.modNot && mods[4] == testcombo.modTap)
                {
                    //warn only if the combos are different
                    bool redefined = false;
                    if (testcombo.keyEventSequence.size() == keyEventSequence.size())
                    {
                        for (int i = 0; i < keyEventSequence.size(); i++)
                        {
                            if (keyEventSequence[i].vcode != testcombo.keyEventSequence[i].vcode
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
                        cout << endl << "WARNING: Ignoring redefinition of Combo: " << line;

                    isDuplicate = true;
                    break;
                }
            }
            if(!isDuplicate)
                allMaps.modCombos.push_back({ key, (unsigned char) mods[0], mods[1], mods[2], mods[3], mods[4], keyEventSequence });
        }
        else
            error("Cannot parse combo rule: " + line);
    }
    return true;
}

bool parseIniAlphaLayout(std::vector<std::string> assembledIni)
{
    string tagFrom = stringToLower(INI_TAG_ALPHA_FROM);
    string tagEnd = stringToLower(INI_TAG_ALPHA_END);

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
            if (!parseKeywordsAlpha_FromTo(mapFromTo, allMaps.alphamap, PRETTY_VK_LABELS))
                error("Cannot parse the " + INI_TAG_ALPHA_FROM + ".." + INI_TAG_ALPHA_TO + " alpha definition");
        }
        else if (inMapFromTo)
        {
            mapFromTo += line + " ";
        }
    }
    return true;
}

//insert all the INCLUDEd sub-sections into the base config section
std::vector<std::string> assembleConfig(int config)
{
    string sectionName = "config_" + to_string(config);
    vector<string> assembledIni = getSectionFromIni(sectionName, sanitizedIniContent);

    if (assembledIni.size() == 0)
    {
        sectionName = "layer_" + to_string(config);
        assembledIni = getSectionFromIni(sectionName, sanitizedIniContent);

        if (assembledIni.size() > 0)
            cout << endl << "INFO: section [layer_x]  should now be named  [config_x]";
    }

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

void initializeAllMaps()
{
    allMaps.modCombos.clear();

    //resetAlphamap()
    {
        for (int i = 0; i < MAX_VCODES; i++)  //initialize to "map to same char"
            allMaps.alphamap[i] = i;
    }

    //resetRewiremap()
    {
        for (int r = 0; r < REWIRE_ROWS; r++)
            for (int c = 0; c < REWIRE_COLS; c++)
                allMaps.rewiremap[r][c] = -1;
    }
}


//processes the sanitized ini that was read on startup or reload
bool parseProcessIniConfig(int config)
{
    initializeAllMaps();

    if (sanitizedIniContent.size() == 0)
    {
        cout << endl << "Capsicain.ini is missing or empty.";
        return false;
    }

    vector<string> assembledConfig = assembleConfig(config);
    if (assembledConfig.size() == 0)
    {
        cout << endl << "No valid configuration for Config " << config;
        return false;
    }

    IFDEBUG cout << endl << "Assembled config #" << config << " : " << dec << assembledConfig.size() << " lines";

    parseIniOptions(assembledConfig);

    parseIniRewires(assembledConfig);

    parseIniCombos(assembledConfig);
    IFDEBUG cout << endl << "Combo  Definitions: " << dec << allMaps.modCombos.size();

    parseIniAlphaLayout(assembledConfig);
    IFDEBUG
    {
        int remapped = 0;
        for (int i = 0; i < MAX_VCODES; i++)
            if (i != allMaps.alphamap[i])
                remapped++;
        cout << endl << "Alpha  Definitions: " << dec << remapped;
    }

    return true;
}

void switchConfig(int config, bool forceReloadSameConfig)
{
    if (!forceReloadSameConfig && config == globalState.activeConfig)
        return;

    int oldConfig = globalState.activeConfig;
    reset();

    if (config == DISABLED_CONFIG_NUMBER)
    {
        globalState.activeConfig = DISABLED_CONFIG_NUMBER;
        globalState.activeConfigName = DISABLED_CONFIG_NAME;
    }
    else if (parseProcessIniConfig(config))
    {
        globalState.activeConfig = config;
        globalState.previousConfig = oldConfig;
        printOptions();
    }
    else if (parseProcessIniConfig(oldConfig))
    {
        cout << endl << endl << "Keeping the current config";
    }
    else
    {
        cout << endl << endl << "ERROR: CANNOT RELOAD CURRENT CONFIG? Switching to config 0";
        globalState.activeConfig = DISABLED_CONFIG_NUMBER;
        globalState.activeConfigName = DISABLED_CONFIG_NAME;
    }

    updateTrayIcon(true, globalState.activeConfig);
    cout << endl << endl << "ACTIVE CONFIG: " << globalState.activeConfig << " = " << globalState.activeConfigName;
}

void resetCapsNumScrollLock()
{ 
    //set NumLock, release CapsLock+Scrolllock
    vector<VKeyEvent> sequence;
    if (!(GetKeyState(VK_NUMLOCK) & 0x0001))
        keySequenceAppendMakeBreakKey(SC_NUMLOCK, sequence);
    if (GetKeyState(VK_CAPITAL) & 0x0001)
        keySequenceAppendMakeBreakKey(SC_CAPS, sequence);
    if (GetKeyState(VK_SCROLL) & 0x0001 && globals.capsicainOnOffKey != SC_SCRLOCK)  //don't mess with ScrLock when it is the enable/disable key
        keySequenceAppendMakeBreakKey(SC_SCRLOCK, sequence);
    if (sequence.size() != 0)
        playKeyEventSequence(sequence);
}

void reset()
{
    resetCapsNumScrollLock();
    releaseAllSentKeys();

    loopState = defaultLoopState;
    modifierState = defaultModifierState;

    GlobalState tmp = globalState; //some settings shall survive the reset
    globalState = defaultGlobalState;
    globalState.activeConfig = tmp.activeConfig;
    globalState.activeConfigName = tmp.activeConfigName;
    globalState.previousConfig = tmp.previousConfig;
    globalState.username = tmp.username;
    globalState.password = tmp.password;
}

//Reset and reload the ini from scratch
void reload()
{
    initializeAllMaps();
    globals = defaultGlobals;
    options = defaultOptions;

    readSanitizeIniFile(sanitizedIniContent);

    parseIniGlobals();
    switchConfig(globalState.activeConfig, true);
}

//Release all keys to 'up' that have been sent out as 'down'
void releaseAllSentKeys()
{
    IFDEBUG cout << endl << "Resetting all sent DOWN keys to UP: " << endl;
    for (int i = 0; i < 255; i++)
    {
        if (globalState.keysDownSent[i])
        {
            sendVKeyEvent({ i, false });
        }
    }
}


void printHelloHeader()
{
    string line1 = "Capsicain v" VERSION;
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

void printOptions()
{
    cout
        << endl << endl << "OPTIONs"
        << endl << (options.debug ? "ON*:" : "off:    ") << " debug output for each key event"
        << endl << (options.flipZy ? "ON*:" : "off:    ") << " Z <-> Y"
        << endl << (options.flipAltWinOnAppleKeyboards ? "ON*:" : "off:    ") << " Alt <-> Win for Apple keyboards"
        << endl << (options.LControlLWinBlocksAlphaMapping ? "ON*:" : "off:    ") << " Left Control and Win block alpha key mapping ('Ctrl + C is never changed')"
        << endl << (options.processOnlyFirstKeyboard ? "ON*:" : "off:    ") << " Process only the keyboard that sent the first key"
        << endl
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
        << "ini version: " << globals.iniVersion << endl
        << "active config: " << globalState.activeConfig << " = " << globalState.activeConfigName << endl
        << "Capsicain on/off key: [" << (globals.capsicainOnOffKey >= 0 ? getPrettyVKLabel(globals.capsicainOnOffKey) : "(not defined)") << "]" << endl
        << "keyboard hardware id: " << globalState.deviceIdKeyboard << endl
        << "Apple keyboard: " << globalState.deviceIsAppleKeyboard << endl
        << "delay between keys in sequences (ms): " << options.delayForKeySequenceMS << endl
        << "number of keys-down sent: " << dec <<   numMakeSent << endl
        << (errorLog.length() > 1 ? "ERROR LOG contains entries" : "clean error log") << " (" << dec << errorLog.length() << " chars)"
        ;

    printOptions();
}

void printLoopState1Incoming()
{
    cout << endl << "(" << setw(5) << dec << loopState.timeSinceLastKeyEventMS << " ms) [" << hex << \
        (loopState.vcode == loopState.scancode ? "" : PRETTY_VK_LABELS[loopState.scancode] + " => ") \
        << getPrettyVKLabelPadded(loopState.vcode, 8) << getSymbolForIKStrokeState(interceptionState.lastIKstroke.state)
        << " =" << hex << interceptionState.lastIKstroke.code << " " << interceptionState.lastIKstroke.state << "]";
}

void printLoopState2Modifier()
{
    cout << " [M:" << hex << modifierState.modifierDown;
    if (modifierState.modifierTapped)  cout << " TAP:" << hex << modifierState.modifierTapped;
    if (modifierState.activeDeadkey)  cout << " DEAD:" << hex << getPrettyVKLabelPadded(modifierState.activeDeadkey, 0);
    cout << "] ";
}

void printLoopState3Timing()
{
    cout << "\t (" << dec << timeMillisecondsSinceTimepoint(loopState.timepointLoopStart) << dec << " ms)";
    loopState.timepointStopwatch = timeSetTimepointNow();
}

void printLoopState4TapState()
{
    cout << (loopState.tappedSlow ? " (slow tap)" : "");
    cout << (loopState.tapped ? " (tap)" : "");
    if (loopState.tapHoldMake)
        cout << " (TH_in:" << hex << interceptionState.lastIKstroke.code << ")";
    if (globalState.tapAndHoldKey >= 0)
        cout << " (TH_out:" << hex << globalState.tapAndHoldKey << ")";

    long sendtime = timeMillisecondsSinceTimepoint(loopState.timepointStopwatch);
    if (sendtime > 2)
        cout << "\t (slow to send: " << dec << sendtime << " ms)";
}

void printKeylabels()
{
    for (int i = 0; i <= 255; i++)
        cout << "sc " << uppercase << hex << i << " = " << PRETTY_VK_LABELS[i] << endl;
}

void printHelp()
{
    cout << "HELP" << endl << endl
        << "Press [ESC] + [key] for core commands" << endl << endl
        << "[H] Help" << endl
        << "[X] Exit" << endl
        << "[0]..[9] switch configs. [0] is the unchangeable empty 'do nothing but listen for commands' config" << endl
        << "[W] flip ALT <-> WIN on Apple keyboards" << endl
        << "[Z] (labeled [Y] on GER keyboard): flip Y <-> Z keys" << endl
        << "[S] Status" << endl
        << "[D] Debug mode output" << endl
        << "[E] Error log" << endl
        << "[C] Print list of key labels for all scancodes" << endl
        << "[R] Reset and reload the .ini file" << endl
        << "[T] Move Taskbar icon to Tray and back" << endl
        << "[U] Username Enter/Playback" << endl
        << "[P] Password Enter/Playback. DO NOT USE if strangers can access your local machine." << endl
        << "[I] Show processed Ini for the active config" << endl
        << "[A] Autohotkey start" << endl
        << "[Y] autohotkeY stop" << endl
        << "[J][K][L][;] Macro Recording: Start,Stop,Playback,Copy macro definition to clipboard." << endl
        << "[,] and [.]: delay between keys in sequences -/+ 1ms " << endl
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

InterceptionKeyStroke vkeyEvent2ikstroke(VKeyEvent vkstroke)
{
    InterceptionKeyStroke iks = { (unsigned short) vkstroke.vcode, 0 };

    if (vkstroke.vcode >= 0xFF)
    {
        error("bug: trying to send an interception keystroke > xFF");
        iks.code = SC_NOP;
    }

    if (vkstroke.vcode >= 0x80)
    {
        iks.code = static_cast<unsigned short>(vkstroke.vcode & 0x7F);
        iks.state |= 2;
    }
    if (!vkstroke.isDownstroke)
        iks.state |= 1;

    return iks;
}

VKeyEvent ikstroke2VKeyEvent(InterceptionKeyStroke ikStroke)
{
    VKeyEvent strk;
    strk.vcode = ikStroke.code;
    if ((ikStroke.state & 2) == 2)
        strk.vcode |= 0x80;
    strk.isDownstroke = ikStroke.state & 1 ? false : true;
    return strk;
}

//handle all special Capsicain VCodes that have no "second key param". Trigger events on downstroke only
void sendCapsicainCodeHandler(VKeyEvent keyEvent)
{
    if (!keyEvent.isDownstroke)
        return;

    switch (keyEvent.vcode)
    {
    case VK_CPS_CAPSON:
    {
        if (!(GetKeyState(VK_CAPITAL) & 0x0001))
        {
            sendVKeyEvent({ SC_CAPS, true });
            sendVKeyEvent({ SC_CAPS, false });
        }
        break;
    }
    case VK_CPS_CAPSOFF:
    {
        if ((GetKeyState(VK_CAPITAL) & 0x0001))
        {
            sendVKeyEvent({ SC_CAPS, true });
            sendVKeyEvent({ SC_CAPS, false });
        }
        break;
    }
    case VK_CPS_CONFIGPREVIOUS:
    {
        switchConfig(globalState.previousConfig, false);
    }
    }

    IFDEBUG cout << endl << "(CapsicainCode: " << getPrettyVKLabelPadded(keyEvent.vcode,0) << ")";
}

void sendResultingKeyOrSequence()
{
    if (loopState.resultingVKeyEventSequence.size() > 0)
    {
        playKeyEventSequence(loopState.resultingVKeyEventSequence);
    }
    else
    {
        IFDEBUG
        {
            if (loopState.vcode > 254)  //real key but not 255 CAPS_ESC
                cout << "\t--> BLOCKED ";
            else if (loopState.scancode != loopState.vcode)
                cout << "\t--> " << PRETTY_VK_LABELS[loopState.vcode] << " " << getSymbolForIKStrokeState(interceptionState.lastIKstroke.state);
            else
                cout << "\t--";
        }
            if (loopState.vcode < 255 && loopState.vcode != SC_NOP)
            {
                sendVKeyEvent({ loopState.vcode, loopState.isDownstroke });
            }
    }
}

//Send out all keys in a sequence
//Sequences are created for anything that requires more than one key event, like AltChar(123)
//Catch and process CPS virtual keys
void playKeyEventSequence(vector<VKeyEvent> keyEventSequence)
{
    VKeyEvent newKeyEvent;
    unsigned int delayBetweenKeyEventsMS = options.delayForKeySequenceMS;
    bool tempReleasedKeys = false; //command to temporarily release all physical keys came in

    //'next key has special meaning'
    bool expectNextSleepValue = false;
    bool expectNextDeadkey = false;
    bool expectNextConfigSwitch = false;

    IFDEBUG cout << "\t--> SEQUENCE (" << dec << keyEventSequence.size() << ")";
    for (VKeyEvent keyEvent : keyEventSequence)
    {
        if (expectNextSleepValue)
        {
            IFDEBUG cout << endl << "vk_cps_sleep: " << keyEvent.vcode;
            Sleep(keyEvent.vcode);
            expectNextSleepValue = false;
        }
        else if (expectNextDeadkey)
        {
            IFDEBUG cout << endl << "vk_cps_deadkey: " << getPrettyVKLabelPadded(keyEvent.vcode, 0);
            modifierState.activeDeadkey = keyEvent.vcode;
            expectNextDeadkey = false;
        }
        else if (expectNextConfigSwitch)
        {
            IFDEBUG cout << endl << "vk_cps_configswitch: " << getPrettyVKLabelPadded(keyEvent.vcode, 0);
            switchConfig(keyEvent.vcode, false);
            expectNextConfigSwitch = false;
        }
        //release and remember all keys that are physically down
        else if (keyEvent.vcode == VK_CPS_TEMPRELEASEKEYS)
        {
            bool tempReleasedKeys = true;
            for (int i = 0; i <= 255; i++)
            {
                globalState.keysDownTempReleased[i] = globalState.keysDownSent[i];
                if (globalState.keysDownSent[i])
                    sendVKeyEvent({ i, false });
            }
            if (globalState.keysDownSentCounter != 0)
                error("BUG: keysDownSentCounter != 0");
        }
        //restore all keys that were down before 'VK_cps_temprelease'
        else if (keyEvent.vcode == VK_CPS_TEMPRESTOREKEYS)
        {
            bool tempReleasedKeys = false;
            for (int i = 0; i <= 255; i++)
            {
                if (globalState.keysDownTempReleased[i])
                {
                    sendVKeyEvent({ i, true });
                    globalState.keysDownTempReleased[i] = false;
                }
            }
        }
        else if (keyEvent.vcode == VK_CPS_SLEEP)
        {
            expectNextSleepValue = true;
        }
        else if (keyEvent.vcode == VK_CPS_DEADKEY)
        {
            expectNextDeadkey = true;
        }
        else if (keyEvent.vcode == VK_CPS_CONFIGSWITCH)
        {
            expectNextConfigSwitch = true;
        }
        else //regular non-escaped keyEvent
        {
            sendVKeyEvent(keyEvent);
            if (keyEvent.vcode == AHK_HOTKEY1 || keyEvent.vcode == AHK_HOTKEY2)
                delayBetweenKeyEventsMS = DEFAULT_DELAY_FOR_AHK_MS;
            else
                Sleep(delayBetweenKeyEventsMS);
        }
    }

    if (tempReleasedKeys)
        error("VK_CPS_TEMPRELEASEKEYS without corresponding VK_CPS_TEMPRESTOREKEYS. Check your config.");
    if (expectNextSleepValue)
        error("BUG: VK_CPS_SLEEP is unfinished");
    if (expectNextDeadkey)
        error("BUG: VK_CPS_DEADKEY is unfinished");
    if (expectNextConfigSwitch)
        error("BUG: VK_CPS_CONFIGSWITCH is unfinished");
}


void sendVKeyEvent(VKeyEvent keyEvent)
{
    if (keyEvent.vcode > 0xFF)
    {
        sendCapsicainCodeHandler(keyEvent);
        return;
    }
    unsigned char scancode = (unsigned char) keyEvent.vcode;

    if (scancode == 0xE4)  //what was that for?
        IFDEBUG cout << " {sending E4} ";

    if (!keyEvent.isDownstroke &&  !globalState.keysDownSent[scancode])  //ignore up when key is already up
    {
        IFDEBUG cout << " >(blocked " << PRETTY_VK_LABELS[scancode] << " UP: was not down)>";
        return;
    }

    //consistency check
    if (globalState.keysDownSent[scancode] == 0 && keyEvent.isDownstroke)
        globalState.keysDownSentCounter++;
    else if (globalState.keysDownSent[scancode] == 1 && !keyEvent.isDownstroke)
        globalState.keysDownSentCounter--;

    globalState.keysDownSent[scancode] = keyEvent.isDownstroke;

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
    
    InterceptionKeyStroke iks = vkeyEvent2ikstroke(keyEvent);
    IFDEBUG cout << " {" << PRETTY_VK_LABELS[keyEvent.vcode] << (keyEvent.isDownstroke ? "v" : "^") << " #" << globalState.keysDownSentCounter << "}";

    interception_send(interceptionState.interceptionContext, interceptionState.interceptionDevice, (InterceptionStroke *)&iks, 1);
//    interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&loopState.originalIKstroke, 1);
}


void keySequenceAppendMakeKey(unsigned short scancode, vector<VKeyEvent> &sequence)
{
    sequence.push_back({ scancode, true });
}
void keySequenceAppendBreakKey(unsigned short scancode, vector<VKeyEvent> &sequence)
{
    sequence.push_back({ scancode, false });
}
void keySequenceAppendMakeBreakKey(unsigned short scancode, vector<VKeyEvent> &sequence)
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
