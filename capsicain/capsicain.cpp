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

struct Globals
{
    string iniVersion = "unnamed version - add 'iniVersion xyz123' to capsicain.ini";
    int activeLayerOnStartup = DEFAULT_ACTIVE_LAYER;
    int delayOnStartupMS = DEFAULT_DELAY_ON_STARTUP_MS;
    bool startMinimized = false;
    bool startInTraybar = false;
    bool startAHK = false;
} global;

struct Options
{
    bool debug = true;
    int delayForKeySequenceMS = DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;
    bool flipZy = false;
    bool flipAltWinOnAppleKeyboards = false;
    bool LControlLWinBlocksAlphaMapping = false;
    bool processOnlyFirstKeyboard = false;
} option;

struct ModifierCombo
{
    int vkey = SC_NOP;
    unsigned short modAnd = 0;
    unsigned short modNot = 0;
    unsigned short modTap = 0;
    vector<VKeyEvent> keyEventSequence;
};

struct AllMaps
{
    int alphamap[MAX_VCODES] = { SC_NOP };
    vector<ModifierCombo> modCombos;// = new vector<ModifierCombo>();
    
    //inkey outkey (tapped)
    //-1 = undefined key
    int rewiremap[REWIRE_ROWS][REWIRE_COLS] = { SC_NOP };
} allMaps;

struct GlobalState
{
    int  activeLayer = 0;
    string activeLayerName = DEFAULT_ACTIVE_LAYER_NAME;

    InterceptionContext interceptionContext = NULL;
    InterceptionDevice interceptionDevice = NULL;
    InterceptionDevice previousInterceptionDevice = NULL;

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
    bool readingUsername = false;
    bool readingPassword = false;

    chrono::steady_clock::time_point timepointPreviousKeyEvent;

} globalState;

struct ModifierState
{
    unsigned short modifierDown = 0;
    unsigned short modifierTapped = 0;
    vector<VKeyEvent> modsTempAltered;

} modifierState;

struct LoopState
{
    unsigned char scancode = SC_NOP; //hardware code sent by Interception
    int vcode = -1; //key code used internally; equals scancode or a Virtual code > FF
    bool isDownstroke = false;
    bool isModifier = false;
    bool tapped = false;
    bool tappedSlow = false;  //autorepeat set in before key release
    bool tapHoldMake = false;  //tap-and-hold action (like LAlt > mod12 // LAlt)

    InterceptionKeyStroke originalIKstroke = { SC_NOP, 0 };

    vector<VKeyEvent> resultingVKeyEventSequence;

    chrono::steady_clock::time_point timepointLoopStart;
    chrono::steady_clock::time_point timepointStopwatch;
    long timeSinceLastKeyEventMS = 0;
} loopState;

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

void parseIniGlobals()
{
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_GLOBAL, sanitizedIniContent);

    for (string line : sectLines)
    {
        string token = stringGetFirstToken(line);
        if (token == "debugonstartup")
            option.debug = true;
        else if (token == "iniversion")
            global.iniVersion = stringGetRestBehindFirstToken(line);
        else if (token == "delayonstartupmS")
            getIntValueForTaggedKey(INI_TAG_GLOBAL, "delayOnStartupMS", global.delayOnStartupMS, sanitizedIniContent);
        else if (token == "startminimized")
            global.startMinimized = true;
        else if (token == "startintraybar")
            global.startInTraybar = true;
        else if (token == "startahk")
            global.startAHK = true;
        else if (token == "activelayeronstartup")
            cout << endl;
        else
            cout << endl << "WARNING: unknown GLOBAL " << token;
    }

    if (!getIntValueForTaggedKey(INI_TAG_GLOBAL, "ActiveLayerOnStartup", global.activeLayerOnStartup, sanitizedIniContent))
        cout << endl << "No ini setting for 'GLOBAL activeLayerOnStartup'. Setting default layer " << global.activeLayerOnStartup;
}

