#include "pch.h"
#include <iostream>

//TODO 
//alt + cursor = 10
//config file

#include <string>
#include <Windows.h>  //for Sleep()

#include "interception.h"
#include "utils.h"

#include "scancodes.h"
#include "capsicain.h"

using namespace std;

string version = "13";

enum KEYSTATE
{
    KEYSTATE_DOWN = 0,
    KEYSTATE_UP = 1,
    KEYSTATE_EXT_DOWN = 2,
    KEYSTATE_EXT_UP = 3,
};

enum CREATE_CHARACTER_MODE
{
    IBM,   //alt + 123
    ANSI,  //alt + 0123
    AHK,   //F14|F15 + char, let AHK handle it
};

const int MAX_KEYMACRO_LENGTH = 10000;  //for testing; in real life, 100 keys = 200 up/downs should be enough

const int DEFAULT_DELAY_SENDMACRO = 5;  //local needs ~1ms, Linux VM 5+ms, RDP fullscreen 10+ for 100% reliable keystroke detection
const int DELAY_FOR_AHK = 100;

const unsigned short AHK_HOTKEY1 = SC_F14;  //this key triggers supporting AHK script
const unsigned short AHK_HOTKEY2 = SC_F15;

struct Mode
{
    bool debug;
    int  activeLayer;
    CREATE_CHARACTER_MODE characterCreationMode;
    unsigned int delayBetweenMacroKeysMS;  //AHK drops keys when they are sent too fast
    bool slashShift;
    bool flipZy;
    bool flipAltWin;
} mode;

struct GlobalState
{
    bool keysDownReceived[256] = { false };
    bool keysDownSent[256] = { false };
    InterceptionContext interceptionContext = NULL;
    InterceptionDevice interceptionDevice = NULL;
    string deviceIdKeyboard = "";
    bool deviceIsAppleKeyboard = false;
} globalState;

struct State
{
    unsigned short scancode;
    InterceptionKeyStroke stroke;
    InterceptionKeyStroke previousStroke;

    unsigned short modifiers;
    bool blockKey;  //true: do not send the current key
    bool isFinalScancode;  //true: don't remap the scancode anymore
    bool isDownstroke;
    bool isExtendedCode;
    bool isCapsDown;
    bool isCapsTapped;
    InterceptionKeyStroke keyMacro[MAX_KEYMACRO_LENGTH];
    int keyMacroLength;  // >0 triggers sending of macro instead of scancode. MUST match the actual # of chars in keyMacro
} state;

string errorLog = "";
void error(string txt)
{
    cout << endl << "ERROR: " << txt;
    errorLog += "\r\n" + txt;
}

void SetModeDefaults()
{
    mode.debug = false;
    mode.activeLayer = 1;
    mode.characterCreationMode = IBM;
    mode.delayBetweenMacroKeysMS = DEFAULT_DELAY_SENDMACRO;  //AHK drops keys when they are sent too fast
    mode.slashShift = true;
    mode.flipZy = true;
    mode.flipAltWin = true;
}

void SetGlobalStateDefaults()
{
    globalState.interceptionContext;
    globalState.interceptionDevice;
    globalState.deviceIdKeyboard = "";
    globalState.deviceIsAppleKeyboard = false;
}

void SetStateDefaults()
{
    state.previousStroke.code = 0;
    state.modifiers = 0;
    state.blockKey = false;  //true: do not send the current key
    state.isFinalScancode = false;  //true: don't remap the scancode anymore
    state.isDownstroke = false;
    state.isExtendedCode = false;
    state.keyMacroLength = 0;  // >0 triggers sending of macro instead of scancode. MUST match the actual # of chars in keyMacro
    state.isCapsDown = false;
    state.isCapsTapped = false;
}

