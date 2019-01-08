#pragma oncce
#include "pch.h"
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <windows.h>
#include <tlhelp32.h>

#include "config.h"
#include "capsicain.h"
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

    replace(line.begin(), line.end(), '\t', ' ');

    line.erase(0, line.find_first_not_of(' '));
    line.erase(line.find_last_not_of(' ') + 1);

    std::string::size_type pos;
    while ((pos = line.find("  ")) != std::string::npos)
        line.replace(pos, 2, " ");

    std::transform(line.begin(), line.end(), line.begin(), ::tolower);
}

// Read .ini file, normalize lines, drop empty lines, drop [[Reference* sections
bool readIniFile(std::vector<string> &iniLines)
{
    iniLines.clear();
    string line;
    bool inReferenceSection = false;
    ifstream f("capsicain.ini");
    if (!f.is_open())
        return false;

    while (getline(f, line)) {
        normalizeLine(line);
        if (line == "")
            continue;
        if (stringStartsWith(line, "[[reference"))
            inReferenceSection = true;
        else if (stringStartsWith(line, "[["))
            inReferenceSection = false;
        if(!inReferenceSection)
            iniLines.push_back(line);
    }

    if (f.bad())
    {
        cout << "Error while reading .ini file";
        return false;
    }

    return true;
}

// Get the lines in section "NAME_3" or if it does not exist, "NAME_DEFAULT".
std::vector<std::string> getSection_nOrDefaultFromIni(std::string sectionName, int sectionVersion, std::vector<string> iniContent)
{
    std::vector<std::string> sectionContent;
    sectionContent = (getSectionFromIni(sectionName + "_" + to_string(sectionVersion), iniContent));
    if(sectionContent.size() == 0)
        sectionContent = getSectionFromIni(sectionName, iniContent);
    return sectionContent;
}

std::vector<std::string> getSectionFromIni(std::string sectionName, std::vector<std::string> iniContent)
{
    std::vector<std::string> sectionContent;
    string sectName = stringToLower(sectionName);
    string line;
    bool inSection = false;

    for(string line : iniContent)
    {
        if (stringStartsWith(line, "[[" + sectName + "]]"))
            inSection = true;
        else if (inSection)
        {
            if (stringStartsWith(line, "[["))
                break;
            sectionContent.push_back(line);
        }
    }
    return sectionContent;
}

bool sectionHasKey(string key, vector<string> sectionLines)
{
    key = stringToLower(key);
    for (string line : sectionLines)
    {
        if (stringGetFirstToken(line) == key)
            return true;
    }
    return false;
}

bool getStringValueForKeyInSection(string key, std::string &value, vector<string> sectionLines)
{
    key = stringToLower(key);
    for (string line : sectionLines)
    {
        if (stringGetFirstToken(line) == key)
        {
            value = stringGetLastToken(line);
            return true;
        }
    }
    return false;
}

bool getIntValueForKeyInSection(string key, int &value, vector<string> sectionLines)
{
    string val;
    if (!getStringValueForKeyInSection(key, val, sectionLines))
        return false;
    try
    {
        value = stoi(val);
    }
    catch (...)
    {
        cout << endl << "Error: not a number: " << val;
        return false;
    }
    return true;
}

//ini parsing
std::string stringGetFirstToken(std::string line)
{
    size_t idx = line.find_first_of(" ");
    if (idx == std::string::npos)
        idx = line.length();
    return line.substr(0, idx);
}
std::string stringGetLastToken(std::string line)
{
    return line.substr(line.find_last_of(' ') + 1);
}

// parse "A B"
bool parseTwoTokenMapping(std::string line, unsigned char &keyIn, unsigned char &keyOut, std::string scLabels[])
{
    string a = stringToUpper(stringGetFirstToken(line));
    string b = stringToUpper(stringGetLastToken(line));
    keyIn = getScancode(a, scLabels);
    keyOut = getScancode(b, scLabels);
    if ((keyIn == 0 && a != "nop") || (keyOut == 0 && b != "nop"))
        return false; //invalid key label
    return true;
}

// parse "a b c MAPTO x y z"
bool parseMapFromTo(std::string mapFromTo, unsigned char (&alphamap)[256], std::string scLabels[])
{
    size_t idx1 = mapFromTo.find("mapto");
    if (idx1 == string::npos)
        return false;
    string tmpfrom = (mapFromTo.substr(0, idx1));
    string tmpto = (mapFromTo.substr(idx1 + 5));
    normalizeLine(tmpfrom);
    normalizeLine(tmpto);
    vector<string> sfrom = stringSplit(tmpfrom, ' ');
    vector<string> sto = stringSplit(tmpto, ' ');

    if (sfrom.size() == 0 || sfrom.size() != sto.size())
    {
        cout << endl << "FROM and TO lists are different size";
        return false;
    }

    for (int i = 0; i < sfrom.size(); i++)
    {
        unsigned char cfrom = getScancode(sfrom[i], scLabels);
        unsigned char cto = getScancode(sto[i], scLabels);
        if ((cfrom == 0 && sfrom[i] != "nop")
            || (cto == 0 && sto[i] != "nop"))
        {
            cout << endl << "Unknown scancode labels: " << sfrom[i] << " and " << sto[i];
            return false;
        }
        alphamap[cfrom] = cto;
    }
    return true;
}

// parse "A B C"
bool parseThreeTokenMapping(std::string line, unsigned char &keyA, unsigned char &keyB, unsigned char &keyC, std::string scLabels[])
{
    vector<string> labels = stringSplit(line, ' ');
    if (labels.size() != 3)
        return false;

    keyA = getScancode(labels[0], scLabels);
    keyB = getScancode(labels[1], scLabels);
    keyC = getScancode(labels[2], scLabels);
    if ( (keyA == 0 && labels[0] != "nop") ||
         (keyB == 0 && labels[1] != "nop") ||
         (keyC == 0 && labels[2] != "nop")
        )
        return false; //invalid key label
    return true;
}