void DetectTapping(const VKeyEvent &originalVKeyEvent)
{
    //Tapped key?
    loopState.tapped = !loopState.isDownstroke
        && (loopState.scancode == globalState.previousKey1EventIn.vcode)
        && (globalState.previousKey1EventIn.isDownstroke);

    //Slow tap?
    loopState.tappedSlow = loopState.tapped
        && (globalState.previousKey2EventIn.vcode == loopState.scancode)
        && (globalState.previousKey2EventIn.isDownstroke);

    if (loopState.tappedSlow)
        loopState.tapped = false;

    //Tap and hold Make?
    if (loopState.isDownstroke)
    {
        if (
            originalVKeyEvent.vcode == globalState.previousKey1EventIn.vcode
            && originalVKeyEvent.vcode == globalState.previousKey2EventIn.vcode
            && !globalState.previousKey1EventIn.isDownstroke
            && globalState.previousKey2EventIn.isDownstroke
            )
        {
            loopState.tapHoldMake = true;
            IFDEBUG cout << endl << "detected tapHold: " << hex << originalVKeyEvent.vcode;
        }
    }
    
    //cannot detect tapHold Break here. This is done by ProcessRewire()

    //remember last three keys
    globalState.previousKey3EventIn = globalState.previousKey2EventIn;
    globalState.previousKey2EventIn = globalState.previousKey1EventIn;
    globalState.previousKey1EventIn = originalVKeyEvent;
}