void SetupConsoleWindow()
{
    //disable quick edit; blocking the console window means the keyboard is dead
    HANDLE Handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(Handle, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    mode &= ~ENABLE_MOUSE_INPUT;
    SetConsoleMode(Handle, mode);

    system("color 8E");  //byte1=background, byte2=text
    string title = ("Capsicain v" + version);
    SetConsoleTitle(wstring(title.begin(), title.end()).c_str());
}

void PrintHello()
{
    cout << "Capsicain v" << version << endl << endl
        << "[LCTRL] + [CAPS] + [ESC] to stop." << endl
        << "[LCTRL] + [CAPS] + [H] for Help";

    cout << endl << endl << (mode.slashShift ? "ON :" : "OFF:") << "Slashes -> Shift ";
    cout << endl << (mode.flipZy ? "ON :" : "OFF:") << "Z<->Y ";
    cout << endl << endl << "capsicain running..." << endl;
}

int main()
{
    SetModeDefaults();
    SetGlobalStateDefaults();
    SetStateDefaults();
    SetupConsoleWindow();

    Sleep(700); //time to release shortcut keys that started capsicain
    raise_process_priority(); //careful: if we spam key events, other processes get no timeslots to process them. Sleep a bit...

    globalState.interceptionContext = interception_create_context();
    interception_set_filter(globalState.interceptionContext, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);

    PrintHello();

    while (interception_receive(
        globalState.interceptionContext,
        globalState.interceptionDevice = interception_wait(globalState.interceptionContext),
        (InterceptionStroke *)&state.stroke, 1) > 0
        )
    {
        state.keyMacroLength = 0;
        state.blockKey = false;  //true: do not send the current key
        state.isFinalScancode = false;  //true: don't remap the scancode anymore
        state.isDownstroke = false;
        state.isExtendedCode = false;

        //check device ID
        if (globalState.deviceIdKeyboard.length() < 2)
        {
            cout << endl << "detecting keyboard...";
            getHardwareId();
            mode.flipAltWin = !globalState.deviceIsAppleKeyboard;
            cout << endl << (globalState.deviceIsAppleKeyboard ? "APPLE keyboard" : "IBM keyboard (flipping Win<>Alt)");
        }

        //evaluate and normalize the stroke
        state.isDownstroke = (state.stroke.state & 1) == 1 ? false : true;
        state.isExtendedCode = (state.stroke.state & 2) == 2;

        if (state.stroke.code > 0xFF)
        {
            error("Received unexpected stroke.code > 255: " + state.stroke.code);
            continue;
        }
        if (state.stroke.code & 0x80)
        {
            error("Received unexpected extended stroke.code > 0x79: " + state.stroke.code);
            continue;
        }

        state.scancode = state.stroke.code;  //scancode will be altered, stroke won't
        if (state.isExtendedCode)  //I track the extended bit in the high bit of the scancode. Assuming Interception never sends stroke.code > 0x7F
            state.scancode |= 0x80;

        globalState.keysDownReceived[state.scancode] = state.isDownstroke;

        //command stroke: RCTRL + CAPS + stroke
        if (state.isDownstroke
            && (state.scancode != SC_CAPS && state.scancode != SC_RCONTROL)
            && (globalState.keysDownReceived[SC_CAPS] && globalState.keysDownReceived[SC_RCONTROL])
            )
        {
            if (state.scancode == SC_ESCAPE)
            {
                break;  //break the main while() loop, exit
            }
            processCoreCommands();
            continue;
        }

        if (mode.activeLayer == 0)  //standard keyboard, except command strokes
        {
            interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&state.stroke, 1);
            continue;
        }

        IFDEBUG cout << endl << " [" << hex << state.stroke.code << "/" << hex << state.scancode << " " << state.stroke.information << " " << state.stroke.state << "]";

        //CapsLock action: track but never forward 
        if (state.scancode == SC_CAPS) {
            processCaps();
            continue;
        }

        //process the key stroke
        processRemapModifiers();
        processTrackModifierState();
        IFDEBUG cout << " [" << hex << state.modifiers << (state.isCapsDown ? " C" : " _") << (state.isCapsTapped ? "T" : "_") << "] ";

        if (!state.isFinalScancode)
            processLayoutIndependentAction();
        if (!state.isFinalScancode)
            processRemapCharacterLayout();
        if (!state.isFinalScancode)
            processLayoutDependentActions();

        sendResultingKeyOrMacro();
    }

    interception_destroy_context(globalState.interceptionContext);

    cout << endl << "bye" << endl;
    return 0;
}

void processCaps()
{
    if (state.isDownstroke) {
        state.isCapsDown = true;
    }
    else
    {
        state.isCapsDown = false;
        if (state.previousStroke.code == SC_CAPS)
        {
            state.isCapsTapped = !state.isCapsTapped;
            IFDEBUG cout << " capsTap:" << (state.isCapsTapped ? "ON " : "OFF");
        }
    }
    state.previousStroke = state.stroke;
}


