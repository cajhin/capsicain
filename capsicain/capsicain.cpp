#pragma once

#include "pch.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <string>
#include <sstream>
#include <chrono>
#include <Windows.h>  //for Sleep()

#include "capsicain.h"
#include "constants.h"
#include "modifiers.h"
#include "scancodes.h"
#include "resource.h"
#include "led.h"
#include "utils.h"

typedef int (*AHKTHREAD)(const wchar_t* aScript, const wchar_t* aCmdLine, const wchar_t* aTitle);
typedef int (*AHKREADY)(int threadid);
typedef int (*AHKADDSCRIPT)(const wchar_t * script, int waitexecute, int threadid);
typedef int (*AHKEXEC)(const wchar_t *, int threadid);
typedef const wchar_t * (*AHKFINDFUNC)(const wchar_t *, int threadid);
typedef const wchar_t * (*AHKFUNCTION)(const wchar_t* func, const wchar_t * param1, const wchar_t * param2, const wchar_t * param3, const wchar_t * param4, const wchar_t * param5, const wchar_t * param6, const wchar_t * param7, const wchar_t * param8, const wchar_t * param9, const wchar_t * param10, int threadid);
struct Ahk
{
    HMODULE handle;
    int threadid;
    AHKTHREAD thread;
    AHKREADY ready;
    AHKADDSCRIPT addScript;
    AHKEXEC exec;
    AHKFINDFUNC findFunc;
    AHKFUNCTION function;
    AHKFUNCTION postFunction;
} ahk;

using namespace std;

//try out if we can play doom when we have a TMK style temp layer shift key
/*
int TESTING_LAYER_SHIFT_KEY = SC_APPS;
int TESTING_LAYER_SHIFT_TO = 9;     // tmp shift to this layer
int TESTING_LAYER_SHIFT_FROM = -1;  // original layer. <0 means undefined
*/

string PRETTY_VK_LABELS[MAX_VCODES]; // contains e.g. [SC_ESCAPE]="ESC"; all VKs incl. > 0xFF

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
    bool protectConsole = true; //drop Pause and Break signals when console is foreground
    bool translateMessyKeys = true; //translate various DOS keys (e.g. Ctrl+Pause=SC_Break -> SC_Pause, Alt+Print=SC_altprint -> sc_print)
    set<int> disableEscKey;
    set<int> forwardEscKey;
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
    bool holdRepeatsAllKeys = false;
    bool disableAHKDelay = false;
    string defaultFunction = "key(%s, m)";
    bool enableMouse = false;
} options;
static const struct Options defaultOptions;

struct ModifierCombo
{
    int vkey = SC_NOP;
    unsigned char deadkey = 0;
    MOD modAnd = 0;
    MOD modOr = 0;
    MOD modNot = 0;
    MOD modTap = 0;
    MOD modTapAnd = 0;
    DEV devAnd = 0;
    DEV devNot = 0;
    vector<VKeyEvent> keyEventSequence;
};

struct AllMaps
{
    //inkey outkey (tapped)
    //-1 = undefined key
    int rewiremap[REWIRE_ROWS][REWIRE_COLS] = { }; //MUST initialize this manually to -1 !!

    map<string, vector<ModifierCombo> > modCombos{
        { INI_TAG_COMBOS, {} },
        { INI_TAG_UPCOMBOS, {} },
        { INI_TAG_TAPCOMBOS, {} },
        { INI_TAG_SLOWCOMBOS, {} }
    };

    int alphamap[MAX_VCODES] = { }; //MUST initialize this manually to 1 1, 2 2, 3 3, ...

    map<int, Executable> executables;
    map<uint8_t, Device> devices;
} allMaps;

struct InterceptionState
{
    int newKeyboardCounter = 0;
    InterceptionContext interceptionContext = NULL;
    InterceptionDevice interceptionDevice = NULL;
    InterceptionDevice previousInterceptionDevice = NULL;
    InterceptionKeyStroke currentIKstroke = { SC_NOP, 0 };
    InterceptionKeyStroke previousIKstroke1 = { SC_NOP, 0 }; //remember history
    InterceptionKeyStroke previousIKstroke2 = { SC_NOP, 0 };
    InterceptionMouseStroke currentIMstroke = { 0 };
    InterceptionDevice lastMouse = NULL;
    InterceptionDevice lastKeyboard = NULL;
} interceptionState;

struct GlobalState
{
    bool capsicainOn = true;

    int  activeConfig = 0;
    string activeConfigName = DEFAULT_ACTIVE_CONFIG_NAME;
    int previousConfig = 1; // switch to this on func(CONFIGPREVIOUS)

    bool realEscapeIsDown = false;

    string deviceIdKeyboard = "";
    string includeDeviceId = "";
    string excludeDeviceId = "";

    bool deviceIsAppleKeyboard = false;

    int keysDownSentCounter = 0;  //tracks how many keys are actually down that Windows knows about
    bool keysDownSent[256] = { false };  //Remember all forwarded to Windows. Sent keys must be 8 bit
    bool keysDownTempReleased[256] = { false };  //Remember all keys that were temporarily released, e.g. to send an Alt-Numpad combo
    set<int> holdKeys[VK_MAX];  //Remember all replaced hold() keys while the physical key is still down

    bool secretSequenceRecording = false;
    bool secretSequencePlayback = false;
    int recordingMacro = -1; //-1: not recording. 1..MAX_SIMPLE_MACROS : this is currently recording. 0=currently recording the 'hard' ESC+J macro
    vector<VKeyEvent> recordedMacros[MAX_NUM_MACROS];  // [0] stores the 'hard' macro
} globalState;
static const struct GlobalState defaultGlobalState;

struct ModifierState
{
    unsigned char activeDeadkey = 0;  //it's not really a modifier though...
    MOD modifierDown = 0;
    MOD modifierTapped = 0;
    MOD modifierForceDown = 0;
    vector<VKeyEvent> modsTempAltered;
    int tapAndHoldKey = -1; //remember the tap-and-hold key as long as it is down
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

} loopState;
static const struct LoopState defaultLoopState;

struct ProfilingTimer
{
    chrono::steady_clock::time_point timepointStopwatch;
    chrono::steady_clock::time_point timepointPreviousKeyEvent;
    chrono::steady_clock::time_point timepointLoopStart = std::chrono::steady_clock::now();

    int countIncoming = 0;
    int countOutgoing = 0;
    unsigned long totalMappingTimeUS = 0;
    unsigned long totalSendingTimeUS = 0;
    unsigned long worstMappingTimeUS = 0;
    unsigned long worstSendingTimeUS = 0;

    chrono::steady_clock::time_point getTimepointNow()
    {
        return std::chrono::steady_clock::now();
    }
    unsigned long stopwatchRestart()
    {
        unsigned long dura = stopwatchReadUS();
        timepointStopwatch = std::chrono::steady_clock::now();
        return dura;
    }
    unsigned long stopwatchReadUS()
    {
        return (unsigned long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - timepointStopwatch).count();
    }
} profiler;
static const struct ProfilingTimer defaultProfiler;

string errorLog = "";
void error(string txt)
{
    cout << endl << "ERROR: " << txt << endl;
    errorLog += "\r\n" + txt;
}