int main()
{
    initConsoleWindow();
    printHelloHeader();

    defineAllPrettyVKLabels(PRETTY_VK_LABELS);
    resetAllStatesToDefault();
    loopState.timepointLoopStart = timeSetTimepointNow();

    if (!readSanitizeIniFile(sanitizedIniContent))
    {
        cout << endl << "No capsicain.ini - exiting..." << endl;
        Sleep(5000);
        return 0;
    }

    parseIniGlobals();
    switchLayer(global.activeLayerOnStartup);

    cout << endl << "Release all keys now... (waiting DelayOnStartupMS = " << global.delayOnStartupMS << " ms)" << endl;
    Sleep(global.delayOnStartupMS); //time to release shortcut keys that started capsicain

    globalState.interceptionContext = interception_create_context();
    interception_set_filter(globalState.interceptionContext, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);

    printHelloFeatures();

    if (global.startAHK)
    {
        string msg = startProgramSameFolder(PROGRAM_NAME_AHK);
        cout << endl << endl << "starting AHK... ";
        cout << (msg == "" ? "OK" : "Not. '" + msg + "'");
    }
    cout << endl << endl << "[ESC] + [X] to stop." << endl << "[ESC] + [H] for Help";
    cout << endl << endl << "capsicain running.... ";

    if (global.startMinimized)
        ShowInTaskbarMinimized();
    if (global.startInTraybar)
        ShowInTraybar();

    raise_process_priority(); //careful: if we spam key events, other processes get no timeslots to process them. Sleep a bit...

    //CORE LOOP
    //wait for the next key from Interception
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

        //Timing. Timer is not precise, sleep() even less; just a rough outline. Expect occasional 30ms sleeps from thread scheduling.
        globalState.timepointPreviousKeyEvent = loopState.timepointLoopStart;
        loopState.timepointLoopStart = timeSetTimepointNow();
        resetLoopState();
        loopState.timeSinceLastKeyEventMS = timeBetweenTimepointsMS(globalState.timepointPreviousKeyEvent, loopState.timepointLoopStart);

        //copy InterceptionKeyStroke (unpleasant to use) to plain VKeyEvent
        VKeyEvent originalVKeyEvent = ikstroke2VKeyEvent(loopState.originalIKstroke);
        loopState.scancode = originalVKeyEvent.vcode;  //scancode is write-once
        loopState.vcode = loopState.scancode;          //vcode may be altered below
        loopState.isDownstroke = originalVKeyEvent.isDownstroke;

        //Tapdance
        DetectTapping(originalVKeyEvent);
        //slow tap breaks tapping
        if (loopState.tappedSlow)
            modifierState.modifierTapped = 0;

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

        //Early check for core ESC commands, in case layer 0 continues without further checks
        //remember the hardware ESC key state
        //do not send v until ^ comes in
        //ESC+X exits, unconditionally
        //ESC+[0]..[9]  layer switch
        if (loopState.scancode == SC_ESCAPE)
        {
            globalState.realEscapeIsDown = loopState.isDownstroke;
        }
        else if (globalState.realEscapeIsDown && loopState.isDownstroke)
        {
            if (loopState.scancode == SC_X) //extra check to make sure Exit is bug free
            {
                ShowInTaskbar();
                break;
            }
            else if (loopState.scancode == SC_0)
            {
                cout << endl << "LAYER CHANGE: " << LAYER_DISABLED;
                switchLayer(LAYER_DISABLED);
                resetLoopState();
                resetModifierState();
                releaseAllSentKeys();
                resetAlphamap();
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

        //Layer 0: standard keyboard, no further processing, just forward everything
        if (globalState.activeLayer == LAYER_DISABLED)
        {
            interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&loopState.originalIKstroke, 1);
            continue;
        }

        //hard rewire all REWIREd keys
        loopState.vcode = loopState.scancode;
        processRewireScancodeToVirtualcode();
        if (loopState.vcode == SC_NOP)   //rewired to NOP to disable keys
        {
            IFDEBUG cout << " (NOP)";
            continue;
        }

        //Hardware ESC is lazy key; if it is rewired, that key must not trigger on ESC+command
        if (loopState.scancode == SC_ESCAPE)
        {
            IFDEBUG cout << endl << "(Hard ESC" << (loopState.isDownstroke ? "v " : "^ ") << ")";
            if (loopState.isDownstroke)
                continue;
            //hardware ESC tapped -> reset state incl. sticky modifiers
            if(loopState.tapped)
            {
                //handle rewired ESC^ (send ESC or whatever it is rewired to)
                IFDEBUG cout << endl << "(Send " << PRETTY_VK_LABELS[loopState.vcode] << "v^ )";
                sendVKeyEvent({ loopState.vcode, true });
                sendVKeyEvent({ loopState.vcode, false });
                //reset state on hardware ESC
                releaseAllSentKeys();
                resetModifierState();
                resetLoopState();
            }
            continue;
        }

        // All other ESC + key commands
        // NOTE: some major key shadowing here...
        // - cherry is good
        // - apple keyboard cannot do RCTRL+CAPS+ESC and ESC+Caps shadows the entire row a-s-d-f-g-....
        // - Dell cant do ctrl-caps-x
        // - Cypher has no RControl... :(
        // - HP shadows the 2-w-s-x and 3-e-d-c lines
        if (globalState.realEscapeIsDown)
        {
            if (loopState.isDownstroke)
            {
                if (processCommand())
                    continue;
                else
                {
                    ShowInTaskbar();
                    break;
                }
            }
            else if(isModifier(loopState.vcode)) //sticky mods with ESC+mod
            {
                continue;
            }
        }

        //Username
        if (globalState.readingUsername)
        {
            if (loopState.vcode == SC_RETURN)
            {
                globalState.readingUsername = false;
                cout << " done";
            }
            else if (loopState.isDownstroke || globalState.username.size() > 0)  //filter upstroke from the command key
                globalState.username.push_back({ loopState.vcode,loopState.isDownstroke });
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
                globalState.password.push_back({ loopState.vcode, loopState.isDownstroke });
            continue;
        }


        IFDEBUG cout << endl << "(" << setw(5) << dec << loopState.timeSinceLastKeyEventMS <<" ms) [" << hex << \
            (loopState.vcode == loopState.scancode ? "" : PRETTY_VK_LABELS[loopState.scancode] + " => ") \
            << getPrettyVKLabelPadded(loopState.vcode,8) << getSymbolForIKStrokeState(loopState.originalIKstroke.state)
            << " =" << hex << loopState.originalIKstroke.code << " " << loopState.originalIKstroke.state << "]";

        //evaluate modifiers
        processModifierState();
    
        IFDEBUG cout << " [M:" << hex << modifierState.modifierDown;
        IFDEBUG if (modifierState.modifierTapped)  cout << " TAP:" << hex << modifierState.modifierTapped;
        IFDEBUG cout << "] ";

        //evaluate modified keys
        processCombos();

        //alphakeys: basic character key layout. Don't remap the Ctrl combos?
        processMapAlphaKeys();

        //break tapped state?
        if (!isModifier(loopState.vcode))
            modifierState.modifierTapped = 0;

        IFDEBUG 
        {
            cout << "\t (" << dec << timeMillisecondsSinceTimepoint(loopState.timepointLoopStart) << dec << " ms)";
            loopState.timepointStopwatch = timeSetTimepointNow();
        }

        sendResultingKeyOrSequence();

        IFDEBUG
        {
            long sendtime = timeMillisecondsSinceTimepoint(loopState.timepointStopwatch);
            if(sendtime > 2)
                cout << "\t (slow to send: " << dec << sendtime << " ms)";
            cout << (loopState.tappedSlow ? " (slow tap)" : "");
            cout << (loopState.tapped ? " (tap)" : "");
            if (globalState.tapAndHoldKey >= 0)
                cout <<" (tapHold:"<<hex<<globalState.tapAndHoldKey<<")";
                ;
        }
    }
    interception_destroy_context(globalState.interceptionContext);

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
    //TODO Long presses, release will not register as tapped.
    if (loopState.tapped)
        modifierState.modifierTapped |= modBitmask;
}