void sendResultingKeyOrMacro()
{
    state.previousStroke = state.stroke;
    if (state.keyMacroLength > 0)
    {
        playMacro();
    }
    else
    {
        IFDEBUG
        {
            if (state.blockKey)
            cout << " BLOCKED ";
            else if (state.stroke.code != (state.scancode & 0x7F))
                cout << " -> " << hex << state.stroke.code << "/" << hex << state.scancode << " " << state.stroke.state;
        }
            if (!state.blockKey) {
                scancode2stroke(state.scancode, state.stroke);
                sendStroke(state.stroke);
            }
    }
}
void processRemapCharacterLayout()
{
    switch (state.scancode)
    {
    case SC_Z:
        if (mode.flipZy && !IS_LCONTROL_DOWN) //do not remap LCTRL+Z/Y (undo/redo)
            state.scancode = SC_Y;
        break;
    case SC_Y:
        if (mode.flipZy && !IS_LCONTROL_DOWN)
            state.scancode = SC_Z;
        break;
    }
}

void processLayoutIndependentAction()
{
    if (state.isCapsDown)
    {
        //those suppress the key UP because DOWN is replaced with macro
        bool blockingScancode = true;
        switch (state.scancode)
        {
            //Undo Redo Cut Copy Paste
        case SC_BACK:
            if (state.isDownstroke) {
                if (IS_SHIFT_DOWN)
                    createMacroKeyComboRemoveShift(SC_LCONTROL, SC_Y, 0, 0);
                else
                    createMacroKeyCombo(SC_LCONTROL, SC_Z, 0, 0);
            }
            break;
        case SC_A:
            if (state.isDownstroke)
                createMacroKeyCombo(SC_LCONTROL, SC_Z, 0, 0);
            break;
        case SC_S:
            if (state.isDownstroke)
                createMacroKeyCombo(SC_LCONTROL, SC_X, 0, 0);
            break;
        case SC_D:
            if (state.isDownstroke)
                createMacroKeyCombo(SC_LCONTROL, SC_C, 0, 0);
            break;
        case SC_F:
            if (state.isDownstroke)
                createMacroKeyCombo(SC_LCONTROL, SC_V, 0, 0);
            break;
        case SC_P:
            if (state.isDownstroke)
                createMacroKeyCombo(SC_NUMLOCK, 0, 0, 0);
            break;
        case SC_LBRACKET:
            if (state.isDownstroke)
                createMacroKeyCombo(SC_SCROLL, 0, 0, 0);
            break;
        case SC_RBRACKET:
            if (state.isDownstroke)
                createMacroKeyCombo(SC_SYSRQ, 0, 0, 0);
            break;
        case SC_EQUALS:
            if (state.isDownstroke)
                createMacroKeyCombo(SC_INSERT, 0, 0, 0);
            break;
        case SC_J:
            if (state.isDownstroke)
                createMacroKeyCombo10timesIfAltDown(SC_LEFT, 0, 0, 0, state.modifiers);
            break;
        case SC_L:
            if (state.isDownstroke)
                createMacroKeyCombo10timesIfAltDown(SC_RIGHT, 0, 0, 0, state.modifiers);
            break;
        case SC_K:
            if (state.isDownstroke)
                createMacroKeyCombo10timesIfAltDown(SC_DOWN, 0, 0, 0, state.modifiers);
            break;
        case SC_I:
            if (state.isDownstroke)
                createMacroKeyCombo10timesIfAltDown(SC_UP, 0, 0, 0, state.modifiers);
            break;
        case SC_O:
            if (state.isDownstroke)
                createMacroKeyCombo10timesIfAltDown(SC_PGUP, 0, 0, 0, state.modifiers);
            break;
        case SC_SEMICOLON:
            if (state.isDownstroke)
                createMacroKeyCombo(SC_DELETE, 0, 0, 0);
            break;
        case SC_DOT:
            if (state.isDownstroke)
                createMacroKeyCombo10timesIfAltDown(SC_PGDOWN, 0, 0, 0, state.modifiers);
            break;
        case SC_Y:
            if (state.isDownstroke)
                createMacroKeyCombo(SC_HOME, 0, 0, 0);
            break;
        case SC_U:
            if (state.isDownstroke)
                createMacroKeyCombo(SC_END, 0, 0, 0);
            break;
        case SC_N:
            if (state.isDownstroke)
                createMacroKeyCombo10timesIfAltDown(SC_LCONTROL, SC_LEFT, 0, 0, state.modifiers);
            break;
        case SC_M:
            if (state.isDownstroke)
                createMacroKeyCombo10timesIfAltDown(SC_LCONTROL, SC_RIGHT, 0, 0, state.modifiers);
            break;
        case SC_0:
        case SC_1:
        case SC_2:
        case SC_3:
        case SC_4:
        case SC_5:
        case SC_6:
        case SC_7:
        case SC_8:
        case SC_9:
            if (state.isDownstroke)
                createMacroKeyCombo(AHK_HOTKEY1, state.scancode, 0, 0);
            break;
        default:
            blockingScancode = false;
        }

        if (blockingScancode)
            state.blockKey = true;
        else  //direct key remapping without macros
        {
            switch (state.scancode)
            {
            case SC_H:
                state.scancode = SC_BACK;
                state.isFinalScancode = true;
                break;
            case SC_BACKSLASH:
                state.scancode = SC_SLASH;
                state.isFinalScancode = true;
                break;
            }
        }
    }
}

