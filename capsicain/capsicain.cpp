#pragma once 

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

string version = "38";

const bool DEFAULT_START_AHK_ON_STARTUP = true;
const int DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS = 5;  //local needs ~1ms, Linux VM 5+ms, RDP fullscreen 10+ for 100% reliable keystroke detection
const int DEFAULT_ACTIVE_LAYER = 0; 
const string DEFAULT_ACTIVE_LAYER_NAME = "(no processing, forward everything)";
const int DEFAULT_DELAY_FOR_AHK = 50;
const unsigned short AHK_HOTKEY1 = SC_F14;  //this key triggers supporting AHK script
const unsigned short AHK_HOTKEY2 = SC_F15;

string scLabels[256]; // contains [01]="ESCAPE" instead of SC_ESCAPE 

struct ModifierCombo
{
	unsigned short key = SC_NOP;
	unsigned short modAnd = 0;
	unsigned short modNot = 0;
	unsigned short modTap = 0;
	//unsigned short modNop = 0; //block these modifiers
	//unsigned short modFor = 0; //forward these modifiers
	vector<Stroke> strokeSequence;
};
vector<ModifierCombo> modCombos;
vector<ModifierCombo> modCombosPost;

struct Feature
{
	string iniVersion = "unnamed version - add 'iniVersion xyz' to capsicain.ini";
	bool debug = false;
	int delayForKeySequenceMS = DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;  //AHK drops keys when they are sent too fast
	bool flipZy = false;
	bool altAltToAlt = false;
	bool shiftShiftToShiftLock = false;
	bool flipAltWinOnAppleKeyboards = false;
	bool lControlBlocksAlphaMapping = false;
	bool processOnlyFirstKeyboard = false;
} feature;

struct KeyModifierIftappedMapping
{
	unsigned char inkey = SC_NOP;
	unsigned char outkey = SC_NOP;
	unsigned char ifTapped = SC_NOP;
};
vector<KeyModifierIftappedMapping> keyModifierIftappedMapping;

struct AlphaMapping
{
	string layerName = "undefined_layerName";
	unsigned char alphamap[256] = { SC_NOP };
} alphaMapping;

struct GlobalState
{
	int  activeLayer = 0;

	unsigned short modifiers = 0;
	unsigned short modsTapped = 0;
	bool lastModBrokeTapping = false;
	bool escapeIsDown = false;

	InterceptionContext interceptionContext = NULL;
	InterceptionDevice interceptionDevice = NULL;
	InterceptionDevice interceptionDevicePreviousKey = NULL;

	string deviceIdKeyboard = "";
	bool deviceIsAppleKeyboard = false;

	vector<Stroke> modsTempAltered;

	Stroke previousStrokeIn = { SC_NOP, 0 };
	Stroke previousStrokeOut = { SC_NOP, 0 };
	bool keysDownSent[256] = { false };
} globalState;

struct LoopState
{
	unsigned short scancode = 0;
	bool isDownstroke = false;
	bool isModifier = false;
	bool wasTapped = false;

	InterceptionKeyStroke originalIKstroke = { SC_NOP, 0 };
	Stroke originalStroke = { SC_NOP, false };

	bool blockKey = false;  //true: do not send the current key
	vector<Stroke> resultingStrokeSequence;
} loopState;

string errorLog = "";
void error(string txt)
{
	cout << endl << "ERROR: " << txt;
	errorLog += "\r\n" + txt;
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
	string title = ("Capsicain v" + version);
	SetConsoleTitle(title.c_str());
}

bool readIniFeatures()
{
	vector<string> iniLines; //sanitized content of the .ini file
	if (!parseConfig(iniLines))
	{
		return false;
	}
	configReadString("DEFAULTS", "iniVersion", feature.iniVersion, iniLines);
	feature.debug = configHasKey("DEFAULTS", "debug", iniLines);
	if (!configReadInt("DEFAULTS", "activeLayer", globalState.activeLayer, iniLines))
	{
		globalState.activeLayer = DEFAULT_ACTIVE_LAYER;
		cout << endl << "Missing ini setting 'activeLayer'. Setting default layer " << DEFAULT_ACTIVE_LAYER;
	}
	if (!configReadInt("DEFAULTS", "delayForKeySequenceMS", feature.delayForKeySequenceMS, iniLines))
	{
		feature.delayForKeySequenceMS = DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;
		cout << endl << "Missing ini setting 'delayForKeySequenceMS'. Using default " << DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;
	}
	feature.flipZy = configHasKey("FEATURES", "flipZy", iniLines);
	feature.shiftShiftToShiftLock = configHasKey("FEATURES", "shiftShiftToShiftLock", iniLines);
	feature.altAltToAlt = configHasKey("FEATURES", "altAltToAlt", iniLines);
	feature.flipAltWinOnAppleKeyboards = configHasKey("FEATURES", "flipAltWinOnAppleKeyboards", iniLines);
	feature.lControlBlocksAlphaMapping = configHasKey("FEATURES", "LControlBlocksAlphaMapping", iniLines);
	feature.processOnlyFirstKeyboard = configHasKey("FEATURES", "processOnlyFirstKeyboard", iniLines);
	return true;
}