string getTimestamp()
{
    auto start = std::chrono::system_clock::now();
    auto legacyStart = std::chrono::system_clock::to_time_t(start);
    char tmBuff[30];
    ctime_s(tmBuff, sizeof(tmBuff), &legacyStart);
    return tmBuff;
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

void InterceptionSendCurrentKeystroke()
{
    interception_send(interceptionState.interceptionContext, interceptionState.interceptionDevice, (InterceptionStroke*)&interceptionState.currentIKstroke, 1);
}

void loadAHK()
{
    if (!ahk.handle)
        ahk.handle = LoadLibrary(TEXT("AutoHotkey64.dll"));
    if (!ahk.handle)
        ahk.handle = LoadLibrary(TEXT("AutoHotkey.dll"));
    if (!ahk.handle) {
        cout << endl
            << "AHK: No AutoHotkey64.dll found. Get one from "
                "https://github.com/thqby/AutoHotkey_H";
    }
    else
    {
        ahk.thread = (AHKTHREAD)GetProcAddress(ahk.handle, "NewThread");
        ahk.ready = (AHKREADY)GetProcAddress(ahk.handle, "ahkReady");
        ahk.addScript = (AHKADDSCRIPT)GetProcAddress(ahk.handle, "addScript");
        ahk.exec = (AHKEXEC)GetProcAddress(ahk.handle, "ahkExec");
        ahk.findFunc = (AHKFINDFUNC)GetProcAddress(ahk.handle, "ahkFindFunc");
        ahk.function = (AHKFUNCTION)GetProcAddress(ahk.handle, "ahkFunction");
        ahk.postFunction = (AHKFUNCTION)GetProcAddress(ahk.handle, "ahkPostFunction");

        auto script = LoadUtf8FileToString(L"capsicain.ini");
        auto idx = script.find(L"[ahk]");
        if (idx == string::npos)
            idx = script.find(L"[AHK]");
        if (idx == string::npos)
        {
            cout << endl << "AHK: INI has no [ahk] section...";
            unloadAHK();
            return;
        }
        script = script.substr(idx + 6);
        if (script.find(L"ersistent") == string::npos && script.find(L"::") == string::npos)
            cout << endl << "AHK: You should add \"Persistent\" to your AHK script if it doesn't have hotkeys...";
        if (script != L"")
        {
            if (ahk.threadid)
                ahk.exec(L"ExitApp", ahk.threadid);
            ahk.threadid = ahk.thread(script.c_str(), L"", L"");
            if (ahk.threadid)
                cout << endl << "AHK: Loaded [ahk] to AutoHotkey64.dll";
            else
                cout << endl << "AHK: Failed to load [ahk] to AutoHotkey64.dll";
        }
    }
}

void unloadAHK()
{
    if (ahk.threadid)
        ahk.exec(L"ExitApp", ahk.threadid);
    if (ahk.handle)
        FreeLibrary(ahk.handle);
    ahk.handle = 0;
    ahk.threadid = 0;
}

bool mousetoKey(InterceptionMouseStroke &mstroke, InterceptionKeyStroke &kstroke)
{
    unsigned short code{0};
    unsigned short state{0};
    // FIXME: Single mouse events can change multiple buttons, but this will only return the first match, sorry
    if (mstroke.state & INTERCEPTION_MOUSE_BUTTON_1_DOWN || mstroke.state & INTERCEPTION_MOUSE_BUTTON_1_UP)
    {
        code = VM_LEFT;
        state = (int)(bool)(mstroke.state & INTERCEPTION_MOUSE_BUTTON_1_DOWN);
    }
    else if (mstroke.state & INTERCEPTION_MOUSE_BUTTON_2_DOWN || mstroke.state & INTERCEPTION_MOUSE_BUTTON_2_UP)
    {
        code = VM_RIGHT;
        state = (int)(bool)(mstroke.state & INTERCEPTION_MOUSE_BUTTON_2_DOWN);
    }
    else if (mstroke.state & INTERCEPTION_MOUSE_BUTTON_3_DOWN || mstroke.state & INTERCEPTION_MOUSE_BUTTON_3_UP)
    {
        code = VM_MIDDLE;
        state = (int)(bool)(mstroke.state & INTERCEPTION_MOUSE_BUTTON_3_DOWN);
    }
    else if (mstroke.state & INTERCEPTION_MOUSE_BUTTON_4_DOWN || mstroke.state & INTERCEPTION_MOUSE_BUTTON_4_UP)
    {
        code = VM_BUTTON4;
        state = (int)(bool)(mstroke.state & INTERCEPTION_MOUSE_BUTTON_4_DOWN);
    }
    else if (mstroke.state & INTERCEPTION_MOUSE_BUTTON_5_DOWN || mstroke.state & INTERCEPTION_MOUSE_BUTTON_5_UP)
    {
        code = VM_BUTTON5;
        state = (int)(bool)(mstroke.state & INTERCEPTION_MOUSE_BUTTON_5_DOWN);
    }
    else if(mstroke.state & INTERCEPTION_MOUSE_WHEEL || mstroke.state & INTERCEPTION_MOUSE_HWHEEL)
    {
        if (mstroke.rolling > 0)
            code = VM_WHEEL_UP;
        else if (mstroke.rolling < 0)
            code = VM_WHEEL_DOWN;
        else if (mstroke.rolling < 0)
            code = VM_WHEEL_LEFT;
        else if (mstroke.rolling > 0)
            code = VM_WHEEL_RIGHT;
    }
    if (code >= VM_LEFT && code <= VM_BUTTON5)
    {
        state = (int)(bool)(mstroke.state & 0xAAAA);
    }
    if (code >= VM_LEFT && code <= VM_WHEEL_RIGHT)
    {
        kstroke.code = code;
        kstroke.state = state;
        return true;
    }
    return false;
}

int main()
{
    if (!initConsoleWindow())
    {
        std::cout << endl << "Capsicain already running - exiting..." << endl;
        Sleep(5000);
        return 0;
    }

    interceptionState.interceptionContext = interception_create_context();

    IFPROF profiler.stopwatchRestart();

    printHelloHeader();

    defineAllPrettyVKLabels(PRETTY_VK_LABELS);

    if (!readSanitizeIniFile(sanitizedIniContent))
    {
        std::cout << endl << "No capsicain.ini - exiting..." << endl;
        Sleep(5000);
        return 0;
    }

    parseIniGlobals();

    if (globals.startAHK)
        loadAHK();

    switchConfig(globals.activeConfigOnStartup, true);

    cout << endl << endl << "[ESC] + [X] to stop." << endl << "[ESC] + [H] for Help";
    cout << endl << endl << "capsicain running.... ";

    if (globals.startInTraybar)
        ShowInTraybar(globalState.activeConfig != 0, globalState.recordingMacro >= 0, globalState.activeConfig);
    else if (globals.startMinimized)
        ShowInTaskbarMinimized();

    if (globals.capsicainOnOffKey == SC_NUMLOCK
        || globals.capsicainOnOffKey == SC_SCRLOCK
        || globals.capsicainOnOffKey == SC_CAPS)
    {
        setLED(globals.capsicainOnOffKey, true);
    }

    IFPROF cout << endl << endl << "Profiling enabled in this build" << endl << "Startup time: " << profiler.stopwatchReadUS() / 1000 << " ms" << endl;

    raise_process_priority(); //careful: if we spam key events, other processes get no timeslots to process them. Sleep a bit...

    interception_set_filter(interceptionState.interceptionContext, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);
    if (options.enableMouse)
        interception_set_filter(interceptionState.interceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_ALL & ~INTERCEPTION_FILTER_MOUSE_MOVE);
    else
        interception_set_filter(interceptionState.interceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_NONE);

    InterceptionDevice device;
    InterceptionStroke stroke;

    //CORE LOOP
    while (true)
    {
        //wait for the next key from Interception
        interception_receive(interceptionState.interceptionContext,
            device = interception_wait(interceptionState.interceptionContext),
            &stroke, 1);

        if (interception_is_mouse(device))
        {
            InterceptionMouseStroke &mstroke = *(InterceptionMouseStroke *) &stroke;
            interceptionState.lastMouse = device;
            InterceptionKeyStroke &kstroke = *(InterceptionKeyStroke *) &stroke;
            if (!mousetoKey(mstroke, kstroke))
                continue;
            interceptionState.currentIMstroke = *(InterceptionMouseStroke *)stroke;
        }
        else
        {
            interceptionState.lastKeyboard = device;
        }

        if (allMaps.devices.find(device) == allMaps.devices.end())
            getHardwareId();

        globalState.deviceIdKeyboard = allMaps.devices[device].id;
        globalState.deviceIsAppleKeyboard = allMaps.devices[device].apple;

        //remember previous two keys to detect tapping and Pause sequence
        interceptionState.previousIKstroke2 = interceptionState.previousIKstroke1;
        interceptionState.previousIKstroke1 = interceptionState.currentIKstroke;

        interceptionState.interceptionDevice = device;
        interceptionState.currentIKstroke = *(InterceptionKeyStroke *)stroke;

        IFPROF
        {
            //Measure Timing. sleep() is not precise; just a rough outline. Expect occasional 30ms sleeps from thread scheduling.
            profiler.timepointPreviousKeyEvent = profiler.timepointLoopStart;
            profiler.timepointLoopStart = profiler.getTimepointNow();
            profiler.stopwatchRestart();
            profiler.countIncoming++;
        }

        //low level debugging, show incoming raw key
        IFTRACE printIKStrokeState(interceptionState.currentIKstroke);

        //clear loop state
        loopState = defaultLoopState;

        //copy InterceptionKeyStroke (unpleasant to use) to plain VKeyEvent
        VKeyEvent originalVKeyEvent = convertIkstroke2VKeyEvent(interceptionState.currentIKstroke);
        loopState.scancode = originalVKeyEvent.vcode;  //scancode is write-once (except for the AppleWinAlt option)
        loopState.vcode = loopState.scancode;          //vcode may be altered below
        loopState.isDownstroke = originalVKeyEvent.isDownstroke;

        //if GLOBAL capsicainEnableDisable is configured, it toggles the ON/OFF state
        if (globals.capsicainOnOffKey != -1)
        {
            if (processOnOffKey())
                continue;
        }
        //if disabled, just forward
        if (!globalState.capsicainOn)
        {
            InterceptionSendCurrentKeystroke();
            continue;
        }

        IFDEBUG if(globalState.activeConfig == 0) cout << ". ";

        //ignore secondary keyboard?
        if (options.processOnlyFirstKeyboard 
            && (interceptionState.previousInterceptionDevice != NULL)
            && (interceptionState.previousInterceptionDevice != interceptionState.interceptionDevice))
        {
            IFDEBUG cout << endl << "Ignore 2nd board (" << interceptionState.interceptionDevice << ") scancode: " << interceptionState.currentIKstroke.code;
            InterceptionSendCurrentKeystroke();
            continue;
        }

        //device id changed / check for Apple Keyboard
        if (interceptionState.previousInterceptionDevice == NULL    //startup
            || interceptionState.previousInterceptionDevice != interceptionState.interceptionDevice)  //keyboard changed
        {
            //getHardwareId();
            //detail to debug the "new device after sleep, reboot after 10 new devices"
            IFTRACE cout << endl
                << "<" << endl
                << "new keyboard: " << (globalState.deviceIsAppleKeyboard ? "Apple keyboard" : "IBM keyboard") << endl
                << "new keyboard count: " << ++interceptionState.newKeyboardCounter << endl
                << "keyboard device id: " << globalState.deviceIdKeyboard << endl
                << "interceptionDevice: " << interceptionState.interceptionDevice << endl
                << getTimestamp()
                << ">" << endl;



            interceptionState.previousInterceptionDevice = interceptionState.interceptionDevice;
        }

        //sanity check
        if (interceptionState.currentIKstroke.code >= 0x80 && interceptionState.currentIKstroke.code < VM_LEFT)
        {
            error("Received unexpected extended Interception Key Stroke code > 0x79: " + to_string(interceptionState.currentIKstroke.code));
            cout << endl << "Please open a ticket on github";
            continue;
        }
        if (interceptionState.currentIKstroke.code == 0)
        {
            error("Received unexpected SC_NOP Key Stroke code 0. Ignoring this.");
            continue;
        }

        //ESC Commands
        if (loopState.scancode == SC_ESCAPE)
        {
            IFDEBUG cout << endl << "(Hard ESC" << (loopState.isDownstroke ? "v " : "^ ") << ")";
            globalState.realEscapeIsDown = loopState.isDownstroke;

            //stop macro recording?
            if (globalState.recordingMacro > 0)
            {
                IFDEBUG cout << endl << "Stop recording macro #" << globalState.recordingMacro;
                //wrap macro in tokens to tmprelease / restore keys, to deal with the physical 'Ctrl down' that started the macro
                if (globalState.recordedMacros[globalState.recordingMacro].size() > 0)
                    globalState.secretSequenceRecording = false;
                {
                    globalState.recordedMacros[globalState.recordingMacro].push_back({ VK_CPS_TEMPRESTOREKEYS,true });
                    globalState.recordedMacros[globalState.recordingMacro].insert(globalState.recordedMacros[globalState.recordingMacro].begin(), { VK_CPS_TEMPRELEASEKEYS,true });
                }
                globalState.recordingMacro = -1;
                updateTrayIcon(true, globalState.recordingMacro >= 0, globalState.activeConfig);
                continue;
            }
        }
        else if (globalState.realEscapeIsDown && loopState.isDownstroke)
        {
            if (globals.forwardEscKey.find(loopState.scancode) == globals.forwardEscKey.end())
                continue;
        }
        else if (globalState.realEscapeIsDown && !loopState.isDownstroke)
        {
            if (globals.disableEscKey.find(loopState.scancode) == globals.disableEscKey.end())
            {
                if (processCommand())
                {
                    continue;
                }
                else
                {
                    setLED(SC_NOP, true); // sync LEDs with Windows state.
                    ShowInTaskbar(); //exit
                    break;
                }
            }
            if (globals.forwardEscKey.find(loopState.scancode) == globals.forwardEscKey.end())
                continue;
        }

        //TESTING the layer shift feature
        /*
        if (loopState.vcode == TESTING_LAYER_SHIFT_KEY)
        {
            if (loopState.isDownstroke)
            {
                if (globalState.activeConfig != TESTING_LAYER_SHIFT_TO)
                {
                    TESTING_LAYER_SHIFT_FROM = globalState.activeConfig;
                    switchConfig(TESTING_LAYER_SHIFT_TO, false);
                }
            }
            else if (TESTING_LAYER_SHIFT_FROM >= 0)
            {
                if (TESTING_LAYER_SHIFT_FROM != globalState.activeConfig)
                {
                    switchConfig(TESTING_LAYER_SHIFT_FROM, false);
                }

                TESTING_LAYER_SHIFT_FROM = -1;
            }

            continue;
        }
        */
        
        //Config 0: standard keyboard, no further processing, just forward everything
        if (globalState.activeConfig == DISABLED_CONFIG_NUMBER)
        {
            InterceptionSendCurrentKeystroke();
            continue;
        }

        //consider include/exclude deviceID options
        if (!globalState.includeDeviceId.empty()
            && globalState.deviceIdKeyboard.find(globalState.includeDeviceId) == string::npos)
        {
            IFDEBUG cout << endl << "Ignore board, deviceId is not included with this config";
            InterceptionSendCurrentKeystroke();
            continue;
        }
        if (!globalState.excludeDeviceId.empty()
            && globalState.deviceIdKeyboard.find(globalState.excludeDeviceId) != string::npos)
        {
            IFDEBUG cout << endl << "Ignore board, deviceId is excluded in this config";
            InterceptionSendCurrentKeystroke();
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

        //Handle Sysrq, ScrLock, Pause, NumLock
        if (!processMessyKeys())
            continue;

        //Tapdance
        detectTapping();
        //slow tap breaks tapping
        if (loopState.tappedSlow)
            modifierState.modifierTapped = 0;

        //hard rewire all REWIREd keys
        processRewireScancodeToVirtualcode();
        if (loopState.vcode == SC_NOP)   //rewired to NOP to disable keys
        {
            IFDEBUG cout << " (r2NOP)";
            continue;
        }

        IFDEBUG
        {
            cout << endl;
            IFPROF cout << "(" << setw(5) << dec << timeBetweenTimepointsUS(profiler.timepointPreviousKeyEvent, profiler.timepointLoopStart) / 1000 << " m) ";
            printLoopState1Input();
        }

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

        IFPROF
        {
        unsigned long mappingtime = profiler.stopwatchRestart();
        profiler.totalMappingTimeUS += mappingtime;
        profiler.countOutgoing++;
        if (mappingtime > profiler.worstMappingTimeUS)
            profiler.worstMappingTimeUS = mappingtime;
        IFDEBUG printLoopStateMappingTime(mappingtime);
        }

        sendResultingKeyOrSequence();
        IFPROF
        {
        unsigned long sendingtime = profiler.stopwatchReadUS();
        profiler.totalSendingTimeUS += sendingtime;
        if (sendingtime > profiler.worstSendingTimeUS)
            profiler.worstSendingTimeUS = sendingtime;
        if (sendingtime > 1000)
            cout << "\t (slow send: " << dec << sendingtime << " u)";
        }

        IFDEBUG printLoopState4TapState();
    }
    interception_destroy_context(interceptionState.interceptionContext);

    cout << endl << "bye" << endl;
    return 0;
}
////////////////////////////////////END MAIN//////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

void betaTest() //ESC+B
{
    setLED(SC_CAPS, true);

    //test SendInput
    //{
    //    // Create a keyboard event structure
    //    INPUT ip;
    //    ip.type = INPUT_KEYBOARD;
    //    ip.ki.time = 0;
    //    ip.ki.dwExtraInfo = 0;

    //    // Press a unicode "key"
    //    ip.ki.dwFlags = KEYEVENTF_UNICODE;
    //    ip.ki.wVk = 0;
    //    ip.ki.wScan = 0x0E8; // è
    //    SendInput(1, &ip, sizeof(INPUT));

    //    // Release key
    //    ip.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    //    SendInput(1, &ip, sizeof(INPUT));
    //}

    ////flip icon
    //options.debug = !options.debug;
    //bool res = ShowInTraybar(options.debug, globalState.recordingMacro >= 0, globalState.activeConfig);
    //if (!res)
    //    cout << endl << "not flipped";
}

bool processOnOffKey()
{
    //handle the @#$ Pause key
    bool pauseKeyTriggeredOnOff = false;
    if (globals.capsicainOnOffKey == VK_CPS_PAUSE)
    {
        //drop all E1 LCTRL
        if (interceptionState.currentIKstroke.state > 3
            && interceptionState.currentIKstroke.code == SC_LCTRL)
        {
            IFTRACE cout << endl << "dropping E2 LCTRL";
            return true;
        }

        if (loopState.scancode == SC_NUMLOCK
            && interceptionState.previousIKstroke1.code == SC_LCTRL
            && interceptionState.previousIKstroke1.state > 3)
        {
            pauseKeyTriggeredOnOff = true;
        }
    }

    //toggle ON/OFF ?
    if (loopState.scancode == globals.capsicainOnOffKey || pauseKeyTriggeredOnOff)
    {
        if (loopState.isDownstroke)
        {
            globalState.capsicainOn = !globalState.capsicainOn;
            updateTrayIcon(globalState.capsicainOn, globalState.recordingMacro >= 0, globalState.activeConfig);
            if (globalState.capsicainOn)
            {
                reset();
                cout << endl << endl << "[" << getPrettyVKLabel(globals.capsicainOnOffKey) << "] -> Capsicain ON";
                cout << endl << "active config: " << globalState.activeConfig << " = " << globalState.activeConfigName;
            }
            else
                cout << endl << endl << "[" << getPrettyVKLabel(globals.capsicainOnOffKey) << "] -> Capsicain OFF";
        }
        IFTRACE cout << endl << pauseKeyTriggeredOnOff;
        //forward only the three keys that have LEDs, to signal the state of capsicain
        if (globals.capsicainOnOffKey == SC_NUMLOCK
            || globals.capsicainOnOffKey == SC_SCRLOCK
            || globals.capsicainOnOffKey == SC_CAPS)
        {
            IFTRACE cout << "OnOff event: setting LED for: " << getPrettyVKLabel(globals.capsicainOnOffKey);
            setLED(globals.capsicainOnOffKey, globalState.capsicainOn);
        }
        return true;
    }

    return false;
}

//handle PRINT, SCRLOCK, PAUSE, NUMLOCK, E1, Exit and Break signals
//return false = drop the key
bool processMessyKeys()
{
    //Alt+Print = ALTPRINT, map to PRINT?
    if (loopState.vcode == SC_ALTPRINT)
    {
        IFTRACE cout << endl << SC_ALTPRINT;
        if (globals.translateMessyKeys)
            loopState.vcode = SC_PRINT;
    }

    //Ctrl+NumLock -> pause signal
    if  (globals.protectConsole
            && loopState.vcode == SC_NUMLOCK
            && IS_LCTRL_DOWN
            && IsCapsicainForegroundWindow()
        )
    {
        if (loopState.isDownstroke)
            cout << endl << "INFO: Ctrl+NumLock detected, which is the 'Pause console' signal. Discarding it so capsicain does not freeze.";
        return false;
    }

    //Ctrl+ScrLock -> exit signal
    if  (globals.protectConsole 
            && loopState.vcode == SC_SCRLOCK
            && IS_LCTRL_DOWN
            && IsCapsicainForegroundWindow()
        )
    {
        if (loopState.isDownstroke)
            cout << endl << "INFO: Ctrl+ScrLock detected, which is the 'Exit console' signal. Discarding it so capsicain does not exit.";
        return false;
    }

    //Ctrl+Pause produces SC_BREAK = Exit signal
    if (loopState.vcode == SC_BREAK)
    {
        IFTRACE cout << endl << "Ctrl+Pause=BREAK";
        //drop SC_BREAK ?
        if (globals.protectConsole
            && IS_LCTRL_DOWN 
            && IsCapsicainForegroundWindow())
        {
            if (loopState.isDownstroke)
                cout << endl << "INFO: Ctrl+Pause detected, which is the BREAK signal. Discarding it so capsicain does not exit.";
            return false;
        }

        //map break to pause
        if(globals.translateMessyKeys)
            loopState.vcode = VK_CPS_PAUSE;
    }

    //translate unmodified pause key sequence to PAUSE (E1 LCTRL NUMLOCK)
    if (globals.translateMessyKeys)
    {
        if (interceptionState.currentIKstroke.state > 3)
        {
            if (loopState.vcode == SC_LCTRL)
            {
                return false;  //drop the ctrl key
            }
            else
            {
                cout << endl << endl << "??? Extended escape code not handled. What is this key???"
                    << "Please open a ticket on github";
                return false;
            }
        }

        if (interceptionState.previousIKstroke1.state > 3)
        {
            if (interceptionState.previousIKstroke1.code != SC_LCTRL)
            {
                cout << endl << "??? unexpected E1 escape sequence. What kind of key is this?";
                return false;
            }

            if (loopState.vcode == SC_NUMLOCK)
            {
                IFDEBUG if (loopState.isDownstroke)
                    cout << endl << ("INFO: Pause key combo (E1 LCTRL NUMLOCK) -> virtual key PAUSE");
                loopState.vcode = VK_CPS_PAUSE;
            }
        }
    }

    return true;
}

void detectTapping()
{
    //Tapped key?
    loopState.tapped =
        !loopState.isDownstroke
        && (interceptionState.currentIKstroke.code == interceptionState.previousIKstroke1.code)
        && ((interceptionState.previousIKstroke1.state & 1) == 0);

    //Slow tap?
    loopState.tappedSlow =
        loopState.tapped
        && (interceptionState.previousIKstroke2.code == interceptionState.currentIKstroke.code)
        && ((interceptionState.previousIKstroke2.state & 1) == 0);

    if (loopState.tappedSlow)
        loopState.tapped = false;

    //Tap and hold Make? (last three same code, and down-up-down sequence)
    if (interceptionState.previousIKstroke1.code == interceptionState.currentIKstroke.code
        && interceptionState.previousIKstroke2.code == interceptionState.currentIKstroke.code
        && ((interceptionState.currentIKstroke.state & 1) == 0)
        && ((interceptionState.previousIKstroke1.state & 1) == 1)
        && ((interceptionState.previousIKstroke2.state & 1) == 0)
        )
    {
        loopState.tapHoldMake = true;
    }

    //cannot detect tapHold Break here. This is done by ProcessRewire()
}

void processModifierState()
{
    MOD modBitmask = getModifierBitmaskForVcode(loopState.vcode);

    //set internal modifier state
    if (loopState.isDownstroke)
        modifierState.modifierDown |= modBitmask;
    else
        modifierState.modifierDown &= ~modBitmask;

    //Default tapping logic without specific rules
    //Tapped mod key sets tapped bitmask. You can combine mod-taps (like tap-Ctrl then tap-Alt).
    if (loopState.tapped)
        modifierState.modifierTapped |= modBitmask;

    modifierState.modifierDown |= modifierState.modifierForceDown;
}

//handle all REWIRE configs. Rewire to new vcode; check for Tapped rules
void processRewireScancodeToVirtualcode()
{
    //ignore auto-repeating tapHold key
    if (loopState.scancode == modifierState.tapAndHoldKey && loopState.isDownstroke)
    {
        loopState.vcode = SC_NOP;
        return;
    }

    int rewoutkey = allMaps.rewiremap[loopState.vcode][REWIRE_OUT];
    if (rewoutkey >= 0)
    {
        //Rewire
        loopState.vcode = rewoutkey;

        //tapped?
        int rewtapkey = allMaps.rewiremap[loopState.scancode][REWIRE_TAP];
        if (loopState.tapped && rewtapkey >= 0)  //ifTapped definition applies
        {
            //rewired tap (like TAB to TAB) clears all previous modifier taps. Good rule? Consider that maybe "outkey tapped" detection happens(?)
            modifierState.modifierTapped = 0;

            //release the preceding "rewired on press" result, only for hardware keys (e.g. "rewire Tab Shift Tab": Shift down was sent when tap arrives)
            loopState.resultingVKeyEventSequence.push_back({ rewoutkey, false });
            //clear the 'modifier down' state for preceding "to mod" def
            if (isModifier(loopState.vcode))
            {
                MOD modBitmask = getModifierBitmaskForVcode(loopState.vcode);
                if (modBitmask != 0)
                    modifierState.modifierDown &= ~modBitmask; //undo previous key down, e.g. clear internal 'MOD10 is down'
            }
            //send ifTapped key
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
                if (modifierState.tapAndHoldKey < 0)
                {
                    modifierState.tapAndHoldKey = loopState.scancode;  //remember the original scancode
                    if(rewtapholdkey <= 255) //send make only for real keys
                        loopState.resultingVKeyEventSequence.push_back({ rewtapholdkey, true });
                    loopState.vcode = rewtapholdkey;

                    //clear the preceding tapped state(s)
                    int rewtappedkey = allMaps.rewiremap[loopState.scancode][REWIRE_TAP];
                    //1. Tap&Hold of a key rewired to modifier always first triggers the generic "modifier tapped"
                    MOD modBitmask1 = getModifierBitmaskForVcode(rewoutkey);
                    if (modBitmask1 != 0)
                        modifierState.modifierTapped &= ~modBitmask1;
                    //2. Explicit "Rewire in out ifTapped" (should probably never combine ifTapped with ifTappedAndHold, but not sure)
                    MOD modBitmask2 = getModifierBitmaskForVcode(rewtappedkey);
                    if (modBitmask2 != 0)
                        modifierState.modifierTapped &= ~modBitmask2;

                    IFTRACE cout << endl << "Make taphold rewired: " << hex << rewtapholdkey;
                }
                else
                    error("Ignoring second tap-and-hold event; only one can be active.");
            }
        }
        //tapHold Break?
        if (!loopState.isDownstroke && loopState.scancode == modifierState.tapAndHoldKey)
        {
            int rewtapholdkey = allMaps.rewiremap[loopState.scancode][REWIRE_TAPHOLD];
            if (rewtapholdkey >= 0)
            {
                modifierState.tapAndHoldKey = -1;
                if (rewtapholdkey < 255) //send break only for real keys
                    loopState.resultingVKeyEventSequence.push_back({ rewtapholdkey, false });
                else
                    loopState.vcode = SC_NOP;
                loopState.vcode = rewtapholdkey;
                IFTRACE cout << endl << "Break taphold rewired: " << hex << rewtapholdkey;
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

bool testDeviceMask(DEV maskAnd, DEV maskNot, int dev)
{
    if (maskAnd == 0xFFFFFFFF && maskNot == 0)
        return true;
    if (dev < 1 || dev > INTERCEPTION_MAX_DEVICE)
        return false;
    DEV mask = 1 << (dev - 1);
    if ((mask & maskAnd) == mask && (mask & maskNot) == 0)
        return true;
    return false;
}

void processCombos()
{
    auto process = [](vector<ModifierCombo> &combos, bool clearTapped = false){
        for (ModifierCombo modcombo : combos)
        {
            if (modcombo.vkey == loopState.vcode && testDeviceMask(modcombo.devAnd, modcombo.devNot, interceptionState.interceptionDevice))
            {
                if (
                    (modifierState.activeDeadkey == modcombo.deadkey) &&
                    (modifierState.modifierDown & modcombo.modAnd) == modcombo.modAnd &&
                    (modcombo.modOr == 0 || (modifierState.modifierDown & modcombo.modOr) > 0) &&
                    (modifierState.modifierDown & modcombo.modNot) == 0 &&
                    (modifierState.modifierTapped & modcombo.modTap) == modcombo.modTap &&
                    ((modifierState.modifierTapped & modcombo.modTapAnd) == modcombo.modTapAnd ||
                    (modifierState.modifierDown & modcombo.modTapAnd) == modcombo.modTapAnd)
                    )
                {
                    loopState.resultingVKeyEventSequence = modcombo.keyEventSequence;
                    if (clearTapped)
                        modifierState.modifierTapped = 0;
                    break;
                }
            }
        }
    };

    if (loopState.isDownstroke)
        process(allMaps.modCombos[INI_TAG_COMBOS], true);
    else
    {
        process(allMaps.modCombos[INI_TAG_UPCOMBOS]);
        if (loopState.tappedSlow)
            process(allMaps.modCombos[INI_TAG_SLOWCOMBOS]);
        if (loopState.tapped)
            process(allMaps.modCombos[INI_TAG_TAPCOMBOS]);
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
        resetCapsNumScrollLock();
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
            ShowInTraybar(globalState.activeConfig != 0 , globalState.recordingMacro >= 0, globalState.activeConfig);
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
        getHardwareId();
        reload();
        cout << endl << (globalState.deviceIsAppleKeyboard ? "APPLE keyboard (flipping Win<>Alt)" : "PC keyboard");
        break;
    case SC_Y:
        cout << "Stop AHK";
        unloadAHK();
        break;
    case SC_I:
    {
        cout << "INI filtered for config " << globalState.activeConfigName;
        vector<string> tmpAssembledConfig = assembleConfig(globalState.activeConfig);
        for (string line : tmpAssembledConfig)
            cout << endl << line;
        break;
    }
    case SC_A:
    {
        cout << "Start AHK";
        loadAHK();
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
        cout << "MACRO 0 START RECORDING";
        globalState.recordingMacro = 0;
        globalState.recordedMacros[0].clear();
        updateTrayIcon(true, globalState.recordingMacro >= 0, globalState.activeConfig);
        break;
    case SC_K:
        if (globalState.recordingMacro == 0)
        {
            while (globalState.recordedMacros[0].size() > 0 && globalState.recordedMacros[0].back().isDownstroke)  //remove all key down at the end, caused by pressing ESC+K
                globalState.recordedMacros[0].pop_back();
            while (globalState.recordedMacros[0].size()>0 && !globalState.recordedMacros[0].front().isDownstroke)  //remove all key-up at the beginning, caused by releasing the shortcut ESC+J
                globalState.recordedMacros[0].erase(globalState.recordedMacros[0].begin());
            cout << "MACRO 0 STOP RECORDING (" << globalState.recordedMacros[0].size() << ")";
        }
        else
            cout << "MACRO 0 RECORDING ALREADY STOPPED";

        globalState.recordingMacro = -1;
        updateTrayIcon(true, globalState.recordingMacro >= 0, globalState.activeConfig);
        break;
    case SC_L:
        cout << "MACRO 0 PLAYBACK";
        playKeyEventSequence(globalState.recordedMacros[0]);
        break;
    case SC_SEMI:
    {
        cout << "COPY MACRO 0 TO CLIPBOARD";
        string macro = "";
        for (VKeyEvent key : globalState.recordedMacros[0])
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
    /*case SC_B:
        betaTest();
        break;*/
    case SC_M:
        options.enableMouse ^= true;
        if (interceptionState.interceptionContext)
        {
            if (options.enableMouse)
            {
                interception_set_filter(interceptionState.interceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_ALL & ~INTERCEPTION_FILTER_MOUSE_MOVE);
                cout << endl << "MOUSE INPUT ENABLED";
            }
            else
            {
                interception_set_filter(interceptionState.interceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_NONE);
                cout << endl << "MOUSE INPUT DISABLED";
            }
        }
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



std::map<uint8_t, Device>* getHardwareId(bool refresh)
{
    if (refresh)
    {
        allMaps.devices.clear();
        for (int i = 0; i <= INTERCEPTION_MAX_DEVICE; ++i)
        {
            wchar_t hardware_id[500] = { 0 };
            string id;
            size_t length = interception_get_hardware_id(interceptionState.interceptionContext, i, hardware_id, sizeof(hardware_id));
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
                continue;
            id = stringToLower(id);
            allMaps.devices[i] = { id, (bool)interception_is_keyboard(i), (id.find("vid_05ac") != string::npos) || (id.find("vid&000205ac") != string::npos) };
        }
    }
    return &allMaps.devices;
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


//reads all GLOBALs from ini, no matter where they are
void parseIniGlobals()
{
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_GLOBAL, sanitizedIniContent);

    for (string line : sectLines)
    {
        string token = stringCopyFirstToken(line);
        if (token == "debugonstartup")
            options.debug = true;
        else if (token == "capsicainonoffkey")
        {
            string s = stringGetRestBehindFirstToken(line);
            int key = getVcode(s, PRETTY_VK_LABELS);
            if (key < 0)
                cout << "ERROR: unknown key label: " << line << endl;
            else if (key > 255 && key != VK_CPS_PAUSE)
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
        else if (token == "donttranslatemessykeys")
            globals.translateMessyKeys = false;
        else if (token == "dontprotectconsole")
            globals.protectConsole = false;
        else if ((token == "activeconfigonstartup") || (token == "activelayeronstartup"))
            cout << endl;
        else if (token == "disableesckey")
        {
            auto keys = stringSplit(stringGetRestBehindFirstToken(line), ' ');
            for (auto s : keys)
            {
                int key = getVcode(s, PRETTY_VK_LABELS);
                globals.disableEscKey.insert(key);
            }
        }
        else if (token == "forwardesckey")
        {
            auto keys = stringSplit(stringGetRestBehindFirstToken(line), ' ');
            for (auto s : keys)
            {
                int key = getVcode(s, PRETTY_VK_LABELS);
                globals.forwardEscKey.insert(key);
            }
        }
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

// Parses the OPTIONS in the given section.
// Returns false if section does not exist.
bool parseIniOptions(std::vector<std::string> assembledIni)
{
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_OPTIONS, assembledIni);
    globalState.activeConfigName = INI_TAG_OPTIONS+" configName is undefined";

    for (string line : sectLines)
    {
        string token = stringCopyFirstToken(line);
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
        else if (token == "includedeviceid")
        {
            globalState.includeDeviceId = stringGetRestBehindFirstToken(line);
            cout << endl << "INFO: this layer is active for devices whose ID contains '" << globalState.includeDeviceId << "'";
        }
        else if (token == "excludedeviceid")
        {
            globalState.excludeDeviceId = stringGetRestBehindFirstToken(line);
            cout << endl << "INFO: this layer is active for devices whose ID does NOT contain '" << globalState.excludeDeviceId << "'";
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
        else if (token == "holdrepeatsallkeys")
        {
            options.holdRepeatsAllKeys = true;
        }
        else if (token == "disableahkdelay")
        {
            options.disableAHKDelay = true;
        }
        else if (token == "defaultfunction")
        {
            options.defaultFunction = stringGetRestBehindFirstToken(line);
        }
        else if (token == "enablemouse")
        {
            options.enableMouse = true;
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
    int keyIn, keyOut, keyTap, keyTapHold;
    for (string line : sectLines)
    {
        keyTap = -1;
        keyTapHold = -1;
        if (parseKeywordRewire(line, keyIn, keyOut, keyTap, keyTapHold, PRETTY_VK_LABELS))
        {
            //duplicate?
            if (allMaps.rewiremap[keyIn][REWIRE_OUT] >= 0)
            {
                cout << endl << "WARNING: ignoring redefinition of " << INI_TAG_REWIRE << " "
                    << PRETTY_VK_LABELS[keyIn] << " " << PRETTY_VK_LABELS[keyOut] << " " << PRETTY_VK_LABELS[keyTap];
                continue;
            }

            if (!isModifier(keyOut) && keyTap > 0)
                cout << endl << "WARNING: 'If-Tapped' definition only makes sense for modifiers: " << INI_TAG_REWIRE << " " << line;

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
    auto parseSect = [](vector<string>& sectLines, vector<ModifierCombo> &combos) {
        MOD mods[6] = { 0 }; //deadkey, and, or, not, tap, tap/and
        vector<VKeyEvent> keyEventSequence;
        for (string line : sectLines)
        {
            int key;
            DEV devs[2] = { 0xFFFFFFFF, 0 };
            if (parseKeywordCombo(line, key, mods, devs, keyEventSequence, PRETTY_VK_LABELS, options.defaultFunction))
            {
                bool isDuplicate = false;
                for (ModifierCombo testcombo : combos)
                {
                    if (key == testcombo.vkey && devs[0] == testcombo.devAnd && devs[1] == testcombo.devNot && mods[0] == testcombo.deadkey && mods[1] == testcombo.modAnd
                        && mods[2] == testcombo.modOr && mods[3] == testcombo.modNot && mods[4] == testcombo.modTap && mods[5] == testcombo.modTapAnd)
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
                    combos.push_back({ key, (unsigned char) mods[0], mods[1], mods[2], mods[3], mods[4], mods[5], devs[0], devs[1], keyEventSequence });
            }
            else
                error("Cannot parse combo rule: " + line);
        }
        return sectLines.size();
    };

    for (auto& kv : allMaps.modCombos)
        kv.second.clear();

    size_t totalLines = 0;
    {
        auto combolines = getTaggedLinesFromIni("COMBO", assembledIni);
        totalLines += parseSect(combolines, allMaps.modCombos[INI_TAG_COMBOS]);
    }
    for (auto& kv : allMaps.modCombos)
    {
        auto lines = getTaggedLinesFromIni(kv.first, assembledIni);
        totalLines += parseSect(lines, kv.second);
    }
    return totalLines > 0;
}

bool parseIniAlphaLayout(std::vector<std::string> assembledIni)
{
    string tagFrom = stringToLower(INI_TAG_ALPHA_FROM);
    string tagEnd = stringToLower(INI_TAG_ALPHA_END);

    string mapFromTo = "";
    bool inMapFromTo = false;
    for (string line : assembledIni)
    {
        string firstToken = stringCopyFirstToken(line);
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

void parseIniExecutables(std::vector<std::string> assembledIni)
{
    allMaps.executables.clear();
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_EXE, assembledIni);
    int tagCounter = 0;
    for (string line : sectLines)
    {
        size_t idIdx = line.find_first_of(' ');
        if (idIdx == string::npos)
        {
            error("Invalid EXE: " + line);
            continue;
        }
        int id;
        if (!stringToInt(line.substr(0, idIdx), id))
        {
            error("Invalid EXE: " + line);
            continue;
        }
        stringstream paramss(line.substr(idIdx + 1));
        string param;
        vector<string> params;
        while(getline(paramss, param, ','))
        {
            ltrim(param);
            rtrim(param);
            params.push_back(param);
        }
        if (params.size() < 2)
        {
            error("Invalid EXE: " + line);
            continue;
        }
        string verb = params[0];
        string path = params[1];
        string args;
        string dir;
        int mode = SW_SHOWDEFAULT;
        if (params.size() > 2)
            args = params[2];
        if (params.size() > 3)
            dir = params[3];
        if (params.size() > 4)
            stringToInt(params[4], mode);

        allMaps.executables[id] = {verb, path, args, dir, mode, NULL};
        tagCounter++;
    }
    IFDEBUG cout << endl << "Exe    Definitions: " << dec << tagCounter;
}


void initializeAllMaps()
{
    for (auto kv : allMaps.modCombos)
        kv.second.clear();

    allMaps.executables.clear();
    getHardwareId();

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

    parseIniExecutables(assembledConfig);

    parseIniCombos(assembledConfig);
    IFDEBUG cout << endl << "Down   Definitions: " << dec << allMaps.modCombos[INI_TAG_COMBOS].size();
    IFDEBUG cout << endl << "Up     Definitions: " << dec << allMaps.modCombos[INI_TAG_UPCOMBOS].size();
    IFDEBUG cout << endl << "Tap    Definitions: " << dec << allMaps.modCombos[INI_TAG_TAPCOMBOS].size();
    IFDEBUG cout << endl << "Slow   Definitions: " << dec << allMaps.modCombos[INI_TAG_SLOWCOMBOS].size();

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

    if (interceptionState.interceptionContext)
    {
        if (options.enableMouse)
            interception_set_filter(interceptionState.interceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_ALL & ~INTERCEPTION_FILTER_MOUSE_MOVE);
        else
            interception_set_filter(interceptionState.interceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_NONE);
    }

    updateTrayIcon(true, globalState.recordingMacro >= 0, globalState.activeConfig);
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
    releaseAllSentKeys();

    loopState = defaultLoopState;
    modifierState = defaultModifierState;

    IFPROF
    {
        chrono::steady_clock::time_point tps = profiler.timepointStopwatch;
        chrono::steady_clock::time_point tppk = profiler.timepointPreviousKeyEvent;
        chrono::steady_clock::time_point tpls = profiler.timepointLoopStart;
        profiler = defaultProfiler;
        profiler.timepointStopwatch = tps;
        profiler.timepointPreviousKeyEvent = tppk;
        profiler.timepointLoopStart = tpls;
    }

    GlobalState tmp = globalState; //some settings shall survive the reset
    globalState = defaultGlobalState;
    globalState.deviceIdKeyboard = tmp.deviceIdKeyboard;
    globalState.activeConfig = tmp.activeConfig;
    globalState.activeConfigName = tmp.activeConfigName;
    globalState.previousConfig = tmp.previousConfig;
    for(int i=0;i<MAX_NUM_MACROS;i++)
        globalState.recordedMacros[i] = tmp.recordedMacros[i];
}

//Reset and reload the ini from scratch
void reload()
{
    initializeAllMaps();
    globals = defaultGlobals;
    options = defaultOptions;

    readSanitizeIniFile(sanitizedIniContent);

    parseIniGlobals();

    if (globals.startAHK)
        loadAHK();
    else
        unloadAHK();

    switchConfig(globalState.activeConfig, true);
}

//Release all keys to 'up' that have been sent out as 'down'
void releaseAllSentKeys()
{
    IFDEBUG cout << endl << "Resetting all sent DOWN keys to UP: " << endl;

    modifierState.modifierForceDown = 0;

    // release backwards to release modifiers and esc last
    for (int i = 255; i >= 0; --i)
    {
        globalState.holdKeys[i].clear();
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
        << endl << (options.debug ? "ON :" : "off: --") << " debug output for each key event"
        << endl << (options.flipZy ? "ON :" : "off: --") << " Z <-> Y"
        << endl << (options.flipAltWinOnAppleKeyboards ? "ON :" : "off: --") << " Alt <-> Win for Apple keyboards"
        << endl << (options.LControlLWinBlocksAlphaMapping ? "ON :" : "off: --") << " Left Control and Win block alpha key mapping ('Ctrl + C is never changed')"
        << endl << (options.processOnlyFirstKeyboard ? "ON :" : "off: --") << " Process only the keyboard that sent the first key"
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
        << "keyboard device id: " << globalState.deviceIdKeyboard << endl
        << "Apple keyboard: " << globalState.deviceIsAppleKeyboard << endl
        << "delay between keys in sequences (ms): " << options.delayForKeySequenceMS << endl
        << "number of keys-down sent: " << dec <<   numMakeSent << endl
        << (errorLog.length() > 1 ? "ERROR LOG contains entries" : "clean error log") << " (" << dec << errorLog.length() << " chars)"
        ;

    IFPROF cout << endl << endl << "Profiling statistics (microseconds)"
        << endl << "Incoming / Sent out: " << profiler.countIncoming << " / " << profiler.countOutgoing
        << endl << "Average mapping time: " << profiler.totalMappingTimeUS / profiler.countOutgoing
        << endl << "Average sending time: " << profiler.totalSendingTimeUS / profiler.countOutgoing
        << endl << "Worst mapping time: " << profiler.worstMappingTimeUS
        << endl << "Worst sending time: " << profiler.worstSendingTimeUS
        ;

    IFDEBUG {
        cout << endl << endl << "Interception keyboards:";
        for (int i = 1; i <= INTERCEPTION_MAX_DEVICE; ++i)
        {
            if (allMaps.devices.find(i) != allMaps.devices.end())
            {
                if (i == 11)
                    cout << endl << "Interception mice:";
                cout << endl << i << ": " << allMaps.devices[i].id;
            }
        }
    }

    printOptions();
}

void printIKStrokeState(InterceptionKeyStroke iks)
{
    cout << endl << "IKS: " << hex << iks.code
        << " " << iks.state
        << " = " << getPrettyVKLabel(iks.code)
        << " i" << iks.information;
}

void printLoopState1Input()
{
    cout
        << " ["
        << dec << setw(2) << interceptionState.interceptionDevice << " " << hex << interceptionState.currentIKstroke.code << " " << interceptionState.currentIKstroke.state
        << "= " << setw(8) << (loopState.vcode == loopState.scancode ? "" : PRETTY_VK_LABELS[loopState.scancode] + " > ")
        << setw(8) << getPrettyVKLabel(loopState.vcode) << setw(2) << left << getSymbolForIKStrokeState(interceptionState.currentIKstroke.state) << right
        << "] ";
}

void printLoopState2Modifier()
{
    string mdown = modifierState.modifierDown > 0 ? stringIntToHex(modifierState.modifierDown,0) : "";
    string mtapp = modifierState.modifierTapped > 0 ? stringIntToHex(modifierState.modifierTapped,0) : "";
    cout << "[M:" << setw(8) << mdown
         << " T:" << setw(8) << mtapp
         << " D:" << setw(6) << (modifierState.activeDeadkey > 0 ? getPrettyVKLabel(modifierState.activeDeadkey): "")
         << "] ";
}

void printLoopStateMappingTime(long us)
{
    cout << "  (" << setw(5) << dec << us << " u)";
}

void printLoopState4TapState()
{
    cout << (loopState.tappedSlow ? " (tap slow)" : "");
    cout << (loopState.tapped ? " (tap)" : "");

    IFTRACE if (loopState.tapHoldMake) 
        cout << " (TapHold:" << hex << interceptionState.currentIKstroke.code << ")";
    if (modifierState.tapAndHoldKey >= 0)
        cout << " (TapHoldKey: " << hex << modifierState.tapAndHoldKey << ")";
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
        << "[I] Show processed Ini for the active config" << endl
        << "[A] Autohotkey start" << endl
        << "[Y] autohotkeY stop" << endl
        << "[J][K][L][;] Macro Recording: Start,Stop,Playback,Copy macro definition to clipboard." << endl
        << "[,] and [.]: delay between keys in sequences -/+ 1ms " << endl
        /*<< "[Q] (dev feature) Stop the debug build if both release and debug are running" << endl*/
        << "[M] Toggle mouse input support" << endl
        << endl << "These commands work anywhere, Capsicain does not have to be the active window."
        ;
}

void normalizeIKStroke(InterceptionKeyStroke &ikstroke) {
    if (ikstroke.code > 0x7F) {
        ikstroke.code &= 0x7F;
        ikstroke.state |= 2;
    }
}

InterceptionKeyStroke convertVkeyEvent2ikstroke(VKeyEvent vkstroke)
{
    InterceptionKeyStroke iks = { (unsigned short) vkstroke.vcode, 0 };

    if (vkstroke.vcode >= 0xFF)
    {
        error("BUG: trying to send an interception keystroke > xFF");
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

VKeyEvent convertIkstroke2VKeyEvent(InterceptionKeyStroke ikStroke)
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

    IFTRACE cout << endl << "(CPS code: " << getPrettyVKLabelPadded(keyEvent.vcode, 0) << ")";

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
        break;
    }
    case VK_CPS_OBFUSCATED_SEQUENCE_START:
    {
        globalState.secretSequencePlayback = true;
        break;
    }
    case VK_CPS_PAUSE:
        if (globals.protectConsole && IsCapsicainForegroundWindow())
        {
            cout << endl << endl << "INFO: Discarding the PAUSE key. " << endl 
                << "      This would freeze Capsicain which is currently the active window (and this would stop your keyboard)";
            break;
        }
        
        //manually send a PAUSE sequence with E1 escape (iks state 4/5)
        IFTRACE cout << endl << "sending the Pause key sequence E1 LCTRL NUMLOCK";
        InterceptionKeyStroke iks_cont = {SC_LCTRL,4,0};
        interception_send(interceptionState.interceptionContext, interceptionState.interceptionDevice, (InterceptionStroke*)&iks_cont, 1);
        InterceptionKeyStroke iks_numl = { SC_NUMLOCK,0,0 };
        interception_send(interceptionState.interceptionContext, interceptionState.interceptionDevice, (InterceptionStroke*)&iks_numl, 1);
        iks_cont.state = 5;
        interception_send(interceptionState.interceptionContext, interceptionState.interceptionDevice, (InterceptionStroke*)&iks_cont, 1);
        iks_numl.state = 1;
        interception_send(interceptionState.interceptionContext, interceptionState.interceptionDevice, (InterceptionStroke*)&iks_numl, 1);

        break;
    }
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
            if (loopState.scancode != loopState.vcode)
                cout << "  --  " << PRETTY_VK_LABELS[loopState.vcode] << getSymbolForIKStrokeState(interceptionState.currentIKstroke.state);
            else
                cout << "  -->";
        }
        {
            sendVKeyEvent({ loopState.vcode, loopState.isDownstroke });
        }
    }
}

bool runExecutable(Executable &exe)
{
    char path[MAX_PATH];
    char args[MAX_PATH];
    char dir[MAX_PATH];
    ZeroMemory(path, MAX_PATH);
    ZeroMemory(args, MAX_PATH);
    ZeroMemory(dir, MAX_PATH);
    ExpandEnvironmentStringsA(exe.path.c_str(), path, MAX_PATH);
    ExpandEnvironmentStringsA(exe.args.c_str(), args, MAX_PATH);
    ExpandEnvironmentStringsA(exe.dir.c_str(), dir, MAX_PATH);

    SHELLEXECUTEINFOA info = {0};
    info.cbSize = sizeof(SHELLEXECUTEINFO);
    info.fMask = SEE_MASK_NO_CONSOLE | SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = exe.verb.c_str();
    info.lpFile = path;
    info.lpParameters = args;
    info.lpDirectory = dir;
    info.nShow = exe.mode;
    ShellExecuteExA(&info);
    exe.proc = info.hProcess;
    exe.pid = GetProcessId(exe.proc);
    exe.proc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, exe.pid);
    auto ret = (INT_PTR)info.hInstApp > 32;
    return ret;
}

void sendAHK(std::string msg)
{
    if (!ahk.handle)
    {
        IFDEBUG error("AHK dll is not loaded");
        return;
    }
    if (!ahk.threadid)
    {
        IFDEBUG error("AHK thread not started");
        return;
    }
    auto args = stringSplit(msg, ',');
    std::vector<std::wstring> wargs;
    for (int i = 0; i <= 10; ++i)
    {
        if (i < args.size())
            wargs.push_back(widen(args[i]));
        else
            wargs.push_back(L"");
    }
    auto wmsg = widen(msg);
    ahk.postFunction(wargs[0].c_str(), wargs[1].c_str(), wargs[2].c_str(), wargs[3].c_str(), wargs[4].c_str(), wargs[5].c_str(), wargs[6].c_str(), wargs[7].c_str(), wargs[8].c_str(), wargs[9].c_str(), wargs[10].c_str(), ahk.threadid);
}

void killExecutableByPath(string path)
{
    auto slash = path.find_last_of("\\/");
    if (slash != string::npos)
        path = path.substr(slash + 1);
    string ext = ".exe";
    if (!std::equal(ext.rbegin(), ext.rend(), path.rbegin()))
        path = path + ext;
    closeOrKillProgram(path);
}

void killExecutable(Executable &exe)
{
    if (exe.pid && GetProcessId(exe.proc) == exe.pid)
    {
        HANDLE hProc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, exe.pid);
        EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM)exe.pid);
        int result = 1; // 0=fail; 1=close; 2=kill
        if (WaitForSingleObject(hProc, 1000) != WAIT_OBJECT_0)
            result = (TerminateProcess(hProc, 0) ? 2 : 0);
        CloseHandle(hProc);
    }
    else
    {
        killExecutableByPath(exe.path);
    }
    exe.proc = 0;
    exe.pid = 0;
    exe.hwnd = 0;
}

//Send out all keys in a sequence
//Sequences are created for anything that requires more than one key event, like AltChar(123)
//Catch and process CPS virtual keys that have a value following in the next key
void playKeyEventSequence(vector<VKeyEvent> keyEventSequence)
{
    if (keyEventSequence.size() == 0) 
    {
        cout << endl << "BUG? keyEventSequence.size == 0" << endl;
        return;
    }

    VKeyEvent newKeyEvent;
    unsigned int delayBetweenKeyEventsMS = options.delayForKeySequenceMS;
    bool tempReleasedKeys = false; //command to temporarily release all physical keys that came before the current combo

    //remember that the next key will be the value for a func key'
    int  expectParamForFuncKey = -1;

    IFDEBUG
        if (!globalState.secretSequencePlayback && keyEventSequence.at(0).vcode != VK_CPS_OBFUSCATED_SEQUENCE_START)
             cout << "  --> SEQUENCE (" << dec << keyEventSequence.size() << ") ";

    for (VKeyEvent keyEvent : keyEventSequence)
    {
        // can be changed during key sequence by delay()
        delayBetweenKeyEventsMS = options.delayForKeySequenceMS;
        int vc = keyEvent.vcode;
        if (globalState.secretSequencePlayback)
            vc = deObfuscateVKey(vc);

        //test if this is the param for the preceding func key in "command + value" sequence
        if (expectParamForFuncKey != -1)
        {
            IFDEBUG cout << "{" + PRETTY_VK_LABELS[expectParamForFuncKey] + "}";
            switch (expectParamForFuncKey)
            {
            case VK_CPS_SLEEP:
                IFTRACE cout << endl << "vk_cps_sleep: " << vc;
                Sleep(vc);
                break;
            case VK_CPS_DEADKEY:
                IFTRACE cout << endl << "vk_cps_deadkey: " << getPrettyVKLabelPadded(vc, 0);
                modifierState.activeDeadkey = vc;
                break;
            case VK_CPS_CONFIGSWITCH:
                IFTRACE cout << endl << "vk_cps_configswitch: " << vc;
                switchConfig(vc, false);
                break;
            case VK_CPS_RECORDMACRO:
            case VK_CPS_RECORDSECRETMACRO:
            {
                int macroNum = vc;

                bool isSecret = false;
                if (expectParamForFuncKey == VK_CPS_RECORDSECRETMACRO)
                    isSecret = true;

                if (macroNum < 1 || macroNum >= MAX_NUM_MACROS)
                    cout << endl << "ERROR in .ini: bad number for macro. Must be 1.." << MAX_NUM_MACROS - 1;
                else if (globalState.recordingMacro != -1)
                    cout << endl << "INFO: a macro is already being recorded: #" << globalState.recordingMacro;
                else
                {
                    IFDEBUG cout << endl << "Start recording " << (isSecret ? "secret" : "") << "macro #" << macroNum << endl;
                    globalState.recordingMacro = macroNum;
                    globalState.recordedMacros[macroNum].clear();

                    if (isSecret)
                    {
                        globalState.secretSequenceRecording = true;
                        globalState.recordedMacros[macroNum].push_back({ VK_CPS_OBFUSCATED_SEQUENCE_START, true });
                    }
                }
                updateTrayIcon(true, globalState.recordingMacro >= 0, globalState.activeConfig);
                break;
            }
            case VK_CPS_PLAYMACRO:
            {
                IFTRACE cout << endl << "vk_cps_playmacro: " << vc;
                int macnum = vc;

                if (macnum < 1 || macnum >= MAX_NUM_MACROS)
                    cout << endl << "ERROR: bad number for macro. Must be 1.." << MAX_NUM_MACROS - 1;
                else
                {
                    if (globalState.recordedMacros[macnum].size() == 0)
                        cout << endl << "INFO macro #" << macnum << " has not been recorded before.";
                    else
                    {
                        playKeyEventSequence(globalState.recordedMacros[macnum]);
                        globalState.secretSequencePlayback = false;
                    }
                }
                break;
            }
            case VK_CPS_HOLDKEY:
            {
                IFTRACE cout << endl << "vk_cps_holdkey: " << getPrettyVKLabelPadded(loopState.vcode, 0) << " -> " << getPrettyVKLabelPadded(vc, 0);
                if (!getKeyHolding(vc))
                {
                    globalState.holdKeys[loopState.vcode].emplace(vc);
                    sendVKeyEvent(keyEvent, false);
                }
                else
                    sendVKeyEvent(keyEvent);
                break;
            }
            case VK_CPS_HOLDMOD:
            {
                if (!getKeyHolding(vc))
                {
                    if (modifierState.modifierDown)
                    {
                        for (int i = 0; i < NUMBER_OF_MODIFIERS; i++)
                        {
                            MOD mask = 1 << i;
                            if (modifierState.modifierDown & mask)
                            {
                                int mod = getModifierForBitmask(mask);
                                IFTRACE cout << endl << "vk_cps_holdmod: " << getPrettyVKLabelPadded(mod, 0) << " -> " << getPrettyVKLabelPadded(vc, 0);
                                globalState.holdKeys[mod].emplace(vc);
                                break;
                            }
                        }
                    }
                    else
                    {
                        globalState.holdKeys[loopState.vcode].emplace(vc);
                    }
                    sendVKeyEvent(keyEvent, false);
                }
                else
                    sendVKeyEvent(keyEvent);
                break;
            }
            case VK_CPS_DELAY:
                IFTRACE cout << endl << "vk_cps_delay: " << vc;
                options.delayForKeySequenceMS = vc;
                break;
            case VK_CPS_KEYDOWN:
                if (vc < 0xFF)
                    sendVKeyEvent(keyEvent);
                if (isModifier(vc))
                {
                    if (keyEvent.isDownstroke)
                    {
                        modifierState.modifierForceDown |= getModifierBitmaskForVcode(vc);
                        modifierState.modifierDown |= modifierState.modifierForceDown;
                    }
                    else
                    {
                        modifierState.modifierForceDown &= ~getModifierBitmaskForVcode(vc);
                        modifierState.modifierDown &= modifierState.modifierForceDown;
                    }
                }
                break;
            case VK_CPS_KEYTOGGLE:
                bool state;
                if (isModifier(vc))
                {
                    auto mask = getModifierBitmaskForVcode(vc);
                    state = modifierState.modifierForceDown & mask;
                    modifierState.modifierForceDown ^= mask;
                    modifierState.modifierDown &= modifierState.modifierForceDown;
                }
                else
                {
                    state = globalState.keysDownSent[vc & 0xFF];
                }
                if (vc < 0xFF)
                    sendVKeyEvent({vc, !state});
                break;
            case VK_CPS_KEYTAP:
                if (!isModifier(vc))
                    break;
                if (keyEvent.isDownstroke)
                    modifierState.modifierTapped |= getModifierBitmaskForVcode(vc);
                else
                    modifierState.modifierTapped &= ~getModifierBitmaskForVcode(vc);
                break;
            case VK_CPS_EXECUTE:
            {
                if (allMaps.executables.find(vc) == allMaps.executables.end())
                {
                    IFDEBUG cout << "Can't find executable " << vc << endl;
                    break;
                }
                runExecutable(allMaps.executables[vc]);
                break;
            }
            case VK_CPS_KILL:
            {
                if (allMaps.executables.find(vc) == allMaps.executables.end())
                {
                    IFDEBUG cout << "Can't find executable " << vc << endl;
                    break;
                }
                killExecutable(allMaps.executables[vc]);
                break;
            }
            case VK_CPS_SENDAHK:
            {
                auto msg = getAHKmsg(vc);
                if (msg != "")
                    sendAHK(msg);
                break;
            }
            default:
                cout << endl << "BUG? unknown expectParamForFuncKey";
            }

            expectParamForFuncKey = -1;
            continue;
        }

        //in no special state, evaluate the key
        if (vc == VK_CPS_TEMPRELEASEKEYS) //release and remember all keys that are physically down
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
        else if (vc == VK_CPS_TEMPRESTOREKEYS) //restore all keys that were down before 'VK_cps_temprelease'
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
        else if (vc == VK_CPS_RELEASEKEYS) //release all keys that are physically down
        {
            for (int i = 0; i <= 255; i++)
                if (globalState.keysDownSent[i])
                    sendVKeyEvent({ i, false }, false);
        }
        //func key with param; wait for next key which is the param
        else if (vc == VK_CPS_SLEEP
            || vc == VK_CPS_DEADKEY
            || vc == VK_CPS_CONFIGSWITCH
            || vc == VK_CPS_RECORDMACRO
            || vc == VK_CPS_RECORDSECRETMACRO
            || vc == VK_CPS_PLAYMACRO
            || vc == VK_CPS_HOLDKEY
            || vc == VK_CPS_HOLDMOD
            || vc == VK_CPS_DELAY
            || vc == VK_CPS_KEYDOWN
            || vc == VK_CPS_KEYTOGGLE
            || vc == VK_CPS_KEYTAP
            || vc == VK_CPS_EXECUTE
            || vc == VK_CPS_KILL
            || vc == VK_CPS_SENDAHK
            )
        {
            expectParamForFuncKey = vc;
        }
        else //regular non-escaped keyEvent
        {
            if(globalState.secretSequencePlayback)
                sendVKeyEvent({ deObfuscateVKey(keyEvent.vcode) , keyEvent.isDownstroke });
            else
                sendVKeyEvent(keyEvent);
            if (!options.disableAHKDelay && (vc == AHK_HOTKEY1 || vc == AHK_HOTKEY2))
                Sleep(DEFAULT_DELAY_FOR_AHK_MS);
            else
                Sleep(delayBetweenKeyEventsMS);
        }
    }

    if (tempReleasedKeys)
        error("VK_CPS_TEMPRELEASEKEYS without corresponding VK_CPS_TEMPRESTOREKEYS. Check your config.");
    if (expectParamForFuncKey != -1)
        error("BUG: func key with param: " + getPrettyVKLabel(expectParamForFuncKey) + "is unfinished");
}

int getKeyHolding(int vcode)
{
    for (int i = 0; i < VK_MAX; i++)
    {
        if (globalState.holdKeys[i].find(vcode) != globalState.holdKeys[i].end())
            return i;
    }
    return 0;
}

std::string getHoldKeyString(std::set<int> &v, std::string delim)
{
    std::string out;
    for (auto it = v.rbegin(); it != v.rend(); ++it)
    {
        out += PRETTY_VK_LABELS[*it];
        out += delim;
    }
    for (int i = 0; i < delim.size(); ++i)
        out.pop_back();
    return out;
}

map<int, int> KEY_TO_MOUSE{
    { VM_LEFT, INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN },
    { VM_RIGHT, INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN },
    { VM_MIDDLE, INTERCEPTION_MOUSE_MIDDLE_BUTTON_DOWN },
    { VM_BUTTON4, INTERCEPTION_MOUSE_BUTTON_4_DOWN },
    { VM_BUTTON5, INTERCEPTION_MOUSE_BUTTON_5_DOWN },
};
bool vkeyToMouse(VKeyEvent keyEvent)
{
    if (keyEvent.vcode < VM_LEFT || keyEvent.vcode > VM_WHEEL_RIGHT)
        return false;

    InterceptionMouseStroke mstroke{0};

    if (keyEvent.vcode >= VM_LEFT && keyEvent.vcode <= VM_BUTTON5)
    {
        mstroke.state = KEY_TO_MOUSE[keyEvent.vcode];
        if (!keyEvent.isDownstroke)
            mstroke.state = mstroke.state << 1;
    }
    else if (keyEvent.vcode == VM_WHEEL_UP)
    {
        mstroke.state = INTERCEPTION_MOUSE_WHEEL;
        mstroke.rolling = 120; //FIXME
    }
    else if (keyEvent.vcode == VM_WHEEL_DOWN)
    {
        mstroke.state = INTERCEPTION_MOUSE_WHEEL;
        mstroke.rolling = -120; //FIXME
    }
    else if (keyEvent.vcode == VM_WHEEL_LEFT)
    {
        mstroke.state = INTERCEPTION_MOUSE_HWHEEL;
        mstroke.rolling = -120; //FIXME
    }
    else if (keyEvent.vcode == VM_WHEEL_RIGHT)
    {
        mstroke.state = INTERCEPTION_MOUSE_HWHEEL;
        mstroke.rolling = 120; //FIXME
    }
    else
    {
        return false;
    }

    if (mstroke.rolling && !keyEvent.isDownstroke)
        return true;

    int dev;
    if (interception_is_mouse(interceptionState.interceptionDevice))
    {
        dev = interceptionState.interceptionDevice;
    }
    else if(interceptionState.lastMouse)
    {
        dev = interceptionState.lastMouse;
    }
    else
    {
        cout << endl << "Error: Don't know which mouse to send this to, use one!";
        return false;
    }

    interception_send(interceptionState.interceptionContext, dev, (InterceptionStroke *)&mstroke, 1);
    return true;
}

void sendVKeyEvent(VKeyEvent keyEvent, bool hold)
{
    IFTRACE cout << endl << "sendVkeyEvent(" << keyEvent.vcode << ")";
    if (keyEvent.vcode < 0)
    {
        cout << endl << "BUG: vcode<0";
        return;
    }

    if (keyEvent.vcode == 0)
    {
        IFDEBUG cout << endl << "{blocked NOP}";
        return;
    }

    if (globalState.holdKeys[keyEvent.vcode].size() && hold)
    {
        int code = keyEvent.vcode;
        set<int> release;
        IFDEBUG cout << " {" << PRETTY_VK_LABELS[code] << (keyEvent.isDownstroke ? " holding " : " released ") << globalState.holdKeys[code].size() << ": " << getHoldKeyString(globalState.holdKeys[code], "+") << "}";
        if (keyEvent.isDownstroke)
        {
            if (options.holdRepeatsAllKeys)
            {
                for (auto it = globalState.holdKeys[code].begin(); it != globalState.holdKeys[code].end(); ++it)
                    sendVKeyEvent({*it, true}, false);
            }
            else
            {
                sendVKeyEvent({*globalState.holdKeys[code].begin(), true}, false);
            }
            return;
        }
        else
        {
            for (auto it = globalState.holdKeys[code].rbegin(); it != globalState.holdKeys[code].rend(); ++it)
                release.emplace(*it);
            globalState.holdKeys[code].clear();
            for (auto key : release)
                sendVKeyEvent({key, false}, false);
            if (release.find(keyEvent.vcode) != release.end())
                return;
        }
    }

    if (keyEvent.vcode > 0xFF || keyEvent.vcode == VK_CPS_PAUSE)
    {
        sendCapsicainCodeHandler(keyEvent);
        return;
    }

    unsigned char scancode = (unsigned char) keyEvent.vcode;

    if (scancode == 0xE4)  //what was that for?
        IFDEBUG cout << " {sending E4} ";

    if (!keyEvent.isDownstroke &&  !globalState.keysDownSent[scancode])  //ignore up when key is already up
    {
        IFDEBUG cout << " {blocked " << PRETTY_VK_LABELS[scancode] << " UP: was not down}";
        return;
    }

    auto holdingkey = getKeyHolding(scancode);
    if (!keyEvent.isDownstroke && holdingkey)  //ignore up when other key is holding it
    {
        IFDEBUG cout << " {blocked " << PRETTY_VK_LABELS[scancode] << " UP: " << PRETTY_VK_LABELS[holdingkey] << " is holding}";
        return;
    }

    //consistency check
    if (globalState.keysDownSent[scancode] == 0 && keyEvent.isDownstroke)
        globalState.keysDownSentCounter++;
    else if (globalState.keysDownSent[scancode] == 1 && !keyEvent.isDownstroke)
        globalState.keysDownSentCounter--;

    globalState.keysDownSent[scancode] = keyEvent.isDownstroke;

    //handle live macro recording
    if (globalState.recordingMacro >= 0)
    {
        if (globalState.recordedMacros[globalState.recordingMacro].size() >= MAX_MACRO_LENGTH -2)  //macro getting too big
        {
            globalState.recordingMacro = -1;
            globalState.secretSequenceRecording = false;
            updateTrayIcon(true, globalState.recordingMacro >= 0, globalState.activeConfig);
            cout << endl << endl << "Macro Length > " << MAX_MACRO_LENGTH << ". Forgotten Macro?" << "Stop recording macro #" << globalState.recordingMacro << endl << endl;
        }
        else
        { 
            //drop upstroke from the starting shortcut?
            if (keyEvent.isDownstroke || globalState.recordedMacros[globalState.recordingMacro].size() > 0 )
            {
                //store the macro obfuscated?
                VKeyEvent obfusc = keyEvent;
                if (globalState.secretSequenceRecording)
                    obfusc.vcode = obfuscateVKey(obfusc.vcode);
                globalState.recordedMacros[globalState.recordingMacro].push_back(obfusc);
            }
        }
    }

    if (vkeyToMouse(keyEvent))
        return;

    InterceptionKeyStroke iks = convertVkeyEvent2ikstroke(keyEvent);
    //hide secret macro recording?
    IFDEBUG
        if(!globalState.secretSequencePlayback)
            cout << " {" << PRETTY_VK_LABELS[keyEvent.vcode] << (keyEvent.isDownstroke ? "v" : "^") << " #" << globalState.keysDownSentCounter << "}";

    int dev;
    if (interception_is_keyboard(interceptionState.interceptionDevice))
    {
        dev = interceptionState.interceptionDevice;
    }
    else if(interceptionState.lastKeyboard)
    {
        dev = interceptionState.lastKeyboard;
    }
    else
    {
        cout << endl << "Don't know which keyboard to send this to, use one!";
        return;
    }

    interception_send(interceptionState.interceptionContext, dev, (InterceptionStroke *)&iks, 1);

    //restore LEDs for ON/OFF indication?
    if (globals.capsicainOnOffKey >0 
        && keyEvent.isDownstroke
        && (globals.capsicainOnOffKey == SC_NUMLOCK || globals.capsicainOnOffKey == SC_SCRLOCK || globals.capsicainOnOffKey == SC_CAPS)
        && (keyEvent.vcode == SC_NUMLOCK || keyEvent.vcode == SC_SCRLOCK || keyEvent.vcode == SC_CAPS ) 
        //does ESC reset ScrLock on some KBs? In that case re-enable ESC check  || keyEvent.vcode == SC_ESCAPE)
        )
    {
        Sleep(50); //give Windows time to register e.g. NumLock key event, since soon we will query its state
        setLED(globals.capsicainOnOffKey, true);
    }
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
    case 0b000: return "v";
    case 0b001: return "^";
    case 0b010: return "v*";
    case 0b011: return "^*";
    case 0b100: return "v**";
    case 0b101: return "^**";
    case 0b001000: return "??TERMSRV_SET_LED down??";
    case 0b001001: return "??TERMSRV_SET_LED up??";
    case 0b010000: return "??TERMSRV_SHADOW down??";
    case 0b010001: return "??TERMSRV_SHADOW up??";
    case 0b100000: return "??TERMSRV_VKPACKET down??";
    case 0b100001: return "??TERMSRV_VKPACKET up??";
    }
    return "???" + to_string(state);
}

int obfuscateVKey(int vk)
{
    return vk ^ 0b0101010101010101;
}
int deObfuscateVKey(int vk)
{
    return vk ^ 0b0101010101010101;
}