void processLayoutDependentActions()
{
    if (state.isCapsTapped) //so far, all caps tap actions are layout dependent (tap+A moves when A moves)
    {
        if (state.scancode != SC_LSHIFT && state.scancode != SC_RSHIFT)  //shift does not break the tapped status so I can type äÄ
        {
            if (state.isDownstroke)
                processCapsTapped();
            else
            {
                state.isCapsTapped = false;
                state.blockKey = true;
            }
        }
    }
}

void processTrackModifierState()
{
    switch (state.scancode)
    {
    case SC_LSHIFT:  //handle LShift+RShift -> CapsLock
        if (state.isDownstroke)
        {
            state.modifiers |= BITMASK_LSHIFT;
            if (IS_RSHIFT_DOWN && !state.isCapsDown && (GetKeyState(VK_CAPITAL) & 0x0001))
                createMacroKeyCombo(SC_CAPS, 0, 0, 0);
        }
        else
            state.modifiers &= ~BITMASK_LSHIFT;
        break;
    case SC_RSHIFT:
        if (state.isDownstroke)
        {
            state.modifiers |= BITMASK_RSHIFT;
            if (IS_LSHIFT_DOWN && !state.isCapsDown && !(GetKeyState(VK_CAPITAL) & 0x0001))
                createMacroKeyCombo(SC_CAPS, 0, 0, 0);
        }
        else
            state.modifiers &= ~BITMASK_RSHIFT;
        break;
    case SC_LALT:  //suppress LALT in CAPS+LALT combos
        if (state.isDownstroke)
        {
            if (IS_LALT_DOWN)
                state.blockKey = true;
            else if (state.isCapsDown)
                state.blockKey = true;
            state.modifiers |= BITMASK_LALT;
        }
        else
        {
            if (!(state.modifiers & BITMASK_LALT))
                state.blockKey = true;
            state.modifiers &= ~BITMASK_LALT;
        }
        break;
    case SC_RALT:
        state.isDownstroke ? state.modifiers |= BITMASK_RALT : state.modifiers &= ~BITMASK_RALT;
        break;
    case SC_LCONTROL:
        state.isDownstroke ? state.modifiers |= BITMASK_LCONTROL : state.modifiers &= ~BITMASK_LCONTROL;
        break;
    case SC_RCONTROL:
        state.isDownstroke ? state.modifiers |= BITMASK_RCONTROL : state.modifiers &= ~BITMASK_RCONTROL;
        break;
    case SC_LWIN:
        state.isDownstroke ? state.modifiers |= BITMASK_LWIN : state.modifiers &= ~BITMASK_LWIN;
        break;
    case SC_RWIN:
        state.isDownstroke ? state.modifiers |= BITMASK_RWIN : state.modifiers &= ~BITMASK_RWIN;
        break;
    }
}