//convert ("xy^_v.", 'v') to 000010
unsigned short parseModString(string modString, char filter)
{
    string binString = "";
    for (int i = 0; i < modString.length(); i++)
    {
        if (modString[i] == filter)
            binString += '1';
        else
            binString += '0';
    }
    return std::stoi(binString, nullptr, 2);
}

bool parseCombo(std::string &funcParams, std::string * scLabels, std::vector<KeyEvent> &strokeSeq)
{
    vector<string> labels = stringSplit(funcParams, '+');
    unsigned short sc;
    for (string label : labels)
    {
        sc = getScancode(label, scLabels);
        if (sc == 0)
            return false;
        strokeSeq.push_back({ sc, true });
    }
    size_t len = strokeSeq.size();
    for (size_t i = len; i > 0; i--)	//copy upstrokes in reverse order
        strokeSeq.push_back({ strokeSeq.at(i - 1).scancode,false });
    return true;
}

//parse H  [^^^v .--. ....] > function(param)
bool parseModCombo(std::string line, unsigned short &key, unsigned short(&mods)[3], std::vector<KeyEvent> &strokeSequence, std::string scLabels[])
{
    string strkey = stringGetFirstToken(line);
    if (strkey.length() < 1)
        return false;
    line = line.substr(strkey.length());

    unsigned short tmpKey = getScancode(strkey, scLabels);
    if (tmpKey == 0)
        return false;
    key = tmpKey;

    line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

    size_t modIdx1 = line.find_first_of('[') + 1;
    size_t modIdx2 = line.find_first_of(']');
    if (modIdx1 < 1 || modIdx2 < 2 || modIdx1 > modIdx2)
        return false;
    string mod = line.substr(modIdx1, modIdx2 - modIdx1);

    mods[0] = parseModString(mod, 'v'); //and 
    mods[1] = parseModString(mod, '!'); //not 
    mods[2] = parseModString(mod, 't'); //tap

    //extract function name + param
    size_t funcIdx1 = line.find_first_of('>') + 1;
    if (funcIdx1 < 2)
        return false;
    size_t funcIdx2 = line.find_first_of('(');
    if (funcIdx2 < funcIdx1 + 2)
        return false;
    size_t funcIdx3 = line.find_first_of(')');
    if (funcIdx3 < funcIdx2 + 2)
        return false;
    string funcName = line.substr(funcIdx1, funcIdx2 - funcIdx1);
    funcIdx2++;
    string funcParams = line.substr(funcIdx2, funcIdx3 - funcIdx2);

    //translate 'function' into a char sequence
    vector<KeyEvent> strokeSeq;
    if (funcName == "key")
    {
        unsigned short sc = getScancode(funcParams, scLabels);
        if (sc == 0)
            return false;
        strokeSeq.push_back({ sc, true });
        strokeSeq.push_back({ sc, false });
    }
    else if (funcName == "combo")
    {
        if (!parseCombo(funcParams, scLabels, strokeSeq))
            return false;
    }
    else if (funcName == "combontimes")
    {
        vector<string> comboTimes = stringSplit(funcParams, ',');
        if (comboTimes.size() != 2)
            return false;
        if (!parseCombo(comboTimes.at(0), scLabels, strokeSeq))
            return false;
        int times = stoi(comboTimes.at(1));
        auto len = strokeSeq.size();
        for (int j = 1; j < times; j++)
            for (int i = 0; i < len; i++)
                strokeSeq.push_back(strokeSeq.at(i));
    }
    else if (funcName == "altchar")
    {
        strokeSeq.push_back({ SC_CPS_ESC, true }); //temp release LSHIFT if it is currently down
        strokeSeq.push_back({ BITMASK_LSHIFT | BITMASK_RSHIFT | BITMASK_LCTRL | BITMASK_RCTRL , false });
        strokeSeq.push_back({ BITMASK_RALT , true });
        strokeSeq.push_back({ SC_CPS_ESC, false });
        for (int i = 0; i < funcParams.length(); i++)
        {
            char c = funcParams[i];
            if (c < '0' || c > '9')
                return false;
            string temp = "NP";
            temp += c;
            unsigned short sc = getScancode(temp, scLabels);
            if (sc == 0)
                return false;
            strokeSeq.push_back({ sc, true });
            strokeSeq.push_back({ sc, false });
        }
        strokeSeq.push_back({ SC_CPS_ESC, false });
    }
    else if (funcName == "moddedkey")
    {
        vector<string> modKeyParams = stringSplit(funcParams, '&');
        if (modKeyParams.size() != 2)
            return false;
        unsigned short sc = getScancode(modKeyParams[0], scLabels);
        if (sc == SC_NOP)
            return false;

        unsigned short modsPress = parseModString(modKeyParams[1], 'v'); //and (press if up)
        unsigned short modsRelease = parseModString(modKeyParams[1], '!'); //not (release if down)

        strokeSeq.push_back({ SC_CPS_ESC, true });
        if (modsPress > 0)
            strokeSeq.push_back({ modsPress, true });
        if (modsRelease > 0)
            strokeSeq.push_back({ modsRelease, false });
        strokeSeq.push_back({ SC_CPS_ESC, false });
        strokeSeq.push_back({ sc, true });
        strokeSeq.push_back({ sc, false });
        strokeSeq.push_back({ SC_CPS_ESC, false }); //second UP does UNDO the temp mod changes
    }
    else if (funcName == "type")
    {

    }
    else
        return false;

    strokeSequence = strokeSeq;
    return true;
}