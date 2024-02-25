#pragma once

#define VERSION "96"

//arbitray limits
#define MAX_VCODES 0x140  //biggest defined code in scancodes.h must be smaller than this
#define MAX_MACRO_LENGTH 200  //stop recording at some point if it was forgotten.
#define MAX_NUM_MACROS 21 //max number of stored macros (mapped later to 1..20, and the 'hard' macro 0)

//constants
#define DISABLED_CONFIG_NUMBER  0 // layer 0 does nothing
#define DISABLED_CONFIG_NAME "Capsicain disabled. \r\nNo processing, forward everything. \r\nOnly ESC-X and ESC-0..9 work."
#define AHK_HOTKEY1 SC_F14  //this key triggers supporting AHK script
#define AHK_HOTKEY2 SC_F15

//defaults
#define DEFAULT_ACTIVE_CONFIG 1
#define DEFAULT_ACTIVE_CONFIG_NAME "Config not initialized. Forwarding all keys."
#define DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS 5  //System may drop keys when they are sent too fast. Local host needs 0-1ms, Linux VM 5+ms for 100% reliable keystroke detection

#define DEFAULT_START_AHK_ON_STARTUP true
#define DEFAULT_DELAY_FOR_AHK_MS 50    //autohotkey is slow

//rewiremap columns
#define REWIRE_COLS 4
#define REWIRE_ROWS MAX_VCODES
//#define REWIRE_IN  0 IN is the row index
#define REWIRE_OUT 0
#define REWIRE_TAP 1
#define REWIRE_DOUBLETAP 2
#define REWIRE_TAPHOLD 3

const std::string INI_TAG_INCLUDE = "INCLUDE";
const std::string INI_TAG_GLOBAL = "GLOBAL";
const std::string INI_TAG_OPTIONS = "OPTION";
const std::string INI_TAG_REWIRE = "REWIRE";
const std::string INI_TAG_COMBOS = "DOWN";
const std::string INI_TAG_UPCOMBOS = "UP";
const std::string INI_TAG_TAPCOMBOS = "TAP";
const std::string INI_TAG_SLOWCOMBOS = "SLOW";
const std::string INI_TAG_REPEATCOMBOS = "REPEAT";
const std::string INI_TAG_ALPHA_FROM = "ALPHA_FROM";
const std::string INI_TAG_ALPHA_TO = "ALPHA_TO";
const std::string INI_TAG_ALPHA_END = "ALPHA_END";
const std::string INI_TAG_EXE = "EXE";

using DEV = uint32_t;