void processRemapModifiers()
{
    state.isFinalScancode = true;
    switch (state.scancode)
    {
    case SC_LBSLASH:
        if (mode.slashShift)
            state.scancode = SC_LSHIFT;
        break;
    case SC_SLASH:
        if (mode.slashShift && !state.isCapsDown)
            state.scancode = SC_RSHIFT;
        break;
    case SC_LWIN:
        if (mode.flipAltWin)
            state.scancode = SC_LALT;
        break;
    case SC_RWIN:
        if (mode.flipAltWin)
            state.scancode = SC_RALT;
        break;
    case SC_LALT:
        if (mode.flipAltWin)
            state.scancode = SC_LWIN;
        break;
    case SC_RALT:
        if (mode.flipAltWin)
            state.scancode = SC_RWIN;
        break;
    default:
        state.isFinalScancode = false;
    }
}

void processCoreCommands()
{
    cout << endl << endl << "::";

    switch (state.scancode)
    {
    case SC_0:
        mode.activeLayer = 0;
        cout << "LAYER 0 active";
        break;
    case SC_1:
        mode.activeLayer = 1;
        cout << "LAYER 1 active";
        break;
    case SC_2:
        mode.activeLayer = 2;
        cout << "LAYER 2 active";
        break;
    case SC_R:
        reset();
        state.isCapsDown = false;
        state.isCapsTapped = false;
        getHardwareId();
        mode.flipAltWin = !globalState.deviceIsAppleKeyboard;
        cout << "RESET" << endl << (globalState.deviceIsAppleKeyboard ? "APPLE keyboard" : "PC keyboard (flipping Win<>Alt)");
        break;
    case SC_D:
        mode.debug = !mode.debug;
        cout << "DEBUG mode: " << (mode.debug ? "ON" : "OFF");
        break;
    case SC_SLASH:
        mode.slashShift = !mode.slashShift;
        cout << "Slash-Shift mode: " << (mode.slashShift ? "ON" : "OFF");
        break;
    case SC_Z:
        mode.flipZy = !mode.flipZy;
        cout << "Flip Z<>Y mode: " << (mode.flipZy ? "ON" : "OFF");
        break;
    case SC_W:
        mode.flipAltWin = !mode.flipAltWin;
        cout << "Flip ALT<>WIN mode: " << (mode.flipAltWin ? "ON" : "OFF") << endl;
        break;
    case SC_E:
        cout << "ERROR LOG: " << endl << errorLog << endl;
        break;
    case SC_S:
        printStatus();
        break;
    case SC_H:
        printHelp();
        break;
    case SC_C:
        cout << "Character creation mode: ";
        switch (mode.characterCreationMode)
        {
        case IBM:
            mode.characterCreationMode = ANSI;
            cout << "ANSI";
            break;
        case ANSI:
            mode.characterCreationMode = AHK;
            cout << "AHK";
            break;
        case AHK:
            mode.characterCreationMode = IBM;
            cout << "IBM";
            break;
        }
        cout << " (" << mode.characterCreationMode << ")";
        break;
    case SC_LBRACKET:
        if (mode.delayBetweenMacroKeysMS >= 2)
            mode.delayBetweenMacroKeysMS -= 1;
        cout << "delay between characters in macros (ms): " << dec << mode.delayBetweenMacroKeysMS;
        break;
    case SC_RBRACKET:
        if (mode.delayBetweenMacroKeysMS <= 100)
            mode.delayBetweenMacroKeysMS += 1;
        cout << "delay between characters in macros (ms): " << dec << mode.delayBetweenMacroKeysMS;
        break;
    default:
        cout << "Unknown command";
        break;
    }
}

void playMacro()
{
    unsigned int delay = mode.delayBetweenMacroKeysMS;
    IFDEBUG cout << " -> SEND MACRO (" << state.keyMacroLength << ")";
    for (int i = 0; i < state.keyMacroLength; i++)
    {
        normalizeKeyStroke(state.keyMacro[i]);
        IFDEBUG cout << " " << state.keyMacro[i].code << ":" << state.keyMacro[i].state;
        sendStroke(state.keyMacro[i]);
        if (state.keyMacro[i].code == AHK_HOTKEY1 || state.keyMacro[i].code == AHK_HOTKEY2)
            delay = DELAY_FOR_AHK;
        Sleep(delay);  //local sys needs ~1ms, RDP fullscreen ~3 ms delay so they can keep up with macro key action
    }
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
        globalState.deviceIsAppleKeyboard = (id.find("VID_05AC") != string::npos);

        IFDEBUG cout << endl << "getHardwareId:" << id << " / Apple keyboard: " << globalState.deviceIsAppleKeyboard;
    }
}

