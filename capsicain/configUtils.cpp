#pragma once
#include "pch.h"
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>

#include "configUtils.h"
#include "capsicain.h"
#include "constants.h"
#include "utils.h"
#include "scancodes.h"
#include "modifiers.h"

using namespace std;

//cut comments, tab to space, trim, single blanks, lowercase
void normalizeLine(string &line)
{
    auto idxComment = line.find_first_of('#');
    if (string::npos != idxComment)
        line.erase(idxComment);

    std::replace(line.begin(), line.end(), '\t', ' ');

    line.erase(0, line.find_first_not_of(' '));
    line.erase(line.find_last_not_of(' ') + 1);

    std::string::size_type pos;
    while ((pos = line.find("  ")) != std::string::npos)
        line.replace(pos, 2, " ");

    std::transform(line.begin(), line.end(), line.begin(), ::tolower);
}

int checkSyntax(std::vector<string> iniLines)
{
    int errors = 0;
    bool inAlpha = false;
    for (std::vector<std::string>::iterator t = iniLines.begin(); t != iniLines.end(); t++) 
    {
        if (stringStartsWith(*t, "alpha_from"))
            inAlpha = true;
        else if (stringStartsWith(*t, "alpha_end"))
        {
            inAlpha = false;
            continue;
        }
        if (inAlpha)
            continue;

        if (
            stringStartsWith(*t, "[") ||
            stringStartsWith(*t, "global") ||
            stringStartsWith(*t, "include") ||
            stringStartsWith(*t, "rewire") ||
            stringStartsWith(*t, "combo") ||
            stringStartsWith(*t, "down") ||
            stringStartsWith(*t, "up") ||
            stringStartsWith(*t, "tap") ||
            stringStartsWith(*t, "slow") ||
            stringStartsWith(*t, "repeat") ||
            stringStartsWith(*t, "option") ||
            stringStartsWith(*t, "exe")
            )
            continue;

        std::cout << "Syntax error: Unknown keyword: " << *t << std::endl;
        errors++;
    }
    return errors;
}

// Read .ini file, normalize lines, drop empty lines, drop [Reference* sections
bool readSanitizeIniFile(std::vector<string> &iniLines)
{
    iniLines.clear();
    string line;
    bool inReferenceSection = false;
    ifstream f("capsicain.ini");
    if (!f.is_open())
        return false;

    bool detectBom = true;  //that nasty UTF BOM that MS loves so much...
    while (getline(f, line)) 
    {
        if (detectBom)
        {
            detectBom = false;
            int i = 0;
            while (i < line.length() && line[i] < 0)
                line[i++] = 32;
        }
        normalizeLine(line);
        if (line == "")
            continue;
        if (stringStartsWith(line, "[ahk]"))
            break;
        if (stringStartsWith(line, "[reference"))
            inReferenceSection = true;
        else if (stringStartsWith(line, "[") && ! stringStartsWith(line, "[ "))
            inReferenceSection = false;
        if(!inReferenceSection)
            iniLines.push_back(line);
    }

    if (f.bad())
    {
        cout << "Error while reading .ini file";
        return false;
    }
    int numErrors = checkSyntax(iniLines);
    if (numErrors>0)
        cout << "**** There are " << numErrors << " errors in your ini file ****";
    return true;
}


//Returns empty vector if section does not exist, or is empty
std::vector<std::string> getSectionFromIni(std::string sectionName, std::vector<std::string> iniContent)
{
    std::vector<std::string> sectionContent;
    string sectName = stringToLower(sectionName);
    string line;
    bool inSection = false;

    for (string line : iniContent)
    {

        if (stringStartsWith(line, "[" + sectName + "]"))
            inSection = true;
        else if (inSection)
        {
            if (stringStartsWith(line, "["))
                break;
            sectionContent.push_back(line);
        }
    }
    if (inSection && sectionContent.size() == 0)
        sectionContent.push_back("option configname empty_layer_do_nothing");
    return sectionContent;
}
//Returns all lines starting with tag, with the tag removed, or empty vector if none
std::vector<std::string> getTaggedLinesFromIni(std::string tag, std::vector<std::string> iniContent)
{
    std::vector<std::string> taggedContent;
    tag = stringToLower(tag);
    string line;
    for (string line : iniContent)
    {
        if (stringCopyFirstToken(line) == tag)
            taggedContent.push_back(stringGetRestBehindFirstToken(line));
    }
    return taggedContent;
}