bool readIniAlphaMappingLayer(int layer)
{
	vector<string> iniLines; //sanitized content of the .ini file
	if (!parseConfigSection("LAYER" + std::to_string(layer), iniLines))
	{
		IFDEBUG cout << endl << "No mapping defined for layer " << layer << endl;
		return false;
	}

	resetAlphaMap();
	string name = "layerName_undefined";

	unsigned char keyIn, keyOut;
	for (string line : iniLines)
	{
		if (stringGetFirstToken(line) == "layername")
		{
			name = stringGetLastToken(line);
			if (name == "")
				name = "(bad layerName, check your config)";
			continue;
		}
		else if (!parseSimpleMapping(line, keyIn, keyOut, scLabels))
		{
			error("[LAYER"+to_string(layer)+"] Cannot parse simple alpha mapping: " + line);
			continue;
		}
		alphaMapping.alphamap[keyIn] = keyOut;
	}
	alphaMapping.layerName = name;
	return true;
}

bool readIniModCombos()
{
	vector<string> iniLines; //sanitized content of the .ini file
	if (!parseConfigSection("MODIFIER_COMBOS", iniLines))
	{
		IFDEBUG cout << endl << "No mapping defined for modifier combos." << endl;
		return true;
	}
	modCombos.clear();
	unsigned short mods[3] = { 0 }; //and, not, tap (nop, for)
	vector<Stroke> strokeSequence;

	for (string line : iniLines)
	{
		unsigned short key;
		if (parseModCombo(line, key, mods, strokeSequence, scLabels))
		{
			IFDEBUG cout << endl << "modCombo: " << line << endl << "    ," << key << " -> "
				<< mods[0] << "," << mods[1] << "," << mods[2] << "," << "sequence:" << strokeSequence.size();
			modCombos.push_back({ key, mods[0], mods[1], mods[2], strokeSequence });
		}
		else
		{
			cout << endl << "Error in .ini: cannot parse: " << line;
			return false;
		}
	}
	return true;
}

bool readIniKeyModifierIftappedMapping()
{
	string sectionLabel = "KEY_MODIFIER_IFTAPPED_MAPPING";
	vector<string> iniLines; //sanitized content of the .ini file
	if (!parseConfigSection(sectionLabel, iniLines))
	{
		IFDEBUG cout << endl << "No modifier-key-iftapped mappings defined" << endl;
		return true;
	}

	keyModifierIftappedMapping.clear();
	unsigned char keyIn, keyOut, keyIftapped;
	for (string line : iniLines)
	{
		if (!parseThreeTokenMapping(line, keyIn, keyOut, keyIftapped, scLabels))
		{
			error("["+ sectionLabel +"] Cannot parse three token mapping: " + line);
			continue;
		}
		keyModifierIftappedMapping.push_back({ keyIn, keyOut, keyIftapped});
	}
	return true;
}

