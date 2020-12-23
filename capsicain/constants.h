#pragma once

#define VERSION "68beta"


//arbitray limits
#define MAX_VCODES 0x120  //biggest defined code in scancodes.h must be smaller than this
#define MAX_MACRO_LENGTH 200  //stop recording at some point if it was forgotten.

//constants
#define LAYER_DISABLED  0 // layer 0 does nothing
#define LAYER_DISABLED_LAYER_NAME "Capsicain disabled. No processing, forward everything. Only ESC-X and ESC-0..9 work."
#define AHK_HOTKEY1 SC_F14  //this key triggers supporting AHK script
#define AHK_HOTKEY2 SC_F15

//defaults
#define DEFAULT_ACTIVE_LAYER 1
#define DEFAULT_ACTIVE_LAYER_NAME "Layer not initialized. Forwarding all keys."
#define DEFAULT_DELAY_ON_STARTUP_MS 0 //time to release all keys (e.g. if capsicain is started via shortcut)
#define DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS 5  //System may drop keys when they are sent too fast. Local host needs 0-1ms, Linux VM 5+ms for 100% reliable keystroke detection

#define DEFAULT_START_AHK_ON_STARTUP true
#define DEFAULT_DELAY_FOR_AHK_MS 50    //autohotkey is slow

//rewiremap columns
#define REWIRE_COLS 4
#define REWIRE_ROWS 256
//#define REWIRE_IN  0 IN is the row index
#define REWIRE_OUT 0
#define REWIRE_TAP 1
#define REWIRE_DOUBLETAP 2
#define REWIRE_TAPHOLD 3

const std::string INI_TAG_INCLUDE = "INCLUDE";
const std::string INI_TAG_GLOBAL = "GLOBAL";
const std::string INI_TAG_OPTIONS = "OPTION";
const std::string INI_TAG_REWIRE = "REWIRE";
const std::string INI_TAG_COMBOS = "COMBO";
const std::string INI_TAG_ALPHA_FROM = "ALPHA_FROM";
const std::string INI_TAG_ALPHA_TO = "ALPHA_TO";
const std::string INI_TAG_ALPHA_END = "ALPHA_END";