bool configHasKey(string key, vector<string> sectionLines)
{
    key = stringToLower(key);
    for (string line : sectionLines)
    {
        if (stringCopyFirstToken(line) == key)
            return true;
    }
    return false;
}

bool configHasTaggedKey(std::string tag, std::string key, std::vector<std::string> sectionLines)
{
    tag = stringToLower(tag);
    key = stringToLower(key);
    for (string line : sectionLines)
    {
        if (stringCopyFirstToken(line) == tag
            && stringCopyFirstToken(stringGetRestBehindFirstToken(line)) == key)
            return true;
    }
    return false;
}

bool getStringValueForTaggedKey(string tag, string key, std::string &value, vector<string> sectionLines)
{
    tag = stringToLower(tag);
    key = stringToLower(key);
    vector<string> splitline;
    value = "";
    for (string line : sectionLines)
    {
        splitline = stringSplit(line, ' ');
        if (splitline.size() < 3)
            continue;
        if (splitline.at(0) == tag && splitline.at(1) == key)
        {
            for (int i = 2; i < splitline.size(); i++)
                value += splitline.at(i) + " ";
            normalizeLine(value);
            return true;
        }
    }
    return false;
}

bool getStringValueForKey(std::string key, std::string &value, vector<string> sectionLines)
{
    key = stringToLower(key);
    for (string line : sectionLines)
    {
        if (stringStartsWith(line, key))
        {
            value = stringGetRestBehindFirstToken(line);
            return true;
        }
    }
    return false;
}

bool getIntValueForTaggedKey(string tag, string key, int &value, vector<string> sectionLines)
{
    string strval;
    if (!getStringValueForTaggedKey(tag, key, strval, sectionLines))
        return false;

    return stringToInt(strval, value);
}

bool getIntValueForKey(std::string key, int &value, vector<std::string> sectionLines)
{
    key = stringToLower(key);
    string strval;
    if (!getStringValueForKey(key, strval, sectionLines))
        return false;

    return stringToInt(strval, value);
}


// parse "a b c ALPHA_TO x y z"
bool parseKeywordsAlpha_FromTo(std::string alpha_to, int (&alphamap)[MAX_VCODES], std::string scLabels[])
{
    size_t idx1 = alpha_to.find(stringToLower(INI_TAG_ALPHA_TO));
    if (idx1 == string::npos)
        return false;
    string tmpfrom = (alpha_to.substr(0, idx1));
    string tmpto = (alpha_to.substr(idx1 + INI_TAG_ALPHA_TO.length()));
    normalizeLine(tmpfrom);
    normalizeLine(tmpto);
    vector<string> sfrom = stringSplit(tmpfrom, ' ');
    vector<string> sto = stringSplit(tmpto, ' ');
    if (sfrom.size() == 0 || sfrom.size() != sto.size())
    {
        cout << endl << "ERROR: " << INI_TAG_ALPHA_FROM << " and " << INI_TAG_ALPHA_TO << " lists are different size";
        return false;
    }

    for (int i = 0; i < sfrom.size(); i++)
    {
        int ifrom = getVcode(sfrom[i], scLabels);
        int ito = getVcode(sto[i], scLabels);
        if (ifrom < 0 || ito < 0)
        {
            cout << endl << "Unknown scancode labels: " << sfrom[i] << " and " << sto[i];
            return false;
        }
        if (alphamap[ifrom] != ifrom)
        {
            cout << endl << "WARNING: Ignoring redefinition of alpha key: " << sfrom[i] << " to " << sto[i];
        }
        alphamap[(unsigned char)ifrom] = (unsigned char)ito;
    }
    return true;
}