bool readIni()
{
	if (!readIniFeatures())
		return false;

	vector<KeyModifierIftappedMapping> oldKeyModIftappedMapping(keyModifierIftappedMapping);
	if (!readIniKeyModifierIftappedMapping())
	{
		keyModifierIftappedMapping = oldKeyModIftappedMapping;
		cout << endl << "Cannot read key-modifier-iftapped mapping. Keeping old mapping." << endl;
	}

	vector<ModifierCombo> oldModCombos(modCombos);
	if (!readIniModCombos())
	{
		modCombos = oldModCombos;
		cout << endl << "Cannot read modifier combos. Keeping old combos." << endl;
	}

	unsigned char oldAlphamap[256];
	std::copy(alphaMapping.alphamap, alphaMapping.alphamap + 256, oldAlphamap);
	if (!readIniAlphaMappingLayer(globalState.activeLayer))
	{
		std::copy(oldAlphamap, oldAlphamap + 256, alphaMapping.alphamap);
		cout << endl << "Alpha mapping layer " << globalState.activeLayer << " is not defined. Setting Layer " << DEFAULT_ACTIVE_LAYER_NAME << endl;
		globalState.activeLayer = DEFAULT_ACTIVE_LAYER;
		alphaMapping.layerName = DEFAULT_ACTIVE_LAYER_NAME;
	}
	return true;
}

void resetCapsNumScrollLock()
{
	//set NumLock, release CapsLock+Scrolllock
	vector<Stroke> sequence;
	if (!(GetKeyState(VK_NUMLOCK) & 0x0001))
		keySequenceAppendMakeBreakKey(SC_NUMLOCK, sequence);
	if (GetKeyState(VK_CAPITAL) & 0x0001)
		keySequenceAppendMakeBreakKey(SC_CAPS, sequence);
	if (GetKeyState(VK_SCROLL) & 0x0001)
		keySequenceAppendMakeBreakKey(SC_SCRLOCK, sequence);
	playStrokeSequence(sequence);
}

void resetAlphaMap()
{
	for (int i = 0; i < 256; i++)
		alphaMapping.alphamap[i] = i;
}

void resetLoopState()
{
	loopState.blockKey = false;
	loopState.isDownstroke = false;
	loopState.scancode = 0;
	loopState.isModifier = false;
	loopState.wasTapped = false;

	InterceptionKeyStroke originalIKstroke = { SC_NOP, false };
	Stroke originalStroke = { SC_NOP, false };
	loopState.resultingStrokeSequence.clear();
}

void resetAllStatesToDefault()
{
	//	globalState.interceptionDevice = NULL;
	globalState.deviceIdKeyboard = "";
	globalState.deviceIsAppleKeyboard = false;
	globalState.previousStrokeOut = { 0,0 };
	globalState.modifiers = 0;
	globalState.modsTapped = 0;
	globalState.lastModBrokeTapping = false;

	feature.delayForKeySequenceMS = DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;

	resetAlphaMap();
	resetCapsNumScrollLock();
	resetLoopState();
}