//handle all REWIRE configs
void processRewireScancodeToVirtualcode()
{
    //TODO this won't work with new rewire style
    if (option.flipAltWinOnAppleKeyboards && globalState.deviceIsAppleKeyboard)
    {
        switch (loopState.scancode)
        {
        case SC_LALT: loopState.vcode = SC_LWIN; break;
        case SC_LWIN: loopState.vcode = SC_LALT; break;
        case SC_RALT: loopState.vcode = SC_RWIN; break;
        case SC_RWIN: loopState.vcode = SC_RALT; break;
        }
    }

    bool finalVcode = false;

    //Rewire to new vcode; check for Tapped rule
    if (!finalVcode)
    {
        //repeating tapHold key?
        if (loopState.scancode == globalState.tapAndHoldKey && loopState.isDownstroke)
        {
            loopState.scancode = SC_NOP;
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
    }

    //update the internal modifier state
    loopState.isModifier = isModifier(loopState.vcode) ? true : false;
}


void processCombos()
{
    if (!loopState.isDownstroke || (modifierState.modifierDown == 0 && modifierState.modifierTapped == 0))
        return;

    for (ModifierCombo modcombo : allMaps.modCombos)
    {
        if (modcombo.vkey == loopState.vcode)
        {
            if (
                (modifierState.modifierDown & modcombo.modAnd) == modcombo.modAnd &&
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
}

void processMapAlphaKeys()
{
    if (loopState.isModifier ||
        (option.LControlLWinBlocksAlphaMapping && (IS_LCTRL_DOWN || IS_LWIN_DOWN)))
    {
        return;
    }

    loopState.vcode = allMaps.alphamap[loopState.vcode];

    if (option.flipZy)
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
            ShowInTraybar();
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
        option.flipAltWinOnAppleKeyboards = !option.flipAltWinOnAppleKeyboards;
        cout << "Flip ALT<>WIN for Apple boards: " << (option.flipAltWinOnAppleKeyboards ? "ON" : "OFF") << endl;
        break;
    case SC_E:
        cout << "ERROR LOG: " << endl << errorLog << endl;
        popupConsole = true;
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
        cout << "INI filtered for layer " << globalState.activeLayerName;
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
        popupConsole = true;
        break;
    case SC_D:
        option.debug = !option.debug;
        cout << "DEBUG mode: " << (option.debug ? "ON" : "OFF");
        popupConsole = true;
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
        option.flipZy = !option.flipZy;
        cout << "Flip Z<>Y mode: " << (option.flipZy ? "ON" : "OFF");
        break;
    case SC_C:
        cout << "List of all Key Labels for scancodes" << endl
             << "------------------------------------" << endl;
        printKeylabels();
        popupConsole = true;
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
        //Sticky modifiers with soft (potentially rewired) ESC+modifier. Tap hard ESC to release.
        if (isModifier(loopState.vcode))
        {
            if (loopState.isDownstroke)
            {
                modifierState.modifierDown |= getBitmaskForModifier(loopState.vcode);
                modifierState.modifierTapped = 0;
                if (isRealModifier(loopState.vcode))
                    sendResultingKeyOrSequence();
                IFDEBUG cout << endl << "Locking modifier: " << PRETTY_VK_LABELS[loopState.vcode];
            }
        }
        else
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
                cout << "\t--> " << PRETTY_VK_LABELS[loopState.vcode] << " " << getSymbolForIKStrokeState(loopState.originalIKstroke.state);
            else
                cout << "\t--";
        }
        if (loopState.vcode < 255 && loopState.vcode != SC_NOP)
        {
            sendVKeyEvent({ loopState.vcode, loopState.isDownstroke });
        }
    }
}

//May contain Capsicain Escape sequences, those are a bit hacky. 
//They are used to temporarily make/break modifiers (for example, ALT+Q -> Shift+1... but don't mess with shift state if it is actually pressed)
//Sequence starts with SC_CPS_ESC DOWN
//Scancodes inside are modifier bitmasks. State DOWN means "set these modifiers if they are up", UP means "clear those if they are down".
//Sequence ends with SC_CPS_ESC UP -> the modifier sequence is played.
//Second SC_CPS_ESC UP -> the previous changes to the modifiers are reverted.
void playKeyEventSequence(vector<VKeyEvent> keyEventSequence)
{
    VKeyEvent newKeyEvent;
    unsigned int delayBetweenKeyEventsMS = option.delayForKeySequenceMS;
    bool tempReleasedKeys = false;
    bool vkSleepTriggered = false;

    IFDEBUG cout << "\t--> SEQUENCE (" << dec << keyEventSequence.size() << ")";
    for (VKeyEvent keyEvent : keyEventSequence)
    {
        if (vkSleepTriggered)
        {
            IFDEBUG cout << endl << "vksleep: " << keyEvent.vcode;
            Sleep(keyEvent.vcode);
            vkSleepTriggered = false;
        }
        //release and remember all keys that are physically down
        if (keyEvent.vcode == VK_CPS_TEMPRELEASEKEYS)
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
        //restore all keys that were down before 'VK_temprelease'
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
            vkSleepTriggered = true;
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

    if(tempReleasedKeys)
        error("VK_CPS_TEMPRELEASEKEYS without corresponding VK_CPS_TEMPRESTOREKEYS. Check your config.");
    if (vkSleepTriggered)
        error("BUG: VK_CPS_SLEEP is unfinished");
}


void getHardwareId()
{
    {
        wchar_t  hardware_id[500] = { 0 };
        string id;
        size_t length = interception_get_hardware_id(globalState.interceptionContext, globalState.interceptionDevice, hardware_id, sizeof(hardware_id));
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
    string title = "Capsicain v" VERSION;
    SetConsoleTitle(title.c_str());
}

// Parses the OPTIONS in the given section.
// Returns false if section does not exist.
bool parseIniOptions(std::vector<std::string> assembledIni)
{
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_OPTIONS, assembledIni);
    globalState.activeLayerName = "OPTION_layerName_undefined";

    for (string line : sectLines)
    {
        string token = stringGetFirstToken(line);
        if (token == "layername")
        {
            globalState.activeLayerName = stringGetRestBehindFirstToken(line);
        }
        else if (token == "debug")
        {
            option.debug = true;
        }
        else if (token == "flipzy")
        {
            option.flipZy = true;
        }
        else if (token == "altalttoalt")
        {
            cout << endl << "OPTION AltAltToAlt is obsolete. You can do this now with 'REWIRE LALT MOD12 // LALT'";
        }
        else if (token == "flipaltwinonapplekeyboards")
        {
            option.flipAltWinOnAppleKeyboards = true;
        }
        else if (token == "lcontrollwinblocksalphamapping")
        {
            option.LControlLWinBlocksAlphaMapping = true;
        }
        else if (token == "processonlyfirstkeyboard")
        {
            option.processOnlyFirstKeyboard = true;
        }
        else if (token == "delayforkeysequencems")
        {
            getIntValueForKey("delayForKeySequenceMS", option.delayForKeySequenceMS, sectLines);
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
    resetRewiremap();

    int tagCounter = 0;
    unsigned char keyIn;
    int keyOut, keyTap, keyTapHold;
    for (string line : sectLines)
    {
        keyTap = -1;
        keyTapHold = -1;
        if (lexRewireRule(line, keyIn, keyOut, keyTap, keyTapHold, PRETTY_VK_LABELS))
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

    unsigned short mods[3] = { 0 }; //and, not, tap (nop, for)
    vector<VKeyEvent> keyEventSequence;

    for (string line : sectLines)
    {
        int key;
        if (lexRule(line, key, mods, keyEventSequence, PRETTY_VK_LABELS))
        {
            bool isDuplicate = false;
            for (ModifierCombo testcombo : allMaps.modCombos)
            {
                if (key == testcombo.vkey && mods[0] == testcombo.modAnd
                    && mods[1] == testcombo.modNot && mods[2] == testcombo.modTap)
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
                        IFDEBUG cout << endl << "WARNING: Ignoring redefinition of Combo: " <<
                        PRETTY_VK_LABELS[key] << " [ " << hex << mods[0] << ", " << mods[1] << ", " << mods[2] << "] > ...";

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

    resetAlphamap();
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
            if (!lexAlphaFromTo(mapFromTo, allMaps.alphamap, PRETTY_VK_LABELS))
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

bool parseProcessIniLayer(int layer)
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

    parseIniRewires(assembledLayer);

    parseIniCombos(assembledLayer);
    IFDEBUG cout << endl << "Combo  Definitions: " << dec << allMaps.modCombos.size();
    parseIniAlphaLayout(assembledLayer);
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

void switchLayer(int layer)
{
    if (layer == LAYER_DISABLED)
    {
        globalState.activeLayer = LAYER_DISABLED;
        globalState.activeLayerName = LAYER_DISABLED_LAYER_NAME;
    }
    else if (parseProcessIniLayer(layer))
    {
        globalState.activeLayer = layer;
    }

    cout << endl << endl << "ACTIVE LAYER: " << globalState.activeLayer << " = " << globalState.activeLayerName;
}

void resetCapsNumScrollLock()
{
    //set NumLock, release CapsLock+Scrolllock
    vector<VKeyEvent> sequence;
    if (!(GetKeyState(VK_NUMLOCK) & 0x0001))
        keySequenceAppendMakeBreakKey(SC_NUMLOCK, sequence);
    if (GetKeyState(VK_CAPITAL) & 0x0001)
        keySequenceAppendMakeBreakKey(SC_CAPS, sequence);
    if (GetKeyState(VK_SCROLL) & 0x0001)
        keySequenceAppendMakeBreakKey(SC_SCRLOCK, sequence);
    if (sequence.size() != 0)
        playKeyEventSequence(sequence);
}

void resetAlphamap()
{
    for (int i = 0; i < MAX_VCODES; i++)  //initialize to "map to same char"
        allMaps.alphamap[i] = i;
}

void resetRewiremap()
{
    for (int r = 0; r < REWIRE_ROWS; r++)
        for (int c = 0; c < REWIRE_COLS; c++)
            allMaps.rewiremap[r][c] = -1;
}


void resetLoopState()
{
    loopState.isDownstroke = false;
    loopState.scancode = 0;
    loopState.isModifier = false;
    loopState.tapped = false;
    loopState.tappedSlow = false;
    loopState.tapHoldMake = false;
    loopState.resultingVKeyEventSequence.clear();
}

void resetModifierState()
{
    modifierState.modifierDown = 0;
    modifierState.modifierTapped = 0;
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

void printHelloFeatures()
{
    cout
        << endl << "ini version: " << global.iniVersion
        << endl << endl << "OPTIONS"
        << endl << (option.flipZy ? "ON:" : "OFF:") << " Z <-> Y"
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
        << "ini version: " << global.iniVersion << endl
        << "keyboard hardware id: " << globalState.deviceIdKeyboard << endl
        << "Apple keyboard: " << globalState.deviceIsAppleKeyboard << endl
        << "active layer: " << globalState.activeLayer << " = " << globalState.activeLayerName << endl
        << "delay between keys in sequences (ms): " << option.delayForKeySequenceMS << endl
        << "number of keys-down sent: " << dec <<   numMakeSent << endl
        << (errorLog.length() > 1 ? "ERROR LOG contains entries" : "clean error log") << " (" << dec << errorLog.length() << " chars)" << endl
        ;
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
        << "[0]..[9] switch layers. [0] is the 'do nothing but listen for commands' layer" << endl
        << "[W] flip ALT <-> WIN on Apple keyboards" << endl
        << "[Z] (labeled [Y] on GER keyboard): flip Y <-> Z keys" << endl
        << "[S] Status" << endl
        << "[D] Debug mode output" << endl
        << "[E] Error log" << endl
        << "[C] Print list of key labels for all scancodes" << endl
        << "[R] Reset and reload the .ini" << endl
        << "[T] Move Taskbar icon to Tray and back" << endl
        << "[U] Username Enter/Playback" << endl
        << "[P] Password Enter/Playback. DO NOT USE if strangers can access your local machine." << endl
        << "[I] Show processed Ini for the active layer" << endl
        << "[A] Autohotkey start" << endl
        << "[Y] autohotkeY stop" << endl
        << "[J][K][L][;] Macro Recording: Start,Stop,Playback,Copy macro definition to clipboard." << endl
        << "[,] and [.]: delay between keys in sequences -/+ 1ms " << endl
        << "[any modifier]: 'sticky modifier' : keeps it pressed until you hit ESC"
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

//handle all special Capsicain VCodes. Trigger events on downstroke only
void sendCapsicainCodeHandler(VKeyEvent keyEvent)
{
    if (!keyEvent.isDownstroke)
        return;

    switch (keyEvent.vcode)
    {
    case VK_CAPSON:
    {
        if (!(GetKeyState(VK_CAPITAL) & 0x0001))
        {
            sendVKeyEvent({ SC_CAPS, true });
            sendVKeyEvent({ SC_CAPS, false });
        }
        break;
    }
    case VK_CAPSOFF:
    {
        if ((GetKeyState(VK_CAPITAL) & 0x0001))
        {
            sendVKeyEvent({ SC_CAPS, true });
            sendVKeyEvent({ SC_CAPS, false });
        }
        break;
    }
    }

    IFDEBUG cout << endl << "(CapsicainCode: " << getPrettyVKLabelPadded(keyEvent.vcode,0) << ")";
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

    interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&iks, 1);
//    interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&loopState.originalIKstroke, 1);
}

void reset()
{
    resetAllStatesToDefault();

    releaseAllSentKeys();

    readSanitizeIniFile(sanitizedIniContent);
    parseIniGlobals();
    switchLayer(globalState.activeLayer);
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