// parse "REWIRE A B"  or  "REWIRE A B C D". Does not touch optional keys that are not defined in the line.
// the // symbol stands for -1 "do nothing with this"
bool parseKeywordRewire(std::string line, int &keyA, int &keyB, int &keyC, int &keyD, std::string scLabels[])
{
    vector<string> labels = stringSplit(line, ' ');
    if (labels.size() < 2 && labels.size() > 4)
    {
        cout << endl << "ERROR: REWIRE must have 2..4 tokens: " << line;
        return false;
    }

    int ikeyA = getVcode(labels[0], scLabels);
    int ikeyB = getVcode(labels[1], scLabels);
    int ikeyC;
    int ikeyD;

    bool hasTapConfig = labels.size() >= 3 && labels[2] != "//";
    if (hasTapConfig)
        ikeyC = getVcode(labels[2], scLabels);

    bool hasTapHoldConfig = labels.size() >= 4 && labels[3] != "//";
    if (hasTapHoldConfig)
        ikeyD = getVcode(labels[3], scLabels);

    if (ikeyA < 0 || ikeyB < 0 || (hasTapConfig && ikeyC < 0) || (hasTapHoldConfig && ikeyD < 0) )
        return false; //invalid key label
    if (ikeyA > 255 && ikeyA != VK_CPS_PAUSE)
    {
        cout << endl << "INFO: rewiring a virtual key (other than PAUSE) does not make sense. Your keyboard never sends VKeys";
        return false;
    }

    keyA = ikeyA;
    keyB = ikeyB;
    if(hasTapConfig)
        keyC = ikeyC;
    if (hasTapHoldConfig)
        keyD = ikeyD;

    return true;
}

//convert ("xyz_&.", '&') to 000010
MOD parseModString(string modString, char filter)
{
    modString.erase(std::remove(modString.begin(), modString.end(), ' '), modString.end());
    MOD mask = 0;
    for (int i = 0; i < min(modString.size(), 32); ++i)
        if (modString[modString.size() - i - 1] == filter)
            mask |= 1 << i;
    return mask;
}

// parses + separated combo that can also include a modstring to keycodes
bool parseComboParams(string funcParams, vector<int> &vcodes, string * scLabels)
{
    size_t count = vcodes.size();
    bool nppFound = stringReplace(funcParams, "np+", "np@");
    vector<string> labels = stringSplit(funcParams, '+');
    if (nppFound)
        for (int i = 0; i < labels.size(); i++)
            if (labels[i] == "np@")
                labels[i] = "np+";

    // support both ..&. and labeled modifiers, but always put modstring modifiers first
    for (auto it = labels.begin(); it != labels.end();) {
        int modsPress = parseModString(*it, '&');
        if (!modsPress)
        {
            ++it;
        }
        else
        {
            for (int i = 0; i < 8; i++)
            {
                int currentMod = modsPress & (1 << i);
                if (currentMod > 0) {
                    vcodes.push_back(getModifierForBitmask(currentMod));
                }
            }
            it = labels.erase(it);
        }
    }

    int isc;
    for (string label : labels)
    {
        isc = getVcode(label, scLabels);
        if (isc < 0)
            continue;
        vcodes.push_back(isc);
    }
    return vcodes.size() > count;
}

// omni function to call the others based on modifiers
bool parseFunctionKey(std::string funcParams, std::string * scLabels, std::vector<VKeyEvent> &strokeSeq)
{
    size_t idx_comma = (int)funcParams.rfind(',');
    size_t idx_plus = (int)funcParams.rfind('+');
    if (idx_comma == string::npos || idx_comma == funcParams.size() - 1 || (idx_plus != string::npos && idx_plus > idx_comma))
        return parseFunctionCombo(funcParams, scLabels, strokeSeq);
    string combo = funcParams.substr(0, idx_comma);
    string type = funcParams.substr(idx_comma + 1);
    bool hold = type.find('h') != string::npos;
    bool release = type.find('r') != string::npos;
    bool holdmods = type.find('m') != string::npos;
    int times = strtol(type.c_str(), nullptr, 10);
    if (times <= 0)
        times = 1;

    if (hold || holdmods)
        return parseFunctionHold(combo, scLabels, strokeSeq, release, holdmods);
    else
        return parseFunctionCombo(combo, scLabels, strokeSeq, release, times);

    return false;
}