int main()
{
	initConsoleWindow();
	printHelloHeader();
	initScancodeLabels(scLabels);
	resetAllStatesToDefault();

	if (!readIni())
	{
		cout << endl << "No capsicain.ini - exiting..." << endl;
		Sleep(5000);
		return 0;
	}

	cout << endl << "Release all keys now...";
	Sleep(1000); //time to release shortcut keys that started capsicain

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
		//ignore secondary keyboard?
		if (feature.processOnlyFirstKeyboard 
			&& (globalState.interceptionDevicePreviousKey != NULL)
			&& (globalState.interceptionDevicePreviousKey != globalState.interceptionDevice))
		{
			IFDEBUG cout << endl << "Ignore 2nd board (" << globalState.interceptionDevice << ") scancode: " << loopState.originalIKstroke.code;
			interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&loopState.originalIKstroke, 1);
			continue;
		}

		resetLoopState();

		//check device ID
 		if (globalState.interceptionDevicePreviousKey == NULL	//startup
			|| globalState.interceptionDevicePreviousKey != globalState.interceptionDevice)  //keyboard changed
		{
			getHardwareId();
			cout << endl << "new keyboard: " << (globalState.deviceIsAppleKeyboard ? "Apple keyboard" : "IBM keyboard");
			resetCapsNumScrollLock();
			globalState.interceptionDevicePreviousKey = globalState.interceptionDevice;
		}
		
		//copy InterceptionKeyStroke (unpleasant to use) to plain Stroke
		loopState.originalStroke = ikstroke2stroke(loopState.originalIKstroke);
		loopState.scancode = loopState.originalStroke.scancode;
		loopState.isDownstroke = loopState.originalStroke.isDownstroke;
		loopState.wasTapped = !loopState.isDownstroke 
			&& (loopState.originalStroke.scancode == globalState.previousStrokeIn.scancode);

		if (loopState.originalIKstroke.code >= 0x80)
		{
			error("Received unexpected extended Interception Key Stroke code > 0x79: " + to_string(loopState.originalIKstroke.code));
			continue;
		}

		//command stroke: ESC + stroke
		// some major key shadowing here...
		// - cherry is good
		// - apple keyboard cannot do RCTRL+CAPS+ESC and ESC+Caps shadows the entire row a-s-d-f-g-....
		// - Dell cant do ctrl-caps-x
		// - Cypher has no RControl... :(
		// - HP shadows the 2-w-s-x and 3-e-d-c lines
		if (loopState.scancode == SC_ESCAPE)
		{
			globalState.escapeIsDown = loopState.isDownstroke;

			//ESC alone will send ESC; otherwise block
			if (!loopState.isDownstroke && globalState.previousStrokeIn.scancode == SC_ESCAPE)
			{
				IFDEBUG cout << " ESC ";
				sendStroke({ SC_ESCAPE, true });
				sendStroke({ SC_ESCAPE, false });
			}
			globalState.previousStrokeIn = loopState.originalStroke;
			globalState.previousStrokeOut = { SC_ESCAPE, false };
			continue;
		}
		else if(globalState.escapeIsDown)
		{
			if (processCommandKeys())
				continue;
			else
				break;
		}

		if (globalState.activeLayer == 0)  //standard keyboard, just forward everything except command strokes
		{
			interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&loopState.originalIKstroke, 1);
			continue;
		}

		/////CONFIGURED RULES//////
		IFDEBUG cout << endl << " [" << scLabels[loopState.scancode] << getSymbolForStrokeState(loopState.originalIKstroke.state)
			<< " =" << hex << loopState.originalIKstroke.code << " " << loopState.originalIKstroke.state << "]";

		processKeyToModifierMapping();
		processModifierState();
	
		IFDEBUG cout << " [M:" << hex << globalState.modifiers;
		IFDEBUG if (globalState.modsTapped)  cout << " TAP:" << hex << globalState.modsTapped;
		IFDEBUG cout << "] ";

		//evaluate modified keys
		if (!loopState.isModifier && (globalState.modifiers > 0 || globalState.modsTapped > 0))
		{
			processModifiedKeys();
		}

		//basic character key layout. Don't remap the Ctrl combos?
		if (!loopState.isModifier && 
			!(IS_LCTRL_DOWN && feature.lControlBlocksAlphaMapping)) 
		{
			processMapAlphaKeys(loopState.scancode);
			if (feature.flipZy)
			{
				switch (loopState.scancode)
				{
				case SC_Y:		loopState.scancode = SC_Z;		break;
				case SC_Z:		loopState.scancode = SC_Y;		break;
				}
			}
		}

		sendResultingKeyOrSequence();
		globalState.previousStrokeOut = { loopState.scancode, loopState.isDownstroke };
		globalState.previousStrokeIn = loopState.originalStroke;
	}
	interception_destroy_context(globalState.interceptionContext);

	cout << endl << "bye" << endl;
	return 0;
}
////////////////////////////////////END MAIN//////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////


//Escaped QX -> return false -> Quit, or process command. 
bool processCommandKeys()
{
	if (loopState.isDownstroke)
	{
		if (loopState.scancode == SC_X)
		{
			return false;  //break the main while() loop, exit
		}
		else if (loopState.scancode == SC_Q) //stop the debug client while the release build keeps running
		{
#ifndef NDEBUG
			return false;
#else
			sendStroke({ SC_ESCAPE, true });
			sendStroke({ SC_X, true });
			sendStroke({ SC_X, false });
			sendStroke({ SC_ESCAPE, false });
			return true;
#endif
		}
		else
		{
			processCommand();
			globalState.previousStrokeIn = loopState.originalStroke;
			return true;
		}
	}
	return true;
}
void processModifiedKeys()
{
	for (ModifierCombo modcombo : modCombos)
	{
		if (modcombo.key == loopState.originalStroke.scancode)
		{
			if (
				(globalState.modifiers & modcombo.modAnd) == modcombo.modAnd &&
				(globalState.modifiers & modcombo.modNot) == 0 &&
				((globalState.modsTapped & modcombo.modTap) == modcombo.modTap)
				)
			{
				if (loopState.isDownstroke)
					loopState.resultingStrokeSequence = modcombo.strokeSequence;
				break;
			}
		}
	}
	globalState.modsTapped = 0;
}
void processMapAlphaKeys(unsigned short &scancode)
{
	if (scancode > 0xFF)
	{
		error("Unexpected scancode > 255 while mapping alphachars: " + std::to_string(scancode));
	}
	else
	{
		scancode = alphaMapping.alphamap[scancode];
	}
}