void printHelp()
{
    cout << "HELP" << endl << endl
        << "[LEFTCTRL] + [CAPS] + [{key}] for core commands" << endl
        << "[ESC] quit" << endl
        << "[S] Status" << endl
        << "[R] Reset" << endl
        << "[E] Error log" << endl
        << "[D] Debug mode output" << endl
        << "[0]..[9] switch layers" << endl
        << "[Z] (or Y on GER keyboard): flip Y<>Z keys" << endl
        << "[W] flip ALT <> WIN" << endl
        << "[/] (is [-] on GER keyboard): Slash -> Shift" << endl
        << "[C] switch character creation mode (Alt+Numpad or AHK)" << endl
        << "[ and ]: pause between macro keys sent -/+ 10ms " << endl
        ;
}
void printStatus()
{
    int numMakeReceived = 0;
    int numMakeSent = 0;
    for (int i = 0; i < 255; i++)
    {
        if (globalState.keysDownReceived[i])
            numMakeReceived++;
        if (globalState.keysDownSent[i])
            numMakeSent++;
    }
    cout << "STATUS" << endl << endl
        << "Capsicain version: " << version << endl
        << "hardware id:" << globalState.deviceIdKeyboard << endl
        << "Apple keyboard: " << globalState.deviceIsAppleKeyboard << endl
        << "current LAYER is: " << mode.activeLayer << endl
        << "modifier state: " << hex << state.modifiers << endl
        << "macro key delay: " << mode.delayBetweenMacroKeysMS << endl
        << "character create mode: " << mode.characterCreationMode << endl
        << "# keys down received: " << numMakeReceived << endl
        << "# keys down sent: " << numMakeSent << endl
        << (errorLog.length() > 1 ? "ERROR LOG contains entries" : "clean error log") << " (" << errorLog.length() << " chars)" << endl
        ;
}

void normalizeKeyStroke(InterceptionKeyStroke &stroke) {
    if (stroke.code > 0x7F) {
        stroke.code &= 0x7F;
        stroke.state |= 2;
    }
}

void scancode2stroke(unsigned short &scancode, InterceptionKeyStroke &istroke)
{
    if (scancode >= 0x80)
    {
        istroke.code = static_cast<unsigned short>(scancode & 0x7F);
        istroke.state |= 2;
    }
    else
    {
        istroke.code = scancode;
        istroke.state &= 0xFD;
    }
}

void sendStroke(InterceptionKeyStroke stroke)
{
    if (stroke.code > 0xFF)
        error("Unexpected scancode > 255: " + stroke.code);
    else
        globalState.keysDownSent[(unsigned char)stroke.code] = (stroke.state & 1) ? false : true;

    interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&stroke, 1);
}

void reset()
{
    for (int i = 0; i < 255; i++)
    {
        globalState.keysDownReceived[i] = 0;
        globalState.keysDownSent[i] = 0;
    }

    state.modifiers = 0;
    mode.delayBetweenMacroKeysMS = DEFAULT_DELAY_SENDMACRO;

    IFDEBUG cout << endl << "Resetting all modifiers to UP" << endl;
    state.keyMacroLength = 0;

    breakKeyMacro(SC_LSHIFT);
    breakKeyMacro(SC_RSHIFT);
    breakKeyMacro(SC_LCONTROL);
    breakKeyMacro(SC_RCONTROL);
    breakKeyMacro(SC_LALT);
    breakKeyMacro(SC_RALT);
    breakKeyMacro(SC_LWIN);
    breakKeyMacro(SC_RWIN);
    breakKeyMacro(SC_CAPS);
    breakKeyMacro(AHK_HOTKEY1);
    breakKeyMacro(AHK_HOTKEY2);

    playMacro();
}