bool parseFunctionCombo(std::string funcParams, std::string * scLabels, std::vector<VKeyEvent> &strokeSeq, bool releaseTemp, int times)
{
    vector<int> keys;
    if (!parseComboParams(funcParams, keys, scLabels))
        return false;
    if (releaseTemp)
        strokeSeq.push_back({ VK_CPS_TEMPRELEASEKEYS, true });
    for (int i = 0; i < times; ++i)
    {
        for (auto it = keys.begin(); it != keys.end(); ++it)
        {
            strokeSeq.push_back({ *it, true });
        }
        for (auto it = keys.rbegin(); it != keys.rend(); ++it)
        {
            strokeSeq.push_back({ *it, false });
        }
    }
    if (releaseTemp)
        strokeSeq.push_back({ VK_CPS_TEMPRESTOREKEYS, true });
    return true;
}

bool parseFunctionHold(std::string funcParams, std::string * scLabels, std::vector<VKeyEvent> &strokeSeq, bool releaseAll, bool holdMods)
{
    vector<int> keys;
    if (!parseComboParams(funcParams, keys, scLabels))
        return false;
    if (releaseAll)
        strokeSeq.push_back({ VK_CPS_RELEASEKEYS, true });
    if (holdMods)
    {
        for (auto it = keys.begin(); it != keys.end(); ++it)
        {
            if (isModifier(*it))
                strokeSeq.push_back({ VK_CPS_HOLDMOD, true });
            else
                strokeSeq.push_back({ VK_CPS_HOLDKEY, true });
            strokeSeq.push_back({ *it, true });
        }
    }
    else
    {
        for (auto it = keys.begin(); it != keys.end(); ++it)
        {
            strokeSeq.push_back({ VK_CPS_HOLDKEY, true });
            strokeSeq.push_back({ *it, true });
        }
    }
    return true;
}

DEV parseDeviceMask(string devstr, char filter) {
    if (devstr.empty())
    {
        if (filter == '&')
            return 0xFFFFFFFF;
        else
            return 0;
    }
    DEV mask = 0;
    auto* devices = getHardwareId(true);
    for (auto dev : stringSplit(devstr, ','))
    {
        if (dev.empty())
            continue;
        for (int i = 1; i <= INTERCEPTION_MAX_DEVICE; ++i)
        {
            if (dev[0] == filter && (*devices)[i].id.find(dev.substr(1)) != string::npos)
                mask |= 1 << (i - 1);
        }
    }
    return mask;
}