//[ESC]+X combos
void processCommand()
{
	cout << endl << endl << "::";

	switch (loopState.scancode)
	{
	case SC_0:
		globalState.activeLayer = DEFAULT_ACTIVE_LAYER;
		alphaMapping.layerName = DEFAULT_ACTIVE_LAYER_NAME;
		cout << "DEFAULT LAYER: " << DEFAULT_ACTIVE_LAYER_NAME;
		break;
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
		int layer = loopState.scancode - 1;
		if (readIniAlphaMappingLayer(layer))
		{
			globalState.activeLayer = layer;
			cout << "LAYER CHANGE: " << layer << " = " << alphaMapping.layerName;
		}
		else
			cout << "LAYER CHANGE: LAYER " << layer << " IS NOT DEFINED. Layer " << globalState.activeLayer << " remains active.";
		break;
	}
	case SC_R:
		cout << "RESET";
		reset();
		getHardwareId();
		cout << endl << (globalState.deviceIsAppleKeyboard ? "APPLE keyboard (flipping Win<>Alt)" : "PC keyboard");
		break;
	case SC_D:
		feature.debug = !feature.debug;
		cout << "DEBUG mode: " << (feature.debug ? "ON" : "OFF");
		break;
	case SC_Z:
		feature.flipZy = !feature.flipZy;
		cout << "Flip Z<>Y mode: " << (feature.flipZy ? "ON" : "OFF");
		break;
	case SC_W:
		feature.flipAltWinOnAppleKeyboards = !feature.flipAltWinOnAppleKeyboards;
		cout << "Flip ALT<>WIN for Apple boards: " << (feature.flipAltWinOnAppleKeyboards ? "ON" : "OFF") << endl;
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
	case SC_LBRACK:
		if (feature.delayForKeySequenceMS >= 2)
			feature.delayForKeySequenceMS -= 1;
		cout << "delay between characters in key sequences (ms): " << dec << feature.delayForKeySequenceMS;
		break;
	case SC_RBRACK:
		if (feature.delayForKeySequenceMS <= 100)
			feature.delayForKeySequenceMS += 1;
		cout << "delay between characters in key sequences (ms): " << dec << feature.delayForKeySequenceMS;
		break;
	case SC_A:
	{
		cout << "Start AHK";
		string msg = startProgramSameFolder("autohotkey.exe");
		if (msg != "")
			cout << endl << "Cannot start: " << msg;
		break;
	}
	case SC_K:
		cout << "Stop AHK";
		closeOrKillProgram("autohotkey.exe");
		break;
	default:
		cout << "Unknown command";
		break;
	}
}


void sendResultingKeyOrSequence()
{
	if (loopState.resultingStrokeSequence.size() > 0)
	{
		playStrokeSequence(loopState.resultingStrokeSequence);
	}
	else
	{
		IFDEBUG
		{
			if (loopState.blockKey)
				cout << "\t--> BLOCKED ";
			else if (loopState.originalStroke.scancode != loopState.scancode)
				cout << "\t--> " << scLabels[loopState.scancode] << " " << getSymbolForStrokeState(loopState.originalIKstroke.state);
			else
				cout << "\t--";
		}
		if (!loopState.blockKey && !loopState.scancode == SC_NOP)
		{
			sendStroke({ loopState.scancode, loopState.isDownstroke });
		}
	}
}