void makeKeyMacro(unsigned short scancode)
{
    state.keyMacro[state.keyMacroLength].code = scancode;
    state.keyMacro[state.keyMacroLength].state = KEYSTATE_DOWN;
    state.keyMacroLength++;
}
void breakKeyMacro(unsigned short scancode)
{
    state.keyMacro[state.keyMacroLength].code = scancode;
    state.keyMacro[state.keyMacroLength].state = KEYSTATE_UP;
    state.keyMacroLength++;
}
void makeBreakKeyMacro(unsigned short scancode)
{
    state.keyMacro[state.keyMacroLength].code = scancode;
    state.keyMacro[state.keyMacroLength].state = KEYSTATE_DOWN;
    state.keyMacroLength++;
    state.keyMacro[state.keyMacroLength].code = scancode;
    state.keyMacro[state.keyMacroLength].state = KEYSTATE_UP;
    state.keyMacroLength++;
}


//Macro wrapped in lshift off temporarily. Example: [shift+backspace] -> ctrl+Y  must suppress the shift, because shift+ctrl+Y is garbage
void createMacroKeyComboRemoveShift(int a, int b, int c, int d)
{
    if (IS_LSHIFT_DOWN)
        breakKeyMacro(SC_LSHIFT);
    createMacroKeyCombo(a, b, c, d);
    if (IS_LSHIFT_DOWN)
        makeKeyMacro(SC_LSHIFT);
}

//press all scancodes, then release them. Pass a,b,0,0 if you need less than 4
void createMacroKeyCombo(int a, int b, int c, int d)
{
    createMacroKeyComboNtimes(a, b, c, d, 1);
}
void createMacroKeyCombo10timesIfAltDown(int a, int b, int c, int d, unsigned short modifiers)
{
    if (modifiers & BITMASK_LALT || modifiers & BITMASK_RALT)
        createMacroKeyComboNtimes(a, b, c, d, 10);
    else
        createMacroKeyComboNtimes(a, b, c, d, 1);
}
void createMacroKeyComboNtimes(int a, int b, int c, int d, int repeat)
{
    unsigned short scancodes[] = { (unsigned short)a, (unsigned short)b, (unsigned short)c, (unsigned short)d };
    for (int rep = 0; rep < repeat; rep++)
    {
        for (int i = 0; i < 4; i++)
        {
            if (scancodes[i] == 0)
                break;
            if (scancodes[i] == SC_LCONTROL && IS_LCONTROL_DOWN)
                continue;
            if (scancodes[i] == SC_LSHIFT && IS_LSHIFT_DOWN)
                continue;
            makeKeyMacro(scancodes[i]);
        }

        for (int i = 3; i >= 0; i--)
        {
            if (scancodes[i] == 0)
                continue;
            if (scancodes[i] == SC_LCONTROL && IS_LCONTROL_DOWN)
                continue;
            if (scancodes[i] == SC_LSHIFT && IS_LSHIFT_DOWN)
                continue;
            breakKeyMacro(scancodes[i]);
        }
    }
}

//virtually push Alt+Numpad 0 1 2 4  for special characters
//pass a b c 0 for 3-digit combos
void createMacroAltNumpad(unsigned short a, unsigned short b, unsigned short c, unsigned short d)
{
    unsigned short fsc[] = { a, b, c, d };

    bool lshift = (state.modifiers & BITMASK_LSHIFT) > 0;
    if (lshift)
        breakKeyMacro(SC_LSHIFT);
    bool rshift = (state.modifiers & BITMASK_RSHIFT) > 0;
    if (rshift)
        breakKeyMacro(SC_RSHIFT);
    makeKeyMacro(SC_LALT);

    for (int i = 0; i < 4 && fsc[i] != 0; i++)
        makeBreakKeyMacro(fsc[i]);

    breakKeyMacro(SC_LALT);
    if (rshift)
        makeKeyMacro(SC_RSHIFT);
    if (lshift)
        makeKeyMacro(SC_LSHIFT);
}