//parse {deadkey-x} keyLabel  [&|^t ....] > function(param)
//returns false if the rule is not valid.
//this translates functions() in the .ini to key sequences (usually with special VK_CPS keys)
bool parseKeywordCombo(std::string line, int &key, MOD(&mods)[6], DEV(&devs)[2], std::vector<VKeyEvent> &strokeSequence, std::string scLabels[], std::string defaultFunction)
{
    string strkey = stringCutFirstToken(line);
    if (strkey.length() < 1)
        return false;

    int deadKey = 0;
    if (stringStartsWith(strkey, "deadkey-"))  //parse optional COMBO DEADKEY-XY  A [...] > func()
    {
        strkey = strkey.substr(8);
        if (strkey.length() < 1)
            return false;
        deadKey = getVcode(strkey, scLabels);
        if (deadKey < 0 || deadKey > 255)
            return false;

        strkey = stringCutFirstToken(line);
        if (strkey.length() < 1)
            return false;
    }

    int itmpKey = getVcode(strkey, scLabels);
    if (itmpKey < 0)
        return false;
    key = itmpKey;

    mods[0] = (unsigned char) deadKey;

    line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

    size_t devIdx1 = line.find_first_of('{');
    size_t devIdx2 = line.find_first_of('}');
    if (devIdx1 > devIdx2 || (devIdx1 == string::npos) != (devIdx2 == string::npos))
        return false;

    if (devIdx1 != string::npos && devIdx2 != string::npos)
    {
        devIdx1++;
        devs[0] = parseDeviceMask(line.substr(devIdx1, devIdx2 - devIdx1), '&');
        devs[1] = parseDeviceMask(line.substr(devIdx1, devIdx2 - devIdx1), '^');
    }

    string mod;
    size_t modIdx1 = line.find_first_of('[');
    size_t modIdx2 = line.find_first_of(']');
    if (modIdx1 > modIdx2 || (modIdx1 == string::npos) != (modIdx2 == string::npos))
        return false;
    if (modIdx1 != string::npos && modIdx2 != string::npos)
    {
        modIdx1++;
        mod = line.substr(modIdx1, modIdx2 - modIdx1);
        mods[1] = parseModString(mod, '&'); //and 
        mods[2] = parseModString(mod, '|'); //or
        mods[3] = parseModString(mod, '^'); //not 
        mods[4] = parseModString(mod, 't'); //tap
        mods[5] = parseModString(mod, '/'); //tap OR hold, but doesn't need separate COMBOs for t and &
        mods[1] |= parseModString(mod, '*'); //tap AND hold, but doesn't need a TapAndHold rewrite
        mods[4] |= parseModString(mod, '*');
    }

    //extract function name + param
    size_t funcIdx1 = line.find_first_of('>') + 1;
    if (funcIdx1 == string::npos || (modIdx2 != string::npos && funcIdx1 < modIdx2))
    {
        cout << endl << "ERROR in ini: missing '>' in: " << line;
        return false;
    }

    stringstream funcss(line.substr(funcIdx1));
    string funcstr;
    vector<string> funcs;
    while(getline(funcss, funcstr, ')'))
        funcs.push_back(funcstr);
    vector<VKeyEvent> strokeSeq;
    for (auto func : funcs)
    {
        string funcName;
        string funcParams;
        size_t sep = func.find_first_of('(');
        if (sep == string::npos)
        {
            char str[255];
            sprintf_s(str, 255, defaultFunction.c_str(), func.c_str());
            func = string(str);
            func.erase(func.find_last_not_of(')') + 1);
            sep = func.find_first_of('(');
            size_t end = func.find_last_of(')');
            if (sep == string::npos)
                return false;
            funcName = func.substr(0, sep);
            sep++;
            funcParams = func.substr(sep, string::npos);
        }
        else
        {
            funcName = func.substr(0, sep);
            sep++;
            funcParams = func.substr(sep, string::npos);
        }

        //translate 'function' into a key sequence
        if (funcName == "key")
        {
            if (!parseFunctionKey(funcParams, scLabels, strokeSeq))
                return false;
        }
        else if (funcName == "combo")
        {
            if (!parseFunctionCombo(funcParams, scLabels, strokeSeq))
                return false;
        }
        else if (funcName == "hold")
        {
            if (!parseFunctionHold(funcParams, scLabels, strokeSeq))
                return false;
        }
        else if (funcName == "moddedhold")
        {
            if (!parseFunctionHold(funcParams, scLabels, strokeSeq, true))
                return false;
        }
        else if (funcName == "holdmods")
        {
            if (!parseFunctionHold(funcParams, scLabels, strokeSeq, false, true))
                return false;
        }
        else if (funcName == "moddedholdmods")
        {
            if (!parseFunctionHold(funcParams, scLabels, strokeSeq, true, true))
                return false;
        }
        else if (funcName == "combontimes")
        {
            size_t idx = (int)funcParams.rfind(',');
            if (idx == string::npos)
                return false;
            string combo = funcParams.substr(0, idx);
            string stime = funcParams.substr(idx + 1);
            if (!parseFunctionCombo(combo, scLabels, strokeSeq, false, stoi(stime)))
                return false;
        }
        else if (funcName == "altchar")
        {
            strokeSeq.push_back({ VK_CPS_TEMPRELEASEKEYS, true }); //temp release LSHIFT if it is currently down
            strokeSeq.push_back({ SC_LALT , true });
            for (int i = 0; i < funcParams.length(); i++)
            {
                char c = funcParams[i];
                string altkey="NP";
                if (c >= '0' && c <= '9')
                    altkey.push_back(c);
                else if (c == '+')
                    altkey = "NP+";
                else if (c >= 'a' && c <= 'f')
                {
                    altkey = "";
                    altkey.push_back(c);
                }
                else
                    return false;

                int isc = getVcode(altkey, scLabels);
                if (isc < 0)
                    return false;
                strokeSeq.push_back({ (unsigned char)isc, true });
                strokeSeq.push_back({ (unsigned char)isc, false });
            }
            strokeSeq.push_back({ SC_LALT , false });
            strokeSeq.push_back({ VK_CPS_TEMPRESTOREKEYS, false });
        }
        else if (funcName == "moddedkey")
        {
            if (!parseFunctionCombo(funcParams, scLabels, strokeSeq, true))
                return false;
        }
        else if (funcName == "sequence")
        {
            vector<string> params = stringSplit(funcParams, '_');
            bool downkeys[256] = { 0 };
            const string SLEEP_TAG = "sleep:";
            const string DELAY_TAG = "delay:";
            const string CONFIGSWITCH_TAG = "configswitch:";

            for (string param : params)
            {
                // &key is down, ^key is up, key is both.
                bool downstroke = true;
                bool upstroke = true;
                if (param.at(0) == '&')
                    upstroke = false;
                else if (param.at(0) == '^')
                    downstroke = false;
                if (!downstroke || !upstroke)
                    param = param.substr(1);

                if (stringStartsWith(param, "pause:"))
                {
                    cout << endl << "WARNING: '_pause:10_' is now written as '_sleep:1000_'." << endl << "Ignoring " << param;
                    continue;
                }
                //handle the "sleep:10" items
                if (stringStartsWith(param, SLEEP_TAG))
                {
                    string sleeptime = param.substr(SLEEP_TAG.length());
                    int stime;
                    if (stringToInt(sleeptime, stime) && stime > 0 && stime <= 30000)
                    {
                        strokeSeq.push_back({ VK_CPS_SLEEP, true });
                        strokeSeq.push_back({ stime, true });
                    }
                    continue;
                }
                if (stringStartsWith(param, DELAY_TAG))
                {
                    string sleeptime = param.substr(DELAY_TAG.length());
                    int stime;
                    if (stringToInt(sleeptime, stime) && stime >= 0 && stime <= 1000)
                    {
                        strokeSeq.push_back({ VK_CPS_DELAY, true });
                        strokeSeq.push_back({ stime, true });
                    }
                    continue;
                }
                //handle the "configswitch:2" items
                if (stringStartsWith(param, CONFIGSWITCH_TAG)) {
                    string configParam = param.substr(CONFIGSWITCH_TAG.length());
                    int configuration = stoi(configParam);
                    if (configuration > 9) {
                        cout << endl << "Sequence() defines configswitch: > 9. Not switching.";
                        continue;
                    }
                    if (configuration < 0) {
                        cout << endl << "Sequence() defines configswitch: < 0. Not switching.";
                        continue;
                    }
                    strokeSeq.push_back({VK_CPS_CONFIGSWITCH, true});
                    strokeSeq.push_back({configuration, true});
                    continue;
                }
                int isc = getVcode(param, scLabels);
                if (isc < 0)
                {
                    cout << endl << "WARNING: Unknown key label in sequence(): " << param;
                    return false;
                }

                if (downstroke)
                {
                    strokeSeq.push_back({ (unsigned char)isc, true });
                    downkeys[(unsigned char)isc] = true;
                }
                if (upstroke)
                {
                    strokeSeq.push_back({ (unsigned char)isc, false });
                    downkeys[(unsigned char)isc] = false;
                }
            }
            //check if all keys were released
            for (int i = 0; i < 256; i++)
            {
                if (downkeys[i])
                {
                    cout << endl << "Sequence() does not release key: " << scLabels[i] << " (discarding this rule)";
                    return false;
                }
            }
        }
        else if (funcName == "deadkey")
        {
            int isc = getVcode(funcParams, scLabels);
            if (isc < 0 || isc > 255)
                return false;
            strokeSeq.push_back({ VK_CPS_DEADKEY, true });
            strokeSeq.push_back({ isc, true });
        }
        else if ( ((funcName == "configswitch")) || (funcName == "layerswitch"))
        {
            int isc;
            bool valid = stringToInt(funcParams, isc);
            if (!valid || isc < 0 || isc > 10)
            {
                cout << endl << "Invalid config switch to: " << funcParams;
                return false;
            }
            strokeSeq.push_back({ VK_CPS_CONFIGSWITCH, true });
            strokeSeq.push_back({ isc, true });
        }
        else if ((funcName == "configprevious") || (funcName == "layerprevious"))
        {
            strokeSeq.push_back({ VK_CPS_CONFIGPREVIOUS, true });
        }
        else if (funcName == "recordmacro" || funcName == "recordsecretmacro" || funcName == "playmacro")
        {
            int macroNum;
            bool valid = stringToInt(funcParams, macroNum);
            if (!valid || macroNum < 1 || macroNum >= MAX_NUM_MACROS)
            {
                cout << endl <<  "Invalid macro number : " << funcParams << " (must be 1.."<< MAX_NUM_MACROS-1 <<")";
                return false;
            }

            if(funcName == "recordmacro")
                strokeSeq.push_back({ VK_CPS_RECORDMACRO, true });
            else if (funcName == "recordsecretmacro")
                strokeSeq.push_back({ VK_CPS_RECORDSECRETMACRO, true });
            else if (funcName == "playmacro")
                strokeSeq.push_back({ VK_CPS_PLAYMACRO, true });

            strokeSeq.push_back({ macroNum, true });
        }
        else if (funcName == "sleep")
        {
            int stime;
            if (stringToInt(funcParams, stime))
            {
                strokeSeq.push_back({ VK_CPS_SLEEP, true });
                strokeSeq.push_back({ stime, true });
            }
        }
        else if (funcName == "delay")
        {
            int stime;
            if (stringToInt(funcParams, stime))
            {
                strokeSeq.push_back({ VK_CPS_DELAY, true });
                strokeSeq.push_back({ stime, true });
            }
        }
        else if (funcName == "release")
        {
            strokeSeq.push_back({ VK_CPS_RELEASEKEYS, true });
        }
        else if (funcName == "nop")
        {
            strokeSeq.push_back({ SC_NOP, true });
        }
        else if (funcName == "down")
        {
            vector<int> keys;
            parseComboParams(funcParams, keys, scLabels);
            for (auto isc : keys)
            {
                strokeSeq.push_back({ VK_CPS_KEYDOWN, true });
                strokeSeq.push_back({ isc, true });
            }
        }
        else if (funcName == "up")
        {
            vector<int> keys;
            parseComboParams(funcParams, keys, scLabels);
            for (auto isc : keys)
            {
                strokeSeq.push_back({ VK_CPS_KEYDOWN, true });
                strokeSeq.push_back({ isc, false });
            }
        }
        else if (funcName == "toggle")
        {
            vector<int> keys;
            parseComboParams(funcParams, keys, scLabels);
            for (auto isc : keys)
            {
                strokeSeq.push_back({ VK_CPS_KEYTOGGLE, true });
                strokeSeq.push_back({ isc, true });
            }
        }
        else if (funcName == "tap")
        {
            vector<int> keys;
            parseComboParams(funcParams, keys, scLabels);
            for (auto isc : keys)
            {
                strokeSeq.push_back({ VK_CPS_KEYTAP, true });
                strokeSeq.push_back({ isc, true });
            }
        }
        else if (funcName == "untap")
        {
            vector<int> keys;
            parseComboParams(funcParams, keys, scLabels);
            for (auto isc : keys)
            {
                strokeSeq.push_back({ VK_CPS_KEYTAP, true });
                strokeSeq.push_back({ isc, false });
            }
        }
        else if (funcName == "exe")
        {
            int exeNum;
            bool valid = stringToInt(funcParams, exeNum);
            if (!valid)
            {
                cout << endl <<  "Invalid executable : " << funcParams;
                return false;
            }
            strokeSeq.push_back({ VK_CPS_EXECUTE, true });
            strokeSeq.push_back({ exeNum, true });
        }
        else if (funcName == "ahk")
        {
            strokeSeq.push_back({ VK_CPS_SENDAHK, true });
            strokeSeq.push_back({ getAHKid(funcParams), true });
        }
        else if (funcName == "kill")
        {
            int exeNum;
            bool valid = stringToInt(funcParams, exeNum);
            if (!valid)
            {
                cout << endl <<  "Invalid executable : " << funcParams;
                return false;
            }
            strokeSeq.push_back({ VK_CPS_KILL, true });
            strokeSeq.push_back({ exeNum, true });
        }
        else
        { 
            return false;
        }
    }

    strokeSequence = strokeSeq;
    return true;
}