void processModifierState()
{
	if (!loopState.isModifier)
		return; 

	unsigned short bitmask = getBitmaskForModifier(loopState.scancode);
	if (loopState.isDownstroke)
		globalState.modifiers |= bitmask;
	else 
		globalState.modifiers &= ~bitmask;

	if ((bitmask & 0xFF00) > 0)
		loopState.blockKey = true;

	bool tappedModConfigFound = false;
	//configured tapped mod?
	if (loopState.wasTapped)
	{
		for (KeyModifierIftappedMapping map : keyModifierIftappedMapping)
		{
			if (loopState.originalStroke.scancode == map.inkey)
			{
				if (map.ifTapped != SC_NOP)
				{
					if (!loopState.blockKey)
						loopState.resultingStrokeSequence.push_back({ loopState.scancode, false });
					loopState.resultingStrokeSequence.push_back({ map.ifTapped, true });
					loopState.resultingStrokeSequence.push_back({ map.ifTapped, false });
					loopState.blockKey = false;
					tappedModConfigFound = true;
					globalState.modsTapped = 0;
				}
				break;
			}
		}
	}

	//Set tapped bitmask. You can combine mod-taps (like tap-Ctrl then tap-Alt).
	//Double-tap clears all taps
	if (!tappedModConfigFound)
	{
		bool sameKey = loopState.originalStroke.scancode == globalState.previousStrokeIn.scancode;
		if (loopState.wasTapped && globalState.lastModBrokeTapping)
			globalState.modsTapped = 0;
		else if (loopState.wasTapped)
			globalState.modsTapped |= bitmask;
		else if (sameKey && loopState.isDownstroke)
		{
			if (globalState.modsTapped & bitmask)
			{
				globalState.lastModBrokeTapping = true;
				globalState.modsTapped &= ~bitmask;
			}
			else
				globalState.lastModBrokeTapping = false;
		}
		else
			globalState.lastModBrokeTapping = false;
	}
}