/* Sending special character with scancodes is no fun.
   There are 3 createCharactermodes:
   1. IBM classic: ALT + NUMPAD 3-digits. Supported by some apps.
   2. ANSI: ALT + NUMPAD 4-digits. Supported by other apps, many Microsoft apps but not all.
   3. AHK: Sends F14|F15 + 1 base character. Requires an AHK script that translates these comobos to characters.
   This is Windows only. For Linux, the combo is [Ctrl]+[Shift]+[U] , {unicode E4 is ö} , [Enter]
*/
void processCapsTapped()
{
    bool shiftXorCaps = IS_SHIFT_DOWN != ((GetKeyState(VK_CAPITAL) & 0x0001) != 0);

    if (mode.characterCreationMode == IBM)
    {
        switch (state.scancode)
        {
        case SC_O:
            if (shiftXorCaps)
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD5, SC_NUMPAD3, 0);
            else
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD4, SC_NUMPAD8, 0);
            break;
        case SC_A:
            if (shiftXorCaps)
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD4, SC_NUMPAD2, 0);
            else
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD3, SC_NUMPAD2, 0);
            break;
        case SC_U:
            if (shiftXorCaps)
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD5, SC_NUMPAD4, 0);
            else
                createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD2, SC_NUMPAD9, 0);
            break;
        case SC_S:
            createMacroAltNumpad(SC_NUMPAD2, SC_NUMPAD2, SC_NUMPAD5, 0);
            break;
        case SC_E:
            createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD1, SC_NUMPAD2, SC_NUMPAD8);
            break;
        case SC_D:
            createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD6, SC_NUMPAD7, 0);
            break;
        case SC_T: // test print from [1] to [Enter]
        {
            for (unsigned short i = 2; i <= 0x1C; i++)
                makeBreakKeyMacro(i);
            break;
        }
        case SC_R: // test2: print 50x10 ä
        {
            for (int outer = 0; outer < 4; outer++)
            {
                for (unsigned short i = 0; i < 40; i++)
                {

                    makeKeyMacro(SC_LALT);
                    makeBreakKeyMacro(SC_NUMPAD1);
                    makeBreakKeyMacro(SC_NUMPAD3);
                    makeBreakKeyMacro(SC_NUMPAD2);
                    breakKeyMacro(SC_LALT);
                }

                makeBreakKeyMacro(SC_RETURN);
            }
            break;
        }
        case SC_L: // Linux test: print 50x10 ä
        {
            for (int outer = 0; outer < 4; outer++)
            {
                for (unsigned short i = 0; i < 40; i++)
                {

                    makeKeyMacro(SC_LCONTROL);
                    makeKeyMacro(SC_LSHIFT);
                    makeKeyMacro(SC_U);
                    breakKeyMacro(SC_U);
                    breakKeyMacro(SC_LSHIFT);
                    breakKeyMacro(SC_LCONTROL);
                    makeBreakKeyMacro(SC_E);
                    makeBreakKeyMacro(SC_4);
                    makeBreakKeyMacro(SC_RETURN);
                }
                makeBreakKeyMacro(SC_RETURN);
            }
            break;
        }
        }
    }
    else if (mode.characterCreationMode == ANSI)
    {
        switch (state.scancode)
        {
        case SC_O:
            if (shiftXorCaps)
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD1, SC_NUMPAD4);
            else
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD4, SC_NUMPAD6);
            break;
        case SC_A:
            if (shiftXorCaps)
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD1, SC_NUMPAD9, SC_NUMPAD6);
            else
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD2, SC_NUMPAD8);
            break;
        case SC_U:
            if (shiftXorCaps)
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD2, SC_NUMPAD0);
            else
                createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD5, SC_NUMPAD2);
            break;
        case SC_S:
            createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD2, SC_NUMPAD2, SC_NUMPAD3);
            break;
        case SC_E:
            createMacroAltNumpad(SC_NUMPAD0, SC_NUMPAD1, SC_NUMPAD2, SC_NUMPAD8);
            break;
        case SC_D:
            createMacroAltNumpad(SC_NUMPAD1, SC_NUMPAD7, SC_NUMPAD6, 0);
            break;
        }
    }
    else if (mode.characterCreationMode == AHK)
    {
        switch (state.scancode)
        {
        case SC_A:
        case SC_O:
        case SC_U:
            createMacroKeyCombo(shiftXorCaps ? AHK_HOTKEY2 : AHK_HOTKEY1, state.scancode, 0, 0);
            break;
        case SC_S:
        case SC_E:
        case SC_D:
        case SC_C:
            createMacroKeyCombo(AHK_HOTKEY1, state.scancode, 0, 0);
            break;
        }
    }

    if (state.keyMacroLength == 0)
    {
        switch (state.scancode) {
        case SC_0:
        case SC_1:
        case SC_2:
        case SC_3:
        case SC_4:
        case SC_5:
        case SC_6:
        case SC_7:
        case SC_8:
        case SC_9:
            createMacroKeyCombo(AHK_HOTKEY2, state.scancode, 0, 0);
            break;
        }
    }
}