void processKeyToModifierMapping()
{
	bool stopProcessing = false;

	if (feature.flipAltWinOnAppleKeyboards && globalState.deviceIsAppleKeyboard)
	{
		switch (loopState.scancode)
		{
		case SC_LALT: loopState.scancode = SC_LWIN; break;
		case SC_LWIN: loopState.scancode = SC_LALT; break;
		case SC_RALT: loopState.scancode = SC_RWIN; break;
		case SC_RWIN: loopState.scancode = SC_RALT; break;
		}
	}

	if (feature.altAltToAlt)
	{
		if (loopState.scancode == SC_LALT || loopState.scancode == SC_RALT)
		{
			if (globalState.previousStrokeIn.scancode == loopState.originalStroke.scancode
				&& globalState.previousStrokeIn.isDownstroke 
				&& loopState.originalStroke.isDownstroke) //auto-repeating alt down
				loopState.scancode = SC_NOP;
			else if (loopState.isDownstroke && IS_MOD12_DOWN)
				globalState.modifiers &= ~BITMASK_MOD12;
			else if (!loopState.isDownstroke && IS_RALT_DOWN)
				loopState.scancode = SC_RALT;
			else if (!loopState.isDownstroke && IS_LALT_DOWN)
				loopState.scancode = SC_LALT;
			else
				loopState.scancode = SC_MOD12;
			stopProcessing = true;
		}
	}


	if(!stopProcessing)
		for (KeyModifierIftappedMapping map : keyModifierIftappedMapping)
		{
			if (loopState.scancode == map.inkey)
			{
				loopState.scancode = map.outkey;
				break;
			}
		}

	if (!stopProcessing && feature.shiftShiftToShiftLock)
	{
		switch (loopState.scancode)
		{
		case SC_LSHIFT:  //handle LShift+RShift -> CapsLock
			if (loopState.isDownstroke
				&& (globalState.modifiers == (BITMASK_RSHIFT))
				&& (GetKeyState(VK_CAPITAL) & 0x0001)) //ask Win for Capslock state
			{
				keySequenceAppendMakeBreakKey(SC_CAPS, loopState.resultingStrokeSequence);
				stopProcessing = true;
			}
			break;
		case SC_RSHIFT:
			if (loopState.isDownstroke
				&& (globalState.modifiers == (BITMASK_LSHIFT))
				&& !(GetKeyState(VK_CAPITAL) & 0x0001))
			{
				keySequenceAppendMakeBreakKey(SC_CAPS, loopState.resultingStrokeSequence);
				stopProcessing = true;
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
void playStrokeSequence(vector<Stroke> strokeSequence)
{
	Stroke newstroke;
	unsigned int delayBetweenKeyEventsMS = feature.delayForKeySequenceMS;
	bool inCpsEscape = false;  //inside an escape sequence, read next stroke

	IFDEBUG cout << "\t--> PLAY STROKE SEQUENCE (" << strokeSequence.size() << ")";
	for (Stroke stroke : strokeSequence)
	{
		if (stroke.scancode == SC_CPS_ESC)
		{
			if (stroke.isDownstroke)
			{
				if(inCpsEscape)
					error("Received double SC_CPS_ESC down.");
				else
				{
					if (globalState.modsTempAltered.size() > 0)
					{
						error("Internal error: previous escape sequence was not undone.");
						globalState.modsTempAltered.clear();
					}
				}
			}			
			else 
			{
				if (inCpsEscape) //play the escape sequence
				{
					for (Stroke strk : globalState.modsTempAltered)
					{
						sendStroke(strk);
						Sleep(delayBetweenKeyEventsMS);
					}
				}
				else //undo the previous sequence
				{
					IFDEBUG cout << endl << "play sequence::UNDO:" << globalState.modsTempAltered.size();
					for (Stroke strk : globalState.modsTempAltered)
					{
						strk.isDownstroke = !strk.isDownstroke;
						sendStroke(strk);
						Sleep(delayBetweenKeyEventsMS);
					}
					globalState.modsTempAltered.clear();
				}
			}
			inCpsEscape = stroke.isDownstroke;
			continue;
		}
		if (inCpsEscape)  //currently only one escape function: conditional break/make of modifiers
		{
			//the scancode is a modifier bitmask. Press/Release keys as necessary.
			//make modifier IF the system knows it is up
			unsigned short tempModChange = stroke.scancode;		//escaped scancode carries the modifier bitmask
			Stroke newstroke;

			if (stroke.isDownstroke)  //make modifiers when they are up
			{
				unsigned short exor = globalState.modifiers ^ tempModChange; //figure out which ones we must press
				tempModChange &= exor;
			}
			else	//break mods if down
				tempModChange &= globalState.modifiers;

			newstroke.isDownstroke = stroke.isDownstroke;

			const int NUMBER_OF_MODIFIERS_ALTERED_IN_SEQUENCES = 8; //high modifiers are skipped because they are never sent anyways.
			for (int i = 0; i < NUMBER_OF_MODIFIERS_ALTERED_IN_SEQUENCES; i++)  //push keycodes for all mods to be altered
			{
				unsigned short modBitmask = tempModChange & (1 << i);
				if (!modBitmask)
					continue;
				unsigned short sc = getModifierForBitmask(modBitmask);
				newstroke.scancode = sc;
				globalState.modsTempAltered.push_back(newstroke);
			}
			continue;
		}
		else //regular non-escaped stroke
		{
			sendStroke(stroke);
			if (stroke.scancode == AHK_HOTKEY1 || stroke.scancode == AHK_HOTKEY2)
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
		globalState.deviceIsAppleKeyboard = (id.find("VID_05AC") != string::npos);

		IFDEBUG cout << endl << "getHardwareId:" << id << " / Apple keyboard: " << globalState.deviceIsAppleKeyboard;
	}
}

void printHelloHeader()
{
	string line1 = "Capsicain v" + version;
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
		<< endl << endl << "ini version: " << feature.iniVersion
		<< endl << "Active Layer: " << globalState.activeLayer << " = " << alphaMapping.layerName

		<< endl << endl << "FEATURES"
		<< endl << (feature.flipZy ? "ON :" : "OFF:") << "Z<->Y "
		<< endl << (feature.shiftShiftToShiftLock ? "ON: " : "OFF: ") << "LShift + RShift -> ShiftLock"
		<< endl << (feature.altAltToAlt ? "ON: " : "OFF: ") << "LAlt + RAlt -> Alt"
		<< endl << (feature.flipAltWinOnAppleKeyboards ? "ON :" : "OFF:") << "Win<->Alt for Apple keyboards"
		<< endl << (feature.lControlBlocksAlphaMapping ? "ON: " : "OFF: ") << "Left Control blocks alpha key mapping"
		<< endl << (feature.processOnlyFirstKeyboard ? "ON: " : "OFF: ") << "Process only the board that sent the first key"
		;
}

void printHelp()
{
	cout << "HELP" << endl << endl
		<< "[ESC] + [{key}] for core commands" << endl
		<< "[X] Exit" << endl
		<< "[Q] (dev feature) Stop the debug build if both release and debug are running" << endl
		<< "[S] Status" << endl
		<< "[R] Reset" << endl
		<< "[D] Debug mode output" << endl
		<< "[E] Error log" << endl
		<< "[A] AHK start" << endl
		<< "[K] AHK end" << endl
		<< "[0]..[9] switch layers. [0] is the 'do nothing but listen for commands' layer" << endl
		<< "[Z] (labeled [Y] on GER keyboard): flip Y<>Z keys" << endl
		<< "[W] flip ALT <> WIN on Apple keyboards" << endl
		<< "[\\] (labeled [<] on GER keyboard): ISO boards only: key cut out of left shift -> Left Shift" << endl
		<< "[/] (labeled [-] on GER keyboard): Slash -> Right Shift" << endl
		<< "[ and ]: pause between keys in sequences -/+ 10ms " << endl
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
		<< "Capsicain version: " << version << endl
		<< "ini version: " << feature.iniVersion << endl
		<< "hardware id:" << globalState.deviceIdKeyboard << endl
		<< "Apple keyboard: " << globalState.deviceIsAppleKeyboard << endl
		<< "active layer: " << globalState.activeLayer << " = " << alphaMapping.layerName << endl
		<< "modifier state: " << hex << globalState.modifiers << endl
		<< "delay between keys in sequences (ms): " << feature.delayForKeySequenceMS << endl
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

InterceptionKeyStroke stroke2ikstroke(Stroke stroke)
{
	InterceptionKeyStroke iks = { stroke.scancode, 0 };
	if (stroke.scancode >= 0x80)
	{
		iks.code = static_cast<unsigned short>(stroke.scancode & 0x7F);
		iks.state |= 2;
	}
	if (!stroke.isDownstroke)
		iks.state |= 1;

	return iks;
}

Stroke ikstroke2stroke(InterceptionKeyStroke ikStroke)
{	
	Stroke strk;
	strk.scancode = ikStroke.code;
	if ((ikStroke.state & 2) == 2)
		strk.scancode |= 0x80;
	strk.isDownstroke = ikStroke.state & 1 ? false : true;
	return strk;
}

void sendStroke(Stroke stroke)
{
	if (stroke.scancode == 0xE4)
		IFDEBUG cout << " (sending E4) ";
	if (stroke.scancode > 0xFF)
	{
		error("Unexpected scancode > 255: " + to_string(stroke.scancode));
		return;
	}
	if (!stroke.isDownstroke &&  !globalState.keysDownSent[(unsigned char)stroke.scancode])  //ignore up when key is already up
	{
		IFDEBUG cout << " >(blocked " << scLabels[stroke.scancode] << " UP: was not down)>";
		return;
	}
	globalState.keysDownSent[(unsigned char)stroke.scancode] = stroke.isDownstroke;

	InterceptionKeyStroke iks = stroke2ikstroke(stroke);
	IFDEBUG cout << " >" << scLabels[stroke.scancode] << (stroke.isDownstroke ? "v" : "^") << ">";

	interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&iks, 1);
}

void reset()
{
	resetAllStatesToDefault();

	for (int i = 0; i < 255; i++)	//Send() suppresses key UP if it thinks it is already up.
		globalState.keysDownSent[i] = true;

	IFDEBUG cout << endl << "Resetting all modifiers to UP" << endl;
	vector<Stroke> strokeSequence;

	strokeSequence.push_back({SC_LSHIFT, false });
	strokeSequence.push_back({SC_RSHIFT, false });
	strokeSequence.push_back({SC_LCTRL, false});
	strokeSequence.push_back({SC_RCTRL, false});
	strokeSequence.push_back({SC_LWIN, false});
	strokeSequence.push_back({SC_RWIN, false});
	strokeSequence.push_back({SC_LALT, false});
	strokeSequence.push_back({SC_RALT, false});
	strokeSequence.push_back({SC_CAPS, false});
	strokeSequence.push_back({AHK_HOTKEY1, false});
	strokeSequence.push_back({AHK_HOTKEY2, false});
	playStrokeSequence(strokeSequence);

	for (int i = 0; i < 255; i++)
	{
		globalState.keysDownSent[i] = false;
	}

	readIni();
}


void keySequenceAppendMakeKey(unsigned short scancode, vector<Stroke> &sequence)
{
	sequence.push_back({ scancode, true });
}
void keySequenceAppendBreakKey(unsigned short scancode, vector<Stroke> &sequence)
{
	sequence.push_back({ scancode, false });
}
void keySequenceAppendMakeBreakKey(unsigned short scancode, vector<Stroke> &sequence)
{
	sequence.push_back({ scancode, true });
	sequence.push_back({ scancode, false });
}

string getSymbolForStrokeState(unsigned short state)
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
