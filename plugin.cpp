#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>
#include <shlobj.h>
#include <windows.h>
#include <knownfolders.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

static constexpr const char* PLUGIN_VERSION = "2.4.9";

const std::vector<std::string> UBE_RACES = {
    "00UBE_HighElfRace",
    "00UBE_BretonRace",
    "00UBE_ImperialRace",
    "00UBE_RedguardRace",
    "00UBE_DarkElfRace",
    "00UBE_WoodElfRace",
    "00UBE_NordRace",
    "00UBE_OrcRace",
    "00UBE_ElderRace",
    "00UBE_KhajiitRace",
    "00UBE_ArgonianRace",
    "00UBE_OrcRaceVampire",
    "00UBE_HighElfRaceVampire",
    "00UBE_BretonRaceVampire",
    "00UBE_ImperialRaceVampire",
    "00UBE_RedguardRaceVampire",
    "00UBE_DarkElfRaceVampire",
    "00UBE_WoodElfRaceVampire",
    "00UBE_NordRaceVampire",
    "00UBE_ElderRaceVampire",
    "00UBE_KhajiitRaceVampire",
    "00UBE_ArgonianRaceVampire"
};

const std::vector<std::string> HIMBO_RACES = {
    "NordRace",
    "ImperialRace",
    "BretonRace",
    "RedguardRace",
    "DarkElfRace",
    "HighElfRace",
    "WoodElfRace",
    "OrcRace",
    "KhajiitRace",
    "ArgonianRace",
    "ElderRace",
    "NordRaceVampire",
    "ImperialRaceVampire",
    "BretonRaceVampire",
    "RedguardRaceVampire",
    "DarkElfRaceVampire",
    "HighElfRaceVampire",
    "WoodElfRaceVampire",
    "OrcRaceVampire",
    "KhajiitRaceVampire",
    "ArgonianRaceVampire",
    "ElderRaceVampire"
};

const std::vector<std::string> EXCLUDED_FROM_UBE_RACES = {
    "- Zeroed Sliders -",
    "-Zeroed Sliders-",
    "Zeroed Sliders",
    "HIMBO Zero for OBody"
};

const std::vector<std::string> PROTECTED_FROM_CLEANING = {
    "- Zeroed Sliders -",
    "-Zeroed Sliders-",
    "Zeroed Sliders",
    "HIMBO Zero for OBody",
    "LS Force Naked",
    "OBody Nude 32",
    "ElderRace"
};

const std::vector<std::string> SPECIAL_BLACKLIST_TYPES = {
    "blacklistedNpcs",
    "blacklistedNpcsPluginFemale",
    "blacklistedNpcsPluginMale",
    "blacklistedRacesFemale",
    "blacklistedRacesMale",
    "blacklistedPresetsFromRandomDistribution"
};

const std::vector<std::string> OUTFIT_ARRAY_TYPES = {
    "blacklistedOutfitsFromORefit",
    "blacklistedOutfitsFromORefitPlugin",
    "outfitsForceRefit"
};

const std::vector<std::string> OUTFIT_FORMID_TYPES = {
    "blacklistedOutfitsFromORefitFormID",
    "outfitsForceRefitFormID"
};

const std::vector<std::string> NPC_FORMID_TYPES = {
    "blacklistedNpcsFormID"
};

const std::vector<std::string> EXCEPTION_3BA_SET_NAMES = {
    "CBBE 3BBB Body Amazing UBE Anus",
    "CBBE 3BBB Body Amazing UBE Anus 150%",
    "3BBB Collision Armor Amazing UBE Anus",
    "SE 3BBB Body Amazing UBE Anus",
    "SE 3BBB Body Amazing UBE Anus 150%"
};

const std::vector<std::string> UBE_SET_IDENTIFIERS = {
    "UBE SE 2.0",
    "UBE SE",
    "UBE 2.0",
    "UBE Vanilla",
    "UBE BHUNP"
};

enum class INIRuleMode {
    STANDARD = -1,
    DISABLED = 0,
    ONCE = 1,
    REMOVE_ONCE = -2,
    REMOVE_ALWAYS = -3,
    EXCLUSIVE_ALWAYS = -4,
    ORGANIZE_REMOVE_ONCE = -6,
    ORGANIZE_EXCLUSIVE_ONCE = -7,
    KEYWORD = 8,
    KEYWORD_EXCLUSIVE = 9,
    KEYWORD_REMOVE = 10,
    KEYWORDCHART = 11,
    KEYWORDCHART_EXCLUSIVE = 12,
    KEYWORDCHART_REMOVE = 13,
    KEYAUTHOR = 14,
    KEYAUTHOR_EXCLUSIVE = 15,
    KEYAUTHOR_REMOVE = 16,
    KEYNORMAL = 17,
    KEYNORMAL_EXCLUSIVE = 18,
    KEYNORMAL_REMOVE = 19,
    KEYUBE = 20,
    KEYUBE_EXCLUSIVE = 21,
    KEYUBE_REMOVE = 22,
    KEYHIMBO = 23,
    KEYHIMBO_EXCLUSIVE = 24,
    KEYHIMBO_REMOVE = 25,
    KEYWORD_ONCE = 26,
    KEYWORD_EXCLUSIVE_ONCE = 27,
    KEYWORD_REMOVE_ONCE = 28,
    KEYWORDCHART_ONCE = 29,
    KEYWORDCHART_EXCLUSIVE_ONCE = 30,
    KEYWORDCHART_REMOVE_ONCE = 31,
    KEYAUTHOR_ONCE = 32,
    KEYAUTHOR_EXCLUSIVE_ONCE = 33,
    KEYAUTHOR_REMOVE_ONCE = 34,
    KEYNORMAL_ONCE = 35,
    KEYNORMAL_EXCLUSIVE_ONCE = 36,
    KEYNORMAL_REMOVE_ONCE = 37,
    KEYUBE_ONCE = 38,
    KEYUBE_EXCLUSIVE_ONCE = 39,
    KEYUBE_REMOVE_ONCE = 40,
    KEYHIMBO_ONCE = 41,
    KEYHIMBO_EXCLUSIVE_ONCE = 42,
    KEYHIMBO_REMOVE_ONCE = 43
};

bool EndsWith(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool StartsWith(const std::string& str, const std::string& prefix) {
    if (prefix.size() > str.size()) return false;
    return str.compare(0, prefix.size(), prefix) == 0;
}

std::string SafeWideStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    
    try {
        int size_needed = WideCharToMultiByte(
            CP_UTF8,
            0,
            wstr.c_str(),
            static_cast<int>(wstr.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );
        
        if (size_needed > 0) {
            std::string result(size_needed, 0);
            int converted = WideCharToMultiByte(
                CP_UTF8,
                0,
                wstr.c_str(),
                static_cast<int>(wstr.size()),
                &result[0],
                size_needed,
                nullptr,
                nullptr
            );
            
            if (converted > 0) {
                return result;
            }
        }
        
        size_needed = WideCharToMultiByte(
            CP_ACP,
            0,
            wstr.c_str(),
            static_cast<int>(wstr.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );
        
        if (size_needed > 0) {
            std::string result(size_needed, 0);
            int converted = WideCharToMultiByte(
                CP_ACP,
                0,
                wstr.c_str(),
                static_cast<int>(wstr.size()),
                &result[0],
                size_needed,
                nullptr,
                nullptr
            );
            
            if (converted > 0) {
                return result;
            }
        }
        
        std::string result;
        result.reserve(wstr.size());
        for (wchar_t wc : wstr) {
            if (wc <= 127) {
                result.push_back(static_cast<char>(wc));
            } else {
                result.push_back('?');
            }
        }
        return result;
        
    } catch (...) {
        std::string result;
        result.reserve(wstr.size());
        for (wchar_t wc : wstr) {
            if (wc <= 127) {
                result.push_back(static_cast<char>(wc));
            } else {
                result.push_back('?');
            }
        }
        return result;
    }
}

std::string GetEnvVar(const std::string& key) {
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, key.c_str()) == 0 && buf != nullptr) {
        std::string value(buf);
        free(buf);
        return value;
    }
    if (buf) free(buf);
    return "";
}

std::string GetDocumentsPath() {
    try {
        wchar_t* path = nullptr;
        HRESULT hr = SHGetKnownFolderPath(
            FOLDERID_Documents,
            0,
            nullptr,
            &path
        );
        
        if (SUCCEEDED(hr) && path != nullptr) {
            std::wstring ws(path);
            CoTaskMemFree(path);
            std::string converted = SafeWideStringToString(ws);
            if (!converted.empty()) {
                return converted;
            }
        }
        
        wchar_t pathBuffer[MAX_PATH] = {0};
        HRESULT result = SHGetFolderPathW(
            nullptr,
            CSIDL_PERSONAL,
            nullptr,
            SHGFP_TYPE_CURRENT,
            pathBuffer
        );
        
        if (SUCCEEDED(result)) {
            std::wstring ws(pathBuffer);
            std::string converted = SafeWideStringToString(ws);
            if (!converted.empty()) {
                return converted;
            }
        }
        
        std::string userProfile = GetEnvVar("USERPROFILE");
        if (!userProfile.empty()) {
            return userProfile + "\\Documents";
        }
        
        std::string homeDrive = GetEnvVar("HOMEDRIVE");
        std::string homePath = GetEnvVar("HOMEPATH");
        if (!homeDrive.empty() && !homePath.empty()) {
            return homeDrive + homePath + "\\Documents";
        }
        
        return "C:\\Users\\Default\\Documents";
        
    } catch (...) {
        return "C:\\Users\\Default\\Documents";
    }
}

std::string GetGamePath() {
    try {
        std::string mo2Path = GetEnvVar("MO2_MODS_PATH");
        if (!mo2Path.empty()) return mo2Path;

        std::string vortexPath = GetEnvVar("VORTEX_MODS_PATH");
        if (!vortexPath.empty()) return vortexPath;

        std::string skyrimMods = GetEnvVar("SKYRIM_MODS_FOLDER");
        if (!skyrimMods.empty()) return skyrimMods;

        std::vector<std::pair<std::string, std::string>> registryKeys = {
            {"SOFTWARE\\WOW6432Node\\Bethesda Softworks\\Skyrim Special Edition", "Installed Path"},
            {"SOFTWARE\\WOW6432Node\\GOG.com\\Games\\1457087920", "path"},
            {"SOFTWARE\\WOW6432Node\\Valve\\Steam\\Apps\\489830", "InstallLocation"},
            {"SOFTWARE\\WOW6432Node\\Valve\\Steam\\Apps\\611670", "InstallLocation"}
        };

        HKEY hKey;
        char pathBuffer[MAX_PATH] = {0};
        DWORD pathSize = sizeof(pathBuffer);

        for (const auto& [key, valueName] : registryKeys) {
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                if (RegQueryValueExA(hKey, valueName.c_str(), NULL, NULL, (LPBYTE)pathBuffer, &pathSize) ==
                    ERROR_SUCCESS) {
                    RegCloseKey(hKey);
                    std::string result(pathBuffer);
                    if (!result.empty()) return result;
                }
                RegCloseKey(hKey);
            }
            pathSize = sizeof(pathBuffer);
        }

        std::vector<std::string> commonPaths = {
            "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special Edition",
            "C:\\Program Files\\Steam\\steamapps\\common\\Skyrim Special Edition",
            "D:\\Steam\\steamapps\\common\\Skyrim Special Edition",
            "E:\\Steam\\steamapps\\common\\Skyrim Special Edition",
            "F:\\Steam\\steamapps\\common\\Skyrim Special Edition",
            "G:\\Steam\\steamapps\\common\\Skyrim Special Edition",
            "C:\\GOG Games\\Skyrim Special Edition",
            "D:\\GOG Games\\Skyrim Special Edition"
        };

        for (const auto& pathCandidate : commonPaths) {
            try {
                if (fs::exists(pathCandidate) && fs::is_directory(pathCandidate)) {
                    return pathCandidate;
                }
            } catch (...) {
                continue;
            }
        }

        return "";
    } catch (...) {
        return "";
    }
}

void CreateDirectoryIfNotExists(const fs::path& path) {
    try {
        if (!fs::exists(path)) {
            fs::create_directories(path);
        }
    } catch (...) {
    }
}

std::string ReadFileWithEncoding(const fs::path& filepath) {
    try {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return "";
        }

        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();
        if (fileSize <= 0) {
            file.close();
            return "";
        }
        
        file.seekg(0, std::ios::beg);
        std::string content;
        content.resize(static_cast<size_t>(fileSize));
        
        if (!file.read(&content[0], fileSize)) {
            file.close();
            return "";
        }
        
        file.close();

        if (content.size() >= 3 &&
            static_cast<unsigned char>(content[0]) == 0xEF &&
            static_cast<unsigned char>(content[1]) == 0xBB &&
            static_cast<unsigned char>(content[2]) == 0xBF) {
            content = content.substr(3);
        }
        else if (content.size() >= 2 &&
                 static_cast<unsigned char>(content[0]) == 0xFF &&
                 static_cast<unsigned char>(content[1]) == 0xFE) {
            content = content.substr(2);
        }
        else if (content.size() >= 2 &&
                 static_cast<unsigned char>(content[0]) == 0xFE &&
                 static_cast<unsigned char>(content[1]) == 0xFF) {
            content = content.substr(2);
        }

        std::string normalized;
        normalized.reserve(content.size());
        
        for (size_t i = 0; i < content.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(content[i]);
            
            if (c == '\r') {
                if (i + 1 < content.size() && content[i + 1] == '\n') {
                    normalized += '\n';
                    ++i;
                } else {
                    normalized += '\n';
                }
            }
            else if (c < 128) {
                normalized += c;
            }
            else if ((c & 0xE0) == 0xC0) {
                if (i + 1 < content.size()) {
                    unsigned char c2 = static_cast<unsigned char>(content[i + 1]);
                    if ((c2 & 0xC0) == 0x80) {
                        int codepoint = ((c & 0x1F) << 6) | (c2 & 0x3F);
                        if (codepoint == 0x2018 || codepoint == 0x2019) {
                            normalized += '\'';
                            i += 1;
                            continue;
                        }
                    }
                }
                normalized += c;
            }
            else if ((c & 0xF0) == 0xE0) {
                if (i + 2 < content.size()) {
                    unsigned char c2 = static_cast<unsigned char>(content[i + 1]);
                    unsigned char c3 = static_cast<unsigned char>(content[i + 2]);
                    if ((c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80) {
                        int codepoint = ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
                        
                        if (codepoint == 0x2018 || codepoint == 0x2019) {
                            normalized += '\'';
                            i += 2;
                            continue;
                        }
                        else if (codepoint == 0x201C || codepoint == 0x201D) {
                            normalized += '"';
                            i += 2;
                            continue;
                        }
                        else if (codepoint == 0x2014) {
                            normalized += '-';
                            i += 2;
                            continue;
                        }
                    }
                }
                normalized += c;
            }
            else {
                normalized += c;
            }
        }

        return normalized;

    } catch (const std::exception& e) {
        return "";
    } catch (...) {
        return "";
    }
}

std::string Trim(const std::string& str) {
    if (str.empty()) return str;
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    if (str.empty()) return tokens;

    std::stringstream ss(str);
    std::string token;
    tokens.reserve(20);

    while (std::getline(ss, token, delimiter)) {
        std::string trimmed = Trim(token);
        if (!trimmed.empty()) {
            tokens.push_back(std::move(trimmed));
        }
    }
    return tokens;
}

std::string EscapeJson(const std::string& str) {
    std::string result;
    result.reserve(str.length() * 1.3);

    for (char c : str) {
        switch (c) {
            case '"':
                result += "\\\"";
                break;
            case '\\':
                result += "\\\\";
                break;
            case '\b':
                result += "\\b";
                break;
            case '\f':
                result += "\\f";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                if (c >= 0x20 && c <= 0x7E) {
                    result += c;
                } else {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                }
        }
    }
    return result;
}

std::string ToLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string StripLeadingTrailingSymbols(const std::string& str) {
    if (str.empty()) return str;
    
    size_t start = 0;
    size_t end = str.length();
    
    if (!str.empty() && !std::isalnum(static_cast<unsigned char>(str[0])) && str[0] != ' ') {
        start = 1;
    }
    
    if (end > start && !std::isalnum(static_cast<unsigned char>(str[end - 1])) && str[end - 1] != ' ') {
        end--;
    }
    
    if (start >= end) return str;
    
    return str.substr(start, end - start);
}

std::string NormalizePresetName(const std::string& name) {
    std::string normalized;
    normalized.reserve(name.length());
    
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            normalized += std::tolower(static_cast<unsigned char>(c));
        }
    }
    
    return normalized;
}

std::string NormalizePresetNameFlexible(const std::string& name) {
    std::string normalized;
    normalized.reserve(name.length());
    
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            normalized += std::tolower(static_cast<unsigned char>(c));
        }
        else if (c == ' ') {
            normalized += ' ';
        }
        else if (c == '-') {
            normalized += ' ';
        }
    }
    
    std::string result;
    bool lastWasSpace = false;
    for (char c : normalized) {
        if (c == ' ') {
            if (!lastWasSpace) {
                result += c;
                lastWasSpace = true;
            }
        } else {
            result += c;
            lastWasSpace = false;
        }
    }
    
    size_t start = result.find_first_not_of(' ');
    size_t end = result.find_last_not_of(' ');
    if (start == std::string::npos) return "";
    return result.substr(start, end - start + 1);
}

std::string CleanFormID(const std::string& formID, std::ofstream& logFile, bool& wasCleaned) {
    wasCleaned = false;
    
    if (formID.length() <= 6) {
        return formID;
    }
    
    std::string prefix = formID.substr(0, 2);
    std::string upperPrefix = prefix;
    std::transform(upperPrefix.begin(), upperPrefix.end(), upperPrefix.begin(), ::toupper);
    
    const std::set<std::string> validPrefixes = {
        "00","01","02","03","04","05","06","07","08","09","0A","0B","0C","0D","0E","0F",
        "10","11","12","13","14","15","16","17","18","19","1A","1B","1C","1D","1E","1F",
        "20","21","22","23","24","25","26","27","28","29","2A","2B","2C","2D","2E","2F",
        "30","31","32","33","34","35","36","37","38","39","3A","3B","3C","3D","3E","3F",
        "40","41","42","43","44","45","46","47","48","49","4A","4B","4C","4D","4E","4F",
        "50","51","52","53","54","55","56","57","58","59","5A","5B","5C","5D","5E","5F",
        "60","61","62","63","64","65","66","67","68","69","6A","6B","6C","6D","6E","6F",
        "70","71","72","73","74","75","76","77","78","79","7A","7B","7C","7D","7E","7F",
        "80","81","82","83","84","85","86","87","88","89","8A","8B","8C","8D","8E","8F",
        "90","91","92","93","94","95","96","97","98","99","9A","9B","9C","9D","9E","9F",
        "A0","A1","A2","A3","A4","A5","A6","A7","A8","A9","AA","AB","AC","AD","AE","AF",
        "B0","B1","B2","B3","B4","B5","B6","B7","B8","B9","BA","BB","BC","BD","BE","BF",
        "C0","C1","C2","C3","C4","C5","C6","C7","C8","C9","CA","CB","CC","CD","CE","CF",
        "D0","D1","D2","D3","D4","D5","D6","D7","D8","D9","DA","DB","DC","DD","DE","DF",
        "E0","E1","E2","E3","E4","E5","E6","E7","E8","E9","EA","EB","EC","ED","EE","EF",
        "F0","F1","F2","F3","F4","F5","F6","F7","F8","F9","FA","FB","FC","FD",
        "FE","FF"
    };
    
    if (validPrefixes.find(upperPrefix) != validPrefixes.end()) {
        return formID;
    }
    
    if (upperPrefix == "XX") {
        wasCleaned = true;
        std::string cleaned = formID.substr(2);
        logFile << "    FormID cleaned: " << formID << " -> " << cleaned 
                << " (removed XX prefix)" << std::endl;
        return cleaned;
    }
    
    wasCleaned = true;
    std::string cleaned = formID.substr(2);
    logFile << "    FormID cleaned: " << formID << " -> " << cleaned 
            << " (removed invalid prefix: " << prefix << ")" << std::endl;
    return cleaned;
}

std::string DecodeHtmlEntities(const std::string& str) {
    std::string result = str;
    size_t pos = 0;
    
    while ((pos = result.find("&amp;", pos)) != std::string::npos) {
        result.replace(pos, 5, "&");
        pos += 1;
    }
    
    pos = 0;
    while ((pos = result.find("&apos;", pos)) != std::string::npos) {
        result.replace(pos, 6, "'");
        pos += 1;
    }
    
    pos = 0;
    while ((pos = result.find("&quot;", pos)) != std::string::npos) {
        result.replace(pos, 6, "\"");
        pos += 1;
    }
    
    pos = 0;
    while ((pos = result.find("&lt;", pos)) != std::string::npos) {
        result.replace(pos, 4, "<");
        pos += 1;
    }
    
    pos = 0;
    while ((pos = result.find("&gt;", pos)) != std::string::npos) {
        result.replace(pos, 4, ">");
        pos += 1;
    }
    
    return result;
}

bool IsPartOfHtmlEntity(const std::string& str, size_t semicolonPos) {
    if (semicolonPos == 0 || semicolonPos >= str.length()) return false;
    
    size_t searchStart = (semicolonPos > 10) ? semicolonPos - 10 : 0;
    
    for (size_t ampPos = semicolonPos - 1; ampPos >= searchStart && ampPos < semicolonPos; ampPos--) {
        char c = str[ampPos];
        
        if (c == '&') {
            if (semicolonPos - ampPos < 2) return false;
            
            bool isValidEntity = true;
            bool hasContent = false;
            
            for (size_t i = ampPos + 1; i < semicolonPos; i++) {
                char ch = str[i];
                if (i == ampPos + 1 && ch == '#') {
                    hasContent = true;
                    continue;
                }
                if (!std::isalnum(static_cast<unsigned char>(ch))) {
                    isValidEntity = false;
                    break;
                }
                hasContent = true;
            }
            
            if (isValidEntity && hasContent) {
                return true;
            }
        } else if (c == ' ' || c == '\t' || c == '|' || c == ',' || c == '=') {
            break;
        }
        
        if (ampPos == 0) break;
    }
    
    return false;
}

size_t FindCommentPosition(const std::string& str, char commentChar) {
    size_t pos = 0;
    
    while (pos < str.length()) {
        pos = str.find(commentChar, pos);
        
        if (pos == std::string::npos) {
            return std::string::npos;
        }
        
        if (commentChar == ';') {
            if (IsPartOfHtmlEntity(str, pos)) {
                pos++;
                continue;
            }
        }
        
        return pos;
    }
    
    return std::string::npos;
}

std::string RemoveCommentsSafely(const std::string& line) {
    std::string result = line;
    
    size_t semicolonPos = FindCommentPosition(result, ';');
    if (semicolonPos != std::string::npos) {
        result = result.substr(0, semicolonPos);
    }
    
    size_t hashPos = result.find('#');
    if (hashPos != std::string::npos) {
        result = result.substr(0, hashPos);
    }
    
    return result;
}

bool IsPluginName(const std::string& presetName) {
    std::string lowerName = ToLowerCase(presetName);
    return (lowerName.find(".esp") != std::string::npos ||
            lowerName.find(".esm") != std::string::npos ||
            lowerName.find(".esl") != std::string::npos);
}

bool IsUBERace(const std::string& raceName) {
    return StartsWith(raceName, "00UBE_");
}

bool IsValidBlacklistType(const std::string& type) {
    return std::find(SPECIAL_BLACKLIST_TYPES.begin(), SPECIAL_BLACKLIST_TYPES.end(), type) != SPECIAL_BLACKLIST_TYPES.end();
}

std::string GetFullBlacklistKey(const std::string& shortType) {
    if (shortType == "Npcs") return "blacklistedNpcs";
    if (shortType == "NpcsPluginFemale") return "blacklistedNpcsPluginFemale";
    if (shortType == "NpcsPluginMale") return "blacklistedNpcsPluginMale";
    if (shortType == "RacesFemale") return "blacklistedRacesFemale";
    if (shortType == "RacesMale") return "blacklistedRacesMale";
    if (shortType == "PresetsFromRandomDistribution") return "blacklistedPresetsFromRandomDistribution";
    
    if (IsValidBlacklistType(shortType)) return shortType;
    
    return "";
}

bool IsOnceMode(INIRuleMode mode) {
    return mode == INIRuleMode::ONCE ||
           mode == INIRuleMode::ORGANIZE_REMOVE_ONCE ||
           mode == INIRuleMode::ORGANIZE_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYWORD_ONCE ||
           mode == INIRuleMode::KEYWORD_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYWORD_REMOVE_ONCE ||
           mode == INIRuleMode::KEYWORDCHART_ONCE ||
           mode == INIRuleMode::KEYWORDCHART_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYWORDCHART_REMOVE_ONCE ||
           mode == INIRuleMode::KEYAUTHOR_ONCE ||
           mode == INIRuleMode::KEYAUTHOR_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYAUTHOR_REMOVE_ONCE ||
           mode == INIRuleMode::KEYNORMAL_ONCE ||
           mode == INIRuleMode::KEYNORMAL_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYNORMAL_REMOVE_ONCE ||
           mode == INIRuleMode::KEYUBE_ONCE ||
           mode == INIRuleMode::KEYUBE_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYUBE_REMOVE_ONCE ||
           mode == INIRuleMode::KEYHIMBO_ONCE ||
           mode == INIRuleMode::KEYHIMBO_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYHIMBO_REMOVE_ONCE;
}

bool IsExclusiveMode(INIRuleMode mode) {
    return mode == INIRuleMode::EXCLUSIVE_ALWAYS ||
           mode == INIRuleMode::ORGANIZE_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYWORD_EXCLUSIVE ||
           mode == INIRuleMode::KEYWORD_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYWORDCHART_EXCLUSIVE ||
           mode == INIRuleMode::KEYWORDCHART_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYAUTHOR_EXCLUSIVE ||
           mode == INIRuleMode::KEYAUTHOR_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYNORMAL_EXCLUSIVE ||
           mode == INIRuleMode::KEYNORMAL_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYUBE_EXCLUSIVE ||
           mode == INIRuleMode::KEYUBE_EXCLUSIVE_ONCE ||
           mode == INIRuleMode::KEYHIMBO_EXCLUSIVE ||
           mode == INIRuleMode::KEYHIMBO_EXCLUSIVE_ONCE;
}

bool IsRemovalMode(INIRuleMode mode) {
    return mode == INIRuleMode::REMOVE_ALWAYS ||
           mode == INIRuleMode::REMOVE_ONCE ||
           mode == INIRuleMode::ORGANIZE_REMOVE_ONCE ||
           mode == INIRuleMode::KEYWORD_REMOVE ||
           mode == INIRuleMode::KEYWORD_REMOVE_ONCE ||
           mode == INIRuleMode::KEYWORDCHART_REMOVE ||
           mode == INIRuleMode::KEYWORDCHART_REMOVE_ONCE ||
           mode == INIRuleMode::KEYAUTHOR_REMOVE ||
           mode == INIRuleMode::KEYAUTHOR_REMOVE_ONCE ||
           mode == INIRuleMode::KEYNORMAL_REMOVE ||
           mode == INIRuleMode::KEYNORMAL_REMOVE_ONCE ||
           mode == INIRuleMode::KEYUBE_REMOVE ||
           mode == INIRuleMode::KEYUBE_REMOVE_ONCE ||
           mode == INIRuleMode::KEYHIMBO_REMOVE ||
           mode == INIRuleMode::KEYHIMBO_REMOVE_ONCE;
}

struct RuleWithPriority {
    std::string key;
    std::string plugin;
    std::vector<std::string> presets;
    std::string extra;
    int applyCount = -1;
    INIRuleMode mode = INIRuleMode::STANDARD;
    std::vector<std::string> filterFragments;
    std::string originalLine;
    fs::path iniPath;
    int priority;
};

int GetRulePriority(INIRuleMode mode) {
    if (mode == INIRuleMode::STANDARD || mode == INIRuleMode::ONCE || mode == INIRuleMode::DISABLED) {
        return 1;
    }
    
    if (mode == INIRuleMode::KEYWORD || mode == INIRuleMode::KEYWORDCHART || mode == INIRuleMode::KEYAUTHOR ||
        mode == INIRuleMode::KEYNORMAL || mode == INIRuleMode::KEYUBE || mode == INIRuleMode::KEYHIMBO ||
        mode == INIRuleMode::KEYWORD_ONCE || mode == INIRuleMode::KEYWORDCHART_ONCE || mode == INIRuleMode::KEYAUTHOR_ONCE ||
        mode == INIRuleMode::KEYNORMAL_ONCE || mode == INIRuleMode::KEYUBE_ONCE || mode == INIRuleMode::KEYHIMBO_ONCE) {
        return 2;
    }
    
    if (IsRemovalMode(mode)) {
        return 3;
    }
    
    if (IsExclusiveMode(mode) || mode == INIRuleMode::EXCLUSIVE_ALWAYS) {
        return 4;
    }
    
    return 1;
}

struct XmlPresetInfo {
    std::string internalName;
    std::string filename;
    bool extractionSuccessful;
};

struct ParsedRule {
    std::string key;
    std::string plugin;
    std::vector<std::string> presets;
    std::string extra;
    int applyCount = -1;
    INIRuleMode mode = INIRuleMode::STANDARD;
    std::vector<std::string> filterFragments;
};

struct SpecialRule {
    std::string ruleType;
    std::string targetKey;
    std::string plugin;
    std::vector<std::string> presets;
    std::string extra;
    int applyCount = -1;
    INIRuleMode mode = INIRuleMode::STANDARD;
    std::vector<std::string> filterFragments;
};

struct OrderedPluginData {
    std::vector<std::pair<std::string, std::vector<std::string>>> orderedData;

    void addPreset(const std::string& plugin, const std::string& preset) {
        auto it = std::find_if(orderedData.begin(), orderedData.end(),
                               [&plugin](const auto& pair) { return pair.first == plugin; });
        if (it == orderedData.end()) {
            orderedData.emplace_back(plugin, std::vector<std::string>{preset});
            orderedData.back().second.reserve(20);
        } else {
            auto& presets = it->second;
            if (std::find(presets.begin(), presets.end(), preset) == presets.end()) {
                presets.push_back(preset);
            }
        }
    }

    void removePreset(const std::string& plugin, const std::string& preset) {
        auto it = std::find_if(orderedData.begin(), orderedData.end(),
                               [&plugin](const auto& pair) { return pair.first == plugin; });
        if (it != orderedData.end()) {
            auto& presets = it->second;
            auto presetIt = std::find_if(presets.begin(), presets.end(), [&preset](const std::string& p) {
                std::string strippedP = p;
                if (!strippedP.empty() && strippedP[0] == '!') {
                    strippedP = strippedP.substr(1);
                }
                std::string strippedTarget = preset;
                if (!strippedTarget.empty() && strippedTarget[0] == '!') {
                    strippedTarget = strippedTarget.substr(1);
                }
                return strippedP == strippedTarget;
            });
            if (presetIt != presets.end()) {
                presets.erase(presetIt);
                if (presets.empty()) {
                    orderedData.erase(it);
                }
            }
        }
    }

    void removePlugin(const std::string& plugin) {
        auto it = std::find_if(orderedData.begin(), orderedData.end(),
                               [&plugin](const auto& pair) { return pair.first == plugin; });
        if (it != orderedData.end()) {
            orderedData.erase(it);
        }
    }

    bool hasPlugin(const std::string& plugin) const {
        return std::any_of(orderedData.begin(), orderedData.end(),
                           [&plugin](const auto& pair) { return pair.first == plugin; });
    }

    size_t getPluginCount() const { return orderedData.size(); }

    size_t getTotalPresetCount() const {
        size_t count = 0;
        for (const auto& [plugin, presets] : orderedData) {
            count += presets.size();
        }
        return count;
    }
};

struct NpcFormIDData {
    std::map<std::string, std::map<std::string, std::vector<std::string>>> data;
    
    void addPresetToFormID(const std::string& plugin, const std::string& formID, const std::string& preset) {
        data[plugin][formID].push_back(preset);
    }
    
    void removePresetFromFormID(const std::string& plugin, const std::string& formID, const std::string& preset) {
        auto pluginIt = data.find(plugin);
        if (pluginIt != data.end()) {
            auto formIDIt = pluginIt->second.find(formID);
            if (formIDIt != pluginIt->second.end()) {
                auto& presets = formIDIt->second;
                auto presetIt = std::find(presets.begin(), presets.end(), preset);
                if (presetIt != presets.end()) {
                    presets.erase(presetIt);
                    if (presets.empty()) {
                        pluginIt->second.erase(formIDIt);
                        if (pluginIt->second.empty()) {
                            data.erase(pluginIt);
                        }
                    }
                }
            }
        }
    }
    
    bool isEmpty() const {
        return data.empty();
    }
    
    size_t getTotalPresetCount() const {
        size_t count = 0;
        for (const auto& [plugin, formIDs] : data) {
            for (const auto& [formID, presets] : formIDs) {
                count += presets.size();
            }
        }
        return count;
    }
};

struct ConfigSettings {
    int backupValue = 1;
    bool modeUBE = true;
    bool modeHIMBO = true;
    bool presetsSmartCleaning = false;
    bool blacklistedPresetsSmartCleaningFromRandomDistribution = false;
    bool blacklistedPresetsSmartCleaningFromAll = false;
    bool outfitsForceReSmartCleaning = false;
    bool conflictSmartResolution = true;
};

struct XmlAnalysisResult {
    bool hasUBE = false;
    bool hasHIMBO = false;
    bool hasConflictingGroups = false;
    std::vector<std::string> conflictingGroupsFound;
    std::string setAttributeValue = "";
    bool setAnalysisPerformed = false;
    bool isException3BA = false;
    bool isUBEBySet = false;
};

struct UBEPresetInfo {
    std::string presetName;
    bool allowedInRaces;
    bool hasConflict;
    std::vector<std::string> conflictingGroups;
};

struct HIMBOPresetInfo {
    std::string presetName;
    bool allowedInRaces;
    bool hasConflict;
    std::vector<std::string> conflictingGroups;
};

struct PresetMatchResult {
    bool found = false;
    std::string actualPresetName = "";
    int matchLevel = 0;
};

struct PresetMapData {
    std::map<std::string, std::string> exactMap;
    std::map<std::string, std::string> normalizedMap;
    std::map<std::string, std::string> filenameToInternalMap;
    std::set<std::string> allValidNames;
    std::set<std::string> ubePresetNames;
    std::set<std::string> himboPresetNames;
};

struct ConflictRuleInfo {
    std::string filename;
    fs::path fullPath;
    std::string originalLine;
    std::string ruleKey;
    std::string target;
    fs::file_time_type fileModTime;
    INIRuleMode mode;
};

struct RuleConflictTracker {
    std::map<std::string, std::vector<ConflictRuleInfo>> exclusiveRules;
    
    void addExclusiveRule(const std::string& key, const std::string& target, 
                         const std::string& filename, const fs::path& fullPath,
                         const std::string& rule, INIRuleMode mode) {
        std::string conflictKey = key + "|" + target;
        
        ConflictRuleInfo info;
        info.filename = filename;
        info.fullPath = fullPath;
        info.originalLine = rule;
        info.ruleKey = key;
        info.target = target;
        info.mode = mode;
        
        try {
            info.fileModTime = fs::last_write_time(fullPath);
        } catch (...) {
            info.fileModTime = fs::file_time_type::min();
        }
        
        exclusiveRules[conflictKey].push_back(info);
    }
    
    bool hasConflict(const std::string& key, const std::string& target) const {
        std::string conflictKey = key + "|" + target;
        auto it = exclusiveRules.find(conflictKey);
        return it != exclusiveRules.end() && it->second.size() > 1;
    }
    
    std::vector<ConflictRuleInfo> getConflictingRules(
        const std::string& key, const std::string& target) const {
        std::string conflictKey = key + "|" + target;
        auto it = exclusiveRules.find(conflictKey);
        return it != exclusiveRules.end() ? it->second : std::vector<ConflictRuleInfo>();
    }
};

struct ConflictResolution {
    std::map<std::string, bool> ruleIsAllowed;
    std::map<std::string, ConflictRuleInfo> dominantRules;
};

struct NpcFormIDRule {
    std::string plugin;
    std::string formID;
    std::vector<std::string> presets;
    std::string extra;
    int applyCount = -1;
    INIRuleMode mode = INIRuleMode::STANDARD;
};

bool FindFileWithFallback(const fs::path& basePath, const std::string& filename, 
                          fs::path& foundPath, std::ofstream& logFile) {
    try {
        fs::path normalPath = basePath / filename;
        
        if (fs::exists(normalPath)) {
            foundPath = normalPath;
            return true;
        }
        
        std::string basePathStr = basePath.string();
        
        if (!basePathStr.empty() && basePathStr.back() != '\\') {
            basePathStr += '\\';
        }
        
        basePathStr += '\\';
        
        basePathStr += filename;
        
        fs::path doubleBackslashPath(basePathStr);
        
        try {
            doubleBackslashPath = fs::canonical(doubleBackslashPath);
            
            if (fs::exists(doubleBackslashPath)) {
                foundPath = doubleBackslashPath;
                return true;
            }
        } catch (...) {
        }

        if (fs::exists(basePath) && fs::is_directory(basePath)) {
            std::string lowerFilename = filename;
            std::transform(lowerFilename.begin(), lowerFilename.end(), 
                         lowerFilename.begin(), ::tolower);
            
            for (const auto& entry : fs::directory_iterator(basePath)) {
                try {
                    std::string entryFilename = entry.path().filename().string();
                    
                    std::string lowerEntryFilename = entryFilename;
                    std::transform(lowerEntryFilename.begin(), lowerEntryFilename.end(), 
                                 lowerEntryFilename.begin(), ::tolower);
                    
                    if (lowerEntryFilename == lowerFilename) {
                        foundPath = entry.path();
                        return true;
                    }
                } catch (...) {
                    continue;
                }
            }
        }
        
        return false;
        
    } catch (const std::exception& e) {
        return false;
    } catch (...) {
        return false;
    }
}

fs::path BuildPathCaseInsensitive(const fs::path& basePath, 
                                  const std::vector<std::string>& components, 
                                  std::ofstream& logFile) {
    try {
        fs::path currentPath = basePath;
        
        for (const auto& component : components) {
            fs::path testPath = currentPath / component;
            
            if (fs::exists(testPath)) {
                currentPath = testPath;
                continue;
            }
            
            std::string lowerComponent = component;
            std::transform(lowerComponent.begin(), lowerComponent.end(), 
                         lowerComponent.begin(), ::tolower);
            
            testPath = currentPath / lowerComponent;
            
            if (fs::exists(testPath)) {
                currentPath = testPath;
                continue;
            }
            
            std::string upperComponent = component;
            std::transform(upperComponent.begin(), upperComponent.end(), 
                         upperComponent.begin(), ::toupper);
            
            testPath = currentPath / upperComponent;
            
            if (fs::exists(testPath)) {
                currentPath = testPath;
                continue;
            }
            
            bool found = false;
            
            if (fs::exists(currentPath) && fs::is_directory(currentPath)) {
                for (const auto& entry : fs::directory_iterator(currentPath)) {
                    try {
                        std::string entryName = entry.path().filename().string();
                        std::string lowerEntryName = entryName;
                        std::transform(lowerEntryName.begin(), lowerEntryName.end(), 
                                     lowerEntryName.begin(), ::tolower);
                        
                        if (lowerEntryName == lowerComponent) {
                            currentPath = entry.path();
                            found = true;
                            break;
                        }
                    } catch (...) {
                        continue;
                    }
                }
            }
            
            if (!found) {
                currentPath = currentPath / component;
            }
        }
        
        return currentPath;
        
    } catch (const std::exception& e) {
        return basePath;
    } catch (...) {
        return basePath;
    }
}

bool IsValidPluginPath(const fs::path& pluginPath, std::ofstream& logFile) {
    const std::vector<std::string> dllNames = {
        "Act1_OBody_NG_PDA_NG.dll",
        "OBody_NG_PDA_NG.dll",
        "OBody_PDA.dll"
    };
    
    for (const auto& dllName : dllNames) {
        fs::path foundPath;
        
        if (FindFileWithFallback(pluginPath, dllName, foundPath, logFile)) {
            return true;
        }
    }
    
    return false;
}

fs::path GetDllDirectory(std::ofstream& logFile) {
    try {
        HMODULE hModule = nullptr;

        static int dummyVariable = 0;

        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(&dummyVariable), &hModule) &&
            hModule != nullptr) {
            wchar_t dllPath[MAX_PATH] = {0};
            DWORD size = GetModuleFileNameW(hModule, dllPath, MAX_PATH);

            if (size > 0) {
                std::wstring wsDllPath(dllPath);
                std::string dllPathStr = SafeWideStringToString(wsDllPath);

                if (!dllPathStr.empty()) {
                    fs::path dllDir = fs::path(dllPathStr).parent_path();
                    return dllDir;
                }
            }
        }

        return fs::path();

    } catch (const std::exception& e) {
        return fs::path();
    } catch (...) {
        return fs::path();
    }
}

std::vector<XmlPresetInfo> ExtractAllPresetsFromXml(const fs::path& xmlPath, std::ofstream& logFile) {
    std::vector<XmlPresetInfo> allPresets;
    
    try {
        std::string filename;
        try {
            filename = xmlPath.stem().string();
        } catch (...) {
            try {
                auto u8name = xmlPath.stem().u8string();
                filename = std::string(u8name.begin(), u8name.end());
            } catch (...) {
                filename = "unknown";
                logFile << "  ERROR: Could not read filename from path" << std::endl;
                return allPresets;
            }
        }
        
        if (!fs::exists(xmlPath)) {
            return allPresets;
        }
        
        std::string content = ReadFileWithEncoding(xmlPath);
        if (content.empty()) {
            return allPresets;
        }
        
        size_t searchPos = 0;
        int presetCount = 0;
        
        while (searchPos < content.length()) {
            size_t presetPos = content.find("<Preset", searchPos);
            if (presetPos == std::string::npos) {
                presetPos = content.find("<preset", searchPos);
            }
            
            if (presetPos == std::string::npos) {
                break;
            }
            
            size_t namePos = content.find("name=", presetPos);
            if (namePos == std::string::npos || namePos > presetPos + 200) {
                searchPos = presetPos + 7;
                continue;
            }
            
            size_t quoteStart = namePos + 5;
            
            while (quoteStart < content.length() && 
                   (content[quoteStart] == ' ' || content[quoteStart] == '\t')) {
                quoteStart++;
            }
            
            if (quoteStart >= content.length()) {
                searchPos = presetPos + 7;
                continue;
            }
            
            char quoteChar = content[quoteStart];
            if (quoteChar != '"' && quoteChar != '\'') {
                searchPos = presetPos + 7;
                continue;
            }
            
            size_t nameStart = quoteStart + 1;
            size_t nameEnd = content.find(quoteChar, nameStart);
            
            if (nameEnd == std::string::npos) {
                searchPos = presetPos + 7;
                continue;
            }
            
            XmlPresetInfo info;
            info.filename = filename;
            info.internalName = content.substr(nameStart, nameEnd - nameStart);
            
            info.internalName = DecodeHtmlEntities(info.internalName);

            size_t semicolonPos = info.internalName.find(';');
            size_t commaPos = info.internalName.find(',');
            size_t truncatePos = std::string::npos;

            if (semicolonPos != std::string::npos && commaPos != std::string::npos) {
                truncatePos = std::min(semicolonPos, commaPos);
            } else if (semicolonPos != std::string::npos) {
                truncatePos = semicolonPos;
            } else if (commaPos != std::string::npos) {
                truncatePos = commaPos;
            }

            if (truncatePos != std::string::npos) {
                info.internalName = info.internalName.substr(0, truncatePos);
                info.internalName = Trim(info.internalName);
            }

            if (info.internalName == "CustomPreset") {
                if (presetCount == 0) {
                    info.internalName = filename;
                } else {
                    info.internalName = filename + " " + std::to_string(presetCount + 1);
                }
                logFile << "INFO: Replaced generic CustomPreset with: "
                        << info.internalName << std::endl;
            }

            info.extractionSuccessful = true;
            allPresets.push_back(info);
            presetCount++;
            
            searchPos = nameEnd + 1;
        }
        
        if (presetCount > 1) {
            logFile << "INFO: Found " << presetCount << " presets in file: " << filename << ".xml" << std::endl;
        }
        
        return allPresets;
        
    } catch (const std::exception& e) {
        logFile << "ERROR in ExtractAllPresetsFromXml: " << e.what() << std::endl;
        return allPresets;
    } catch (...) {
        logFile << "ERROR in ExtractAllPresetsFromXml: Unknown exception" << std::endl;
        return allPresets;
    }
}

XmlPresetInfo ExtractPresetInfoFromXml(const fs::path& xmlPath, std::ofstream& logFile) {
    auto allPresets = ExtractAllPresetsFromXml(xmlPath, logFile);
    
    if (!allPresets.empty()) {
        return allPresets[0];
    }
    
    XmlPresetInfo emptyInfo;
    emptyInfo.extractionSuccessful = false;
    emptyInfo.filename = xmlPath.stem().string();
    return emptyInfo;
}

std::string ExtractPresetNameFromXml(const fs::path& xmlPath, std::ofstream& logFile) {
    XmlPresetInfo info = ExtractPresetInfoFromXml(xmlPath, logFile);
    
    if (info.extractionSuccessful && !info.internalName.empty()) {
        return info.internalName;
    }
    
    return info.filename;
}

XmlAnalysisResult AnalyzeXmlGroups(const fs::path& xmlPath, std::ofstream& logFile) {
    XmlAnalysisResult result;
    
    try {
        if (!fs::exists(xmlPath)) return result;
        
        std::string content = ReadFileWithEncoding(xmlPath);
        if (content.empty()) return result;
        
        std::string lowerContent = content;
        std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
        
        const std::vector<std::string> ubePatterns = {
            "<group name=\"ube\"",
            "<group name='ube'",
            "<group name=\"ube female\"",
            "<group name='ube female'",
            "<group name=\"ube male\"",
            "<group name='ube male'",
            "<group name=\"ube vanilla\"",
            "<group name='ube vanilla'",
            "<group name=\"ube vanilla dawnguard\"",
            "<group name='ube vanilla dawnguard'",
            "<group name=\"ube vanilla dragonborn\"",
            "<group name='ube vanilla dragonborn'"
        };
        
        const std::vector<std::string> himboPatterns = {
            "<group name=\"himbo\"",
            "<group name='himbo'",
            "<group name=\"himbo/\"",
            "<group name='himbo/'"
        };
        
        for (const auto& pattern : ubePatterns) {
            if (lowerContent.find(pattern) != std::string::npos) {
                result.hasUBE = true;
                break;
            }
        }
        
        for (const auto& pattern : himboPatterns) {
            if (lowerContent.find(pattern) != std::string::npos) {
                result.hasHIMBO = true;
                break;
            }
        }
        
        const std::vector<std::string> conflictingPatterns = {
            "3ba", "3bbb", "cbbe"
        };
        
        size_t pos = 0;
        while (pos < lowerContent.length()) {
            size_t groupStart = lowerContent.find("<group name=", pos);
            if (groupStart == std::string::npos) break;
            
            size_t nameStart = lowerContent.find_first_of("\"'", groupStart);
            if (nameStart == std::string::npos) break;
            
            char quoteChar = lowerContent[nameStart];
            size_t nameEnd = lowerContent.find(quoteChar, nameStart + 1);
            if (nameEnd == std::string::npos) break;
            
            std::string groupName = lowerContent.substr(nameStart + 1, nameEnd - nameStart - 1);
            
            for (const auto& pattern : conflictingPatterns) {
                if (groupName.find(pattern) != std::string::npos) {
                    result.hasConflictingGroups = true;
                    std::string originalGroupName = content.substr(nameStart + 1, nameEnd - nameStart - 1);
                    if (std::find(result.conflictingGroupsFound.begin(), result.conflictingGroupsFound.end(), 
                                 originalGroupName) == result.conflictingGroupsFound.end()) {
                        result.conflictingGroupsFound.push_back(originalGroupName);
                    }
                    break;
                }
            }
            
            pos = nameEnd + 1;
        }
        
        bool needsSetAnalysis = (!result.hasUBE && !result.hasHIMBO) || 
                                 result.hasConflictingGroups ||
                                 (result.hasUBE && result.hasHIMBO);
        
        if (needsSetAnalysis) {
            size_t presetPos = lowerContent.find("<preset");
            if (presetPos != std::string::npos) {
                size_t setPos = lowerContent.find("set=", presetPos);
                
                size_t presetEnd = lowerContent.find(">", presetPos);
                
                if (setPos != std::string::npos && setPos < presetEnd) {
                    size_t quoteStart = setPos + 4;
                    
                    while (quoteStart < lowerContent.length() && 
                           (lowerContent[quoteStart] == ' ' || lowerContent[quoteStart] == '\t')) {
                        quoteStart++;
                    }
                    
                    if (quoteStart < lowerContent.length()) {
                        char setQuoteChar = content[quoteStart];
                        if (setQuoteChar == '"' || setQuoteChar == '\'') {
                            size_t setValueStart = quoteStart + 1;
                            size_t setValueEnd = content.find(setQuoteChar, setValueStart);
                            
                            if (setValueEnd != std::string::npos) {
                                result.setAttributeValue = content.substr(setValueStart, setValueEnd - setValueStart);
                                result.setAnalysisPerformed = true;
                                
                                std::string lowerSetValue = result.setAttributeValue;
                                std::transform(lowerSetValue.begin(), lowerSetValue.end(), 
                                             lowerSetValue.begin(), ::tolower);
                                
                                logFile << "  Set analysis: Found set=\"" << result.setAttributeValue << "\"" << std::endl;
                                
                                for (const auto& exceptionName : EXCEPTION_3BA_SET_NAMES) {
                                    if (result.setAttributeValue == exceptionName) {
                                        result.isException3BA = true;
                                        logFile << "  Exception 3BA: Detected exact match: \"" << exceptionName << "\"" << std::endl;
                                        logFile << "  Exception 3BA: Will be categorized as Normal 3BA, NOT UBE" << std::endl;
                                        break;
                                    }
                                }
                                
                                for (const auto& ubeId : UBE_SET_IDENTIFIERS) {
                                    if (result.setAttributeValue.find(ubeId) != std::string::npos) {
                                        result.isUBEBySet = true;
                                        logFile << "  UBE by set: Detected identifier: \"" << ubeId << "\"" << std::endl;
                                        break;
                                    }
                                }
                                
                                if (lowerSetValue.find("ube") != std::string::npos && !result.isException3BA) {
                                    result.hasUBE = true;
                                    logFile << "  Set analysis: Detected UBE from set attribute" << std::endl;
                                }
                                
                                if (lowerSetValue.find("himbo") != std::string::npos) {
                                    result.hasHIMBO = true;
                                    logFile << "  Set analysis: Detected HIMBO from set attribute" << std::endl;
                                }
                                
                                bool hasConflictInSet = false;
                                for (const auto& pattern : conflictingPatterns) {
                                    if (lowerSetValue.find(pattern) != std::string::npos) {
                                        hasConflictInSet = true;
                                        if (std::find(result.conflictingGroupsFound.begin(), 
                                                     result.conflictingGroupsFound.end(), 
                                                     pattern) == result.conflictingGroupsFound.end()) {
                                            result.conflictingGroupsFound.push_back(pattern);
                                        }
                                    }
                                }
                                
                                if (hasConflictInSet) {
                                    result.hasConflictingGroups = true;
                                    logFile << "  Set analysis: Detected conflicting body types from set attribute" << std::endl;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if (!result.hasUBE && !result.hasHIMBO && !result.hasConflictingGroups) {
            std::string filename;
            try {
                filename = xmlPath.stem().string();
            } catch (...) {
                try {
                    auto u8name = xmlPath.stem().u8string();
                    filename = std::string(u8name.begin(), u8name.end());
                } catch (...) {
                    filename = "";
                }
            }
            
            if (!filename.empty()) {
                std::string lowerFilename = filename;
                std::transform(lowerFilename.begin(), lowerFilename.end(), 
                             lowerFilename.begin(), ::tolower);
                
                logFile << "  Filename fallback: Analyzing filename: " << filename << std::endl;
                
                if (lowerFilename.find("ube") != std::string::npos) {
                    result.hasUBE = true;
                    logFile << "  Filename fallback: Detected UBE from filename" << std::endl;
                }
                
                if (lowerFilename.find("himbo") != std::string::npos) {
                    result.hasHIMBO = true;
                    logFile << "  Filename fallback: Detected HIMBO from filename" << std::endl;
                }
                
                for (const auto& pattern : conflictingPatterns) {
                    if (lowerFilename.find(pattern) != std::string::npos) {
                        result.hasConflictingGroups = true;
                        if (std::find(result.conflictingGroupsFound.begin(), 
                                     result.conflictingGroupsFound.end(), 
                                     pattern) == result.conflictingGroupsFound.end()) {
                            result.conflictingGroupsFound.push_back(pattern);
                        }
                        logFile << "  Filename fallback: Detected conflicting pattern: " << pattern << std::endl;
                    }
                }
            }
        }
        
        return result;
        
    } catch (...) {
        logFile << "ERROR analyzing XML groups: " << xmlPath.string() << std::endl;
        return result;
    }
}

std::string ExtractTextInParentheses(const std::string& text) {
    std::string result;
    size_t openPos = text.find('(');
    
    while (openPos != std::string::npos) {
        size_t closePos = text.find(')', openPos);
        if (closePos != std::string::npos) {
            std::string content = text.substr(openPos + 1, closePos - openPos - 1);
            if (!result.empty()) {
                result += " ";
            }
            result += content;
            openPos = text.find('(', closePos);
        } else {
            break;
        }
    }
    
    return result;
}

bool MatchesAllFragments(const std::string& text, const std::vector<std::string>& fragments) {
    if (fragments.empty()) return false;
    
    std::string lowerText = ToLowerCase(text);
    
    for (const auto& fragment : fragments) {
        std::string lowerFragment = ToLowerCase(fragment);
        if (lowerText.find(lowerFragment) == std::string::npos) {
            return false;
        }
    }
    
    return true;
}

std::vector<std::string> FindMatchingPresetsByKeyWord(const PresetMapData& presetData, 
                                                      const std::vector<std::string>& fragments,
                                                      std::ofstream& logFile) {
    std::vector<std::string> matchingPresets;
    
    if (fragments.empty()) {
        logFile << "    WARNING: No search fragments provided for KeyWord mode" << std::endl;
        return matchingPresets;
    }
    
    for (const auto& [presetName, _] : presetData.exactMap) {
        if (MatchesAllFragments(presetName, fragments)) {
            matchingPresets.push_back(presetName);
        }
    }
    
    logFile << "    KeyWord search with fragments: [";
    for (size_t i = 0; i < fragments.size(); i++) {
        logFile << "\"" << fragments[i] << "\"";
        if (i < fragments.size() - 1) logFile << ", ";
    }
    logFile << "] found " << matchingPresets.size() << " matching presets" << std::endl;
    
    return matchingPresets;
}

std::vector<std::string> FindMatchingPresetsByKeyWordChart(const PresetMapData& presetData, 
                                                           const std::vector<std::string>& fragments,
                                                           std::ofstream& logFile) {
    std::vector<std::string> matchingPresets;
    
    if (fragments.empty()) {
        logFile << "    WARNING: No search fragments provided for KeyWordChart mode" << std::endl;
        return matchingPresets;
    }
    
    for (const auto& [presetName, _] : presetData.exactMap) {
        std::string textInParentheses = ExtractTextInParentheses(presetName);
        if (!textInParentheses.empty() && MatchesAllFragments(textInParentheses, fragments)) {
            matchingPresets.push_back(presetName);
        }
    }
    
    logFile << "    KeyWordChart search in parentheses with fragments: [";
    for (size_t i = 0; i < fragments.size(); i++) {
        logFile << "\"" << fragments[i] << "\"";
        if (i < fragments.size() - 1) logFile << ", ";
    }
    logFile << "] found " << matchingPresets.size() << " matching presets" << std::endl;
    
    return matchingPresets;
}

std::vector<std::string> FindMatchingPresetsByKeyAuthor(const PresetMapData& presetData, 
                                                        const std::vector<std::string>& fragments,
                                                        std::ofstream& logFile) {
    std::vector<std::string> matchingPresets;
    
    if (fragments.empty()) {
        logFile << "    WARNING: No author specified for KeyAuthor mode" << std::endl;
        return matchingPresets;
    }
    
    for (const auto& [presetName, _] : presetData.exactMap) {
        if (MatchesAllFragments(presetName, fragments)) {
            matchingPresets.push_back(presetName);
        }
    }
    
    std::string authorName = fragments[0];
    logFile << "    KeyAuthor search for author \"" << authorName << "\"";
    if (fragments.size() > 1) {
        logFile << " with additional filters: [";
        for (size_t i = 1; i < fragments.size(); i++) {
            logFile << "\"" << fragments[i] << "\"";
            if (i < fragments.size() - 1) logFile << ", ";
        }
        logFile << "]";
    }
    logFile << " found " << matchingPresets.size() << " matching presets" << std::endl;
    
    return matchingPresets;
}

std::vector<std::string> FindMatchingPresetsByFamily(const PresetMapData& presetData,
                                                      const std::vector<std::string>& requestedPresets,
                                                      const std::string& family,
                                                      std::ofstream& logFile,
                                                      std::vector<std::string>& notFoundPresets) {
    std::vector<std::string> matchingPresets;
    notFoundPresets.clear();
    
    const std::set<std::string>* familySet = nullptr;
    
    if (family == "KeyNormal") {
        for (const auto& preset : requestedPresets) {
            if (presetData.ubePresetNames.find(preset) == presetData.ubePresetNames.end() &&
                presetData.himboPresetNames.find(preset) == presetData.himboPresetNames.end() &&
                presetData.exactMap.find(preset) != presetData.exactMap.end()) {
                matchingPresets.push_back(preset);
            } else if (presetData.exactMap.find(preset) == presetData.exactMap.end()) {
                notFoundPresets.push_back(preset);
            }
        }
    } else if (family == "KeyUBE") {
        familySet = &presetData.ubePresetNames;
    } else if (family == "KeyHIMBO") {
        familySet = &presetData.himboPresetNames;
    }
    
    if (familySet) {
        for (const auto& preset : requestedPresets) {
            if (familySet->find(preset) != familySet->end()) {
                matchingPresets.push_back(preset);
            } else {
                notFoundPresets.push_back(preset);
            }
        }
    }
    
    logFile << "    " << family << " search: " << matchingPresets.size() 
            << " found, " << notFoundPresets.size() << " not found in family" << std::endl;
    
    return matchingPresets;
}

NpcFormIDRule ParseNpcFormIDRuleLine(const std::string& value, std::ofstream& logFile) {
    NpcFormIDRule rule;
    
    std::vector<std::string> parts = Split(value, '|');
    if (parts.size() >= 2) {
        rule.plugin = Trim(parts[0]);
        
        std::vector<std::string> formIDAndPresets = Split(parts[1], ',');
        
        if (!formIDAndPresets.empty()) {
            std::string rawFormID = Trim(formIDAndPresets[0]);
            bool wasCleaned = false;
            rule.formID = CleanFormID(rawFormID, logFile, wasCleaned);
            
            for (size_t i = 1; i < formIDAndPresets.size(); i++) {
                rule.presets.push_back(Trim(formIDAndPresets[i]));
            }
        }
        
        if (parts.size() >= 3) {
            rule.extra = Trim(parts[2]);
            
            if (rule.extra.empty()) {
                rule.mode = INIRuleMode::STANDARD;
                rule.applyCount = -1;
            } else if (rule.extra == "-") {
                rule.mode = INIRuleMode::REMOVE_ALWAYS;
                rule.applyCount = -1;
            } else if (rule.extra == "*") {
                rule.mode = INIRuleMode::EXCLUSIVE_ALWAYS;
                rule.applyCount = -1;
            } else {
                try {
                    rule.applyCount = std::stoi(rule.extra);
                    if (rule.applyCount == 1) {
                        rule.mode = INIRuleMode::ONCE;
                    } else if (rule.applyCount == 0) {
                        rule.mode = INIRuleMode::DISABLED;
                    }
                } catch (...) {
                    rule.mode = INIRuleMode::DISABLED;
                    rule.applyCount = 0;
                }
            }
        }
    }
    
    return rule;
}

SpecialRule ParseSpecialRuleLine(const std::string& key, const std::string& value) {
    SpecialRule rule;
    rule.ruleType = key;

    std::vector<std::string> parts = Split(value, '|');
    if (parts.size() >= 2) {
        rule.plugin = Trim(parts[0]);
        rule.presets = Split(parts[1], ',');

        if (parts.size() >= 3) {
            rule.extra = Trim(parts[2]);
            
            if (rule.extra == "KeyWord") {
                rule.mode = INIRuleMode::KEYWORD;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWord*") {
                rule.mode = INIRuleMode::KEYWORD_EXCLUSIVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWord-") {
                rule.mode = INIRuleMode::KEYWORD_REMOVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWordChart") {
                rule.mode = INIRuleMode::KEYWORDCHART;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWordChart*") {
                rule.mode = INIRuleMode::KEYWORDCHART_EXCLUSIVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWordChart-") {
                rule.mode = INIRuleMode::KEYWORDCHART_REMOVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyAuthor") {
                rule.mode = INIRuleMode::KEYAUTHOR;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyAuthor*") {
                rule.mode = INIRuleMode::KEYAUTHOR_EXCLUSIVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyAuthor-") {
                rule.mode = INIRuleMode::KEYAUTHOR_REMOVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyNormal") {
                rule.mode = INIRuleMode::KEYNORMAL;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyNormal*") {
                rule.mode = INIRuleMode::KEYNORMAL_EXCLUSIVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyNormal-") {
                rule.mode = INIRuleMode::KEYNORMAL_REMOVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyUBE") {
                rule.mode = INIRuleMode::KEYUBE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyUBE*") {
                rule.mode = INIRuleMode::KEYUBE_EXCLUSIVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyUBE-") {
                rule.mode = INIRuleMode::KEYUBE_REMOVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyHIMBO") {
                rule.mode = INIRuleMode::KEYHIMBO;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyHIMBO*") {
                rule.mode = INIRuleMode::KEYHIMBO_EXCLUSIVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyHIMBO-") {
                rule.mode = INIRuleMode::KEYHIMBO_REMOVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra.empty()) {
                rule.mode = INIRuleMode::STANDARD;
                rule.applyCount = -1;
            } else if (rule.extra == "-") {
                rule.mode = INIRuleMode::REMOVE_ALWAYS;
                rule.applyCount = -1;
            } else if (rule.extra == "*") {
                rule.mode = INIRuleMode::EXCLUSIVE_ALWAYS;
                rule.applyCount = -1;
            } else {
                try {
                    rule.applyCount = std::stoi(rule.extra);
                    if (rule.applyCount == 1) {
                        rule.mode = INIRuleMode::ONCE;
                    } else if (rule.applyCount == 0) {
                        rule.mode = INIRuleMode::DISABLED;
                    } else {
                        rule.mode = INIRuleMode::DISABLED;
                        rule.applyCount = 0;
                    }
                } catch (...) {
                    rule.mode = INIRuleMode::DISABLED;
                    rule.applyCount = 0;
                }
            }
        } else {
            rule.mode = INIRuleMode::STANDARD;
            rule.applyCount = -1;
        }
    }

    if (rule.ruleType == "raceFemaleUBE") {
        rule.targetKey = "raceFemale";
    } else if (rule.ruleType == "raceMaleAny") {
        rule.targetKey = "raceMale";
    } else if (StartsWith(rule.ruleType, "blacklisted")) {
        std::string shortType = rule.ruleType.substr(11);
        rule.targetKey = GetFullBlacklistKey(shortType);
    }

    return rule;
}

SpecialRule ParseFormIDRuleLine(const std::string& key, const std::string& value, std::ofstream& logFile) {
    SpecialRule rule;
    rule.ruleType = key;
    rule.targetKey = key;
    
    std::vector<std::string> parts = Split(value, '|');
    if (parts.size() >= 2) {
        rule.plugin = Trim(parts[0]);
        
        std::vector<std::string> rawFormIDs = Split(parts[1], ',');
        for (const auto& rawID : rawFormIDs) {
            bool wasCleaned = false;
            std::string cleanedID = CleanFormID(Trim(rawID), logFile, wasCleaned);
            rule.presets.push_back(cleanedID);
        }
        
        if (parts.size() >= 3) {
            rule.extra = Trim(parts[2]);
            
            if (rule.extra.empty()) {
                rule.mode = INIRuleMode::STANDARD;
                rule.applyCount = -1;
            } else if (rule.extra == "-") {
                rule.mode = INIRuleMode::REMOVE_ALWAYS;
                rule.applyCount = -1;
            } else if (rule.extra == "*") {
                rule.mode = INIRuleMode::EXCLUSIVE_ALWAYS;
                rule.applyCount = -1;
            } else {
                try {
                    rule.applyCount = std::stoi(rule.extra);
                    if (rule.applyCount == 1) {
                        rule.mode = INIRuleMode::ONCE;
                    } else if (rule.applyCount == 0) {
                        rule.mode = INIRuleMode::DISABLED;
                    } else {
                        rule.mode = INIRuleMode::DISABLED;
                        rule.applyCount = 0;
                    }
                } catch (...) {
                    rule.mode = INIRuleMode::DISABLED;
                    rule.applyCount = 0;
                }
            }
        } else {
            rule.mode = INIRuleMode::STANDARD;
            rule.applyCount = -1;
        }
    }
    
    return rule;
}

SpecialRule ParseOutfitRuleLine(const std::string& key, const std::string& value) {
    SpecialRule rule;
    rule.ruleType = "outfits";
    
    std::vector<std::string> parts = Split(value, '|');
    if (parts.size() >= 2) {
        std::string targetSection = Trim(parts[0]);
        
        if (std::find(OUTFIT_ARRAY_TYPES.begin(), OUTFIT_ARRAY_TYPES.end(), targetSection) != OUTFIT_ARRAY_TYPES.end()) {
            rule.targetKey = targetSection;
            rule.plugin = "";
            rule.presets = Split(parts[1], ',');
            
            if (parts.size() >= 3) {
                rule.extra = Trim(parts[2]);
                
                if (rule.extra.empty()) {
                    rule.mode = INIRuleMode::STANDARD;
                    rule.applyCount = -1;
                } else if (rule.extra == "-") {
                    rule.mode = INIRuleMode::REMOVE_ALWAYS;
                    rule.applyCount = -1;
                } else if (rule.extra == "*") {
                    rule.mode = INIRuleMode::EXCLUSIVE_ALWAYS;
                    rule.applyCount = -1;
                } else {
                    try {
                        rule.applyCount = std::stoi(rule.extra);
                        if (rule.applyCount == 1) {
                            rule.mode = INIRuleMode::ONCE;
                        } else if (rule.applyCount == 0) {
                            rule.mode = INIRuleMode::DISABLED;
                        } else {
                            rule.mode = INIRuleMode::DISABLED;
                            rule.applyCount = 0;
                        }
                    } catch (...) {
                        rule.mode = INIRuleMode::DISABLED;
                        rule.applyCount = 0;
                    }
                }
            } else {
                rule.mode = INIRuleMode::STANDARD;
                rule.applyCount = -1;
            }
        }
    }
    
    return rule;
}

ParsedRule ParseRuleLine(const std::string& key, const std::string& value) {
    ParsedRule rule;
    rule.key = key;

    std::vector<std::string> parts = Split(value, '|');
    if (parts.size() >= 2) {
        rule.plugin = Trim(parts[0]);
        rule.presets = Split(parts[1], ',');

        if (parts.size() >= 3) {
            rule.extra = Trim(parts[2]);
            
            if (rule.extra == "KeyWord1") {
                rule.mode = INIRuleMode::KEYWORD_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWord1*") {
                rule.mode = INIRuleMode::KEYWORD_EXCLUSIVE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWord1-") {
                rule.mode = INIRuleMode::KEYWORD_REMOVE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWordChart1") {
                rule.mode = INIRuleMode::KEYWORDCHART_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWordChart1*") {
                rule.mode = INIRuleMode::KEYWORDCHART_EXCLUSIVE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWordChart1-") {
                rule.mode = INIRuleMode::KEYWORDCHART_REMOVE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyAuthor1") {
                rule.mode = INIRuleMode::KEYAUTHOR_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyAuthor1*") {
                rule.mode = INIRuleMode::KEYAUTHOR_EXCLUSIVE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyAuthor1-") {
                rule.mode = INIRuleMode::KEYAUTHOR_REMOVE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyNormal1") {
                rule.mode = INIRuleMode::KEYNORMAL_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyNormal1*") {
                rule.mode = INIRuleMode::KEYNORMAL_EXCLUSIVE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyNormal1-") {
                rule.mode = INIRuleMode::KEYNORMAL_REMOVE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyUBE1") {
                rule.mode = INIRuleMode::KEYUBE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyUBE1*") {
                rule.mode = INIRuleMode::KEYUBE_EXCLUSIVE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyUBE1-") {
                rule.mode = INIRuleMode::KEYUBE_REMOVE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyHIMBO1") {
                rule.mode = INIRuleMode::KEYHIMBO_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyHIMBO1*") {
                rule.mode = INIRuleMode::KEYHIMBO_EXCLUSIVE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyHIMBO1-") {
                rule.mode = INIRuleMode::KEYHIMBO_REMOVE_ONCE;
                rule.applyCount = 1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWord") {
                rule.mode = INIRuleMode::KEYWORD;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWord*") {
                rule.mode = INIRuleMode::KEYWORD_EXCLUSIVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWord-") {
                rule.mode = INIRuleMode::KEYWORD_REMOVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWordChart") {
                rule.mode = INIRuleMode::KEYWORDCHART;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWordChart*") {
                rule.mode = INIRuleMode::KEYWORDCHART_EXCLUSIVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyWordChart-") {
                rule.mode = INIRuleMode::KEYWORDCHART_REMOVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyAuthor") {
                rule.mode = INIRuleMode::KEYAUTHOR;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyAuthor*") {
                rule.mode = INIRuleMode::KEYAUTHOR_EXCLUSIVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyAuthor-") {
                rule.mode = INIRuleMode::KEYAUTHOR_REMOVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyNormal") {
                rule.mode = INIRuleMode::KEYNORMAL;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyNormal*") {
                rule.mode = INIRuleMode::KEYNORMAL_EXCLUSIVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyNormal-") {
                rule.mode = INIRuleMode::KEYNORMAL_REMOVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyUBE") {
                rule.mode = INIRuleMode::KEYUBE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyUBE*") {
                rule.mode = INIRuleMode::KEYUBE_EXCLUSIVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyUBE-") {
                rule.mode = INIRuleMode::KEYUBE_REMOVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyHIMBO") {
                rule.mode = INIRuleMode::KEYHIMBO;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyHIMBO*") {
                rule.mode = INIRuleMode::KEYHIMBO_EXCLUSIVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "KeyHIMBO-") {
                rule.mode = INIRuleMode::KEYHIMBO_REMOVE;
                rule.applyCount = -1;
                rule.filterFragments = rule.presets;
                rule.presets.clear();
            } else if (rule.extra == "1-") {
                rule.mode = INIRuleMode::ORGANIZE_REMOVE_ONCE;
                rule.applyCount = 1;
            } else if (rule.extra == "1*") {
                rule.mode = INIRuleMode::ORGANIZE_EXCLUSIVE_ONCE;
                rule.applyCount = 1;
            } else if (rule.extra.empty()) {
                rule.mode = INIRuleMode::STANDARD;
                rule.applyCount = -1;
            } else if (rule.extra == "-") {
                rule.mode = INIRuleMode::REMOVE_ALWAYS;
                rule.applyCount = -1;
            } else if (rule.extra == "*") {
                rule.mode = INIRuleMode::EXCLUSIVE_ALWAYS;
                rule.applyCount = -1;
            } else {
                try {
                    rule.applyCount = std::stoi(rule.extra);
                    if (rule.applyCount == 1) {
                        rule.mode = INIRuleMode::ONCE;
                    } else if (rule.applyCount == 0) {
                        rule.mode = INIRuleMode::DISABLED;
                    } else {
                        rule.mode = INIRuleMode::DISABLED;
                        rule.applyCount = 0;
                    }
                } catch (...) {
                    rule.mode = INIRuleMode::DISABLED;
                    rule.applyCount = 0;
                }
            }
        } else {
            rule.mode = INIRuleMode::STANDARD;
            rule.applyCount = -1;
        }
    }

    return rule;
}

ConfigSettings ReadConfigFromIni(const fs::path& iniPath, std::ofstream& logFile) {
    ConfigSettings settings;
    
    try {
        if (!fs::exists(iniPath)) {
            logFile << "Creating config INI at: " << iniPath.string() << std::endl;
            std::ofstream createIni(iniPath, std::ios::out | std::ios::trunc);
            if (createIni.is_open()) {
                createIni << "[Original backup]" << std::endl;
                createIni << "Backup = 1" << std::endl;
                createIni << std::endl;
                createIni << "[blacklistedPresetsHIMBO]" << std::endl;
                createIni << "ModeHIMBO = true" << std::endl;
                createIni << std::endl;
                createIni << "[blacklistedPresetsShowInOBodyMenu]" << std::endl;
                createIni << "ModeUBE = true" << std::endl;
                createIni << std::endl;
                createIni << "[Presets_Smart_Cleaning]" << std::endl;
                createIni << "Smart_Cleaning = false" << std::endl;
                createIni << std::endl;
                createIni << "[blacklistedPresets_Smart_Cleaning_FromRandomDistribution]" << std::endl;
                createIni << "Smart_Cleaning = false" << std::endl;
                createIni << std::endl;
                createIni << "[blacklistedPresets_Smart_Cleaning_FromAll]" << std::endl;
                createIni << "Smart_Cleaning = false" << std::endl;
                createIni << std::endl;
                createIni << "[outfitsForceRe_Smart_Cleaning]" << std::endl;
                createIni << "Smart_Cleaning = false" << std::endl;
                createIni << std::endl;
                createIni << "[conflictINI_Smart_Resolution]" << std::endl;
                createIni << "Smart_Resolution = true" << std::endl;
                createIni.close();
                logFile << "SUCCESS: Config INI created with default values" << std::endl;
                settings.backupValue = 1;
                settings.modeUBE = true;
                settings.modeHIMBO = true;
                settings.presetsSmartCleaning = false;
                settings.blacklistedPresetsSmartCleaningFromRandomDistribution = false;
                settings.blacklistedPresetsSmartCleaningFromAll = false;
                settings.outfitsForceReSmartCleaning = false;
                settings.conflictSmartResolution = true;
                return settings;
            } else {
                logFile << "ERROR: Could not create config INI file" << std::endl;
                return settings;
            }
        }

        std::string content = ReadFileWithEncoding(iniPath);
        if (content.empty()) {
            logFile << "ERROR: Could not read config INI file" << std::endl;
            return settings;
        }

        std::stringstream ss(content);
        std::string line;
        std::string currentSection;

        while (std::getline(ss, line)) {
            std::string trimmedLine = Trim(line);

            if (trimmedLine.empty() || trimmedLine[0] == ';' || trimmedLine[0] == '#') {
                continue;
            }

            if (trimmedLine[0] == '[' && trimmedLine.back() == ']') {
                currentSection = trimmedLine.substr(1, trimmedLine.length() - 2);
                continue;
            }

            size_t equalPos = trimmedLine.find('=');
            if (equalPos != std::string::npos) {
                std::string key = Trim(trimmedLine.substr(0, equalPos));
                std::string value = Trim(trimmedLine.substr(equalPos + 1));

                if (currentSection == "Original backup" && key == "Backup") {
                    if (value == "true" || value == "True" || value == "TRUE") {
                        settings.backupValue = 2;
                        logFile << "Read config: Backup = true (always backup mode)" << std::endl;
                    } else {
                        try {
                            settings.backupValue = std::stoi(value);
                            logFile << "Read config: Backup = " << settings.backupValue << std::endl;
                        } catch (...) {
                            logFile << "Warning: Invalid Backup value, using default (1)" << std::endl;
                            settings.backupValue = 1;
                        }
                    }
                } else if (currentSection == "blacklistedPresetsHIMBO" && key == "ModeHIMBO") {
                    if (value == "true" || value == "True" || value == "TRUE") {
                        settings.modeHIMBO = true;
                        logFile << "Read config: ModeHIMBO = true" << std::endl;
                    } else if (value == "false" || value == "False" || value == "FALSE") {
                        settings.modeHIMBO = false;
                        logFile << "Read config: ModeHIMBO = false" << std::endl;
                    } else {
                        logFile << "Warning: Invalid ModeHIMBO value, using default (true)" << std::endl;
                        settings.modeHIMBO = true;
                    }
                } else if (currentSection == "blacklistedPresetsShowInOBodyMenu" && key == "ModeUBE") {
                    if (value == "true" || value == "True" || value == "TRUE") {
                        settings.modeUBE = true;
                        logFile << "Read config: ModeUBE = true" << std::endl;
                    } else if (value == "false" || value == "False" || value == "FALSE") {
                        settings.modeUBE = false;
                        logFile << "Read config: ModeUBE = false" << std::endl;
                    } else {
                        logFile << "Warning: Invalid ModeUBE value, using default (true)" << std::endl;
                        settings.modeUBE = true;
                    }
                } else if (currentSection == "Presets_Smart_Cleaning" && key == "Smart_Cleaning") {
                    if (value == "true" || value == "True" || value == "TRUE") {
                        settings.presetsSmartCleaning = true;
                        logFile << "Read config: Presets_Smart_Cleaning = true" << std::endl;
                    } else if (value == "false" || value == "False" || value == "FALSE") {
                        settings.presetsSmartCleaning = false;
                        logFile << "Read config: Presets_Smart_Cleaning = false" << std::endl;
                    } else {
                        logFile << "Warning: Invalid Presets_Smart_Cleaning value, using default (false)" << std::endl;
                        settings.presetsSmartCleaning = false;
                    }
                } else if (currentSection == "blacklistedPresets_Smart_Cleaning_FromRandomDistribution" && key == "Smart_Cleaning") {
                    if (value == "true" || value == "True" || value == "TRUE") {
                        settings.blacklistedPresetsSmartCleaningFromRandomDistribution = true;
                        logFile << "Read config: blacklistedPresets_Smart_Cleaning_FromRandomDistribution = true" << std::endl;
                    } else if (value == "false" || value == "False" || value == "FALSE") {
                        settings.blacklistedPresetsSmartCleaningFromRandomDistribution = false;
                        logFile << "Read config: blacklistedPresets_Smart_Cleaning_FromRandomDistribution = false" << std::endl;
                    } else {
                        logFile << "Warning: Invalid blacklistedPresets_Smart_Cleaning_FromRandomDistribution value, using default (false)" << std::endl;
                        settings.blacklistedPresetsSmartCleaningFromRandomDistribution = false;
                    }
                } else if (currentSection == "blacklistedPresets_Smart_Cleaning_FromAll" && key == "Smart_Cleaning") {
                    if (value == "true" || value == "True" || value == "TRUE") {
                        settings.blacklistedPresetsSmartCleaningFromAll = true;
                        logFile << "Read config: blacklistedPresets_Smart_Cleaning_FromAll = true" << std::endl;
                    } else if (value == "false" || value == "False" || value == "FALSE") {
                        settings.blacklistedPresetsSmartCleaningFromAll = false;
                        logFile << "Read config: blacklistedPresets_Smart_Cleaning_FromAll = false" << std::endl;
                    } else {
                        logFile << "Warning: Invalid blacklistedPresets_Smart_Cleaning_FromAll value, using default (false)" << std::endl;
                        settings.blacklistedPresetsSmartCleaningFromAll = false;
                    }
                } else if (currentSection == "outfitsForceRe_Smart_Cleaning" && key == "Smart_Cleaning") {
                    if (value == "true" || value == "True" || value == "TRUE") {
                        settings.outfitsForceReSmartCleaning = true;
                        logFile << "Read config: outfitsForceRe_Smart_Cleaning = true" << std::endl;
                    } else if (value == "false" || value == "False" || value == "FALSE") {
                        settings.outfitsForceReSmartCleaning = false;
                        logFile << "Read config: outfitsForceRe_Smart_Cleaning = false" << std::endl;
                    } else {
                        logFile << "Warning: Invalid outfitsForceRe_Smart_Cleaning value, using default (false)" << std::endl;
                        settings.outfitsForceReSmartCleaning = false;
                    }
                } else if (currentSection == "conflictINI_Smart_Resolution" && key == "Smart_Resolution") {
                    if (value == "true" || value == "True" || value == "TRUE") {
                        settings.conflictSmartResolution = true;
                        logFile << "Read config: conflictINI_Smart_Resolution = true" << std::endl;
                    } else if (value == "false" || value == "False" || value == "FALSE") {
                        settings.conflictSmartResolution = false;
                        logFile << "Read config: conflictINI_Smart_Resolution = false" << std::endl;
                    } else {
                        logFile << "Warning: Invalid conflictINI_Smart_Resolution value, using default (true)" << std::endl;
                        settings.conflictSmartResolution = true;
                    }
                }
            }
        }

        return settings;
        
    } catch (const std::exception& e) {
        logFile << "ERROR in ReadConfigFromIni: " << e.what() << std::endl;
        return settings;
    } catch (...) {
        logFile << "ERROR in ReadConfigFromIni: Unknown exception" << std::endl;
        return settings;
    }
}

void UpdateBackupConfigInIni(const fs::path& iniPath, std::ofstream& logFile, int originalValue) {
    try {
        if (!fs::exists(iniPath)) {
            logFile << "ERROR: Config INI file does not exist for update" << std::endl;
            return;
        }

        if (originalValue == 2) {
            logFile << "INFO: Backup = true detected, INI will not be updated (always backup mode)" << std::endl;
            return;
        }

        std::string content = ReadFileWithEncoding(iniPath);
        if (content.empty()) {
            logFile << "ERROR: Could not read config INI file for update" << std::endl;
            return;
        }

        std::stringstream ss(content);
        std::vector<std::string> lines;
        std::string line;
        std::string currentSection;
        bool backupValueUpdated = false;
        lines.reserve(100);

        while (std::getline(ss, line)) {
            lines.push_back(line);
        }

        for (auto& fileLine : lines) {
            std::string cleanLine = fileLine;

            size_t commentPos = cleanLine.find(';');
            if (commentPos != std::string::npos) {
                cleanLine = cleanLine.substr(0, commentPos);
            }

            commentPos = cleanLine.find('#');
            if (commentPos != std::string::npos) {
                cleanLine = cleanLine.substr(0, commentPos);
            }

            cleanLine = Trim(cleanLine);

            if (cleanLine[0] == '[' && cleanLine.back() == ']') {
                currentSection = cleanLine.substr(1, cleanLine.length() - 2);
                continue;
            }

            if (currentSection == "Original backup") {
                size_t equalPos = cleanLine.find('=');
                if (equalPos != std::string::npos) {
                    std::string key = Trim(cleanLine.substr(0, equalPos));
                    if (key == "Backup") {
                        fileLine = "Backup = 0";
                        backupValueUpdated = true;
                        break;
                    }
                }
            }
        }

        if (!backupValueUpdated) {
            logFile << "Warning: Backup value not found in INI during update" << std::endl;
            return;
        }

        std::ofstream outFile(iniPath, std::ios::out | std::ios::trunc);
        if (!outFile.is_open()) {
            logFile << "ERROR: Could not open config INI file for writing" << std::endl;
            return;
        }

        for (const auto& outputLine : lines) {
            outFile << outputLine << std::endl;
        }

        outFile.close();
        if (outFile.fail()) {
            logFile << "ERROR: Failed to write config INI file" << std::endl;
        } else {
            logFile << "SUCCESS: Config updated (Backup = 0)" << std::endl;
        }

    } catch (const std::exception& e) {
        logFile << "ERROR in UpdateBackupConfigInIni: " << e.what() << std::endl;
    } catch (...) {
        logFile << "ERROR in UpdateBackupConfigInIni: Unknown exception" << std::endl;
    }
}

void UpdateIniRuleCount(const fs::path& iniPath, const std::string& originalLine, int newCount) {
    try {
        std::string content = ReadFileWithEncoding(iniPath);
        if (content.empty()) return;

        std::stringstream ss(content);
        std::vector<std::string> lines;
        std::string line;
        lines.reserve(200);

        while (std::getline(ss, line)) {
            lines.push_back(line);
        }

        for (auto& fileLine : lines) {
            std::string cleanLine = RemoveCommentsSafely(fileLine);
            std::string originalLineClean = Trim(originalLine);

            cleanLine = Trim(cleanLine);

            if (cleanLine == originalLineClean) {
                size_t lastPipe = fileLine.rfind('|');
                if (lastPipe != std::string::npos) {
                    std::string beforePipe = fileLine.substr(0, lastPipe + 1);
                    fileLine = beforePipe + std::to_string(newCount);
                }
                break;
            }
        }

        std::ofstream outFile(iniPath, std::ios::out | std::ios::trunc);
        if (outFile.is_open()) {
            for (const auto& outputLine : lines) {
                outFile << outputLine << std::endl;
            }
            outFile.close();
        }

    } catch (...) {
    }
}

ConflictResolution ResolveConflicts(RuleConflictTracker& conflictTracker,
                                    bool smartResolutionEnabled,
                                    std::ofstream& logFile) {
    ConflictResolution resolution;
    
    if (!smartResolutionEnabled) {
        logFile << "Smart Conflict Resolution DISABLED - All conflicting rules will be SKIPPED" << std::endl;
        
        for (const auto& [conflictKey, rules] : conflictTracker.exclusiveRules) {
            if (rules.size() > 1) {
                for (const auto& rule : rules) {
                    std::string ruleIdentifier = rule.filename + "|" + rule.originalLine;
                    resolution.ruleIsAllowed[ruleIdentifier] = false;
                }
            } else if (rules.size() == 1) {
                std::string ruleIdentifier = rules[0].filename + "|" + rules[0].originalLine;
                resolution.ruleIsAllowed[ruleIdentifier] = true;
            }
        }
        
        return resolution;
    }
    
    logFile << std::endl;
    logFile << "Smart Conflict Resolution ENABLED - Resolving conflicts automatically..." << std::endl;
    logFile << "CONFLICT RESOLUTION CRITERIA: Most recent INI file (newest modification date) wins" << std::endl;
    logFile << "----------------------------------------------------" << std::endl;
    
    for (const auto& [conflictKey, rules] : conflictTracker.exclusiveRules) {
        if (rules.size() > 1) {
            const ConflictRuleInfo* dominantRule = nullptr;
            fs::file_time_type mostRecentTime = fs::file_time_type::min();
            
            for (const auto& rule : rules) {
                if (rule.fileModTime > mostRecentTime) {
                    mostRecentTime = rule.fileModTime;
                    dominantRule = &rule;
                }
            }
            
            if (dominantRule) {
                resolution.dominantRules[conflictKey] = *dominantRule;
                
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    mostRecentTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                auto timeT = std::chrono::system_clock::to_time_t(sctp);
                
                std::tm tm;
                localtime_s(&tm, &timeT);
                char timeBuffer[32];
                strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &tm);
                
                logFile << "Conflict resolved for: " << conflictKey << std::endl;
                logFile << "  DOMINANT RULE (most recent file - will be applied):" << std::endl;
                logFile << "    FILE: " << dominantRule->filename << std::endl;
                logFile << "    MODIFIED: " << timeBuffer << std::endl;
                logFile << "    RULE: " << dominantRule->originalLine << std::endl;
                logFile << "  DISABLED RULES (older files - changed to |0):" << std::endl;
                
                for (const auto& rule : rules) {
                    std::string ruleIdentifier = rule.filename + "|" + rule.originalLine;
                    
                    if (rule.fileModTime == mostRecentTime &&
                        rule.filename == dominantRule->filename &&
                        rule.originalLine == dominantRule->originalLine) {
                        resolution.ruleIsAllowed[ruleIdentifier] = true;
                    } else {
                        resolution.ruleIsAllowed[ruleIdentifier] = false;
                        
                        UpdateIniRuleCount(rule.fullPath, rule.originalLine, 0);
                        
                        auto oldSctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                            rule.fileModTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                        );
                        auto oldTimeT = std::chrono::system_clock::to_time_t(oldSctp);
                        
                        std::tm oldTm;
                        localtime_s(&oldTm, &oldTimeT);
                        char oldTimeBuffer[32];
                        strftime(oldTimeBuffer, sizeof(oldTimeBuffer), "%Y-%m-%d %H:%M:%S", &oldTm);
                        
                        logFile << "    FILE: " << rule.filename << std::endl;
                        logFile << "    MODIFIED: " << oldTimeBuffer << " (OLDER)" << std::endl;
                        logFile << "    RULE: " << rule.originalLine << " -> changed to |0" << std::endl;
                    }
                }
                
                logFile << std::endl;
            }
            
        } else if (rules.size() == 1) {
            std::string ruleIdentifier = rules[0].filename + "|" + rules[0].originalLine;
            resolution.ruleIsAllowed[ruleIdentifier] = true;
        }
    }
    
    logFile << "----------------------------------------------------" << std::endl;
    logFile << std::endl;
    
    return resolution;
}

void GenerateConflictReport(const RuleConflictTracker& conflictTracker,
                           const ConflictResolution& resolution,
                           bool smartResolutionEnabled,
                           const fs::path& logINIAnalysisPath,
                           std::ofstream& mainLogFile) {
    
    std::ofstream iniLog(logINIAnalysisPath, std::ios::out | std::ios::app);
    if (!iniLog.is_open()) {
        mainLogFile << "ERROR: Could not update INI Analysis Log for conflicts" << std::endl;
        return;
    }
    
    bool hasConflicts = false;
    for (const auto& [conflictKey, rules] : conflictTracker.exclusiveRules) {
        if (rules.size() > 1) {
            hasConflicts = true;
            break;
        }
    }
    
    if (hasConflicts) {
        iniLog << std::endl;
        iniLog << "====================================================" << std::endl;
        iniLog << "PRIORITY CONFLICT OBODY PDA REPORT" << std::endl;
        iniLog << "====================================================" << std::endl;
        iniLog << std::endl;
        
        if (smartResolutionEnabled) {
            iniLog << "SMART CONFLICT RESOLUTION: ENABLED" << std::endl;
            iniLog << "RESOLUTION CRITERIA: Most recent INI file wins (newest modification date)" << std::endl;
            iniLog << "Conflicts have been automatically resolved." << std::endl;
            iniLog << "Dominant rules (from newest files) will be applied." << std::endl;
            iniLog << "Other conflicting rules (from older files) have been changed to |0" << std::endl;
        } else {
            iniLog << "SMART CONFLICT RESOLUTION: DISABLED" << std::endl;
            iniLog << "ALL conflicting rules have been SKIPPED (not applied)." << std::endl;
            iniLog << "No INI files have been modified." << std::endl;
        }
        
        iniLog << std::endl;
        iniLog << "MULTIPLE EXCLUSIVE RULES (*) DETECTED FOR SAME TARGET" << std::endl;
        iniLog << std::endl;
        iniLog << "When multiple INI rules use exclusive modes (*) for the same target," << std::endl;
        iniLog << "they conflict with each other. Only ONE exclusive rule can control" << std::endl;
        iniLog << "a target at a time." << std::endl;
        iniLog << std::endl;
        
        for (const auto& [conflictKey, rules] : conflictTracker.exclusiveRules) {
            if (rules.size() > 1) {
                auto parts = Split(conflictKey, '|');
                if (parts.size() == 2) {
                    iniLog << std::endl;
                    iniLog << "TARGET: " << parts[0] << " = " << parts[1] << std::endl;
                    
                    if (smartResolutionEnabled) {
                        iniLog << "RESOLUTION STATUS: RESOLVED AUTOMATICALLY" << std::endl;
                        iniLog << "CRITERIA: Most recent INI file (newest modification date)" << std::endl;
                        
                        auto dominantIt = resolution.dominantRules.find(conflictKey);
                        if (dominantIt != resolution.dominantRules.end()) {
                            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                                dominantIt->second.fileModTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                            );
                            auto timeT = std::chrono::system_clock::to_time_t(sctp);
                            
                            std::tm tm;
                            localtime_s(&tm, &timeT);
                            char timeBuffer[32];
                            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &tm);
                            
                            iniLog << "DOMINANT RULE (APPLIED - NEWEST FILE):" << std::endl;
                            iniLog << "  FILE: " << dominantIt->second.filename << std::endl;
                            iniLog << "  MODIFIED: " << timeBuffer << std::endl;
                            iniLog << "  RULE: " << dominantIt->second.originalLine << std::endl;
                            iniLog << std::endl;
                            iniLog << "DISABLED RULES (CHANGED TO |0 - OLDER FILES):" << std::endl;
                        }
                    } else {
                        iniLog << "RESOLUTION STATUS: NOT RESOLVED (ALL SKIPPED)" << std::endl;
                        iniLog << "CONFLICTING FILES AND RULES:" << std::endl;
                    }
                    
                    for (const auto& rule : rules) {
                        std::string ruleId = rule.filename + "|" + rule.originalLine;
                        bool isAllowed = resolution.ruleIsAllowed.count(ruleId) > 0 &&
                                        resolution.ruleIsAllowed.at(ruleId);
                        
                        if (!isAllowed || !smartResolutionEnabled) {
                            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                                rule.fileModTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                            );
                            auto timeT = std::chrono::system_clock::to_time_t(sctp);
                            
                            std::tm tm;
                            localtime_s(&tm, &timeT);
                            char timeBuffer[32];
                            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &tm);
                            
                            iniLog << "  FILE: " << rule.filename;
                            if (!isAllowed && smartResolutionEnabled) {
                                iniLog << " [DISABLED -> |0] (OLDER)";
                            }
                            iniLog << std::endl;
                            iniLog << "  MODIFIED: " << timeBuffer << std::endl;
                            iniLog << "  RULE: " << rule.originalLine << std::endl;
                            iniLog << std::endl;
                        }
                    }
                    
                    if (!smartResolutionEnabled) {
                        iniLog << std::endl;
                        iniLog << "SOLUTION OPTIONS:" << std::endl;
                        iniLog << "1. Enable Smart Conflict Resolution in configuration INI" << std::endl;
                        iniLog << "2. Manually choose ONE rule to keep as exclusive (*)" << std::endl;
                        iniLog << "3. Change others to additive (remove *) or disable (|0)" << std::endl;
                        iniLog << "4. Example fixes:" << std::endl;
                        iniLog << "   - Change: " << parts[0] << " = " << parts[1] << "|Preset1,Preset2|*" << std::endl;
                        iniLog << "   - To:     " << parts[0] << " = " << parts[1] << "|Preset1,Preset2|  (additive)" << std::endl;
                        iniLog << "   - Or:     " << parts[0] << " = " << parts[1] << "|Preset1,Preset2|0 (disabled)" << std::endl;
                    }
                    
                    iniLog << "----------------------------------------------------" << std::endl;
                }
            }
        }
        iniLog << std::endl;
    }
    
    iniLog.close();
    
    if (hasConflicts) {
        mainLogFile << "CONFLICTS DETECTED: Multiple exclusive rules (*) found. Check INI Analysis log." << std::endl;
        if (smartResolutionEnabled) {
            mainLogFile << "CONFLICTS RESOLVED: Newest INI files take priority." << std::endl;
        }
    }
}

std::pair<std::vector<std::string>, std::vector<UBEPresetInfo>> ProcessUBEXmlPresets(
    const fs::path& bodySlidePresetsPath, std::ofstream& logFile) {
    
    std::vector<std::string> allUBEPresetsForBlacklist;
    std::vector<UBEPresetInfo> ubePresetsInfo;
    std::vector<std::string> excludedFromRacesXmlFiles;
    
    try {
        if (!fs::exists(bodySlidePresetsPath)) {
            logFile << "WARNING: BodySlide presets folder not found: " << bodySlidePresetsPath.string() << std::endl;
            return {allUBEPresetsForBlacklist, ubePresetsInfo};
        }
        
        logFile << std::endl;
        logFile << "Scanning for UBE XML presets (with multi-preset support and exception handling)..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;
        
        int totalXmlScanned = 0;
        int totalUbeFound = 0;
        int totalConflicting = 0;
        int conflictingButNameHasUBE = 0;
        int multiPresetFilesProcessed = 0;
        int exception3BAFilesSkipped = 0;
        int resolvedBySetAttribute = 0;
        
        for (const auto& entry : fs::directory_iterator(bodySlidePresetsPath)) {
            try {
                if (entry.is_regular_file()) {
                    std::string filename;
                    try {
                        auto u8name = entry.path().filename().u8string();
                        filename = std::string(u8name.begin(), u8name.end());
                    } catch (...) {
                        try {
                            filename = entry.path().filename().string();
                        } catch (...) {
                            logFile << "  [ERROR] Could not read filename for entry" << std::endl;
                            continue;
                        }
                    }

                    if (EndsWith(filename, ".xml")) {
                        totalXmlScanned++;
                        
                        try {
                            XmlAnalysisResult analysis = AnalyzeXmlGroups(entry.path(), logFile);
                            
                            if (analysis.isException3BA) {
                                exception3BAFilesSkipped++;
                                logFile << "  [SKIP - 3BA EXCEPTION] " << filename << std::endl;
                                logFile << "    Detected 3BA exception set name (one of 5 variants)" << std::endl;
                                logFile << "    This preset is 3BA/CBBE, NOT UBE - skipping UBE categorization" << std::endl;
                                continue;
                            }
                            
                            if (analysis.hasUBE) {
                                std::vector<XmlPresetInfo> allPresetsInFile = ExtractAllPresetsFromXml(entry.path(), logFile);
                                
                                if (allPresetsInFile.empty()) {
                                    logFile << "  WARNING: UBE file but no presets extracted: " << filename << std::endl;
                                    continue;
                                }
                                
                                std::string detectionSource = "Groups";
                                if (analysis.setAnalysisPerformed && !analysis.setAttributeValue.empty()) {
                                    detectionSource = "Set attribute";
                                } else if (!analysis.setAnalysisPerformed) {
                                    detectionSource = "Filename";
                                }
                                
                                if (allPresetsInFile.size() > 1) {
                                    multiPresetFilesProcessed++;
                                    logFile << "  [UBE MULTI-PRESET via " << detectionSource << "] " << filename 
                                            << " contains " << allPresetsInFile.size() << " UBE presets" << std::endl;
                                } else {
                                    logFile << "  [UBE DETECTED via " << detectionSource << "] " << filename << std::endl;
                                }
                                
                                if (!analysis.setAttributeValue.empty()) {
                                    logFile << "    Set value: \"" << analysis.setAttributeValue << "\"" << std::endl;
                                }
                                
                                for (const auto& presetInfo : allPresetsInFile) {
                                    std::string presetName = presetInfo.internalName;
                                    
                                    if (presetName.empty()) {
                                        logFile << "    WARNING: Empty preset name in: " << filename << std::endl;
                                        continue;
                                    }
                                    
                                    std::string lowerPresetName = presetName;
                                    std::transform(lowerPresetName.begin(), lowerPresetName.end(), 
                                                 lowerPresetName.begin(), ::tolower);
                                    bool presetNameContainsUBE = (lowerPresetName.find("ube") != std::string::npos);
                                    
                                    if (analysis.hasConflictingGroups) {
                                        totalConflicting++;
                                        
                                        bool shouldBeUBE = false;
                                        std::string resolutionReason = "";
                                        
                                        if (analysis.isUBEBySet) {
                                            shouldBeUBE = true;
                                            resolutionReason = "UBE identifier in set attribute";
                                            resolvedBySetAttribute++;
                                        }
                                        else if (presetNameContainsUBE) {
                                            shouldBeUBE = true;
                                            resolutionReason = "preset name contains 'UBE'";
                                        }
                                        else {
                                            shouldBeUBE = false;
                                            resolutionReason = "no clear UBE indicator (excluded from UBE races)";
                                        }
                                        
                                        UBEPresetInfo info;
                                        info.presetName = presetName;
                                        info.hasConflict = true;
                                        info.conflictingGroups = analysis.conflictingGroupsFound;
                                        info.allowedInRaces = shouldBeUBE;
                                        
                                        allUBEPresetsForBlacklist.push_back(presetName);
                                        
                                        if (shouldBeUBE) {
                                            ubePresetsInfo.push_back(info);
                                            conflictingButNameHasUBE++;
                                            logFile << "    CONFLICT RESOLVED (added to UBE races): " 
                                                    << presetName << std::endl;
                                            logFile << "      Reason: " << resolutionReason << std::endl;
                                        } else {
                                            if (std::find(excludedFromRacesXmlFiles.begin(), excludedFromRacesXmlFiles.end(), 
                                                         filename) == excludedFromRacesXmlFiles.end()) {
                                                excludedFromRacesXmlFiles.push_back(filename);
                                            }
                                            logFile << "    CONFLICT (excluded from UBE races): " << presetName << std::endl;
                                            logFile << "      Reason: " << resolutionReason << std::endl;
                                        }
                                        
                                        logFile << "      Has UBE group but also contains: ";
                                        for (size_t i = 0; i < analysis.conflictingGroupsFound.size(); i++) {
                                            logFile << analysis.conflictingGroupsFound[i];
                                            if (i < analysis.conflictingGroupsFound.size() - 1) {
                                                logFile << ", ";
                                            }
                                        }
                                        logFile << std::endl;
                                        
                                    } else {
                                        UBEPresetInfo info;
                                        info.presetName = presetName;
                                        info.hasConflict = false;
                                        info.allowedInRaces = true;
                                        
                                        allUBEPresetsForBlacklist.push_back(presetName);
                                        ubePresetsInfo.push_back(info);
                                        totalUbeFound++;
                                        logFile << "    Found UBE preset: " << presetName << std::endl;
                                    }
                                }
                            }
                        } catch (const std::exception& e) {
                            logFile << "  [ERROR] Exception processing UBE in " << filename << ": " << e.what() << std::endl;
                        } catch (...) {
                            logFile << "  [ERROR] Unknown exception processing UBE in " << filename << std::endl;
                        }
                    }
                }
            } catch (const std::exception& e) {
                logFile << "  [ERROR] Exception in UBE directory iteration: " << e.what() << std::endl;
                continue;
            } catch (...) {
                logFile << "  [ERROR] Unknown exception in UBE directory iteration" << std::endl;
                continue;
            }
        }

        logFile << std::endl;
        logFile << "UBE XML Scan Summary:" << std::endl;
        logFile << "  Total XML files scanned: " << totalXmlScanned << std::endl;
        logFile << "  3BA exception files skipped: " << exception3BAFilesSkipped << std::endl;
        logFile << "  Multi-preset UBE files found: " << multiPresetFilesProcessed << std::endl;
        logFile << "  Valid UBE presets found: " << totalUbeFound << std::endl;
        logFile << "  Conflicting presets (UBE + 3BA/3BBB/CBBE): " << totalConflicting << std::endl;
        logFile << "  Conflicts resolved by set attribute: " << resolvedBySetAttribute << std::endl;
        logFile << "  Conflicts resolved by preset name: " << (conflictingButNameHasUBE - resolvedBySetAttribute) << std::endl;
        logFile << "  Total presets for blacklist: " << allUBEPresetsForBlacklist.size() << std::endl;
        logFile << "  Total presets for UBE races: " << ubePresetsInfo.size() << std::endl;
        
        if (totalConflicting > 0) {
            logFile << std::endl;
            logFile << "WARNING: The following presets have both UBE and 3BA/3BBB/CBBE indicators:" << std::endl;
            logFile << "All conflicting presets were added to blacklist." << std::endl;
            logFile << "Resolution priority: 1) Set attribute with UBE identifier, 2) Preset name contains 'UBE'" << std::endl;
            logFile << "Presets without clear UBE indicators were excluded from UBE races." << std::endl;
            logFile << "You can manually add excluded presets via INI rules if needed." << std::endl;
            logFile << std::endl;
        }
        
        if (exception3BAFilesSkipped > 0) {
            logFile << std::endl;
            logFile << "3BA EXCEPTION FILES DETECTED AND SKIPPED:" << std::endl;
            logFile << "----------------------------------------------------" << std::endl;
            logFile << exception3BAFilesSkipped << " file(s) with 3BA exception set names (5 variants)" << std::endl;
            logFile << "(CBBE/SE 3BBB Body Amazing UBE Anus variants)" << std::endl;
            logFile << "were detected and correctly skipped from UBE categorization." << std::endl;
            logFile << "These are 3BA/CBBE presets and will be available for normal (non-UBE) usage." << std::endl;
            logFile << std::endl;
        }
        
        if (!excludedFromRacesXmlFiles.empty()) {
            logFile << std::endl;
            logFile << "XML FILES EXCLUDED FROM UBE RACES:" << std::endl;
            logFile << "----------------------------------------------------" << std::endl;
            logFile << "The following XML files contain UBE groups but were excluded from UBE races" << std::endl;
            logFile << "because they also contain 3BA/3BBB/CBBE groups and lack clear UBE indicators." << std::endl;
            logFile << "Please review these files to determine if they are intended for UBE." << std::endl;
            logFile << std::endl;
            
            for (const auto& xmlFile : excludedFromRacesXmlFiles) {
                logFile << "  - " << xmlFile << std::endl;
            }
            
            logFile << std::endl;
            logFile << "If any of these presets should be included in UBE races, you can:" << std::endl;
            logFile << "  1. Add a UBE identifier to the set attribute (e.g., 'UBE SE 2.0')" << std::endl;
            logFile << "  2. Rename the preset name inside XML to include 'UBE'" << std::endl;
            logFile << "  3. Manually add them using INI rules in OBodyNG_PDA_*.ini files" << std::endl;
            logFile << "  4. Contact the preset author to clarify the intended body type" << std::endl;
            logFile << std::endl;
        }
        
        logFile << std::endl;
        
    } catch (const std::exception& e) {
        logFile << "ERROR scanning BodySlide presets: " << e.what() << std::endl;
    } catch (...) {
        logFile << "ERROR scanning BodySlide presets: Unknown exception" << std::endl;
    }
    
    return {allUBEPresetsForBlacklist, ubePresetsInfo};
}

std::pair<std::vector<std::string>, std::vector<HIMBOPresetInfo>> ProcessHIMBOXmlPresets(
    const fs::path& bodySlidePresetsPath, std::ofstream& logFile) {
    
    std::vector<std::string> allHIMBOPresetsForBlacklist;
    std::vector<HIMBOPresetInfo> himboPresetsInfo;
    std::vector<std::string> excludedFromRacesXmlFiles;
    
    try {
        if (!fs::exists(bodySlidePresetsPath)) {
            logFile << "WARNING: BodySlide presets folder not found: " << bodySlidePresetsPath.string() << std::endl;
            return {allHIMBOPresetsForBlacklist, himboPresetsInfo};
        }
        
        logFile << std::endl;
        logFile << "Scanning for HIMBO XML presets (with multi-preset support)..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;
        
        int totalXmlScanned = 0;
        int totalHimboFound = 0;
        int totalConflicting = 0;
        int conflictingButNameHasHIMBO = 0;
        int multiPresetFilesProcessed = 0;
        
        for (const auto& entry : fs::directory_iterator(bodySlidePresetsPath)) {
            try {
                if (entry.is_regular_file()) {
                    std::string filename;
                    try {
                        auto u8name = entry.path().filename().u8string();
                        filename = std::string(u8name.begin(), u8name.end());
                    } catch (...) {
                        try {
                            filename = entry.path().filename().string();
                        } catch (...) {
                            continue;
                        }
                    }

                    if (EndsWith(filename, ".xml")) {
                        totalXmlScanned++;
                        
                        try {
                            XmlAnalysisResult analysis = AnalyzeXmlGroups(entry.path(), logFile);
                            
                            if (analysis.hasHIMBO) {
                                std::vector<XmlPresetInfo> allPresetsInFile = ExtractAllPresetsFromXml(entry.path(), logFile);
                                
                                if (allPresetsInFile.empty()) {
                                    logFile << "  WARNING: HIMBO file but no presets extracted: " << filename << std::endl;
                                    continue;
                                }
                                
                                std::string detectionSource = "Groups";
                                if (analysis.setAnalysisPerformed && !analysis.setAttributeValue.empty()) {
                                    detectionSource = "Set attribute";
                                } else if (!analysis.setAnalysisPerformed) {
                                    detectionSource = "Filename";
                                }
                                
                                if (allPresetsInFile.size() > 1) {
                                    multiPresetFilesProcessed++;
                                    logFile << "  [HIMBO MULTI-PRESET via " << detectionSource << "] " << filename 
                                            << " contains " << allPresetsInFile.size() << " HIMBO presets" << std::endl;
                                } else {
                                    logFile << "  [HIMBO DETECTED via " << detectionSource << "] " << filename << std::endl;
                                }
                                
                                if (!analysis.setAttributeValue.empty()) {
                                    logFile << "    Set value: \"" << analysis.setAttributeValue << "\"" << std::endl;
                                }
                                
                                for (const auto& presetInfo : allPresetsInFile) {
                                    std::string presetName = presetInfo.internalName;
                                    
                                    if (presetName.empty()) {
                                        logFile << "    WARNING: Empty preset name in: " << filename << std::endl;
                                        continue;
                                    }
                                    
                                    std::string lowerPresetName = presetName;
                                    std::transform(lowerPresetName.begin(), lowerPresetName.end(), 
                                                 lowerPresetName.begin(), ::tolower);
                                    bool filenameContainsHIMBO = (lowerPresetName.find("himbo") != std::string::npos);
                                    
                                    if (analysis.hasUBE || analysis.hasConflictingGroups) {
                                        totalConflicting++;
                                        
                                        HIMBOPresetInfo info;
                                        info.presetName = presetName;
                                        info.hasConflict = true;
                                        info.conflictingGroups = analysis.conflictingGroupsFound;
                                        info.allowedInRaces = filenameContainsHIMBO;
                                        
                                        allHIMBOPresetsForBlacklist.push_back(presetName);
                                        
                                        if (filenameContainsHIMBO) {
                                            himboPresetsInfo.push_back(info);
                                            conflictingButNameHasHIMBO++;
                                            logFile << "    HIMBO CONFLICT (preset has HIMBO in name, added to male races): " 
                                                    << presetName << std::endl;
                                        } else {
                                            if (std::find(excludedFromRacesXmlFiles.begin(), excludedFromRacesXmlFiles.end(), 
                                                         filename) == excludedFromRacesXmlFiles.end()) {
                                                excludedFromRacesXmlFiles.push_back(filename);
                                            }
                                            logFile << "    HIMBO CONFLICT (excluded from male races): " << presetName << std::endl;
                                        }
                                        
                                        if (analysis.hasUBE) {
                                            logFile << "      Has HIMBO group but also contains UBE" << std::endl;
                                        }
                                        if (!analysis.conflictingGroupsFound.empty()) {
                                            logFile << "      Has HIMBO group but also contains: ";
                                            for (size_t i = 0; i < analysis.conflictingGroupsFound.size(); i++) {
                                                logFile << analysis.conflictingGroupsFound[i];
                                                if (i < analysis.conflictingGroupsFound.size() - 1) {
                                                    logFile << ", ";
                                                }
                                            }
                                            logFile << std::endl;
                                        }
                                        
                                    } else {
                                        HIMBOPresetInfo info;
                                        info.presetName = presetName;
                                        info.hasConflict = false;
                                        info.allowedInRaces = true;
                                        
                                        allHIMBOPresetsForBlacklist.push_back(presetName);
                                        himboPresetsInfo.push_back(info);
                                        totalHimboFound++;
                                        logFile << "    Found HIMBO preset: " << presetName << std::endl;
                                    }
                                }
                            }
                        } catch (const std::exception& e) {
                            logFile << "  [ERROR] Exception processing HIMBO in " << filename << ": " << e.what() << std::endl;
                        } catch (...) {
                            logFile << "  [ERROR] Unknown exception processing HIMBO in " << filename << std::endl;
                        }
                    }
                }
            } catch (const std::exception& e) {
                logFile << "  [ERROR] Exception in HIMBO directory iteration: " << e.what() << std::endl;
                continue;
            } catch (...) {
                logFile << "  [ERROR] Unknown exception in HIMBO directory iteration" << std::endl;
                continue;
            }
        }

        logFile << std::endl;
        logFile << "HIMBO XML Scan Summary:" << std::endl;
        logFile << "  Total XML files scanned: " << totalXmlScanned << std::endl;
        logFile << "  Multi-preset HIMBO files found: " << multiPresetFilesProcessed << std::endl;
        logFile << "  Valid HIMBO presets found: " << totalHimboFound << std::endl;
        logFile << "  Conflicting presets (HIMBO + UBE/3BA/3BBB/CBBE): " << totalConflicting << std::endl;
        logFile << "  Conflicting but preset name has HIMBO (added to male races): " << conflictingButNameHasHIMBO << std::endl;
        logFile << "  Total presets for blacklist: " << allHIMBOPresetsForBlacklist.size() << std::endl;
        logFile << "  Total presets for HIMBO male races: " << himboPresetsInfo.size() << std::endl;
        
        if (!excludedFromRacesXmlFiles.empty()) {
            logFile << std::endl;
            logFile << "XML FILES EXCLUDED FROM HIMBO MALE RACES:" << std::endl;
            logFile << "----------------------------------------------------" << std::endl;
            logFile << "The following XML files contain HIMBO groups but were excluded from male races" << std::endl;
            logFile << "because they also contain UBE/3BA/3BBB/CBBE groups and don't have 'HIMBO' in the preset name." << std::endl;
            logFile << std::endl;
            
            for (const auto& xmlFile : excludedFromRacesXmlFiles) {
                logFile << "  - " << xmlFile << std::endl;
            }
            
            logFile << std::endl;
        }
        
        logFile << std::endl;
        
    } catch (const std::exception& e) {
        logFile << "ERROR scanning BodySlide presets for HIMBO: " << e.what() << std::endl;
    } catch (...) {
        logFile << "ERROR scanning BodySlide presets for HIMBO: Unknown exception" << std::endl;
    }
    
    return {allHIMBOPresetsForBlacklist, himboPresetsInfo};
}

bool ApplyUBEPresetsToJson(std::map<std::string, OrderedPluginData>& processedData,
                           const std::vector<std::string>& allPresetsForBlacklist,
                           const std::vector<UBEPresetInfo>& presetsForRaces,
                           std::ofstream& logFile) {
    if (allPresetsForBlacklist.empty()) {
        logFile << "No UBE presets to apply (none found)" << std::endl;
        return false;
    }
    
    try {
        logFile << "Applying UBE presets to JSON..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;
        
        int presetsAddedToBlacklist = 0;
        int racesCreatedOrUpdated = 0;
        int totalPresetsAddedToRaces = 0;
        int excludedPresetsCount = 0;
        
        auto& blacklistData = processedData["blacklistedPresetsFromRandomDistribution"];
        
        for (const auto& presetName : allPresetsForBlacklist) {
            bool alreadyExists = false;
            for (const auto& [plugin, presets] : blacklistData.orderedData) {
                if (std::find(presets.begin(), presets.end(), presetName) != presets.end()) {
                    alreadyExists = true;
                    break;
                }
            }
            
            if (!alreadyExists) {
                blacklistData.addPreset("", presetName);
                presetsAddedToBlacklist++;
                logFile << "  Added to blacklist: " << presetName << std::endl;
            }
        }
        
        auto& raceFemaleData = processedData["raceFemale"];
        
        for (const auto& ubeRace : UBE_RACES) {
            bool raceWasCreated = !raceFemaleData.hasPlugin(ubeRace);
            int presetsAddedToThisRace = 0;
            
            for (const auto& presetInfo : presetsForRaces) {
                if (!presetInfo.allowedInRaces) {
                    continue;
                }
                
                const std::string& presetName = presetInfo.presetName;
                
                bool isExcluded = std::find(EXCLUDED_FROM_UBE_RACES.begin(), EXCLUDED_FROM_UBE_RACES.end(), 
                                           presetName) != EXCLUDED_FROM_UBE_RACES.end();
                
                if (isExcluded) {
                    if (raceWasCreated && presetsAddedToThisRace == 0) {
                        excludedPresetsCount++;
                    }
                    continue;
                }
                
                bool presetExists = false;
                for (const auto& [race, presets] : raceFemaleData.orderedData) {
                    if (race == ubeRace) {
                        if (std::find(presets.begin(), presets.end(), presetName) != presets.end()) {
                            presetExists = true;
                            break;
                        }
                    }
                }
                
                if (!presetExists) {
                    raceFemaleData.addPreset(ubeRace, presetName);
                    presetsAddedToThisRace++;
                    totalPresetsAddedToRaces++;
                }
            }
            
            if (raceWasCreated && presetsAddedToThisRace > 0) {
                racesCreatedOrUpdated++;
                logFile << "  Created race: " << ubeRace << " with " << presetsAddedToThisRace << " presets"
                        << std::endl;
            } else if (presetsAddedToThisRace > 0) {
                logFile << "  Updated race: " << ubeRace << " (+" << presetsAddedToThisRace << " presets)"
                        << std::endl;
            }
        }
        
        logFile << std::endl;
        logFile << "UBE Application Summary:" << std::endl;
        logFile << "  Presets added to blacklist: " << presetsAddedToBlacklist << std::endl;
        logFile << "  UBE races created/updated: " << racesCreatedOrUpdated << std::endl;
        logFile << "  Total presets added to races: " << totalPresetsAddedToRaces << std::endl;
        if (excludedPresetsCount > 0) {
            logFile << "  Presets excluded from UBE races (Zeroed Sliders variants): " 
                    << excludedPresetsCount << std::endl;
        }
        logFile << std::endl;
        
        return (presetsAddedToBlacklist > 0 || totalPresetsAddedToRaces > 0);
        
    } catch (const std::exception& e) {
        logFile << "ERROR in ApplyUBEPresetsToJson: " << e.what() << std::endl;
        return false;
    } catch (...) {
        logFile << "ERROR in ApplyUBEPresetsToJson: Unknown exception" << std::endl;
        return false;
    }
}

bool ApplyHIMBOPresetsToJson(std::map<std::string, OrderedPluginData>& processedData,
                              const std::vector<std::string>& allPresetsForBlacklist,
                              const std::vector<HIMBOPresetInfo>& presetsForRaces,
                              const ConfigSettings& config,
                              std::ofstream& logFile) {
    
    if (!config.modeHIMBO) {
        logFile << "HIMBO mode disabled in configuration, skipping HIMBO preset application" << std::endl;
        return false;
    }
    
    if (allPresetsForBlacklist.empty()) {
        logFile << "No HIMBO presets to apply (none found)" << std::endl;
        return false;
    }
    
    try {
        logFile << "Applying HIMBO presets to JSON (ModeHIMBO = true)..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;
        
        int presetsAddedToBlacklist = 0;
        int racesCreatedOrUpdated = 0;
        int totalPresetsAddedToRaces = 0;
        
        auto& blacklistData = processedData["blacklistedPresetsFromRandomDistribution"];
        
        for (const auto& presetName : allPresetsForBlacklist) {
            bool alreadyExists = false;
            for (const auto& [plugin, presets] : blacklistData.orderedData) {
                if (std::find(presets.begin(), presets.end(), presetName) != presets.end()) {
                    alreadyExists = true;
                    break;
                }
            }
            
            if (!alreadyExists) {
                blacklistData.addPreset("", presetName);
                presetsAddedToBlacklist++;
                logFile << "  Added HIMBO to blacklist: " << presetName << std::endl;
            }
        }
        
        auto& raceMaleData = processedData["raceMale"];
        
        for (const auto& race : HIMBO_RACES) {
            bool raceWasCreated = !raceMaleData.hasPlugin(race);
            int presetsAddedToThisRace = 0;
            
            for (const auto& presetInfo : presetsForRaces) {
                if (!presetInfo.allowedInRaces) {
                    continue;
                }
                
                const std::string& presetName = presetInfo.presetName;
                
                bool presetExists = false;
                for (const auto& [existingRace, presets] : raceMaleData.orderedData) {
                    if (existingRace == race) {
                        if (std::find(presets.begin(), presets.end(), presetName) != presets.end()) {
                            presetExists = true;
                            break;
                        }
                    }
                }
                
                if (!presetExists) {
                    raceMaleData.addPreset(race, presetName);
                    presetsAddedToThisRace++;
                    totalPresetsAddedToRaces++;
                }
            }
            
            if (raceWasCreated && presetsAddedToThisRace > 0) {
                racesCreatedOrUpdated++;
                logFile << "  Created male race: " << race << " with " << presetsAddedToThisRace << " HIMBO presets"
                        << std::endl;
            } else if (presetsAddedToThisRace > 0) {
                logFile << "  Updated male race: " << race << " (+" << presetsAddedToThisRace << " HIMBO presets)"
                        << std::endl;
            }
        }
        
        logFile << std::endl;
        logFile << "HIMBO Application Summary:" << std::endl;
        logFile << "  HIMBO presets added to blacklist: " << presetsAddedToBlacklist << std::endl;
        logFile << "  Male races created/updated: " << racesCreatedOrUpdated << std::endl;
        logFile << "  Total HIMBO presets added to male races: " << totalPresetsAddedToRaces << std::endl;
        logFile << std::endl;
        
        return (presetsAddedToBlacklist > 0 || totalPresetsAddedToRaces > 0);
        
    } catch (const std::exception& e) {
        logFile << "ERROR in ApplyHIMBOPresetsToJson: " << e.what() << std::endl;
        return false;
    } catch (...) {
        logFile << "ERROR in ApplyHIMBOPresetsToJson: Unknown exception" << std::endl;
        return false;
    }
}

bool ApplySpecialRules(std::map<std::string, OrderedPluginData>& processedData,
                      const std::vector<SpecialRule>& specialRules,
                      const PresetMapData& masterPresetMap,
                      std::ofstream& logFile) {
    
    if (specialRules.empty()) {
        return false;
    }
    
    try {
        logFile << std::endl;
        logFile << "PHASE 3.5: Processing Special Rules (UBE races, Any races, Blacklists, Outfits)..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;
        
        int totalSpecialRulesProcessed = 0;
        int totalSpecialRulesApplied = 0;
        int totalSpecialRulesSkipped = 0;
        int totalSpecialPresetsAdded = 0;
        int totalSpecialPresetsRemoved = 0;
        
        for (const auto& rule : specialRules) {
            totalSpecialRulesProcessed++;
            
            if (rule.targetKey.empty()) {
                logFile << "  Skipped special rule (invalid target key): " << rule.ruleType << std::endl;
                totalSpecialRulesSkipped++;
                continue;
            }
            
            bool shouldApply = false;
            
            if (std::find(NPC_FORMID_TYPES.begin(), NPC_FORMID_TYPES.end(), rule.targetKey) != NPC_FORMID_TYPES.end()) {
                shouldApply = true;
                
                if (rule.applyCount == 0 || rule.mode == INIRuleMode::DISABLED) {
                    totalSpecialRulesSkipped++;
                    logFile << "  SKIPPED (disabled): " << rule.targetKey << " -> " << rule.plugin << std::endl;
                    continue;
                }
                
                auto& data = processedData[rule.targetKey];
                
                if (rule.mode == INIRuleMode::EXCLUSIVE_ALWAYS) {
                    if (data.hasPlugin(rule.plugin)) {
                        data.removePlugin(rule.plugin);
                    }
                    for (const auto& formID : rule.presets) {
                        data.addPreset(rule.plugin, formID);
                        totalSpecialPresetsAdded++;
                    }
                    logFile << "  Applied FormID NPC exclusive: " << rule.targetKey 
                            << " -> " << rule.plugin 
                            << " -> Replaced with " << rule.presets.size() << " FormIDs" << std::endl;
                    totalSpecialRulesApplied++;
                    
                } else if (rule.mode == INIRuleMode::REMOVE_ALWAYS || rule.mode == INIRuleMode::REMOVE_ONCE) {
                    int removedCount = 0;
                    for (const auto& formID : rule.presets) {
                        size_t beforeCount = data.getTotalPresetCount();
                        data.removePreset(rule.plugin, formID);
                        if (data.getTotalPresetCount() < beforeCount) {
                            removedCount++;
                            totalSpecialPresetsRemoved++;
                        }
                    }
                    logFile << "  Applied FormID NPC removal: " << rule.targetKey 
                            << " -> " << rule.plugin 
                            << " -> Removed " << removedCount << " FormIDs" << std::endl;
                    totalSpecialRulesApplied++;
                    
                } else {
                    for (const auto& formID : rule.presets) {
                        size_t beforeCount = data.getTotalPresetCount();
                        data.addPreset(rule.plugin, formID);
                        if (data.getTotalPresetCount() > beforeCount) {
                            totalSpecialPresetsAdded++;
                        }
                    }
                    logFile << "  Applied FormID NPC additive: " << rule.targetKey 
                            << " -> " << rule.plugin 
                            << " -> Added " << rule.presets.size() << " FormIDs" << std::endl;
                    totalSpecialRulesApplied++;
                }
                
                continue;
            }
            
            if (std::find(OUTFIT_FORMID_TYPES.begin(), OUTFIT_FORMID_TYPES.end(), rule.targetKey) != OUTFIT_FORMID_TYPES.end()) {
                shouldApply = true;
                
                if (rule.applyCount == 0 || rule.mode == INIRuleMode::DISABLED) {
                    totalSpecialRulesSkipped++;
                    logFile << "  SKIPPED (disabled): " << rule.targetKey << " -> " << rule.plugin << std::endl;
                    continue;
                }
                
                auto& data = processedData[rule.targetKey];
                
                if (rule.mode == INIRuleMode::EXCLUSIVE_ALWAYS) {
                    if (data.hasPlugin(rule.plugin)) {
                        data.removePlugin(rule.plugin);
                    }
                    for (const auto& formID : rule.presets) {
                        data.addPreset(rule.plugin, formID);
                        totalSpecialPresetsAdded++;
                    }
                    logFile << "  Applied Outfit FormID exclusive: " << rule.targetKey 
                            << " -> " << rule.plugin 
                            << " -> Replaced with " << rule.presets.size() << " FormIDs" << std::endl;
                    totalSpecialRulesApplied++;
                    
                } else if (rule.mode == INIRuleMode::REMOVE_ALWAYS || rule.mode == INIRuleMode::REMOVE_ONCE) {
                    int removedCount = 0;
                    for (const auto& formID : rule.presets) {
                        size_t beforeCount = data.getTotalPresetCount();
                        data.removePreset(rule.plugin, formID);
                        if (data.getTotalPresetCount() < beforeCount) {
                            removedCount++;
                            totalSpecialPresetsRemoved++;
                        }
                    }
                    logFile << "  Applied Outfit FormID removal: " << rule.targetKey 
                            << " -> " << rule.plugin 
                            << " -> Removed " << removedCount << " FormIDs" << std::endl;
                    totalSpecialRulesApplied++;
                    
                } else {
                    for (const auto& formID : rule.presets) {
                        size_t beforeCount = data.getTotalPresetCount();
                        data.addPreset(rule.plugin, formID);
                        if (data.getTotalPresetCount() > beforeCount) {
                            totalSpecialPresetsAdded++;
                        }
                    }
                    logFile << "  Applied Outfit FormID additive: " << rule.targetKey 
                            << " -> " << rule.plugin 
                            << " -> Added " << rule.presets.size() << " FormIDs" << std::endl;
                    totalSpecialRulesApplied++;
                }
                
                continue;
            }
            
            if (std::find(OUTFIT_ARRAY_TYPES.begin(), OUTFIT_ARRAY_TYPES.end(), rule.targetKey) != OUTFIT_ARRAY_TYPES.end()) {
                shouldApply = true;
                
                if (rule.applyCount == 0 || rule.mode == INIRuleMode::DISABLED) {
                    totalSpecialRulesSkipped++;
                    logFile << "  SKIPPED (disabled): Outfit array " << rule.targetKey << std::endl;
                    continue;
                }
                
                auto& data = processedData[rule.targetKey];
                
                if (rule.mode == INIRuleMode::EXCLUSIVE_ALWAYS) {
                    data.orderedData.clear();
                    for (const auto& item : rule.presets) {
                        data.addPreset("", item);
                        totalSpecialPresetsAdded++;
                    }
                    logFile << "  Applied Outfit exclusive: " << rule.targetKey 
                            << " -> Replaced with " << rule.presets.size() << " items" << std::endl;
                    totalSpecialRulesApplied++;
                    
                } else if (rule.mode == INIRuleMode::REMOVE_ALWAYS || rule.mode == INIRuleMode::REMOVE_ONCE) {
                    int removedCount = 0;
                    for (const auto& item : rule.presets) {
                        size_t beforeCount = data.getTotalPresetCount();
                        data.removePreset("", item);
                        if (data.getTotalPresetCount() < beforeCount) {
                            removedCount++;
                            totalSpecialPresetsRemoved++;
                        }
                    }
                    logFile << "  Applied Outfit removal: " << rule.targetKey 
                            << " -> Removed " << removedCount << " items" << std::endl;
                    totalSpecialRulesApplied++;
                    
                } else {
                    int addedCount = 0;
                    for (const auto& item : rule.presets) {
                        size_t beforeCount = data.getTotalPresetCount();
                        data.addPreset("", item);
                        if (data.getTotalPresetCount() > beforeCount) {
                            addedCount++;
                            totalSpecialPresetsAdded++;
                        }
                    }
                    logFile << "  Applied Outfit additive: " << rule.targetKey 
                            << " -> Added " << addedCount << " new items" << std::endl;
                    totalSpecialRulesApplied++;
                }
                
                continue;
            }
            
            if (rule.mode >= INIRuleMode::KEYWORD && rule.mode <= INIRuleMode::KEYHIMBO_REMOVE_ONCE) {
                shouldApply = true;
                
                auto& data = processedData[rule.targetKey];
                std::vector<std::string> matchingPresets;
                std::vector<std::string> notFoundPresets;
                
                switch (rule.mode) {
                    case INIRuleMode::KEYWORD:
                    case INIRuleMode::KEYWORD_EXCLUSIVE:
                    case INIRuleMode::KEYWORD_REMOVE:
                    case INIRuleMode::KEYWORD_ONCE:
                    case INIRuleMode::KEYWORD_EXCLUSIVE_ONCE:
                    case INIRuleMode::KEYWORD_REMOVE_ONCE:
                        matchingPresets = FindMatchingPresetsByKeyWord(masterPresetMap, rule.filterFragments, logFile);
                        break;
                        
                    case INIRuleMode::KEYWORDCHART:
                    case INIRuleMode::KEYWORDCHART_EXCLUSIVE:
                    case INIRuleMode::KEYWORDCHART_REMOVE:
                    case INIRuleMode::KEYWORDCHART_ONCE:
                    case INIRuleMode::KEYWORDCHART_EXCLUSIVE_ONCE:
                    case INIRuleMode::KEYWORDCHART_REMOVE_ONCE:
                        matchingPresets = FindMatchingPresetsByKeyWordChart(masterPresetMap, rule.filterFragments, logFile);
                        break;
                        
                    case INIRuleMode::KEYAUTHOR:
                    case INIRuleMode::KEYAUTHOR_EXCLUSIVE:
                    case INIRuleMode::KEYAUTHOR_REMOVE:
                    case INIRuleMode::KEYAUTHOR_ONCE:
                    case INIRuleMode::KEYAUTHOR_EXCLUSIVE_ONCE:
                    case INIRuleMode::KEYAUTHOR_REMOVE_ONCE:
                        matchingPresets = FindMatchingPresetsByKeyAuthor(masterPresetMap, rule.filterFragments, logFile);
                        break;
                        
                    case INIRuleMode::KEYNORMAL:
                    case INIRuleMode::KEYNORMAL_EXCLUSIVE:
                    case INIRuleMode::KEYNORMAL_REMOVE:
                    case INIRuleMode::KEYNORMAL_ONCE:
                    case INIRuleMode::KEYNORMAL_EXCLUSIVE_ONCE:
                    case INIRuleMode::KEYNORMAL_REMOVE_ONCE:
                        matchingPresets = FindMatchingPresetsByFamily(masterPresetMap, rule.filterFragments, "KeyNormal", logFile, notFoundPresets);
                        break;
                        
                    case INIRuleMode::KEYUBE:
                    case INIRuleMode::KEYUBE_EXCLUSIVE:
                    case INIRuleMode::KEYUBE_REMOVE:
                    case INIRuleMode::KEYUBE_ONCE:
                    case INIRuleMode::KEYUBE_EXCLUSIVE_ONCE:
                    case INIRuleMode::KEYUBE_REMOVE_ONCE:
                        matchingPresets = FindMatchingPresetsByFamily(masterPresetMap, rule.filterFragments, "KeyUBE", logFile, notFoundPresets);
                        break;
                        
                    case INIRuleMode::KEYHIMBO:
                    case INIRuleMode::KEYHIMBO_EXCLUSIVE:
                    case INIRuleMode::KEYHIMBO_REMOVE:
                    case INIRuleMode::KEYHIMBO_ONCE:
                    case INIRuleMode::KEYHIMBO_EXCLUSIVE_ONCE:
                    case INIRuleMode::KEYHIMBO_REMOVE_ONCE:
                        matchingPresets = FindMatchingPresetsByFamily(masterPresetMap, rule.filterFragments, "KeyHIMBO", logFile, notFoundPresets);
                        break;
                        
                    default:
                        break;
                }
                
                for (const auto& notFound : notFoundPresets) {
                    logFile << "    WARNING: Special rule preset '" << notFound << "' not found in specified family - SKIPPED" << std::endl;
                }
                
                if (!matchingPresets.empty()) {
                    if (IsExclusiveMode(rule.mode)) {
                        if (data.hasPlugin(rule.plugin)) {
                            data.removePlugin(rule.plugin);
                        }
                        for (const auto& preset : matchingPresets) {
                            data.addPreset(rule.plugin, preset);
                        }
                        logFile << "  Applied special exclusive filtering: " << rule.ruleType 
                                << " -> Target: " << rule.plugin << " -> Replaced with " 
                                << matchingPresets.size() << " matching presets" << std::endl;
                        
                    } else if (IsRemovalMode(rule.mode)) {
                        int removedCount = 0;
                        for (const auto& preset : matchingPresets) {
                            size_t beforeCount = data.getTotalPresetCount();
                            data.removePreset(rule.plugin, preset);
                            if (data.getTotalPresetCount() < beforeCount) {
                                removedCount++;
                                totalSpecialPresetsRemoved++;
                            }
                        }
                        logFile << "  Applied special removal filtering: " << rule.ruleType 
                                << " -> Target: " << rule.plugin << " -> Removed " 
                                << removedCount << " matching presets" << std::endl;
                        
                    } else {
                        int addedCount = 0;
                        for (const auto& preset : matchingPresets) {
                            size_t beforeCount = data.getTotalPresetCount();
                            data.addPreset(rule.plugin, preset);
                            if (data.getTotalPresetCount() > beforeCount) {
                                addedCount++;
                                totalSpecialPresetsAdded++;
                            }
                        }
                        logFile << "  Applied special additive filtering: " << rule.ruleType 
                                << " -> Target: " << rule.plugin << " -> Added " 
                                << addedCount << " new matching presets" << std::endl;
                    }
                    
                    totalSpecialRulesApplied++;
                } else {
                    logFile << "  No presets matched special filtering criteria: " << rule.ruleType 
                            << " -> Target: " << rule.plugin << std::endl;
                }
                
            } else if (rule.applyCount == -1 || rule.applyCount > 0) {
                shouldApply = true;
                
                auto& data = processedData[rule.targetKey];
                
                int presetsAdded = 0;
                for (const auto& preset : rule.presets) {
                    size_t beforeCount = data.getTotalPresetCount();
                    data.addPreset(rule.plugin, preset);
                    if (data.getTotalPresetCount() > beforeCount) {
                        presetsAdded++;
                        totalSpecialPresetsAdded++;
                    }
                }
                
                if (presetsAdded > 0) {
                    totalSpecialRulesApplied++;
                    logFile << "  Applied special rule: " << rule.ruleType
                            << " -> Target: " << rule.plugin << " -> Added "
                            << presetsAdded << " new presets" << std::endl;
                } else {
                    logFile << "  No new presets added (all already exist): " << rule.ruleType
                            << " -> Target: " << rule.plugin << std::endl;
                }
                
            } else {
                shouldApply = false;
                totalSpecialRulesSkipped++;
                logFile << "  Skipped special rule (count=0): " << rule.ruleType
                        << " -> Target: " << rule.plugin << std::endl;
            }
        }
        
        logFile << std::endl;
        logFile << "Special Rules Summary:" << std::endl;
        logFile << "  Total special rules processed: " << totalSpecialRulesProcessed << std::endl;
        logFile << "  Total special rules applied: " << totalSpecialRulesApplied << std::endl;
        logFile << "  Total special rules skipped: " << totalSpecialRulesSkipped << std::endl;
        logFile << "  Total special presets added: " << totalSpecialPresetsAdded << std::endl;
        logFile << "  Total special presets removed: " << totalSpecialPresetsRemoved << std::endl;
        logFile << std::endl;
        
        return (totalSpecialRulesApplied > 0);
        
    } catch (const std::exception& e) {
        logFile << "ERROR in ApplySpecialRules: " << e.what() << std::endl;
        return false;
    } catch (...) {
        logFile << "ERROR in ApplySpecialRules: Unknown exception" << std::endl;
        return false;
    }
}

bool PerformSimpleJsonIntegrityCheck(const fs::path& jsonPath, std::ofstream& logFile) {
    try {
        logFile << "Performing SIMPLE JSON integrity check at startup..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;

        if (!fs::exists(jsonPath)) {
            logFile << "ERROR: JSON file does not exist at: " << jsonPath.string() << std::endl;
            return false;
        }

        auto fileSize = fs::file_size(jsonPath);
        if (fileSize < 10) {
            logFile << "ERROR: JSON file is too small (" << fileSize << " bytes)" << std::endl;
            return false;
        }
        
        std::string content = ReadFileWithEncoding(jsonPath);
        if (content.empty()) {
            logFile << "ERROR: JSON file is empty after reading" << std::endl;
            return false;
        }

        logFile << "JSON file size: " << fileSize << " bytes" << std::endl;

        content = content.substr(content.find_first_not_of(" \t\r\n"));
        content = content.substr(0, content.find_last_not_of(" \t\r\n") + 1);

        if (content.empty() || content[0] != '{' || content[content.length() - 1] != '}') {
            logFile << "ERROR: JSON does not start with '{' or end with '}'" << std::endl;
            return false;
        }

        int braceCount = 0;
        int bracketCount = 0;
        int parenCount = 0;
        bool inString = false;
        bool escape = false;
        int line = 1;
        int col = 1;

        for (size_t i = 0; i < content.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(content[i]);
            
            if (c == '\n') {
                line++;
                col = 1;
                continue;
            }
            col++;

            if (escape) {
                escape = false;
                continue;
            }

            if (c == '\\') {
                escape = true;
                continue;
            }

            if (c == '"') {
                inString = !inString;
                continue;
            }

            if (!inString) {
                switch (c) {
                    case '{':
                        braceCount++;
                        break;
                    case '}':
                        braceCount--;
                        if (braceCount < 0) {
                            logFile << "ERROR: Unbalanced closing brace '}' at line " << line << ", column " << col
                                    << std::endl;
                            return false;
                        }
                        break;
                    case '[':
                        bracketCount++;
                        break;
                    case ']':
                        bracketCount--;
                        if (bracketCount < 0) {
                            logFile << "ERROR: Unbalanced closing bracket ']' at line " << line << ", column " << col
                                    << std::endl;
                            return false;
                        }
                        break;
                    case '(':
                        parenCount++;
                        break;
                    case ')':
                        parenCount--;
                        if (parenCount < 0) {
                            logFile << "ERROR: Unbalanced closing parenthesis ')' at line " << line << ", column "
                                    << col << std::endl;
                            return false;
                        }
                        break;
                }
            }
        }

        if (braceCount != 0) {
            logFile << "ERROR: Unbalanced braces (missing " << (braceCount > 0 ? "closing" : "opening")
                    << " braces: " << abs(braceCount) << ")" << std::endl;
            return false;
        }

        if (bracketCount != 0) {
            logFile << "ERROR: Unbalanced brackets (missing " << (bracketCount > 0 ? "closing" : "opening")
                    << " brackets: " << abs(bracketCount) << ")" << std::endl;
            return false;
        }

        if (parenCount != 0) {
            logFile << "ERROR: Unbalanced parentheses (missing " << (parenCount > 0 ? "closing" : "opening")
                    << " parentheses: " << abs(parenCount) << ")" << std::endl;
            return false;
        }

        const std::vector<std::string> expectedKeys = {
            "npcFormID",       "npc",           "factionFemale", "factionMale",
            "npcPluginFemale", "npcPluginMale", "raceFemale",    "raceMale"};

        int foundKeys = 0;
        for (const auto& key : expectedKeys) {
            if (content.find("\"" + key + "\"") != std::string::npos) {
                foundKeys++;
            }
        }

        if (foundKeys < 6) {
            logFile << "ERROR: JSON appears to be corrupted or not a valid OBody config file" << std::endl;
            logFile << " Expected at least 6 OBody keys, found only " << foundKeys << std::endl;
            return false;
        }

        std::string cleanContent = content;
        bool inStr = false;
        bool esc = false;
        for (size_t i = 0; i < cleanContent.length(); i++) {
            if (esc) {
                cleanContent[i] = ' ';
                esc = false;
                continue;
            }

            if (cleanContent[i] == '\\') {
                esc = true;
                cleanContent[i] = ' ';
                continue;
            }

            if (cleanContent[i] == '"') {
                inStr = !inStr;
                cleanContent[i] = ' ';
                continue;
            }

            if (inStr) {
                cleanContent[i] = ' ';
            }
        }

        if (cleanContent.find(",,") != std::string::npos) {
            logFile << "ERROR: Found double comma ',,' in JSON structure" << std::endl;
            return false;
        }

        if (cleanContent.find(",}") != std::string::npos) {
            logFile << "WARNING: Found comma before closing brace ',}' (may cause issues)" << std::endl;
        }

        if (cleanContent.find(",]") != std::string::npos) {
            logFile << "WARNING: Found comma before closing bracket ',]' (may cause issues)" << std::endl;
        }

        logFile << "SUCCESS: JSON passed SIMPLE integrity check" << std::endl;
        logFile << " Found " << foundKeys << " valid OBody keys" << std::endl;
        logFile << " Braces balanced: " << (braceCount == 0 ? "YES" : "NO") << std::endl;
        logFile << " Brackets balanced: " << (bracketCount == 0 ? "YES" : "NO") << std::endl;
        logFile << " Basic structure: VALID" << std::endl;
        logFile << std::endl;

        return true;
    } catch (const std::exception& e) {
        logFile << "ERROR in PerformSimpleJsonIntegrityCheck: " << e.what() << std::endl;
        return false;
    } catch (...) {
        logFile << "ERROR in PerformSimpleJsonIntegrityCheck: Unknown exception" << std::endl;
        return false;
    }
}

bool PerformTripleValidation(const fs::path& jsonPath, const fs::path& backupPath, std::ofstream& logFile) {
    try {
        if (!fs::exists(jsonPath)) {
            logFile << "ERROR: JSON file does not exist for validation: " << jsonPath.string() << std::endl;
            return false;
        }

        auto fileSize = fs::file_size(jsonPath);
        if (fileSize < 10) {
            logFile << "ERROR: JSON file is too small (" << fileSize << " bytes)" << std::endl;
            return false;
        }

        std::string content = ReadFileWithEncoding(jsonPath);
        if (content.empty()) {
            logFile << "ERROR: JSON file is empty after reading" << std::endl;
            return false;
        }

        content = Trim(content);
        if (content.empty() || content[0] != '{' || content[content.length() - 1] != '}') {
            logFile << "ERROR: JSON file does not have proper structure (missing braces)" << std::endl;
            return false;
        }

        int braceCount = 0;
        int bracketCount = 0;
        bool inString = false;
        bool escape = false;

        for (char c : content) {
            if (c == '"' && !escape) {
                inString = !inString;
            } else if (!inString) {
                if (c == '{')
                    braceCount++;
                else if (c == '}')
                    braceCount--;
                else if (c == '[')
                    bracketCount++;
                else if (c == ']')
                    bracketCount--;
            }

            escape = (c == '\\' && !escape);
        }

        if (braceCount != 0 || bracketCount != 0) {
            logFile << "ERROR: JSON has unbalanced braces/brackets (braces: " << braceCount
                    << ", brackets: " << bracketCount << ")" << std::endl;
            return false;
        }

        const std::vector<std::string> expectedKeys = {
            "npcFormID",       "npc",           "factionFemale", "factionMale",
            "npcPluginFemale", "npcPluginMale", "raceFemale",    "raceMale"};

        int foundKeys = 0;
        for (const auto& key : expectedKeys) {
            if (content.find("\"" + key + "\"") != std::string::npos) {
                foundKeys++;
            }
        }

        if (foundKeys < 6) {
            logFile << "ERROR: JSON appears corrupted (missing expected keys, found only " << foundKeys << " out of "
                    << expectedKeys.size() << ")" << std::endl;
            return false;
        }

        logFile << "SUCCESS: JSON file passed TRIPLE validation (" << fileSize << " bytes, " << foundKeys
                << " valid keys found)" << std::endl;
        return true;
    } catch (const std::exception& e) {
        logFile << "ERROR in PerformTripleValidation: " << e.what() << std::endl;
        return false;
    } catch (...) {
        logFile << "ERROR in PerformTripleValidation: Unknown exception" << std::endl;
        return false;
    }
}

bool PerformLiteralJsonBackup(const fs::path& originalJsonPath, const fs::path& backupJsonPath,
                              std::ofstream& logFile) {
    try {
        if (!fs::exists(originalJsonPath)) {
            logFile << "ERROR: Original JSON file does not exist at: " << originalJsonPath.string() << std::endl;
            return false;
        }

        CreateDirectoryIfNotExists(backupJsonPath.parent_path());

        std::error_code ec;
        fs::copy_file(originalJsonPath, backupJsonPath, fs::copy_options::overwrite_existing, ec);

        if (ec) {
            logFile << "ERROR: Failed to copy JSON file directly: " << ec.message() << std::endl;
            return false;
        }

        try {
            auto originalSize = fs::file_size(originalJsonPath);
            auto backupSize = fs::file_size(backupJsonPath);

            if (originalSize == backupSize && originalSize > 0) {
                logFile << "SUCCESS: LITERAL JSON backup completed to: " << backupJsonPath.string() << std::endl;
                logFile << "Backup file size: " << backupSize << " bytes (verified identical to original)" << std::endl;
                return true;
            } else {
                logFile << "ERROR: Backup file size mismatch - Original: " << originalSize << ", Backup: " << backupSize
                        << std::endl;
                return false;
            }

        } catch (...) {
            logFile << "SUCCESS: LITERAL JSON backup completed (size verification failed but backup exists)"
                    << std::endl;
            return true;
        }

    } catch (const std::exception& e) {
        logFile << "ERROR in PerformLiteralJsonBackup: " << e.what() << std::endl;
        return false;
    } catch (...) {
        logFile << "ERROR in PerformLiteralJsonBackup: Unknown exception" << std::endl;
        return false;
    }
}

bool MoveCorruptedJsonToAnalysis(const fs::path& corruptedJsonPath, const fs::path& analysisDir,
                                 std::ofstream& logFile) {
    try {
        if (!fs::exists(corruptedJsonPath)) {
            logFile << "WARNING: Corrupted JSON file does not exist for analysis" << std::endl;
            return false;
        }

        CreateDirectoryIfNotExists(analysisDir);

        auto now = std::chrono::system_clock::now();
        std::time_t time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &time_t);

        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &tm);

        fs::path analysisFile =
            analysisDir / ("OBody_presetDistributionConfig_corrupted_" + std::string(timestamp) + ".json");

        std::error_code ec;
        fs::copy_file(corruptedJsonPath, analysisFile, fs::copy_options::overwrite_existing, ec);

        if (ec) {
            logFile << "ERROR: Failed to move corrupted JSON to analysis folder: " << ec.message() << std::endl;
            return false;
        }

        logFile << "SUCCESS: Corrupted JSON moved to analysis folder: " << analysisFile.string() << std::endl;
        return true;
    } catch (const std::exception& e) {
        logFile << "ERROR in MoveCorruptedJsonToAnalysis: " << e.what() << std::endl;
        return false;
    } catch (...) {
        logFile << "ERROR in MoveCorruptedJsonToAnalysis: Unknown exception" << std::endl;
        return false;
    }
}

bool RestoreJsonFromBackup(const fs::path& backupJsonPath, const fs::path& originalJsonPath,
                           const fs::path& analysisDir, std::ofstream& logFile) {
    try {
        if (!fs::exists(backupJsonPath)) {
            logFile << "ERROR: Backup JSON file does not exist: " << backupJsonPath.string() << std::endl;
            return false;
        }

        if (!PerformTripleValidation(backupJsonPath, fs::path(), logFile)) {
            logFile << "ERROR: Backup JSON file is also corrupted, cannot restore" << std::endl;
            return false;
        }

        logFile << "WARNING: Original JSON appears corrupted, restoring from backup..." << std::endl;

        if (fs::exists(originalJsonPath)) {
            MoveCorruptedJsonToAnalysis(originalJsonPath, analysisDir, logFile);
        }

        std::error_code ec;
        fs::copy_file(backupJsonPath, originalJsonPath, fs::copy_options::overwrite_existing, ec);

        if (ec) {
            logFile << "ERROR: Failed to restore JSON from backup: " << ec.message() << std::endl;
            return false;
        }

        if (PerformTripleValidation(originalJsonPath, fs::path(), logFile)) {
            logFile << "SUCCESS: JSON restored from backup successfully" << std::endl;
            return true;
        } else {
            logFile << "ERROR: Restored JSON is still invalid" << std::endl;
            return false;
        }

    } catch (const std::exception& e) {
        logFile << "ERROR in RestoreJsonFromBackup: " << e.what() << std::endl;
        return false;
    } catch (...) {
        logFile << "ERROR in RestoreJsonFromBackup: Unknown exception" << std::endl;
        return false;
    }
}

PresetMapData BuildPresetNameMap(const fs::path& bodySlidePresetsPath, std::ofstream& logFile) {
    PresetMapData presetData;
    
    try {
        if (!fs::exists(bodySlidePresetsPath)) {
            logFile << "WARNING: BodySlide presets folder not found for building preset map" << std::endl;
            return presetData;
        }
        
        logFile << std::endl;
        logFile << "Building Master Preset Map from XML files..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;
        
        int totalXmlFiles = 0;
        int totalPresetsExtracted = 0;
        int multiPresetFiles = 0;
        int usingFilenameAsFallback = 0;
        int filenameFailed = 0;
        int ubePresetsDetected = 0;
        int himboPresetsDetected = 0;
        
        for (const auto& entry : fs::directory_iterator(bodySlidePresetsPath)) {
            try {
                if (entry.is_regular_file()) {
                    std::string filename;
                    try {
                        auto u8name = entry.path().filename().u8string();
                        filename = std::string(u8name.begin(), u8name.end());
                    } catch (...) {
                        try {
                            filename = entry.path().filename().string();
                        } catch (...) {
                            filenameFailed++;
                            logFile << "  [ERROR] Could not read filename for entry" << std::endl;
                            continue;
                        }
                    }
                    
                    if (EndsWith(filename, ".xml")) {
                        totalXmlFiles++;
                        
                        try {
                            std::vector<XmlPresetInfo> allPresetsInFile = ExtractAllPresetsFromXml(entry.path(), logFile);
                            
                            if (allPresetsInFile.size() > 1) {
                                multiPresetFiles++;
                                logFile << "  [MULTI-PRESET FILE] " << filename << " contains " 
                                        << allPresetsInFile.size() << " presets" << std::endl;
                            }
                            
                            XmlAnalysisResult analysis = AnalyzeXmlGroups(entry.path(), logFile);
                            
                            for (const auto& info : allPresetsInFile) {
                                std::string presetNameToUse;
                                
                                if (info.extractionSuccessful && !info.internalName.empty()) {
                                    presetNameToUse = info.internalName;
                                    totalPresetsExtracted++;
                                    
                                    presetData.filenameToInternalMap[info.filename] = info.internalName;
                                    
                                    if (analysis.hasUBE && !analysis.isException3BA) {
                                        presetData.ubePresetNames.insert(presetNameToUse);
                                        ubePresetsDetected++;
                                        logFile << "  [UBE DETECTED] " << presetNameToUse 
                                                << " (file: " << filename << ")" << std::endl;
                                    }
                                    
                                    if (analysis.hasHIMBO) {
                                        presetData.himboPresetNames.insert(presetNameToUse);
                                        himboPresetsDetected++;
                                        logFile << "  [HIMBO DETECTED] " << presetNameToUse 
                                                << " (file: " << filename << ")" << std::endl;
                                    }
                                    
                                    if (info.filename != info.internalName) {
                                        logFile << "  [MAPPING] File: " << info.filename 
                                                << " -> Internal: " << info.internalName << std::endl;
                                    }
                                    
                                } else {
                                    presetNameToUse = info.filename;
                                    usingFilenameAsFallback++;
                                    logFile << "  [WARNING] Failed to extract from: " << filename << std::endl;
                                }
                                
                                presetData.exactMap[presetNameToUse] = presetNameToUse;
                                presetData.allValidNames.insert(presetNameToUse);
                                presetData.allValidNames.insert(info.filename);
                                
                                std::string normalized = NormalizePresetNameFlexible(presetNameToUse);
                                if (!normalized.empty()) {
                                    if (presetData.normalizedMap.find(normalized) == presetData.normalizedMap.end()) {
                                        presetData.normalizedMap[normalized] = presetNameToUse;
                                    }
                                }
                                
                                std::string normalizedFilename = NormalizePresetNameFlexible(info.filename);
                                if (!normalizedFilename.empty()) {
                                    if (presetData.normalizedMap.find(normalizedFilename) == presetData.normalizedMap.end()) {
                                        presetData.normalizedMap[normalizedFilename] = presetNameToUse;
                                    }
                                }
                            }
                            
                        } catch (const std::exception& e) {
                            logFile << "  [ERROR] Exception processing " << filename << ": " << e.what() << std::endl;
                        } catch (...) {
                            logFile << "  [ERROR] Unknown exception processing " << filename << std::endl;
                        }
                    }
                }
            } catch (const std::exception& e) {
                logFile << "  [ERROR] Exception in directory iteration: " << e.what() << std::endl;
                continue;
            } catch (...) {
                logFile << "  [ERROR] Unknown exception in directory iteration" << std::endl;
                continue;
            }
        }
        
        logFile << std::endl;
        logFile << "Smart Cleaning: Preset Map Building Summary" << std::endl;
        logFile << "  Total XML files found: " << totalXmlFiles << std::endl;
        logFile << "  Files with multiple presets: " << multiPresetFiles << std::endl;
        logFile << "  Total presets extracted: " << totalPresetsExtracted << std::endl;
        logFile << "  Failed extractions (using filename): " << usingFilenameAsFallback << std::endl;
        logFile << "  Filename read failures: " << filenameFailed << std::endl;
        logFile << "  Total unique presets in map: " << presetData.exactMap.size() << std::endl;
        logFile << "  Total valid names (including filenames): " << presetData.allValidNames.size() << std::endl;
        logFile << "  UBE presets detected: " << ubePresetsDetected << std::endl;
        logFile << "  HIMBO presets detected: " << himboPresetsDetected << std::endl;
        logFile << std::endl;
        
    } catch (const std::exception& e) {
        logFile << "ERROR in BuildPresetNameMap: " << e.what() << std::endl;
    } catch (...) {
        logFile << "ERROR in BuildPresetNameMap: Unknown exception" << std::endl;
    }
    
    return presetData;
}

PresetMatchResult FindPresetMatch(const std::string& jsonPresetName, 
                                   const PresetMapData& presetData,
                                   std::ofstream& logFile) {
    PresetMatchResult result;
    
    if (jsonPresetName.empty()) {
        return result;
    }
    
    std::string cleanPresetName = jsonPresetName;
    if (!cleanPresetName.empty() && cleanPresetName[0] == '!') {
        cleanPresetName = cleanPresetName.substr(1);
    }
    
    if (IsPluginName(cleanPresetName)) {
        result.found = true;
        result.actualPresetName = cleanPresetName;
        result.matchLevel = 0;
        logFile << "    [PLUGIN DETECTED] Skipping cleaning for plugin: " << cleanPresetName << std::endl;
        return result;
    }
    
    auto exactIt = presetData.exactMap.find(cleanPresetName);
    if (exactIt != presetData.exactMap.end()) {
        result.found = true;
        result.actualPresetName = exactIt->second;
        result.matchLevel = 1;
        return result;
    }
    
    std::string decodedCleanName = DecodeHtmlEntities(cleanPresetName);
    if (decodedCleanName != cleanPresetName) {
        auto decodedIt = presetData.exactMap.find(decodedCleanName);
        if (decodedIt != presetData.exactMap.end()) {
            result.found = true;
            result.actualPresetName = decodedIt->second;
            result.matchLevel = 2;
            logFile << "    [INFO] Matched with HTML entity decoding: " 
                    << cleanPresetName << " -> " << decodedCleanName << std::endl;
            return result;
        }
    }
    
    std::string normalizedJsonName = NormalizePresetNameFlexible(cleanPresetName);
    for (const auto& [xmlName, actualName] : presetData.exactMap) {
        std::string normalizedXmlName = NormalizePresetNameFlexible(xmlName);
        if (normalizedJsonName == normalizedXmlName) {
            result.found = true;
            result.actualPresetName = actualName;
            result.matchLevel = 3;
            logFile << "    [INFO] Matched with normalization: " 
                    << cleanPresetName << " -> " << actualName << std::endl;
            return result;
        }
    }
    
    if (presetData.allValidNames.find(cleanPresetName) != presetData.allValidNames.end()) {
        auto filenameIt = presetData.filenameToInternalMap.find(cleanPresetName);
        if (filenameIt != presetData.filenameToInternalMap.end()) {
            result.found = true;
            result.actualPresetName = filenameIt->second;
            result.matchLevel = 4;
            return result;
        } else {
            result.found = true;
            result.actualPresetName = cleanPresetName;
            result.matchLevel = 4;
            return result;
        }
    }
    
    std::string normalized = NormalizePresetNameFlexible(cleanPresetName);
    if (!normalized.empty()) {
        auto normalizedIt = presetData.normalizedMap.find(normalized);
        if (normalizedIt != presetData.normalizedMap.end()) {
            result.found = true;
            result.actualPresetName = normalizedIt->second;
            result.matchLevel = 5;
            return result;
        }
    }
    
    std::vector<std::string> variations;
    variations.push_back(cleanPresetName);
    variations.push_back(Trim(cleanPresetName));
    
    std::string noSpaces = cleanPresetName;
    noSpaces.erase(std::remove(noSpaces.begin(), noSpaces.end(), ' '), noSpaces.end());
    variations.push_back(noSpaces);
    
    std::string lowerCase = ToLowerCase(cleanPresetName);
    variations.push_back(lowerCase);
    
    for (const auto& variation : variations) {
        if (presetData.allValidNames.find(variation) != presetData.allValidNames.end()) {
            result.found = true;
            result.actualPresetName = variation;
            result.matchLevel = 6;
            return result;
        }
    }
    
    return result;
}

void PerformSmartCleaning(std::map<std::string, OrderedPluginData>& processedData,
                          const ConfigSettings& config,
                          const fs::path& bodySlidePresetsPath,
                          std::ofstream& logFile,
                          std::vector<std::string>& missingPresetsFromIni) {
    
    bool anyCleaningEnabled = config.presetsSmartCleaning || 
                              config.blacklistedPresetsSmartCleaningFromRandomDistribution ||
                              config.blacklistedPresetsSmartCleaningFromAll ||
                              config.outfitsForceReSmartCleaning;
    
    if (!anyCleaningEnabled) {
        logFile << "Smart Cleaning: All cleaning options disabled in configuration" << std::endl;
        return;
    }
    
    logFile << std::endl;
    logFile << "Performing Smart Cleaning with Intelligent Preset Matching..." << std::endl;
    logFile << "----------------------------------------------------" << std::endl;
    
    PresetMapData presetData = BuildPresetNameMap(bodySlidePresetsPath, logFile);
    
    if (presetData.exactMap.empty()) {
        logFile << "Smart Cleaning: No presets found in BodySlide folder, skipping cleaning" << std::endl;
        return;
    }
    
    int totalPresetsRemoved = 0;
    int totalPresetsKept = 0;
    int totalPresetsCorrected = 0;
    int totalPluginsProtected = 0;
    std::set<std::string> removedPresets;
    std::map<std::string, std::string> correctedPresets;
    
    if (config.presetsSmartCleaning) {
        logFile << "Cleaning regular presets (npcFormID, npc, faction, raceFemale, raceMale, etc)..." << std::endl;
        
        const std::vector<std::string> keysToClean = {
            "npcFormID", "npc", "factionFemale", "factionMale",
            "npcPluginFemale", "npcPluginMale", "raceFemale", "raceMale"
        };
        
        for (const auto& key : keysToClean) {
            auto& data = processedData[key];
            std::vector<std::pair<std::string, std::vector<std::string>>> cleanedData;
            
            for (auto& [plugin, presets] : data.orderedData) {
                std::vector<std::string> cleanedPresets;
                
                for (const auto& preset : presets) {
                    std::string cleanPreset = preset;
                    bool hasExclamation = false;
                    
                    if (!cleanPreset.empty() && cleanPreset[0] == '!') {
                        cleanPreset = cleanPreset.substr(1);
                        hasExclamation = true;
                    }
                    
                    PresetMatchResult matchResult = FindPresetMatch(cleanPreset, presetData, logFile);
                    
                    if (matchResult.found) {
                        std::string finalPresetName = matchResult.actualPresetName;
                        
                        if (hasExclamation && (finalPresetName.empty() || finalPresetName[0] != '!')) {
                            finalPresetName = "!" + finalPresetName;
                        }
                        
                        cleanedPresets.push_back(finalPresetName);
                        totalPresetsKept++;
                        
                        if (matchResult.matchLevel == 0) {
                            totalPluginsProtected++;
                        }
                        
                        if (preset != finalPresetName && matchResult.matchLevel > 0) {
                            totalPresetsCorrected++;
                            correctedPresets[preset] = finalPresetName;
                            logFile << "  Corrected in " << key << "/" << plugin << ": \"" 
                                    << preset << "\" -> \"" << finalPresetName << "\" (Level " 
                                    << matchResult.matchLevel << " match)" << std::endl;
                        }
                    } else {
                        removedPresets.insert(cleanPreset);
                        totalPresetsRemoved++;
                        logFile << "  Removed from " << key << "/" << plugin << ": " << cleanPreset << std::endl;
                        
                        if (std::find(missingPresetsFromIni.begin(), missingPresetsFromIni.end(), cleanPreset) == missingPresetsFromIni.end()) {
                            missingPresetsFromIni.push_back(cleanPreset);
                        }
                    }
                }
                
                if (!cleanedPresets.empty()) {
                    cleanedData.emplace_back(plugin, cleanedPresets);
                }
            }
            
            data.orderedData = cleanedData;
        }
    }
    
    if (config.blacklistedPresetsSmartCleaningFromRandomDistribution) {
        logFile << "Cleaning blacklistedPresetsFromRandomDistribution..." << std::endl;
        
        auto& data = processedData["blacklistedPresetsFromRandomDistribution"];
        std::vector<std::pair<std::string, std::vector<std::string>>> cleanedData;
        
        for (auto& [plugin, presets] : data.orderedData) {
            std::vector<std::string> cleanedPresets;
            
            for (const auto& preset : presets) {
                std::string cleanPreset = preset;
                bool hasExclamation = false;
                
                if (!cleanPreset.empty() && cleanPreset[0] == '!') {
                    cleanPreset = cleanPreset.substr(1);
                    hasExclamation = true;
                }
                
                bool isProtected = false;
                for (const auto& protectedPreset : PROTECTED_FROM_CLEANING) {
                    if (cleanPreset == protectedPreset) {
                        isProtected = true;
                        break;
                    }
                }
                
                if (isProtected) {
                    cleanedPresets.push_back(preset);
                    totalPresetsKept++;
                    logFile << "  Protected preset kept in blacklistedPresetsFromRandomDistribution: " << cleanPreset << std::endl;
                } else {
                    PresetMatchResult matchResult = FindPresetMatch(cleanPreset, presetData, logFile);
                    
                    if (matchResult.found) {
                        std::string finalPresetName = matchResult.actualPresetName;
                        
                        if (hasExclamation && (finalPresetName.empty() || finalPresetName[0] != '!')) {
                            finalPresetName = "!" + finalPresetName;
                        }
                        
                        cleanedPresets.push_back(finalPresetName);
                        totalPresetsKept++;
                        
                        if (matchResult.matchLevel == 0) {
                            totalPluginsProtected++;
                        }
                        
                        if (preset != finalPresetName && matchResult.matchLevel > 0) {
                            totalPresetsCorrected++;
                            correctedPresets[preset] = finalPresetName;
                            logFile << "  Corrected in blacklistedPresetsFromRandomDistribution: \"" 
                                    << preset << "\" -> \"" << finalPresetName << "\" (Level " 
                                    << matchResult.matchLevel << " match)" << std::endl;
                        }
                    } else {
                        removedPresets.insert(cleanPreset);
                        totalPresetsRemoved++;
                        logFile << "  Removed from blacklistedPresetsFromRandomDistribution: " << cleanPreset << std::endl;
                        
                        if (std::find(missingPresetsFromIni.begin(), missingPresetsFromIni.end(), cleanPreset) == missingPresetsFromIni.end()) {
                            missingPresetsFromIni.push_back(cleanPreset);
                        }
                    }
                }
            }
            
            if (!cleanedPresets.empty()) {
                cleanedData.emplace_back(plugin, cleanedPresets);
            }
        }
        
        data.orderedData = cleanedData;
    }

    if (config.blacklistedPresetsSmartCleaningFromAll) {
        logFile << "Cleaning other blacklisted sections..." << std::endl;
        
        const std::vector<std::string> blacklistKeysToClean = {
            "blacklistedNpcs",
            "blacklistedNpcsPluginFemale",
            "blacklistedNpcsPluginMale",
            "blacklistedRacesFemale",
            "blacklistedRacesMale"
        };
        
        for (const auto& key : blacklistKeysToClean) {
            auto& data = processedData[key];
            std::vector<std::pair<std::string, std::vector<std::string>>> cleanedData;
            
            for (auto& [plugin, presets] : data.orderedData) {
                std::vector<std::string> cleanedPresets;
                
                for (const auto& preset : presets) {
                    std::string cleanPreset = preset;
                    bool hasExclamation = false;
                    
                    if (!cleanPreset.empty() && cleanPreset[0] == '!') {
                        cleanPreset = cleanPreset.substr(1);
                        hasExclamation = true;
                    }
                    
                    bool isProtected = false;
                    for (const auto& protectedPreset : PROTECTED_FROM_CLEANING) {
                        if (cleanPreset == protectedPreset) {
                            isProtected = true;
                            break;
                        }
                    }
                    
                    if (isProtected) {
                        cleanedPresets.push_back(preset);
                        totalPresetsKept++;
                        logFile << "  Protected preset kept in " << key << ": " << cleanPreset << std::endl;
                    } else {
                        PresetMatchResult matchResult = FindPresetMatch(cleanPreset, presetData, logFile);
                        
                        if (matchResult.found) {
                            std::string finalPresetName = matchResult.actualPresetName;
                            
                            if (hasExclamation && (finalPresetName.empty() || finalPresetName[0] != '!')) {
                                finalPresetName = "!" + finalPresetName;
                            }
                            
                            cleanedPresets.push_back(finalPresetName);
                            totalPresetsKept++;
                            
                            if (matchResult.matchLevel == 0) {
                                totalPluginsProtected++;
                            }
                            
                            if (preset != finalPresetName && matchResult.matchLevel > 0) {
                                totalPresetsCorrected++;
                                correctedPresets[preset] = finalPresetName;
                                logFile << "  Corrected in " << key << ": \"" 
                                        << preset << "\" -> \"" << finalPresetName << "\" (Level " 
                                        << matchResult.matchLevel << " match)" << std::endl;
                            }
                        } else {
                            removedPresets.insert(cleanPreset);
                            totalPresetsRemoved++;
                            logFile << "  Removed from " << key << ": " << cleanPreset << std::endl;
                            
                            if (std::find(missingPresetsFromIni.begin(), missingPresetsFromIni.end(), cleanPreset) == missingPresetsFromIni.end()) {
                                missingPresetsFromIni.push_back(cleanPreset);
                            }
                        }
                    }
                }
                
                if (!cleanedPresets.empty()) {
                    cleanedData.emplace_back(plugin, cleanedPresets);
                }
            }
            
            data.orderedData = cleanedData;
        }
    }

    if (config.outfitsForceReSmartCleaning) {
        logFile << std::endl;
        logFile << "====================================================" << std::endl;
        logFile << "OUTFIT SMART CLEANING: ENABLED BUT BYPASSED" << std::endl;
        logFile << "====================================================" << std::endl;
        logFile << std::endl;
        logFile << "The following 5 sections are PRESERVED as-is:" << std::endl;
        logFile << "  1. blacklistedOutfitsFromORefitFormID" << std::endl;
        logFile << "  2. blacklistedOutfitsFromORefit" << std::endl;
        logFile << "  3. blacklistedOutfitsFromORefitPlugin" << std::endl;
        logFile << "  4. outfitsForceRefitFormID" << std::endl;
        logFile << "  5. outfitsForceRefit" << std::endl;
        logFile << std::endl;
        logFile << "REASON: These sections contain armor/outfit names and plugin references," << std::endl;
        logFile << "        NOT body presets. Smart Cleaning only validates against BodySlide XMLs." << std::endl;
        logFile << std::endl;
        logFile << "NO CLEANING PERFORMED - All entries preserved." << std::endl;
        logFile << "====================================================" << std::endl;
        logFile << std::endl;
    }
    
    logFile << std::endl;
    logFile << "Smart Cleaning Summary:" << std::endl;
    logFile << "  Total presets removed: " << totalPresetsRemoved << std::endl;
    logFile << "  Total presets kept: " << totalPresetsKept << std::endl;
    logFile << "  Total presets corrected: " << totalPresetsCorrected << std::endl;
    logFile << "  Total plugins protected (not cleaned): " << totalPluginsProtected << std::endl;
    logFile << "  Unique presets removed: " << removedPresets.size() << std::endl;
    
    if (!missingPresetsFromIni.empty()) {
        logFile << std::endl;
        logFile << "WARNING: The following presets were referenced but not found in BodySlide folder:" << std::endl;
        logFile << "These may have been added by INI rules. Please verify if they need to be downloaded:" << std::endl;
        for (const auto& missingPreset : missingPresetsFromIni) {
            logFile << "  - " << missingPreset << std::endl;
        }
        logFile << std::endl;
        logFile << "You can search for these presets on Nexus Mods or other modding sites." << std::endl;
    }
    
    logFile << std::endl;
}

void GenerateDoctorLog(const fs::path& bodySlidePresetsPath, const fs::path& logDoctorPath, std::ofstream& mainLogFile) {
    try {
        mainLogFile << "Generating Doctor Log..." << std::endl;
        mainLogFile << "Searching in: " << bodySlidePresetsPath.string() << std::endl;

        std::ofstream doctorLog(logDoctorPath, std::ios::out | std::ios::trunc);
        if (!doctorLog.is_open()) {
            mainLogFile << "ERROR: Could not create Doctor Log file" << std::endl;
            return;
        }

        doctorLog << "====================================================" << std::endl;
        doctorLog << "OBody NG Preset Distribution Assistant NG - Doctor" << std::endl;
        doctorLog << "====================================================" << std::endl;
        doctorLog << std::endl;

        if (!fs::exists(bodySlidePresetsPath)) {
            doctorLog << "ERROR: BodySlide presets folder not found at:" << std::endl;
            doctorLog << bodySlidePresetsPath.string() << std::endl;
            mainLogFile << "ERROR: Folder does not exist: " << bodySlidePresetsPath.string() << std::endl;
            doctorLog.close();
            return;
        }

        mainLogFile << "Folder exists, scanning files..." << std::endl;

        std::vector<std::string> xmlFiles;
        int totalScanned = 0;
        int totalXmlFound = 0;
        int filenameFailed = 0;

        try {
            for (const auto& entry : fs::directory_iterator(bodySlidePresetsPath)) {
                try {
                    totalScanned++;

                    if (entry.is_regular_file()) {
                        std::string filename;
                        try {
                            auto u8name = entry.path().filename().u8string();
                            filename = std::string(u8name.begin(), u8name.end());
                        } catch (...) {
                            try {
                                filename = entry.path().filename().string();
                            } catch (...) {
                                filenameFailed++;
                                mainLogFile << "ERROR reading filename in Doctor Log generation" << std::endl;
                                continue;
                            }
                        }

                        if (EndsWith(filename, ".xml")) {
                            xmlFiles.push_back(filename);
                            totalXmlFound++;
                        }
                    }
                } catch (const std::exception& e) {
                    mainLogFile << "ERROR reading file entry: " << e.what() << std::endl;
                } catch (...) {
                    mainLogFile << "ERROR reading file entry: Unknown exception" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            mainLogFile << "ERROR iterating directory: " << e.what() << std::endl;
            doctorLog << "ERROR: Could not read directory" << std::endl;
            doctorLog.close();
            return;
        }

        mainLogFile << "Total files scanned: " << totalScanned << std::endl;
        mainLogFile << "Total XML files found: " << totalXmlFound << std::endl;
        if (filenameFailed > 0) {
            mainLogFile << "Filename read failures: " << filenameFailed << std::endl;
        }

        std::sort(xmlFiles.begin(), xmlFiles.end());

        doctorLog << "XML FILES FOUND IN SLIDER PRESETS FOLDER:" << std::endl;
        doctorLog << "Total files: " << xmlFiles.size() << std::endl;
        doctorLog << std::endl;

        for (const auto& xmlFile : xmlFiles) {
            doctorLog << xmlFile << std::endl;
        }

        doctorLog << std::endl;
        doctorLog << "====================================================" << std::endl;

        doctorLog.close();

        if (doctorLog.fail()) {
            mainLogFile << "ERROR: Failed to write Doctor Log file" << std::endl;
        } else {
            mainLogFile << "SUCCESS: Doctor Log created with " << xmlFiles.size() << " files" << std::endl;
        }

    } catch (const std::exception& e) {
        mainLogFile << "CRITICAL ERROR in GenerateDoctorLog: " << e.what() << std::endl;
    } catch (...) {
        mainLogFile << "CRITICAL ERROR in GenerateDoctorLog: Unknown exception" << std::endl;
    }
}

void GenerateSmartCleaningLog(const PresetMapData& presetData, const fs::path& logSmartCleaningPath, std::ofstream& mainLogFile) {
    try {
        mainLogFile << "Generating Smart Cleaning Log with extracted preset names..." << std::endl;
        
        std::ofstream smartCleaningLog(logSmartCleaningPath, std::ios::out | std::ios::trunc);
        if (!smartCleaningLog.is_open()) {
            mainLogFile << "ERROR: Could not create Smart Cleaning Log file" << std::endl;
            return;
        }
        
        smartCleaningLog << "====================================================" << std::endl;
        smartCleaningLog << "Smart Cleaning Preset Reference" << std::endl;
        smartCleaningLog << "====================================================" << std::endl;
        smartCleaningLog << std::endl;
        
        std::vector<std::string> presetNames;
        for (const auto& [presetName, _] : presetData.exactMap) {
            presetNames.push_back(presetName);
        }
        
        std::sort(presetNames.begin(), presetNames.end());
        
        smartCleaningLog << "PRESET NAMES EXTRACTED FROM XML FILES (INTERNAL <Preset name=\"...\">):" << std::endl;
        smartCleaningLog << "Total presets: " << presetNames.size() << std::endl;
        smartCleaningLog << std::endl;
        
        for (const auto& presetName : presetNames) {
            smartCleaningLog << presetName << std::endl;
        }
        
        smartCleaningLog << std::endl;
        smartCleaningLog << "----------------------------------------------------" << std::endl;
        smartCleaningLog << std::endl;
        
        smartCleaningLog << "FILENAME TO INTERNAL NAME MAPPING:" << std::endl;
        smartCleaningLog << "(Shows XML filenames that differ from internal preset names)" << std::endl;
        smartCleaningLog << std::endl;
        
        int mappingCount = 0;
        for (const auto& [filename, internalName] : presetData.filenameToInternalMap) {
            if (filename != internalName) {
                smartCleaningLog << "File: " << filename << ".xml" << std::endl;
                smartCleaningLog << " -> Internal: " << internalName << std::endl;
                smartCleaningLog << std::endl;
                mappingCount++;
            }
        }
        
        if (mappingCount == 0) {
            smartCleaningLog << "All XML filenames match their internal preset names." << std::endl;
        } else {
            smartCleaningLog << "Total mappings: " << mappingCount << std::endl;
        }
        
        smartCleaningLog << std::endl;
        smartCleaningLog << "====================================================" << std::endl;
        
        smartCleaningLog.close();
        
        if (smartCleaningLog.fail()) {
            mainLogFile << "ERROR: Failed to write Smart Cleaning Log file" << std::endl;
        } else {
            mainLogFile << "SUCCESS: Smart Cleaning Log created with " << presetNames.size() << " presets at: " << logSmartCleaningPath.string() << std::endl;
        }
        
    } catch (const std::exception& e) {
        mainLogFile << "ERROR in GenerateSmartCleaningLog: " << e.what() << std::endl;
    } catch (...) {
        mainLogFile << "ERROR in GenerateSmartCleaningLog: Unknown exception" << std::endl;
    }
}

void GenerateHelperLog(const PresetMapData& presetData, const fs::path& logHelperPath, std::ofstream& mainLogFile) {
    try {
        mainLogFile << "Generating Helper Log with preset list and examples..." << std::endl;
        
        std::ofstream helperLog(logHelperPath, std::ios::out | std::ios::trunc);
        if (!helperLog.is_open()) {
            mainLogFile << "ERROR: Could not create Helper Log file" << std::endl;
            return;
        }
        
        helperLog << "====================================================" << std::endl;
        helperLog << "OBody NG Preset Distribution Assistant NG - Helper" << std::endl;
        helperLog << "====================================================" << std::endl;
        helperLog << std::endl;
        
        helperLog << "This is a complete list of all presets installed in your Skyrim." << std::endl;
        helperLog << "You can use them to create INI files and manage presets intelligently." << std::endl;
        helperLog << "It also helps you know if you have certain presets installed or not, making your life much easier." << std::endl;
        helperLog << std::endl;
        helperLog << "You can visit the wiki at:" << std::endl;
        helperLog << "https://john95ac.github.io/website-documents-John95AC/OBody_NG_Preset_Distribution_Assistant_NG/index.html" << std::endl;
        helperLog << "which includes a rule generator for INI files." << std::endl;
        helperLog << std::endl;
        helperLog << "Below are some examples of how to use the INI manager in different cases." << std::endl;
        helperLog << std::endl;
        helperLog << std::endl;

        helperLog << "====================================================" << std::endl;
        helperLog << "INSTALLED PRESETS LIST (" << presetData.exactMap.size() << " total)" << std::endl;
        helperLog << "====================================================" << std::endl;
        helperLog << std::endl;

        std::vector<std::string> normalPresets;
        std::vector<std::string> ubePresets;
        std::vector<std::string> himboPresets;

        for (const auto& [presetName, _] : presetData.exactMap) {
            if (presetData.himboPresetNames.find(presetName) != presetData.himboPresetNames.end()) {
                himboPresets.push_back(presetName);
            } else if (presetData.ubePresetNames.find(presetName) != presetData.ubePresetNames.end()) {
                ubePresets.push_back(presetName);
            } else {
                normalPresets.push_back(presetName);
            }
        }

        std::sort(normalPresets.begin(), normalPresets.end());
        std::sort(ubePresets.begin(), ubePresets.end());
        std::sort(himboPresets.begin(), himboPresets.end());

        helperLog << "NORMAL PRESETS (CBBE/3BA/BHUNP/etc.) - " << normalPresets.size() << " total" << std::endl;
        helperLog << std::endl;

        for (const auto& preset : normalPresets) {
            helperLog << preset << std::endl;
        }

        if (!ubePresets.empty()) {
            helperLog << std::endl;
            helperLog << "UBE PRESETS (Female) - " << ubePresets.size() << " total" << std::endl;
            helperLog << std::endl;

            for (const auto& preset : ubePresets) {
                helperLog << preset << std::endl;
            }
        }

        if (!himboPresets.empty()) {
            helperLog << std::endl;
            helperLog << "HIMBO PRESETS (Male) - " << himboPresets.size() << " total" << std::endl;
            helperLog << std::endl;

            for (const auto& preset : himboPresets) {
                helperLog << preset << std::endl;
            }
        }
        
        helperLog << std::endl;
        helperLog << "----------------------------------------------------" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";OBody_Preset_Distribution_Assistant-NG rules to create an INI" << std::endl;
        helperLog << ";Don't be scared, check the web or review the examples below" << std::endl;
        helperLog << std::endl;
        helperLog << ";OBodyNG_PDA_(mod_name).ini - This is the INI structure" << std::endl;
        helperLog << ";You can use the mod examples entirely or create your own INI, but you must respect the naming convention" << std::endl;
        helperLog << ";Example: OBodyNG_PDA_MyMod.ini or OBodyNG_PDA_Custom_Bodies.ini" << std::endl;
        helperLog << std::endl;
        helperLog << ";Code design examples: Very similar to SPID but shorter and simpler." << std::endl;
        helperLog << std::endl;
        helperLog << "; npcFormID = Plugin.esp|FormID,Preset,...|, 1, 0, -, *, 1-, 1*, KeyWord, KeyNormal, etc.  FormID with presets" << std::endl;
        helperLog << "; npc = EditorID|Preset,...|, 1, 0, -, *, 1-, 1*, KeyWord, KeyNormal, etc.                 EditorID name like 000Rabbit_NPC or Serana" << std::endl;
        helperLog << "; factionFemale = Faction|Preset,...|, 1, 0, -, *, 1-, 1*, KeyWord, KeyNormal, etc.        Faction name like ImperialFaction or KhajiitFaction" << std::endl;
        helperLog << "; factionMale = Faction|Preset,...|, 1, 0, -, *, 1-, 1*, KeyWord, KeyNormal, etc." << std::endl;
        helperLog << "; npcPluginFemale = Plugin.esp|Preset,...|, 1, 0, -, *, 1-, 1*, KeyWord, KeyNormal, etc.   The name of the ESP with defined bodies" << std::endl;
        helperLog << "; npcPluginMale = Plugin.esp|Preset,...|, 1, 0, -, *, 1-, 1*, KeyWord, KeyNormal, etc." << std::endl;
        helperLog << "; raceFemale = Race|Preset,...|, 1, 0, -, *, 1-, 1*, KeyWord, KeyNormal, etc.              Works with NordRace, OrcRace, etc., and custom races" << std::endl;
        helperLog << "; raceMale = Race|Preset,...|, 1, 0, -, *, 1-, 1*, KeyWord, KeyNormal, etc." << std::endl;
        helperLog << std::endl;
        helperLog << "; NEW SPECIAL RULES:" << std::endl;
        helperLog << "; raceFemaleUBE = 00UBE_*|Preset,...|, 1, 0, -, *, 1-, 1*, KeyWord, KeyNormal, etc.      For UBE female races" << std::endl;
        helperLog << "; raceMaleAny = AnyRace|Preset,...|, 1, 0, -, *, 1-, 1*, KeyWord, KeyNormal, etc.         For any male race" << std::endl;
        helperLog << "; blacklisted = Type|Preset,...|, 1, 0, -, *, 1-, 1*, KeyWord, KeyNormal, etc.           For blacklist types" << std::endl;
        helperLog << std::endl;
        helperLog << "; BLACKLIST TYPES AVAILABLE:" << std::endl;
        helperLog << "; blacklistedNpcs, blacklistedNpcsPluginFemale, blacklistedNpcsPluginMale" << std::endl;
        helperLog << "; blacklistedRacesFemale, blacklistedRacesMale, blacklistedPresetsFromRandomDistribution" << std::endl;
        helperLog << std::endl;
        helperLog << "; NEW FORMID RULES:" << std::endl;
        helperLog << "; blacklistedNpcsFormID = Plugin.esp|FormID1,FormID2|, 1, 0, -, *                        NPC FormIDs in plugin" << std::endl;
        helperLog << std::endl;
        helperLog << "; NEW OUTFIT RULES:" << std::endl;
        helperLog << "; outfits = blacklistedOutfitsFromORefit|Outfit1,Outfit2|, 1, 0, -, *                    Outfit names to blacklist" << std::endl;
        helperLog << "; outfits = blacklistedOutfitsFromORefitPlugin|Plugin.esp|, 1, 0, -, *                   Plugin to blacklist outfits" << std::endl;
        helperLog << "; outfits = outfitsForceRefit|Outfit1,Outfit2|, 1, 0, -, *                               Outfits to force refit" << std::endl;
        helperLog << "; blacklistedOutfitsFromORefitFormID = Plugin.esp|FormID1,FormID2|, 1, 0, -, *           Outfit FormIDs to blacklist" << std::endl;
        helperLog << "; outfitsForceRefitFormID = Plugin.esp|FormID1,FormID2|, 1, 0, -, *                      Outfit FormIDs to force refit" << std::endl;
        helperLog << std::endl;
        helperLog << "; NEW FILTERING MODES:" << std::endl;
        helperLog << "; KeyWord, KeyWord*, KeyWord-, KeyWord1, KeyWord1*, KeyWord1- - Search entire preset name" << std::endl;
        helperLog << "; KeyWordChart, KeyWordChart*, KeyWordChart-, KeyWordChart1, KeyWordChart1*, KeyWordChart1- - Search only inside parentheses ()" << std::endl;
        helperLog << "; KeyAuthor, KeyAuthor*, KeyAuthor-, KeyAuthor1, KeyAuthor1*, KeyAuthor1- - Search by author name with optional filters" << std::endl;
        helperLog << "; KeyNormal, KeyNormal*, KeyNormal-, KeyNormal1, KeyNormal1*, KeyNormal1- - Filter by Normal preset family (CBBE/3BA/BHUNP)" << std::endl;
        helperLog << "; KeyUBE, KeyUBE*, KeyUBE-, KeyUBE1, KeyUBE1*, KeyUBE1- - Filter by UBE preset family" << std::endl;
        helperLog << "; KeyHIMBO, KeyHIMBO*, KeyHIMBO-, KeyHIMBO1, KeyHIMBO1*, KeyHIMBO1- - Filter by HIMBO preset family" << std::endl;
        helperLog << "; Use * for exclusive (replace all), - for removal (delete matches), 1 for one-time application" << std::endl;
        helperLog << std::endl;
        
        helperLog << "----------------------------------------------------" << std::endl;
        helperLog << "SPECIAL RULE EXAMPLES" << std::endl;
        helperLog << "----------------------------------------------------" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";Example 1: Add presets to all UBE female races" << std::endl;
        helperLog << "raceFemaleUBE = 00UBE_*|MyUBEPreset1,MyUBEPreset2|" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";Example 2: Add presets to any male race (one time only)" << std::endl;
        helperLog << "raceMaleAny = NordRace|MyMalePreset1,MyMalePreset2|1" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";Example 3: Add to blacklisted NPCs" << std::endl;
        helperLog << "blacklisted = blacklistedNpcs|BadPreset1,BadPreset2|" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";Example 4: Blacklist NPC by FormID" << std::endl;
        helperLog << "blacklistedNpcsFormID = Skyrim.esm|0001339C,0001A696|" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";Example 5: Blacklist outfit by name" << std::endl;
        helperLog << "outfits = blacklistedOutfitsFromORefit|LS Force Naked,OBody Nude 32|" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";Example 6: Blacklist outfit by plugin" << std::endl;
        helperLog << "outfits = blacklistedOutfitsFromORefitPlugin|NewmChainmail.esp|" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";Example 7: Force refit outfit by FormID" << std::endl;
        helperLog << "outfitsForceRefitFormID = [full_inu] Queen Marika's Dress.esp|FE000803|" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";Example 8: Remove specific presets once, then disable" << std::endl;
        helperLog << "raceFemale = NordRace|UnwantedPreset1,UnwantedPreset2|1-" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";Example 9: Replace all with specific presets once" << std::endl;
        helperLog << "raceFemale = NordRace|OnlyThesePresets1,OnlyThesePresets2|1*" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";Example 10: Filter by Normal family (only CBBE/3BA/BHUNP presets)" << std::endl;
        helperLog << "raceFemale = NordRace|Preset1,Preset2,Preset3|KeyNormal" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";Example 11: Filter by UBE family with exclusive replacement (one time)" << std::endl;
        helperLog << "raceFemale = 00UBE_NordRace|UBEPreset1,UBEPreset2|KeyUBE1*" << std::endl;
        helperLog << std::endl;
        
        helperLog << ";Example 12: npcFormID with FormID and presets" << std::endl;
        helperLog << "npcFormID = YurianaWench.esp|000817,PresetA,PresetB|" << std::endl;
        helperLog << std::endl;
        
        if (normalPresets.size() >= 2) {
            std::vector<std::string> randomPresets;
            std::random_device rd;
            std::mt19937 gen(rd());

            if (normalPresets.size() <= 5) {
                randomPresets = normalPresets;
            } else {
                std::sample(normalPresets.begin(), normalPresets.end(), std::back_inserter(randomPresets),
                           std::min(static_cast<size_t>(5), normalPresets.size()), gen);
            }
            
            helperLog << "----------------------------------------------------" << std::endl;
            helperLog << "TRADITIONAL EXAMPLES" << std::endl;
            helperLog << "----------------------------------------------------" << std::endl;
            helperLog << std::endl;
            
            helperLog << ";YurianaWench example" << std::endl;
            helperLog << "npcPluginFemale = YurianaWench.esp|";
            for (size_t i = 0; i < randomPresets.size() && i < 2; i++) {
                if (i > 0) helperLog << ",";
                helperLog << randomPresets[i];
            }
            helperLog << "|" << std::endl;
            helperLog << std::endl;
            
            helperLog << ";Immersive Wenches example" << std::endl;
            helperLog << "npcPluginFemale = Immersive Wenches.esp|";
            for (size_t i = 0; i < randomPresets.size() && i < 2; i++) {
                if (i > 0) helperLog << ",";
                helperLog << randomPresets[i];
            }
            helperLog << "|" << std::endl;
            helperLog << std::endl;
        }
        
        helperLog << "====================================================" << std::endl;
        
        helperLog.close();
        
        if (helperLog.fail()) {
            mainLogFile << "ERROR: Failed to write Helper Log file" << std::endl;
        } else {
            mainLogFile << "SUCCESS: Helper Log created successfully at: " << logHelperPath.string() << std::endl;
        }
        
    } catch (const std::exception& e) {
        mainLogFile << "ERROR in GenerateHelperLog: " << e.what() << std::endl;
    } catch (...) {
        mainLogFile << "ERROR in GenerateHelperLog: Unknown exception" << std::endl;
    }
}

void GenerateINIAnalysisLog(const fs::path& dataPath, const fs::path& logINIAnalysisPath, std::ofstream& mainLogFile) {
    try {
        mainLogFile << "Generating INI Analysis Log..." << std::endl;
        
        std::ofstream iniLog(logINIAnalysisPath, std::ios::out | std::ios::trunc);
        if (!iniLog.is_open()) {
            mainLogFile << "ERROR: Could not create INI Analysis Log file" << std::endl;
            return;
        }
        
        iniLog << "====================================================" << std::endl;
        iniLog << "OBody NG Preset Distribution Assistant NG" << std::endl;
        iniLog << "INI Files Analysis Report" << std::endl;
        iniLog << "====================================================" << std::endl;
        iniLog << std::endl;
        
        int totalINIFiles = 0;
        int totalRulesFound = 0;
        
        try {
            if (!fs::exists(dataPath)) {
                iniLog << "ERROR: Data path does not exist: " << dataPath.string() << std::endl;
                mainLogFile << "ERROR: Data path does not exist for INI Analysis" << std::endl;
                iniLog.close();
                return;
            }
            
            if (!fs::is_directory(dataPath)) {
                iniLog << "ERROR: Data path is not a directory: " << dataPath.string() << std::endl;
                mainLogFile << "ERROR: Data path is not a directory for INI Analysis" << std::endl;
                iniLog.close();
                return;
            }
            
            std::vector<fs::path> iniFiles;
            
            try {
                for (const auto& entry : fs::directory_iterator(dataPath)) {
                    try {
                        if (!entry.is_regular_file()) {
                            continue;
                        }
                        
                        std::string filename;
                        try {
                            auto wfilename = entry.path().filename().wstring();
                            filename = SafeWideStringToString(wfilename);
                        } catch (...) {
                            try {
                                auto u8name = entry.path().filename().u8string();
                                filename = std::string(u8name.begin(), u8name.end());
                            } catch (...) {
                                try {
                                    filename = entry.path().filename().string();
                                } catch (...) {
                                    mainLogFile << "  WARNING: Could not read filename in INI Analysis, skipping entry" << std::endl;
                                    continue;
                                }
                            }
                        }
                        
                        if (filename.empty()) {
                            continue;
                        }
                        
                        std::string lowerFilename = ToLowerCase(filename);
                        
                        if (StartsWith(lowerFilename, "obodyng_pda_") && EndsWith(lowerFilename, ".ini")) {
                            iniFiles.push_back(entry.path());
                        }
                        
                    } catch (const std::exception& e) {
                        mainLogFile << "  WARNING: Exception reading file entry in INI Analysis: " << e.what() << std::endl;
                        continue;
                    } catch (...) {
                        mainLogFile << "  WARNING: Unknown exception reading file entry in INI Analysis" << std::endl;
                        continue;
                    }
                }
            } catch (const std::exception& e) {
                iniLog << "ERROR scanning directory: " << e.what() << std::endl;
                iniLog << "This may be due to special characters in file paths." << std::endl;
                iniLog << "Attempting alternative scan method..." << std::endl;
                iniLog << std::endl;
                mainLogFile << "WARNING: Standard directory scan failed in INI Analysis: " << e.what() << std::endl;
                mainLogFile << "Attempting alternative method..." << std::endl;
                
                std::string dataPathStr = dataPath.string();
                if (!dataPathStr.empty() && dataPathStr.back() != '\\') {
                    dataPathStr += '\\';
                }
                dataPathStr += '\\';
                
                try {
                    fs::path fallbackPath(dataPathStr);
                    
                    if (fs::exists(fallbackPath) && fs::is_directory(fallbackPath)) {
                        for (const auto& entry : fs::directory_iterator(fallbackPath)) {
                            try {
                                if (!entry.is_regular_file()) {
                                    continue;
                                }
                                
                                std::string filename;
                                try {
                                    auto wfilename = entry.path().filename().wstring();
                                    filename = SafeWideStringToString(wfilename);
                                } catch (...) {
                                    try {
                                        auto u8name = entry.path().filename().u8string();
                                        filename = std::string(u8name.begin(), u8name.end());
                                    } catch (...) {
                                        try {
                                            filename = entry.path().filename().string();
                                        } catch (...) {
                                            continue;
                                        }
                                    }
                                }
                                
                                if (filename.empty()) {
                                    continue;
                                }
                                
                                std::string lowerFilename = ToLowerCase(filename);
                                
                                if (StartsWith(lowerFilename, "obodyng_pda_") && EndsWith(lowerFilename, ".ini")) {
                                    iniFiles.push_back(entry.path());
                                }
                                
                            } catch (...) {
                                continue;
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    iniLog << "ERROR: Alternative scan method also failed: " << e.what() << std::endl;
                    mainLogFile << "ERROR: Alternative INI scan method failed: " << e.what() << std::endl;
                } catch (...) {
                    iniLog << "ERROR: Alternative scan method failed with unknown exception" << std::endl;
                    mainLogFile << "ERROR: Alternative INI scan failed with unknown exception" << std::endl;
                }
            }
            
            if (iniFiles.empty()) {
                iniLog << "No OBodyNG_PDA_*.ini files found in data directory." << std::endl;
                iniLog << "Searched path: " << dataPath.string() << std::endl;
                mainLogFile << "INFO: No INI files found for analysis" << std::endl;
            } else {
                mainLogFile << "Found " << iniFiles.size() << " INI files for analysis" << std::endl;
            }
            
            for (const auto& iniPath : iniFiles) {
                std::string filename;
                try {
                    auto wfilename = iniPath.filename().wstring();
                    filename = SafeWideStringToString(wfilename);
                } catch (...) {
                    try {
                        auto u8name = iniPath.filename().u8string();
                        filename = std::string(u8name.begin(), u8name.end());
                    } catch (...) {
                        try {
                            filename = iniPath.filename().string();
                        } catch (...) {
                            filename = "unknown.ini";
                        }
                    }
                }
                
                totalINIFiles++;
                
                iniLog << "----------------------------------------------------" << std::endl;
                iniLog << "FILE: " << filename << std::endl;
                iniLog << "----------------------------------------------------" << std::endl;
                
                std::string iniContent = ReadFileWithEncoding(iniPath);
                if (iniContent.empty()) {
                    iniLog << "ERROR: Could not read file or file is empty" << std::endl;
                    iniLog << std::endl;
                    continue;
                }
                
                size_t totalLines = std::count(iniContent.begin(), iniContent.end(), '\n');
                size_t fileSize = iniContent.size();
                
                iniLog << "File size: " << fileSize << " bytes" << std::endl;
                iniLog << "Total lines: " << totalLines << std::endl;
                iniLog << std::endl;
                
                std::stringstream iniStream(iniContent);
                std::string line;
                int lineNumber = 0;
                int rulesInFile = 0;
                int commentsInFile = 0;
                int emptyLinesInFile = 0;
                
                while (std::getline(iniStream, line)) {
                    lineNumber++;
                    
                    if (line.size() > 10000) {
                        iniLog << "  [LINE " << lineNumber << "] WARNING: Abnormally long line (" 
                               << line.size() << " chars) - SKIPPED" << std::endl;
                        continue;
                    }
                    
                    while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || 
                           line.back() == '\r' || line.back() == '\n')) {
                        line.pop_back();
                    }
                    
                    std::string originalLine = line;
                    
                    line = RemoveCommentsSafely(line);
                    
                    std::string trimmedLine = Trim(line);
                    
                    if (trimmedLine.empty()) {
                        if (!originalLine.empty() && (originalLine[0] == ';' || originalLine[0] == '#')) {
                            commentsInFile++;
                        } else {
                            emptyLinesInFile++;
                        }
                        continue;
                    }
                    
                    size_t equalPos = trimmedLine.find('=');
                    if (equalPos != std::string::npos) {
                        std::string key = Trim(trimmedLine.substr(0, equalPos));
                        std::string value = Trim(trimmedLine.substr(equalPos + 1));
                        
                        if (!value.empty()) {
                            rulesInFile++;
                            totalRulesFound++;
                            
                            iniLog << "  [LINE " << lineNumber << "] " << key << " = " << value << std::endl;
                        }
                    }
                }
                
                iniLog << std::endl;
                iniLog << "Summary for " << filename << ":" << std::endl;
                iniLog << "  Total rules detected: " << rulesInFile << std::endl;
                iniLog << "  Comment lines: " << commentsInFile << std::endl;
                iniLog << "  Empty lines: " << emptyLinesInFile << std::endl;
                iniLog << std::endl;
            }
            
        } catch (const std::exception& e) {
            iniLog << "ERROR scanning directory: " << e.what() << std::endl;
            mainLogFile << "ERROR in INI Analysis: " << e.what() << std::endl;
        } catch (...) {
            iniLog << "ERROR scanning directory: Unknown exception" << std::endl;
            mainLogFile << "ERROR in INI Analysis: Unknown exception" << std::endl;
        }
        
        iniLog << "====================================================" << std::endl;
        iniLog << "GLOBAL SUMMARY" << std::endl;
        iniLog << "====================================================" << std::endl;
        iniLog << "Total INI files processed: " << totalINIFiles << std::endl;
        iniLog << "Total rules found: " << totalRulesFound << std::endl;
        iniLog << "====================================================" << std::endl;
        
        iniLog.close();
        
        if (iniLog.fail()) {
            mainLogFile << "ERROR: Failed to write INI Analysis Log file" << std::endl;
        } else {
            mainLogFile << "SUCCESS: INI Analysis Log created with " << totalINIFiles 
                        << " files and " << totalRulesFound << " rules analyzed" << std::endl;
        }
        
    } catch (const std::exception& e) {
        mainLogFile << "ERROR in GenerateINIAnalysisLog: " << e.what() << std::endl;
    } catch (...) {
        mainLogFile << "ERROR in GenerateINIAnalysisLog: Unknown exception" << std::endl;
    }
}

bool CorrectJsonIndentation(const fs::path& jsonPath, const fs::path& analysisDir, std::ofstream& logFile) {
    try {
        logFile << "Checking and correcting JSON indentation hierarchy..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;

        if (!fs::exists(jsonPath)) {
            logFile << "ERROR: JSON file does not exist for indentation correction" << std::endl;
            return false;
        }

        std::string originalContent = ReadFileWithEncoding(jsonPath);
        if (originalContent.empty()) {
            logFile << "ERROR: JSON file is empty for indentation correction" << std::endl;
            return false;
        }

        bool needsCorrection = false;
        std::vector<std::string> lines;
        std::stringstream ss(originalContent);
        std::string line;

        while (std::getline(ss, line)) {
            lines.push_back(line);
        }

        for (const auto& currentLine : lines) {
            if (currentLine.empty()) continue;
            if (currentLine.find_first_not_of(" \t") == std::string::npos) continue;

            size_t leadingSpaces = 0;
            size_t leadingTabs = 0;
            for (char c : currentLine) {
                if (c == ' ')
                    leadingSpaces++;
                else if (c == '\t')
                    leadingTabs++;
                else
                    break;
            }

            if (leadingTabs > 0 || (leadingSpaces > 0 && leadingSpaces % 4 != 0)) {
                needsCorrection = true;
                break;
            }
        }

        if (!needsCorrection) {
            for (size_t i = 0; i < lines.size() - 1; i++) {
                std::string currentTrimmed = Trim(lines[i]);

                if (EndsWith(currentTrimmed, "{") || EndsWith(currentTrimmed, "[")) {
                    char openChar = currentTrimmed.back();
                    char closeChar = (openChar == '{') ? '}' : ']';

                    for (size_t j = i + 1; j < lines.size(); j++) {
                        std::string nextTrimmed = Trim(lines[j]);

                        if (nextTrimmed == std::string(1, closeChar) ||
                            nextTrimmed == std::string(1, closeChar) + ",") {
                            bool hasOnlyWhitespace = true;
                            for (size_t k = i + 1; k < j; k++) {
                                if (!Trim(lines[k]).empty()) {
                                    hasOnlyWhitespace = false;
                                    break;
                                }
                            }

                            if (hasOnlyWhitespace) {
                                needsCorrection = true;
                                logFile << "DETECTED: Multi-line empty container found at lines " << (i + 1) << "-"
                                        << (j + 1) << ", needs inline correction" << std::endl;
                                break;
                            }
                        }

                        if (!nextTrimmed.empty() && nextTrimmed != std::string(1, closeChar) &&
                            nextTrimmed != std::string(1, closeChar) + ",") {
                            break;
                        }
                    }

                    if (needsCorrection) break;
                }
            }
        }

        if (!needsCorrection) {
            logFile << "SUCCESS: JSON indentation is already correct (perfect 4-space hierarchy with inline empty "
                       "containers)"
                    << std::endl;
            logFile << std::endl;
            return true;
        }

        logFile << "DETECTED: JSON indentation needs correction - reformatting entire file with perfect 4-space "
                   "hierarchy and inline empty containers..."
                << std::endl;

        std::ostringstream correctedJson;
        int indentLevel = 0;
        bool inString = false;
        bool escape = false;

        auto isEmptyBlock = [&originalContent](size_t startPos, char openChar, char closeChar) -> bool {
            size_t pos = startPos + 1;
            int depth = 1;
            bool inStr = false;
            bool esc = false;

            while (pos < originalContent.length() && depth > 0) {
                char c = originalContent[pos];

                if (esc) {
                    esc = false;
                    pos++;
                    continue;
                }

                if (c == '\\' && inStr) {
                    esc = true;
                    pos++;
                    continue;
                }

                if (c == '"') {
                    inStr = !inStr;
                } else if (!inStr) {
                    if (c == openChar) {
                        depth++;
                    } else if (c == closeChar) {
                        depth--;
                        if (depth == 0) {
                            std::string between = originalContent.substr(startPos + 1, pos - startPos - 1);
                            std::string trimmedBetween = Trim(between);
                            return trimmedBetween.empty();
                        }
                    }
                }
                pos++;
            }
            return false;
        };

        for (size_t i = 0; i < originalContent.length(); i++) {
            char c = originalContent[i];

            if (escape) {
                correctedJson << c;
                escape = false;
                continue;
            }

            if (c == '\\' && inString) {
                correctedJson << c;
                escape = true;
                continue;
            }

            if (c == '"' && !escape) {
                inString = !inString;
                correctedJson << c;
                continue;
            }

            if (inString) {
                correctedJson << c;
                continue;
            }

            switch (c) {
                case '{':
                case '[':
                    if (isEmptyBlock(i, c, (c == '{') ? '}' : ']')) {
                        size_t pos = i + 1;
                        int depth = 1;
                        bool inStr = false;
                        bool esc = false;

                        while (pos < originalContent.length() && depth > 0) {
                            char nextChar = originalContent[pos];

                            if (esc) {
                                esc = false;
                                pos++;
                                continue;
                            }

                            if (nextChar == '\\' && inStr) {
                                esc = true;
                                pos++;
                                continue;
                            }

                            if (nextChar == '"') {
                                inStr = !inStr;
                            } else if (!inStr) {
                                if (nextChar == c) {
                                    depth++;
                                } else if (nextChar == ((c == '{') ? '}' : ']')) {
                                    depth--;
                                }
                            }
                            pos++;
                        }

                        correctedJson << c << ((c == '{') ? '}' : ']');
                        i = pos - 1;

                        if (i + 1 < originalContent.length()) {
                            size_t nextNonSpace = i + 1;
                            while (nextNonSpace < originalContent.length() &&
                                   std::isspace(static_cast<unsigned char>(originalContent[nextNonSpace]))) {
                                nextNonSpace++;
                            }

                            if (nextNonSpace < originalContent.length() && originalContent[nextNonSpace] != ',' &&
                                originalContent[nextNonSpace] != '}' && originalContent[nextNonSpace] != ']') {
                                correctedJson << '\n';
                                for (int j = 0; j < indentLevel * 4; j++) {
                                    correctedJson << ' ';
                                }
                            }
                        }
                    } else {
                        correctedJson << c << '\n';
                        indentLevel++;
                        for (int j = 0; j < indentLevel * 4; j++) {
                            correctedJson << ' ';
                        }
                    }
                    break;

                case '}':
                case ']':
                    correctedJson << '\n';
                    indentLevel--;
                    for (int j = 0; j < indentLevel * 4; j++) {
                        correctedJson << ' ';
                    }
                    correctedJson << c;

                    if (i + 1 < originalContent.length()) {
                        size_t nextNonSpace = i + 1;
                        while (nextNonSpace < originalContent.length() && std::isspace(static_cast<unsigned char>(originalContent[nextNonSpace]))) {
                            nextNonSpace++;
                        }

                        if (nextNonSpace < originalContent.length() && originalContent[nextNonSpace] != ',' &&
                            originalContent[nextNonSpace] != '}' && originalContent[nextNonSpace] != ']') {
                            correctedJson << '\n';
                            for (int j = 0; j < indentLevel * 4; j++) {
                                correctedJson << ' ';
                            }
                        }
                    }
                    break;

                case ',':
                    correctedJson << c << '\n';
                    for (int j = 0; j < indentLevel * 4; j++) {
                        correctedJson << ' ';
                    }
                    break;

                case ':':
                    correctedJson << c << ' ';
                    break;

                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    break;

                default:
                    correctedJson << c;
                    break;
            }
        }

        std::string correctedContent = correctedJson.str();

        std::vector<std::string> finalLines;
        std::stringstream finalSS(correctedContent);
        std::string finalLine;

        while (std::getline(finalSS, finalLine)) {
            while (!finalLine.empty() && finalLine.back() == ' ') {
                finalLine.pop_back();
            }
            finalLines.push_back(finalLine);
        }

        std::ostringstream finalJson;
        for (size_t i = 0; i < finalLines.size(); i++) {
            finalJson << finalLines[i];
            if (i < finalLines.size() - 1) {
                finalJson << '\n';
            }
        }

        std::string finalContent = finalJson.str();

        fs::path tempPath = jsonPath;
        tempPath.replace_extension(".indent_corrected.tmp");

        std::ofstream tempFile(tempPath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!tempFile.is_open()) {
            logFile << "ERROR: Could not create temporary file for indentation correction" << std::endl;
            return false;
        }

        tempFile << finalContent;
        tempFile.close();

        if (tempFile.fail()) {
            logFile << "ERROR: Failed to write corrected JSON to temporary file" << std::endl;
            return false;
        }

        if (!PerformTripleValidation(tempPath, fs::path(), logFile)) {
            logFile << "ERROR: Corrected JSON failed integrity check" << std::endl;
            MoveCorruptedJsonToAnalysis(tempPath, analysisDir, logFile);
            try {
                fs::remove(tempPath);
            } catch (...) {
            }
            return false;
        }

        std::error_code ec;
        fs::rename(tempPath, jsonPath, ec);

        if (ec) {
            logFile << "ERROR: Failed to replace original with corrected JSON: " << ec.message() << std::endl;
            try {
                fs::remove(tempPath);
            } catch (...) {
            }
            return false;
        }

        if (PerformTripleValidation(jsonPath, fs::path(), logFile)) {
            logFile << "SUCCESS: JSON indentation corrected successfully" << std::endl;
            logFile << " Applied perfect 4-space hierarchy with inline empty containers (including multi-line empty "
                       "detection)"
                    << std::endl;
            logFile << std::endl;
            return true;
        } else {
            logFile << "ERROR: Final corrected JSON failed integrity check" << std::endl;
            return false;
        }

    } catch (const std::exception& e) {
        logFile << "ERROR in CorrectJsonIndentation: " << e.what() << std::endl;
        return false;
    } catch (...) {
        logFile << "ERROR in CorrectJsonIndentation: Unknown exception" << std::endl;
        return false;
    }
}

std::vector<std::pair<std::string, std::vector<std::string>>> parseOrderedPlugins(const std::string& content) {
    std::vector<std::pair<std::string, std::vector<std::string>>> result;
    if (content.empty()) return result;

    const char* str = content.c_str();
    size_t len = content.length();
    size_t pos = 0;
    const size_t maxIters = std::max(100000ULL, len / 10);
    size_t iter = 0;

    result.reserve(200);

    try {
        while (pos < len && iter++ < maxIters) {
            while (pos < len && std::isspace(static_cast<unsigned char>(str[pos]))) ++pos;
            if (pos >= len) break;

            if (str[pos] != '"') {
                ++pos;
                continue;
            }

            size_t keyStart = pos + 1;
            ++pos;

            while (pos < len) {
                if (str[pos] == '"') {
                    size_t backslashCount = 0;
                    size_t checkPos = pos - 1;
                    while (checkPos > 0 && str[checkPos] == '\\') {
                        backslashCount++;
                        checkPos--;
                    }
                    if (backslashCount % 2 == 0) break;
                }
                ++pos;
            }

            if (pos >= len) break;

            std::string plugin = content.substr(keyStart, pos - keyStart);
            ++pos;

            while (pos < len && std::isspace(static_cast<unsigned char>(str[pos]))) ++pos;
            if (pos >= len || str[pos] != ':') {
                ++pos;
                continue;
            }

            ++pos;
            while (pos < len && std::isspace(static_cast<unsigned char>(str[pos]))) ++pos;
            if (pos >= len || str[pos] != '[') {
                ++pos;
                continue;
            }

            ++pos;

            std::vector<std::string> presets;
            presets.reserve(50);
            size_t presetIter = 0;

            while (pos < len && presetIter++ < maxIters) {
                while (pos < len && std::isspace(static_cast<unsigned char>(str[pos]))) ++pos;
                if (pos >= len) break;

                if (str[pos] == ']') {
                    ++pos;
                    break;
                }

                if (str[pos] != '"') {
                    ++pos;
                    continue;
                }

                size_t presetStart = pos + 1;
                ++pos;

                while (pos < len) {
                    if (str[pos] == '"') {
                        size_t backslashCount = 0;
                        size_t checkPos = pos - 1;
                        while (checkPos > 0 && str[checkPos] == '\\') {
                            backslashCount++;
                            checkPos--;
                        }
                        if (backslashCount % 2 == 0) break;
                    }
                    ++pos;
                }

                if (pos >= len) break;

                std::string preset = content.substr(presetStart, pos - presetStart);
                presets.push_back(std::move(preset));
                ++pos;

                while (pos < len && std::isspace(static_cast<unsigned char>(str[pos]))) ++pos;
                if (pos < len && str[pos] == ',') {
                    ++pos;
                    while (pos < len && std::isspace(static_cast<unsigned char>(str[pos]))) ++pos;
                }
            }

            while (pos < len && std::isspace(static_cast<unsigned char>(str[pos]))) ++pos;
            if (pos < len && str[pos] == ',') ++pos;

            if (!plugin.empty()) {
                result.emplace_back(std::move(plugin), std::move(presets));
            }
        }
    } catch (...) {
    }

    return result;
}

std::vector<std::string> parseArray(const std::string& content) {
    std::vector<std::string> result;
    if (content.empty()) return result;

    const char* str = content.c_str();
    size_t len = content.length();
    size_t pos = 0;
    const size_t maxIters = std::max(100000ULL, len / 10);
    size_t iter = 0;

    result.reserve(100);

    try {
        while (pos < len && iter++ < maxIters) {
            while (pos < len && std::isspace(static_cast<unsigned char>(str[pos]))) ++pos;
            if (pos >= len) break;

            if (str[pos] == ']') break;

            if (str[pos] != '"') {
                ++pos;
                continue;
            }

            size_t valueStart = pos + 1;
            ++pos;

            while (pos < len) {
                if (str[pos] == '"') {
                    size_t backslashCount = 0;
                    size_t checkPos = pos - 1;
                    while (checkPos > 0 && str[checkPos] == '\\') {
                        backslashCount++;
                        checkPos--;
                    }
                    if (backslashCount % 2 == 0) break;
                }
                ++pos;
            }

            if (pos >= len) break;

            std::string value = content.substr(valueStart, pos - valueStart);
            result.push_back(std::move(value));
            ++pos;

            while (pos < len && std::isspace(static_cast<unsigned char>(str[pos]))) ++pos;
            if (pos < len && str[pos] == ',') ++pos;
        }
    } catch (...) {
    }

    return result;
}

bool parseBooleanValue(const std::string& content) {
    std::string trimmed = Trim(content);
    std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(), ::tolower);
    return (trimmed == "true");
}

std::string PreserveOriginalSections(const std::string& originalJson,
                                      const std::map<std::string, OrderedPluginData>& processedData,
                                      const NpcFormIDData& npcFormIDProcessedData,
                                      bool currentBlacklistedPresetsShowValue,
                                      bool newBlacklistedPresetsShowValue,
                                      std::ofstream& logFile) {
    try {
        const std::set<std::string> validKeys = {"npc", "factionFemale", "factionMale",
                                                  "npcPluginFemale", "npcPluginMale", "raceFemale", "raceMale",
                                                  "blacklistedNpcsFormID", "blacklistedOutfitsFromORefitFormID", 
                                                  "outfitsForceRefitFormID"};

        const std::set<std::string> arrayKeys = {"blacklistedPresetsFromRandomDistribution", "blacklistedNpcs",
                                                  "blacklistedNpcsPluginFemale", "blacklistedNpcsPluginMale",
                                                  "blacklistedRacesFemale", "blacklistedRacesMale",
                                                  "blacklistedOutfitsFromORefit", "blacklistedOutfitsFromORefitPlugin",
                                                  "outfitsForceRefit"};

        std::string result = originalJson;

        if (!npcFormIDProcessedData.isEmpty()) {
            std::string keyPattern = "\"npcFormID\"";
            size_t keyPos = result.find(keyPattern);

            if (keyPos != std::string::npos) {
                size_t colonPos = result.find(":", keyPos);
                if (colonPos != std::string::npos) {
                    size_t valueStart = colonPos + 1;

                    while (valueStart < result.length() && std::isspace(static_cast<unsigned char>(result[valueStart]))) {
                        valueStart++;
                    }

                    size_t valueEnd = valueStart;
                    if (valueStart < result.length() && result[valueStart] == '{') {
                        int braceCount = 1;
                        valueEnd = valueStart + 1;
                        bool inString = false;
                        bool escape = false;

                        while (valueEnd < result.length() && braceCount > 0) {
                            char c = result[valueEnd];

                            if (c == '"' && !escape) {
                                inString = !inString;
                            } else if (!inString) {
                                if (c == '{')
                                    braceCount++;
                                else if (c == '}')
                                    braceCount--;
                            }

                            escape = (c == '\\' && !escape);
                            valueEnd++;
                        }

                        std::ostringstream newValue;
                        newValue << "{\n";

                        bool firstPlugin = true;
                        for (const auto& [plugin, formIDs] : npcFormIDProcessedData.data) {
                            if (!firstPlugin) newValue << ",\n";
                            firstPlugin = false;

                            newValue << "        \"" << EscapeJson(plugin) << "\": {\n";

                            bool firstFormID = true;
                            for (const auto& [formID, presets] : formIDs) {
                                if (!firstFormID) newValue << ",\n";
                                firstFormID = false;

                                newValue << "            \"" << EscapeJson(formID) << "\": [\n";

                                bool firstPreset = true;
                                for (const auto& preset : presets) {
                                    if (!firstPreset) newValue << ",\n";
                                    firstPreset = false;
                                    newValue << "                \"" << EscapeJson(preset) << "\"";
                                }

                                newValue << "\n            ]";
                            }

                            newValue << "\n        }";
                        }

                        newValue << "\n    }";

                        result.replace(valueStart, valueEnd - valueStart, newValue.str());
                        logFile << "INFO: Successfully updated npcFormID with double-level structure and proper 4-space indentation"
                                << std::endl;
                    }
                }
            }
        }

        for (const auto& [key, data] : processedData) {
            if (validKeys.count(key) && !data.orderedData.empty()) {
                std::string keyPattern = "\"" + key + "\"";
                size_t keyPos = result.find(keyPattern);

                if (keyPos != std::string::npos) {
                    size_t colonPos = result.find(":", keyPos);
                    if (colonPos != std::string::npos) {
                        size_t valueStart = colonPos + 1;

                        while (valueStart < result.length() && std::isspace(static_cast<unsigned char>(result[valueStart]))) {
                            valueStart++;
                        }

                        size_t valueEnd = valueStart;
                        if (valueStart < result.length() && result[valueStart] == '{') {
                            int braceCount = 1;
                            valueEnd = valueStart + 1;
                            bool inString = false;
                            bool escape = false;

                            while (valueEnd < result.length() && braceCount > 0) {
                                char c = result[valueEnd];

                                if (c == '"' && !escape) {
                                    inString = !inString;
                                } else if (!inString) {
                                    if (c == '{')
                                        braceCount++;
                                    else if (c == '}')
                                        braceCount--;
                                }

                                escape = (c == '\\' && !escape);
                                valueEnd++;
                            }

                            std::ostringstream newValue;
                            newValue << "{\n";

                            bool first = true;
                            for (const auto& [plugin, presets] : data.orderedData) {
                                if (!first) newValue << ",\n";
                                first = false;

                                newValue << "        \"" << EscapeJson(plugin) << "\": [\n";

                                bool firstPreset = true;
                                for (const auto& preset : presets) {
                                    if (!firstPreset) newValue << ",\n";
                                    firstPreset = false;

                                    newValue << "            \"" << EscapeJson(preset) << "\"";
                                }

                                newValue << "\n        ]";
                            }

                            newValue << "\n    }";

                            result.replace(valueStart, valueEnd - valueStart, newValue.str());
                            logFile << "INFO: Successfully updated key '" << key << "' with proper 4-space indentation"
                                    << std::endl;
                        }
                    }
                }
            } else if (arrayKeys.count(key) && !data.orderedData.empty()) {
                std::string keyPattern = "\"" + key + "\"";
                size_t keyPos = result.find(keyPattern);

                if (keyPos != std::string::npos) {
                    size_t colonPos = result.find(":", keyPos);
                    if (colonPos != std::string::npos) {
                        size_t valueStart = colonPos + 1;

                        while (valueStart < result.length() && std::isspace(static_cast<unsigned char>(result[valueStart]))) {
                            valueStart++;
                        }

                        size_t valueEnd = valueStart;
                        if (valueStart < result.length() && result[valueStart] == '[') {
                            int bracketCount = 1;
                            valueEnd = valueStart + 1;
                            bool inString = false;
                            bool escape = false;

                            while (valueEnd < result.length() && bracketCount > 0) {
                                char c = result[valueEnd];

                                if (c == '"' && !escape) {
                                    inString = !inString;
                                } else if (!inString) {
                                    if (c == '[')
                                        bracketCount++;
                                    else if (c == ']')
                                        bracketCount--;
                                }

                                escape = (c == '\\' && !escape);
                                valueEnd++;
                            }

                            std::ostringstream newValue;
                            newValue << "[\n";

                            bool first = true;
                            for (const auto& [plugin, presets] : data.orderedData) {
                                for (const auto& preset : presets) {
                                    if (!first) newValue << ",\n";
                                    first = false;
                                    newValue << "        \"" << EscapeJson(preset) << "\"";
                                }
                            }

                            newValue << "\n    ]";

                            result.replace(valueStart, valueEnd - valueStart, newValue.str());
                            logFile << "INFO: Successfully updated array key '" << key
                                    << "' with proper 4-space indentation" << std::endl;
                        }
                    }
                }
            }
        }

        if (currentBlacklistedPresetsShowValue != newBlacklistedPresetsShowValue) {
            std::string keyPattern = "\"blacklistedPresetsShowInOBodyMenu\"";
            size_t keyPos = result.find(keyPattern);

            if (keyPos != std::string::npos) {
                size_t colonPos = result.find(":", keyPos);
                if (colonPos != std::string::npos) {
                    size_t valueStart = colonPos + 1;

                    while (valueStart < result.length() && std::isspace(static_cast<unsigned char>(result[valueStart]))) {
                        valueStart++;
                    }

                    size_t valueEnd = valueStart;
                    while (valueEnd < result.length() && 
                           (std::isalpha(static_cast<unsigned char>(result[valueEnd])) || result[valueEnd] == '_')) {
                        valueEnd++;
                    }

                    std::string newBoolValue = newBlacklistedPresetsShowValue ? "true" : "false";
                    result.replace(valueStart, valueEnd - valueStart, newBoolValue);
                    logFile << "INFO: Updated blacklistedPresetsShowInOBodyMenu to " << newBoolValue << std::endl;
                }
            }
        }

        return result;
    } catch (const std::exception& e) {
        logFile << "ERROR in PreserveOriginalSections: " << e.what() << std::endl;
        return originalJson;
    } catch (...) {
        logFile << "ERROR in PreserveOriginalSections: Unknown exception" << std::endl;
        return originalJson;
    }
}

bool CheckIfChangesNeeded(const std::string& originalJson,
                         const std::map<std::string, OrderedPluginData>& processedData,
                         const NpcFormIDData& npcFormIDProcessedData,
                         bool currentBlacklistedPresetsShowValue,
                         bool newBlacklistedPresetsShowValue) {
    const std::vector<std::string> validKeys = {"npc", "factionFemale", "factionMale",
                                                "npcPluginFemale", "npcPluginMale", "raceFemale", "raceMale",
                                                "blacklistedNpcsFormID", "blacklistedOutfitsFromORefitFormID",
                                                "outfitsForceRefitFormID"};

    const std::vector<std::string> arrayKeys = {"blacklistedPresetsFromRandomDistribution", "blacklistedNpcs",
                                                "blacklistedNpcsPluginFemale", "blacklistedNpcsPluginMale",
                                                "blacklistedRacesFemale", "blacklistedRacesMale",
                                                "blacklistedOutfitsFromORefit", "blacklistedOutfitsFromORefitPlugin",
                                                "outfitsForceRefit"};

    if (currentBlacklistedPresetsShowValue != newBlacklistedPresetsShowValue) {
        return true;
    }

    if (!npcFormIDProcessedData.isEmpty()) {
        return true;
    }

    for (const auto& key : validKeys) {
        auto it = processedData.find(key);
        if (it != processedData.end() && !it->second.orderedData.empty()) {
            return true;
        }
    }

    for (const auto& key : arrayKeys) {
        auto it = processedData.find(key);
        if (it != processedData.end() && !it->second.orderedData.empty()) {
            return true;
        }
    }

    return false;
}

std::tuple<bool, std::string, bool> ReadCompleteJson(const fs::path& jsonPath,
                                                      std::map<std::string, OrderedPluginData>& processedData,
                                                      NpcFormIDData& npcFormIDProcessedData,
                                                      std::ofstream& logFile) {
    try {
        if (!fs::exists(jsonPath)) {
            logFile << "ERROR: JSON file does not exist at: " << jsonPath.string() << std::endl;
            return {false, "", true};
        }

        if (!PerformTripleValidation(jsonPath, fs::path(), logFile)) {
            logFile << "ERROR: JSON integrity check failed" << std::endl;
            return {false, "", true};
        }

        logFile << "Reading existing JSON from: " << jsonPath.string() << std::endl;

        std::string jsonContent = ReadFileWithEncoding(jsonPath);

        if (jsonContent.empty() || jsonContent.size() < 2) {
            logFile << "ERROR: JSON file is empty or too small after reading" << std::endl;
            return {false, "", true};
        }

        const std::vector<std::string> validKeys = {"npc", "factionFemale", "factionMale",
                                                    "npcPluginFemale", "npcPluginMale", "raceFemale", "raceMale"};

        const std::vector<std::string> pluginArrayKeys = {
            "blacklistedNpcsFormID",
            "blacklistedOutfitsFromORefitFormID",
            "outfitsForceRefitFormID"
        };

        const std::vector<std::string> arrayKeys = {"blacklistedPresetsFromRandomDistribution", "blacklistedNpcs",
                                                     "blacklistedNpcsPluginFemale", "blacklistedNpcsPluginMale",
                                                     "blacklistedRacesFemale", "blacklistedRacesMale",
                                                     "blacklistedOutfitsFromORefit", "blacklistedOutfitsFromORefitPlugin",
                                                     "outfitsForceRefit"};

        size_t npcFormIDPos = jsonContent.find("\"npcFormID\"");
        if (npcFormIDPos != std::string::npos) {
            size_t colonPos = jsonContent.find(":", npcFormIDPos);
            if (colonPos != std::string::npos) {
                size_t openBrace = jsonContent.find("{", colonPos);
                if (openBrace != std::string::npos) {
                    int braceCount = 1;
                    size_t pos = openBrace + 1;
                    size_t closeBrace = std::string::npos;
                    bool inString = false;
                    bool escape = false;

                    while (pos < jsonContent.length() && braceCount > 0) {
                        char c = jsonContent[pos];

                        if (c == '"' && !escape) {
                            inString = !inString;
                        } else if (!inString) {
                            if (c == '{') {
                                braceCount++;
                            } else if (c == '}') {
                                braceCount--;
                                if (braceCount == 0) {
                                    closeBrace = pos;
                                    break;
                                }
                            }
                        }

                        escape = (c == '\\' && !escape);
                        pos++;
                    }

                    if (closeBrace != std::string::npos) {
                        std::string npcFormIDContent = jsonContent.substr(openBrace + 1, closeBrace - openBrace - 1);
                        
                        size_t pluginPos = 0;
                        while (pluginPos < npcFormIDContent.length()) {
                            pluginPos = npcFormIDContent.find("\"", pluginPos);
                            if (pluginPos == std::string::npos) break;

                            size_t pluginStart = pluginPos + 1;
                            size_t pluginEnd = npcFormIDContent.find("\"", pluginStart);
                            if (pluginEnd == std::string::npos) break;

                            std::string plugin = npcFormIDContent.substr(pluginStart, pluginEnd - pluginStart);

                            size_t pluginColon = npcFormIDContent.find(":", pluginEnd);
                            if (pluginColon == std::string::npos) break;

                            size_t pluginOpenBrace = npcFormIDContent.find("{", pluginColon);
                            if (pluginOpenBrace == std::string::npos) break;

                            int pluginBraceCount = 1;
                            size_t pluginBracePos = pluginOpenBrace + 1;
                            size_t pluginCloseBrace = std::string::npos;
                            bool pluginInString = false;
                            bool pluginEscape = false;

                            while (pluginBracePos < npcFormIDContent.length() && pluginBraceCount > 0) {
                                char c = npcFormIDContent[pluginBracePos];

                                if (c == '"' && !pluginEscape) {
                                    pluginInString = !pluginInString;
                                } else if (!pluginInString) {
                                    if (c == '{') {
                                        pluginBraceCount++;
                                    } else if (c == '}') {
                                        pluginBraceCount--;
                                        if (pluginBraceCount == 0) {
                                            pluginCloseBrace = pluginBracePos;
                                            break;
                                        }
                                    }
                                }

                                pluginEscape = (c == '\\' && !pluginEscape);
                                pluginBracePos++;
                            }

                            if (pluginCloseBrace != std::string::npos) {
                                std::string formIDsContent = npcFormIDContent.substr(pluginOpenBrace + 1, pluginCloseBrace - pluginOpenBrace - 1);
                                
                                size_t formIDPos = 0;
                                while (formIDPos < formIDsContent.length()) {
                                    formIDPos = formIDsContent.find("\"", formIDPos);
                                    if (formIDPos == std::string::npos) break;

                                    size_t formIDStart = formIDPos + 1;
                                    size_t formIDEnd = formIDsContent.find("\"", formIDStart);
                                    if (formIDEnd == std::string::npos) break;

                                    std::string formID = formIDsContent.substr(formIDStart, formIDEnd - formIDStart);

                                    size_t formIDColon = formIDsContent.find(":", formIDEnd);
                                    if (formIDColon == std::string::npos) break;

                                    size_t formIDOpenBracket = formIDsContent.find("[", formIDColon);
                                    if (formIDOpenBracket == std::string::npos) break;

                                    int bracketCount = 1;
                                    size_t bracketPos = formIDOpenBracket + 1;
                                    size_t formIDCloseBracket = std::string::npos;
                                    bool bracketInString = false;
                                    bool bracketEscape = false;

                                    while (bracketPos < formIDsContent.length() && bracketCount > 0) {
                                        char c = formIDsContent[bracketPos];

                                        if (c == '"' && !bracketEscape) {
                                            bracketInString = !bracketInString;
                                        } else if (!bracketInString) {
                                            if (c == '[') {
                                                bracketCount++;
                                            } else if (c == ']') {
                                                bracketCount--;
                                                if (bracketCount == 0) {
                                                    formIDCloseBracket = bracketPos;
                                                    break;
                                                }
                                            }
                                        }

                                        bracketEscape = (c == '\\' && !bracketEscape);
                                        bracketPos++;
                                    }

                                    if (formIDCloseBracket != std::string::npos) {
                                        std::string presetsContent = formIDsContent.substr(formIDOpenBracket + 1, formIDCloseBracket - formIDOpenBracket - 1);
                                        auto presets = parseArray(presetsContent);

                                        for (const auto& preset : presets) {
                                            npcFormIDProcessedData.addPresetToFormID(plugin, formID, preset);
                                        }
                                    }

                                    formIDPos = formIDCloseBracket != std::string::npos ? formIDCloseBracket + 1 : formIDEnd + 1;
                                }
                            }

                            pluginPos = pluginCloseBrace != std::string::npos ? pluginCloseBrace + 1 : pluginEnd + 1;
                        }
                    }
                }
            }
        }

        for (const auto& key : validKeys) {
            processedData[key] = OrderedPluginData();

            size_t keyPos = jsonContent.find("\"" + key + "\"");
            if (keyPos != std::string::npos) {
                size_t colonPos = jsonContent.find(":", keyPos);
                if (colonPos != std::string::npos) {
                    size_t openBrace = jsonContent.find("{", colonPos);
                    if (openBrace != std::string::npos) {
                        int braceCount = 1;
                        size_t pos = openBrace + 1;
                        size_t closeBrace = std::string::npos;
                        bool inString = false;
                        bool escape = false;

                        while (pos < jsonContent.length() && braceCount > 0) {
                            char c = jsonContent[pos];

                            if (c == '"' && !escape) {
                                inString = !inString;
                            } else if (!inString) {
                                if (c == '{') {
                                    braceCount++;
                                } else if (c == '}') {
                                    braceCount--;
                                    if (braceCount == 0) {
                                        closeBrace = pos;
                                        break;
                                    }
                                }
                            }

                            escape = (c == '\\' && !escape);
                            pos++;
                        }

                        if (closeBrace != std::string::npos) {
                            std::string keyContent = jsonContent.substr(openBrace + 1, closeBrace - openBrace - 1);
                            auto orderedPlugins = parseOrderedPlugins(keyContent);

                            for (const auto& p : orderedPlugins) {
                                for (const auto& preset : p.second) {
                                    processedData[key].addPreset(p.first, preset);
                                }
                            }
                        }
                    }
                }
            }
        }

        for (const auto& key : pluginArrayKeys) {
            processedData[key] = OrderedPluginData();

            size_t keyPos = jsonContent.find("\"" + key + "\"");
            if (keyPos != std::string::npos) {
                size_t colonPos = jsonContent.find(":", keyPos);
                if (colonPos != std::string::npos) {
                    size_t openBrace = jsonContent.find("{", colonPos);
                    if (openBrace != std::string::npos) {
                        int braceCount = 1;
                        size_t pos = openBrace + 1;
                        size_t closeBrace = std::string::npos;
                        bool inString = false;
                        bool escape = false;

                        while (pos < jsonContent.length() && braceCount > 0) {
                            char c = jsonContent[pos];

                            if (c == '"' && !escape) {
                                inString = !inString;
                            } else if (!inString) {
                                if (c == '{') {
                                    braceCount++;
                                } else if (c == '}') {
                                    braceCount--;
                                    if (braceCount == 0) {
                                        closeBrace = pos;
                                        break;
                                    }
                                }
                            }

                            escape = (c == '\\' && !escape);
                            pos++;
                        }

                        if (closeBrace != std::string::npos) {
                            std::string keyContent = jsonContent.substr(openBrace + 1, closeBrace - openBrace - 1);
                            auto orderedPlugins = parseOrderedPlugins(keyContent);

                            for (const auto& p : orderedPlugins) {
                                for (const auto& preset : p.second) {
                                    processedData[key].addPreset(p.first, preset);
                                }
                            }
                        }
                    }
                }
            }
        }

        for (const auto& key : arrayKeys) {
            processedData[key] = OrderedPluginData();

            size_t keyPos = jsonContent.find("\"" + key + "\"");
            if (keyPos != std::string::npos) {
                size_t colonPos = jsonContent.find(":", keyPos);
                if (colonPos != std::string::npos) {
                    size_t openBracket = jsonContent.find("[", colonPos);
                    if (openBracket != std::string::npos) {
                        int bracketCount = 1;
                        size_t pos = openBracket + 1;
                        size_t closeBracket = std::string::npos;
                        bool inString = false;
                        bool escape = false;

                        while (pos < jsonContent.length() && bracketCount > 0) {
                            char c = jsonContent[pos];

                            if (c == '"' && !escape) {
                                inString = !inString;
                            } else if (!inString) {
                                if (c == '[') {
                                    bracketCount++;
                                } else if (c == ']') {
                                    bracketCount--;
                                    if (bracketCount == 0) {
                                        closeBracket = pos;
                                        break;
                                    }
                                }
                            }

                            escape = (c == '\\' && !escape);
                            pos++;
                        }

                        if (closeBracket != std::string::npos) {
                            std::string keyContent = jsonContent.substr(openBracket + 1, closeBracket - openBracket - 1);
                            auto arrayValues = parseArray(keyContent);

                            for (const auto& value : arrayValues) {
                                processedData[key].addPreset("", value);
                            }
                        }
                    }
                }
            }
        }

        bool blacklistedPresetsShowValue = true;
        std::string boolKey = "\"blacklistedPresetsShowInOBodyMenu\"";
        size_t boolKeyPos = jsonContent.find(boolKey);
        if (boolKeyPos != std::string::npos) {
            size_t colonPos = jsonContent.find(":", boolKeyPos);
            if (colonPos != std::string::npos) {
                size_t valueStart = colonPos + 1;
                while (valueStart < jsonContent.length() && std::isspace(static_cast<unsigned char>(jsonContent[valueStart]))) {
                    valueStart++;
                }

                size_t valueEnd = valueStart;
                while (valueEnd < jsonContent.length() && 
                       (std::isalpha(static_cast<unsigned char>(jsonContent[valueEnd])) || jsonContent[valueEnd] == '_')) {
                    valueEnd++;
                }

                std::string boolValue = jsonContent.substr(valueStart, valueEnd - valueStart);
                blacklistedPresetsShowValue = parseBooleanValue(boolValue);
                logFile << "Read blacklistedPresetsShowInOBodyMenu: " << (blacklistedPresetsShowValue ? "true" : "false") << std::endl;
            }
        }

        logFile << "Loaded existing data from JSON:" << std::endl;
        if (!npcFormIDProcessedData.isEmpty()) {
            logFile << "  npcFormID: " << npcFormIDProcessedData.getTotalPresetCount() << " total entries" << std::endl;
        }
        for (const auto& [key, data] : processedData) {
            size_t count = data.getTotalPresetCount();
            if (count > 0) {
                logFile << "  " << key << ": " << data.getPluginCount() << " plugins, " << count << " presets"
                        << std::endl;
            }
        }
        logFile << std::endl;

        return {true, jsonContent, blacklistedPresetsShowValue};
    } catch (const std::exception& e) {
        logFile << "ERROR in ReadCompleteJson: " << e.what() << std::endl;
        return {false, "", true};
    } catch (...) {
        logFile << "ERROR in ReadCompleteJson: Unknown exception occurred" << std::endl;
        return {false, "", true};
    }
}

bool WriteJsonAtomically(const fs::path& jsonPath, const std::string& content, const fs::path& analysisDir,
                         std::ofstream& logFile) {
    try {
        fs::path tempPath = jsonPath;
        tempPath.replace_extension(".tmp");

        std::ofstream tempFile(tempPath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!tempFile.is_open()) {
            logFile << "ERROR: Could not create temporary JSON file" << std::endl;
            return false;
        }

        tempFile << content;
        tempFile.close();

        if (tempFile.fail()) {
            logFile << "ERROR: Failed to write to temporary JSON file" << std::endl;
            return false;
        }

        if (!PerformTripleValidation(tempPath, fs::path(), logFile)) {
            logFile << "ERROR: Temporary JSON file failed integrity check" << std::endl;
            MoveCorruptedJsonToAnalysis(tempPath, analysisDir, logFile);
            try {
                fs::remove(tempPath);
            } catch (...) {
            }
            return false;
        }

        std::error_code ec;
        fs::rename(tempPath, jsonPath, ec);

        if (ec) {
            logFile << "ERROR: Failed to move temporary file to final location: " << ec.message() << std::endl;
            try {
                fs::remove(tempPath);
            } catch (...) {
            }
            return false;
        }

        if (PerformTripleValidation(jsonPath, fs::path(), logFile)) {
            logFile << "SUCCESS: JSON file written atomically and verified" << std::endl;
            return true;
        } else {
            logFile << "ERROR: Final JSON file failed integrity check" << std::endl;
            MoveCorruptedJsonToAnalysis(jsonPath, analysisDir, logFile);
            return false;
        }

    } catch (const std::exception& e) {
        logFile << "ERROR in WriteJsonAtomically: " << e.what() << std::endl;
        return false;
    } catch (...) {
        logFile << "ERROR in WriteJsonAtomically: Unknown exception" << std::endl;
        return false;
    }
}

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* skse) {
    try {
        SKSE::Init(skse);

        std::string documentsPath;
        std::string gamePath;

        try {
            documentsPath = GetDocumentsPath();
            gamePath = GetGamePath();
        } catch (...) {
            documentsPath = "C:\\Users\\Default\\Documents";
            gamePath = "";
        }

        fs::path dataPath;
        fs::path sksePluginsPath;
        bool pathDetectionSuccessful = false;

        fs::path logFilePath = fs::path(documentsPath) / "My Games" / "Skyrim Special Edition" / "SKSE" /
                               "OBody_NG_Preset_Distribution_Assistant-NG.log";
        CreateDirectoryIfNotExists(logFilePath.parent_path());

        std::ofstream logFile(logFilePath, std::ios::out | std::ios::trunc);

        auto now = std::chrono::system_clock::now();
        std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm buf;
        localtime_s(&buf, &in_time_t);

        logFile << "====================================================" << std::endl;
        logFile << "OBody NG Preset Distribution Assistant NG v2.4.9" << std::endl;
        logFile << "Log created on: " << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << std::endl;
        logFile << "====================================================" << std::endl << std::endl;

        logFile << "Searching for game installation..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;

        std::string mo2OverwritePath = GetEnvVar("MO_OVERWRITE_PATH");

        if (!mo2OverwritePath.empty()) {
            fs::path mo2Path = fs::path(mo2OverwritePath) / "SKSE" / "Plugins";
            logFile << "Trying MO2 Overwrite path: " << mo2Path.string() << std::endl;

            if (fs::exists(mo2Path)) {
                if (IsValidPluginPath(mo2Path, logFile)) {
                    sksePluginsPath = mo2Path;
                    dataPath = fs::path(mo2OverwritePath);
                    logFile << "Valid installation in MO2 Overwrite" << std::endl;
                    pathDetectionSuccessful = true;
                }
            }
        }

        if (!pathDetectionSuccessful && !gamePath.empty()) {
            fs::path standardPath = fs::path(gamePath) / "Data" / "SKSE" / "Plugins";
            logFile << "Trying standard game path: " << standardPath.string() << std::endl;

            if (fs::exists(standardPath)) {
                if (IsValidPluginPath(standardPath, logFile)) {
                    sksePluginsPath = standardPath;
                    dataPath = fs::path(gamePath) / "Data";
                    logFile << "Valid installation at standard game path" << std::endl;
                    pathDetectionSuccessful = true;
                } else {
                    logFile << "Standard path exists but DLL not found" << std::endl;
                }
            } else {
                logFile << "Standard path does not exist" << std::endl;
            }
        }

        if (!pathDetectionSuccessful) {
            logFile << std::endl;
            logFile << "FALLBACK MODE: DLL Directory Detection" << std::endl;
            logFile << "(Wabbajack/MO2 Portable/Nolvus/Non-standard)" << std::endl;
            logFile << std::endl;

            fs::path dllDir = GetDllDirectory(logFile);
            
            if (!dllDir.empty()) {
                fs::path calculatedGamePath = dllDir.parent_path().parent_path().parent_path();
                
                logFile << "DLL directory detected: " << dllDir.string() << std::endl;
                logFile << "Calculated game path: " << calculatedGamePath.string() << std::endl;
                logFile << std::endl;
                
                dataPath = BuildPathCaseInsensitive(calculatedGamePath, {"Data"}, logFile);
                sksePluginsPath = BuildPathCaseInsensitive(dataPath, {"SKSE", "Plugins"}, logFile);
                
                if (IsValidPluginPath(sksePluginsPath, logFile)) {
                    logFile << std::endl;
                    logFile << "DLL directory method successful" << std::endl;
                    logFile << std::endl;
                    pathDetectionSuccessful = true;
                }
            }
        }

        if (!pathDetectionSuccessful) {
            logFile << std::endl;
            logFile << "CRITICAL ERROR: NO VALID PATH DETECTED" << std::endl;
            logFile << std::endl;
            logFile << "All detection methods failed:" << std::endl;
            logFile << "  METHOD 1 (MO2 Variables): FAILED" << std::endl;
            logFile << "  METHOD 2 (Registry/Standard): " << (gamePath.empty() ? "FAILED (empty)" : "FAILED (DLL not found)") << std::endl;
            logFile << "  METHOD 3 (DLL Directory): FAILED" << std::endl;
            logFile << std::endl;
            logFile << "POSSIBLE SOLUTIONS:" << std::endl;
            logFile << "1. Reinstall OBody NG and this plugin" << std::endl;
            logFile << "2. Run SKSE through Mod Organizer 2 if using MO2" << std::endl;
            logFile << "3. For Wabbajack/Nolvus: Ensure the DLL is in Data/SKSE/Plugins/" << std::endl;
            logFile << "4. Check mod installation in your mod manager" << std::endl;
            logFile << "5. Verify Skyrim SE is properly installed" << std::endl;
            logFile << std::endl;
            logFile << "====================================================" << std::endl;
            logFile.close();

            return false;
        }

        logFile << std::endl;
        logFile << "SUCCESS: Paths detected successfully" << std::endl;
        logFile << "Data path: " << dataPath.string() << std::endl;
        logFile << "SKSE Plugins path: " << sksePluginsPath.string() << std::endl;
        logFile << std::endl;

        CreateDirectoryIfNotExists(sksePluginsPath);

        fs::path configIniPath;
        fs::path tempConfigPath;
        
        if (FindFileWithFallback(sksePluginsPath, "OBody_NG_Preset_Distribution_Assistant_NG.ini", 
                                 tempConfigPath, logFile)) {
            configIniPath = tempConfigPath;
        } else {
            configIniPath = sksePluginsPath / "OBody_NG_Preset_Distribution_Assistant_NG.ini";
        }
        
        fs::path jsonOutputPath;
        fs::path tempJsonPath;
        
        if (FindFileWithFallback(sksePluginsPath, "OBody_presetDistributionConfig.json", 
                                 tempJsonPath, logFile)) {
            jsonOutputPath = tempJsonPath;
        } else {
            jsonOutputPath = sksePluginsPath / "OBody_presetDistributionConfig.json";
        }
        
        fs::path backupJsonPath = BuildPathCaseInsensitive(
            sksePluginsPath, 
            {"Backup_OBody_DPA"}, 
            logFile
        ) / "OBody_presetDistributionConfig.json";
        
        fs::path analysisDir = BuildPathCaseInsensitive(
            sksePluginsPath, 
            {"Backup_OBody_DPA", "Analysis"}, 
            logFile
        );
        
        fs::path bodySlidePresetsPath = BuildPathCaseInsensitive(
            dataPath, 
            {"CalienteTools", "BodySlide", "SliderPresets"}, 
            logFile
        );

        logFile << "Reading configuration..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;
        ConfigSettings config = ReadConfigFromIni(configIniPath, logFile);

        logFile << std::endl;
        if (!PerformSimpleJsonIntegrityCheck(jsonOutputPath, logFile)) {
            logFile << std::endl;
            logFile << "CRITICAL: JSON failed simple integrity check at startup - Attempting to restore from backup..."
                    << std::endl;

            if (RestoreJsonFromBackup(backupJsonPath, jsonOutputPath, analysisDir, logFile)) {
                logFile << "SUCCESS: JSON restored from backup. Proceeding with the normal process."
                        << std::endl;
            } else {
                logFile << std::endl;
                logFile << "CRITICAL ERROR: Could not restore from backup. The JSON file is likely corrupted and no valid backup is available."
                        << std::endl;
                logFile << "Process terminated to prevent further damage." << std::endl;
                logFile << std::endl;
                logFile << "RECOMMENDED ACTIONS:" << std::endl;
                logFile << "1. Check the analysis folder for the corrupted file: " << analysisDir.string()
                        << std::endl;
                logFile << "2. Manually check for any older backups or reinstall the mod providing the base JSON file."
                        << std::endl;
                logFile << "3. Contact the mod author if the problem persists." << std::endl;
                logFile << "====================================================" << std::endl;
                logFile.close();

                return false;
            }
        }

        logFile << "JSON passed initial integrity check or was restored - proceeding with normal process..."
                << std::endl;
        logFile << std::endl;

        const std::set<std::string> validKeys = {
            "npc", "factionFemale", "factionMale",
            "npcPluginFemale", "npcPluginMale", "raceFemale", "raceMale"};

        std::map<std::string, OrderedPluginData> processedData;
        NpcFormIDData npcFormIDProcessedData;
        
        for (const auto& key : validKeys) {
            processedData[key] = OrderedPluginData();
        }
        processedData["blacklistedPresetsFromRandomDistribution"] = OrderedPluginData();
        processedData["blacklistedNpcs"] = OrderedPluginData();
        processedData["blacklistedNpcsFormID"] = OrderedPluginData();
        processedData["blacklistedNpcsPluginFemale"] = OrderedPluginData();
        processedData["blacklistedNpcsPluginMale"] = OrderedPluginData();
        processedData["blacklistedRacesFemale"] = OrderedPluginData();
        processedData["blacklistedRacesMale"] = OrderedPluginData();
        processedData["blacklistedOutfitsFromORefitFormID"] = OrderedPluginData();
        processedData["blacklistedOutfitsFromORefit"] = OrderedPluginData();
        processedData["blacklistedOutfitsFromORefitPlugin"] = OrderedPluginData();
        processedData["outfitsForceRefitFormID"] = OrderedPluginData();
        processedData["outfitsForceRefit"] = OrderedPluginData();

        bool backupPerformed = false;

        if (config.backupValue == 1 || config.backupValue == 2) {
            if (config.backupValue == 2) {
                logFile << "Backup enabled (Backup = true), performing LITERAL backup always..."
                        << std::endl;
            } else {
                logFile << "Backup enabled (Backup = 1), performing LITERAL backup..." << std::endl;
            }

            if (PerformLiteralJsonBackup(jsonOutputPath, backupJsonPath, logFile)) {
                backupPerformed = true;
                if (config.backupValue != 2) {
                    UpdateBackupConfigInIni(configIniPath, logFile, config.backupValue);
                }
            } else {
                logFile << "ERROR: LITERAL backup failed, continuing with normal process..." << std::endl;
            }

        } else {
            logFile << "Backup disabled (Backup = 0), skipping backup" << std::endl;
        }

        logFile << std::endl;

        auto [readSuccess, originalJsonContent, currentBlacklistedPresetsShow] = 
            ReadCompleteJson(jsonOutputPath, processedData, npcFormIDProcessedData, logFile);

        if (!readSuccess) {
            logFile << "JSON read failed, attempting to restore from backup..." << std::endl;
            if (fs::exists(backupJsonPath) &&
                RestoreJsonFromBackup(backupJsonPath, jsonOutputPath, analysisDir, logFile)) {
                logFile << "Backup restoration successful, retrying JSON read..." << std::endl;
                auto retryResult = ReadCompleteJson(jsonOutputPath, processedData, npcFormIDProcessedData, logFile);
                readSuccess = std::get<0>(retryResult);
                originalJsonContent = std::get<1>(retryResult);
                currentBlacklistedPresetsShow = std::get<2>(retryResult);
            }

            if (!readSuccess) {
                logFile << "Process truncated due to JSON read error. No INI processing or updates performed."
                        << std::endl;
                logFile << "====================================================" << std::endl;
                logFile.close();

                return false;
            } else {
                logFile << "JSON read successful after restoration" << std::endl;
            }
        }

        fs::path logDoctorPath = fs::path(documentsPath) / "My Games" / "Skyrim Special Edition" / "SKSE" /
                                 "OBody_NG_Preset_Distribution_Assistant-NG_Doctor.log";
        GenerateDoctorLog(bodySlidePresetsPath, logDoctorPath, logFile);

        logFile << std::endl;
        logFile << "PHASE 1: Building Master Preset Map from XML files..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;
        PresetMapData masterPresetMap = BuildPresetNameMap(bodySlidePresetsPath, logFile);

        fs::path logSmartCleaningPath = fs::path(documentsPath) / "My Games" / "Skyrim Special Edition" / "SKSE" /
                                        "OBody_NG_Preset_Distribution_Assistant-NG_Smart_Cleaning.log";
        GenerateSmartCleaningLog(masterPresetMap, logSmartCleaningPath, logFile);

        fs::path logHelperPath = fs::path(documentsPath) / "My Games" / "Skyrim Special Edition" / "SKSE" /
                                 "OBody_NG_Preset_Distribution_Assistant-NG_List-Helper.log";
        GenerateHelperLog(masterPresetMap, logHelperPath, logFile);

        fs::path logINIAnalysisPath = fs::path(documentsPath) / "My Games" / "Skyrim Special Edition" / "SKSE" /
                                      "OBody_NG_Preset_Distribution_Assistant-NG_Analysis_INIs.log";
        GenerateINIAnalysisLog(dataPath, logINIAnalysisPath, logFile);

        logFile << std::endl;
        logFile << "PHASE 2: Processing XML files (UBE/HIMBO) - Establishing automatic base..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;
        
        auto [allPresetsForBlacklist, presetsForRaces] = ProcessUBEXmlPresets(bodySlidePresetsPath, logFile);
        bool ubeChangesApplied = ApplyUBEPresetsToJson(processedData, allPresetsForBlacklist, presetsForRaces, logFile);

        auto [allHIMBOPresetsForBlacklist, himboPresetsForRaces] = ProcessHIMBOXmlPresets(bodySlidePresetsPath, logFile);
        bool himboChangesApplied = ApplyHIMBOPresetsToJson(processedData, allHIMBOPresetsForBlacklist, himboPresetsForRaces, config, logFile);

        logFile << std::endl;
        logFile << "PHASE 3: Processing INI rules with PRIORITY SYSTEM (User customization)..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;
        logFile << "Priority Order: 1=Basic | 2=Additive | 3=Removal(-) | 4=Exclusive(*)" << std::endl;
        logFile << "----------------------------------------------------" << std::endl;

        int totalRulesProcessed = 0;
        int totalRulesApplied = 0;
        int totalRulesSkipped = 0;
        int totalPresetsRemoved = 0;
        int totalPluginsRemoved = 0;
        int totalFilesProcessed = 0;
        int totalFilteringRulesApplied = 0;
        int totalNpcFormIDRulesProcessed = 0;
        std::vector<SpecialRule> specialRules;
        RuleConflictTracker conflictTracker;
        std::vector<RuleWithPriority> allRulesToApply;

        logFile << "Scanning for OBodyNG_PDA_*.ini files..." << std::endl;
        logFile << "Primary path: " << dataPath.string() << std::endl;
        logFile << std::endl;
        
        std::vector<fs::path> iniFilesToProcess;
        
        if (fs::exists(dataPath) && fs::is_directory(dataPath)) {
            logFile << "Scanning primary path for INIs..." << std::endl;
            
            try {
                for (const auto& entry : fs::directory_iterator(dataPath)) {
                    try {
                        if (entry.is_regular_file()) {
                            std::string filename = entry.path().filename().string();
                            std::string lowerFilename = filename;
                            std::transform(lowerFilename.begin(), lowerFilename.end(), 
                                         lowerFilename.begin(), ::tolower);
                            
                            if (lowerFilename.find("obodyng_pda_") == 0 && 
                                lowerFilename.find(".ini") != std::string::npos) {
                                iniFilesToProcess.push_back(entry.path());
                                logFile << "  Found INI: " << filename << std::endl;
                            }
                        }
                    } catch (...) {
                        continue;
                    }
                }
            } catch (...) {
                logFile << "Error scanning primary path for INIs" << std::endl;
            }
        }
        
        if (iniFilesToProcess.empty()) {
            logFile << "No INIs found in primary path, trying fallback with double backslash..." << std::endl;
            
            std::string dataPathStr = dataPath.string();
            if (!dataPathStr.empty() && dataPathStr.back() != '\\') {
                dataPathStr += '\\';
            }
            dataPathStr += '\\';
            
            try {
                fs::path fallbackPath(dataPathStr);
                
                if (fs::exists(fallbackPath) && fs::is_directory(fallbackPath)) {
                    logFile << "Fallback path exists: " << fallbackPath.string() << std::endl;
                    
                    for (const auto& entry : fs::directory_iterator(fallbackPath)) {
                        try {
                            if (entry.is_regular_file()) {
                                std::string filename = entry.path().filename().string();
                                std::string lowerFilename = filename;
                                std::transform(lowerFilename.begin(), lowerFilename.end(), 
                                             lowerFilename.begin(), ::tolower);
                                
                                if (lowerFilename.find("obodyng_pda_") == 0 && 
                                    lowerFilename.find(".ini") != std::string::npos) {
                                    iniFilesToProcess.push_back(entry.path());
                                    logFile << "  Found INI (fallback): " << filename << std::endl;
                                }
                            }
                        } catch (...) {
                            continue;
                        }
                    }
                } else {
                    logFile << "Fallback path does not exist" << std::endl;
                }
            } catch (const std::exception& e) {
                logFile << "Fallback search failed: " << e.what() << std::endl;
            } catch (...) {
                logFile << "Fallback search failed: Unknown error" << std::endl;
            }
        }
        
        if (iniFilesToProcess.empty()) {
            logFile << std::endl;
            logFile << "No INI files found. Process will continue with existing JSON data." << std::endl;
            logFile << std::endl;
        } else {
            logFile << std::endl;
            logFile << "Total INI files found: " << iniFilesToProcess.size() << std::endl;
            logFile << std::endl;
            
            for (const auto& iniPath : iniFilesToProcess) {
                try {
                    std::string filename = iniPath.filename().string();
                    logFile << std::endl << "Reading file: " << filename << std::endl;
                    totalFilesProcessed++;

                    std::string iniContent = ReadFileWithEncoding(iniPath);
                    if (iniContent.empty()) {
                        logFile << "  ERROR: Could not read file or file is empty" << std::endl;
                        continue;
                    }

                    size_t totalLines = std::count(iniContent.begin(), iniContent.end(), '\n');
                    logFile << "  File size: " << iniContent.size() << " bytes, " 
                            << totalLines << " lines detected" << std::endl;

                    std::stringstream iniStream(iniContent);
                    std::string line;
                    int rulesInFile = 0;
                    int specialRulesInFile = 0;

                    while (std::getline(iniStream, line)) {
                        std::string originalLine = line;

                        if (line.size() > 10000) {
                            logFile << "  WARNING: Skipping abnormally long line (" 
                                    << line.size() << " chars)" << std::endl;
                            continue;
                        }

                        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || 
                               line.back() == '\r' || line.back() == '\n')) {
                            line.pop_back();
                        }

                        line = RemoveCommentsSafely(line);

                        size_t equalPos = line.find('=');
                        if (equalPos != std::string::npos) {
                            std::string key = Trim(line.substr(0, equalPos));
                            std::string value = Trim(line.substr(equalPos + 1));

                            if (key == "npcFormID" && !value.empty()) {
                                NpcFormIDRule npcRule = ParseNpcFormIDRuleLine(value, logFile);
                                
                                if (!npcRule.plugin.empty() && !npcRule.formID.empty()) {
                                    if (npcRule.mode == INIRuleMode::EXCLUSIVE_ALWAYS) {
                                        npcFormIDProcessedData.data[npcRule.plugin][npcRule.formID] = npcRule.presets;
                                    } else if (npcRule.mode != INIRuleMode::DISABLED) {
                                        for (const auto& preset : npcRule.presets) {
                                            npcFormIDProcessedData.addPresetToFormID(npcRule.plugin, npcRule.formID, preset);
                                        }
                                    }
                                    
                                    totalNpcFormIDRulesProcessed++;
                                    logFile << "  npcFormID rule: " << npcRule.plugin << " | FormID: " << npcRule.formID 
                                            << " | Presets: " << npcRule.presets.size() << std::endl;
                                }
                                continue;
                            }

                            if (std::find(NPC_FORMID_TYPES.begin(), NPC_FORMID_TYPES.end(), key) != NPC_FORMID_TYPES.end() && !value.empty()) {
                                SpecialRule specialRule = ParseFormIDRuleLine(key, value, logFile);
                                if (!specialRule.targetKey.empty()) {
                                    specialRules.push_back(specialRule);
                                    specialRulesInFile++;
                                    logFile << "  NPC FormID rule: " << key << " -> Plugin: " << specialRule.plugin << std::endl;
                                }
                                continue;
                            }

                            if (std::find(OUTFIT_FORMID_TYPES.begin(), OUTFIT_FORMID_TYPES.end(), key) != OUTFIT_FORMID_TYPES.end() && !value.empty()) {
                                SpecialRule specialRule = ParseFormIDRuleLine(key, value, logFile);
                                if (!specialRule.targetKey.empty()) {
                                    specialRules.push_back(specialRule);
                                    specialRulesInFile++;
                                    logFile << "  Outfit FormID rule: " << key << " -> Plugin: " << specialRule.plugin << std::endl;
                                }
                                continue;
                            }

                            if (key == "outfits" && !value.empty()) {
                                SpecialRule specialRule = ParseOutfitRuleLine(key, value);
                                if (!specialRule.targetKey.empty()) {
                                    specialRules.push_back(specialRule);
                                    specialRulesInFile++;
                                    logFile << "  Outfit array rule: " << specialRule.targetKey << std::endl;
                                }
                                continue;
                            }

                            if ((key == "raceFemaleUBE" || key == "raceMaleAny" || StartsWith(key, "blacklisted")) && !value.empty()) {
                                SpecialRule specialRule = ParseSpecialRuleLine(key, value);
                                
                                if (!specialRule.targetKey.empty()) {
                                    specialRules.push_back(specialRule);
                                    specialRulesInFile++;
                                    
                                    if (IsExclusiveMode(specialRule.mode)) {
                                        conflictTracker.addExclusiveRule(specialRule.targetKey, specialRule.plugin, 
                                                                        filename, iniPath, originalLine, specialRule.mode);
                                    }
                                    
                                    logFile << "  Special rule detected: " << key << " -> " << specialRule.targetKey
                                            << " -> Plugin/Target: " << specialRule.plugin << std::endl;
                                } else {
                                    logFile << "  ERROR: Invalid special rule: " << key << std::endl;
                                }
                                continue;
                            }

                            if (validKeys.count(key) && !value.empty()) {
                                ParsedRule rule = ParseRuleLine(key, value);

                                if (!rule.plugin.empty()) {
                                    rulesInFile++;
                                    totalRulesProcessed++;

                                    RuleWithPriority ruleWithPriority;
                                    ruleWithPriority.key = rule.key;
                                    ruleWithPriority.plugin = rule.plugin;
                                    ruleWithPriority.presets = rule.presets;
                                    ruleWithPriority.extra = rule.extra;
                                    ruleWithPriority.applyCount = rule.applyCount;
                                    ruleWithPriority.mode = rule.mode;
                                    ruleWithPriority.filterFragments = rule.filterFragments;
                                    ruleWithPriority.originalLine = originalLine;
                                    ruleWithPriority.iniPath = iniPath;
                                    ruleWithPriority.priority = GetRulePriority(rule.mode);

                                    if (IsExclusiveMode(rule.mode)) {
                                        conflictTracker.addExclusiveRule(key, rule.plugin, filename, 
                                                                        iniPath, originalLine, rule.mode);
                                    }

                                    allRulesToApply.push_back(ruleWithPriority);
                                }
                            }
                        }
                    }

                    logFile << "  Rules in file: " << rulesInFile
                            << " | Special rules: " << specialRulesInFile << std::endl;
                            
                } catch (const std::exception& e) {
                    logFile << "ERROR processing INI file: " << e.what() << std::endl;
                } catch (...) {
                    logFile << "ERROR processing INI file: Unknown exception" << std::endl;
                }
            }
        }

        logFile << std::endl;
        logFile << "Total rules collected: " << allRulesToApply.size() << std::endl;
        logFile << "Total npcFormID rules processed: " << totalNpcFormIDRulesProcessed << std::endl;
        logFile << "Sorting rules by priority..." << std::endl;
        logFile << std::endl;

        std::stable_sort(allRulesToApply.begin(), allRulesToApply.end(), 
            [](const RuleWithPriority& a, const RuleWithPriority& b) {
                return a.priority < b.priority;
            });
        
        ConflictResolution conflictResolution = ResolveConflicts(conflictTracker, config.conflictSmartResolution, logFile);
        
        GenerateConflictReport(conflictTracker, conflictResolution, config.conflictSmartResolution, logINIAnalysisPath, logFile);

        logFile << "Applying rules in priority order..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;

        for (const auto& ruleData : allRulesToApply) {
            std::string ruleIdentifier = ruleData.iniPath.filename().string() + "|" + ruleData.originalLine;
            
            bool isConflicted = conflictTracker.hasConflict(ruleData.key, ruleData.plugin);
            bool isAllowed = true;
            
            if (isConflicted) {
                auto it = conflictResolution.ruleIsAllowed.find(ruleIdentifier);
                if (it != conflictResolution.ruleIsAllowed.end()) {
                    isAllowed = it->second;
                } else {
                    isAllowed = false;
                }
            }
            
            if (!isAllowed) {
                totalRulesSkipped++;
                logFile << "  SKIPPED (CONFLICT RESOLVED): Priority=" << ruleData.priority 
                        << " | " << ruleData.key << " -> " << ruleData.plugin << std::endl;
                continue;
            }
            
            bool shouldApply = false;
            bool needsUpdate = false;
            int newCount = ruleData.applyCount;
            
            if (ruleData.mode >= INIRuleMode::KEYWORD && ruleData.mode <= INIRuleMode::KEYHIMBO_REMOVE_ONCE) {
                shouldApply = true;
                totalFilteringRulesApplied++;
                
                if (IsOnceMode(ruleData.mode)) {
                    needsUpdate = true;
                    newCount = 0;
                }
                
                auto& data = processedData[ruleData.key];
                std::vector<std::string> matchingPresets;
                std::vector<std::string> notFoundPresets;
                
                switch (ruleData.mode) {
                    case INIRuleMode::KEYWORD:
                    case INIRuleMode::KEYWORD_EXCLUSIVE:
                    case INIRuleMode::KEYWORD_REMOVE:
                    case INIRuleMode::KEYWORD_ONCE:
                    case INIRuleMode::KEYWORD_EXCLUSIVE_ONCE:
                    case INIRuleMode::KEYWORD_REMOVE_ONCE:
                        matchingPresets = FindMatchingPresetsByKeyWord(masterPresetMap, ruleData.filterFragments, logFile);
                        break;
                        
                    case INIRuleMode::KEYWORDCHART:
                    case INIRuleMode::KEYWORDCHART_EXCLUSIVE:
                    case INIRuleMode::KEYWORDCHART_REMOVE:
                    case INIRuleMode::KEYWORDCHART_ONCE:
                    case INIRuleMode::KEYWORDCHART_EXCLUSIVE_ONCE:
                    case INIRuleMode::KEYWORDCHART_REMOVE_ONCE:
                        matchingPresets = FindMatchingPresetsByKeyWordChart(masterPresetMap, ruleData.filterFragments, logFile);
                        break;
                        
                    case INIRuleMode::KEYAUTHOR:
                    case INIRuleMode::KEYAUTHOR_EXCLUSIVE:
                    case INIRuleMode::KEYAUTHOR_REMOVE:
                    case INIRuleMode::KEYAUTHOR_ONCE:
                    case INIRuleMode::KEYAUTHOR_EXCLUSIVE_ONCE:
                    case INIRuleMode::KEYAUTHOR_REMOVE_ONCE:
                        matchingPresets = FindMatchingPresetsByKeyAuthor(masterPresetMap, ruleData.filterFragments, logFile);
                        break;
                        
                    case INIRuleMode::KEYNORMAL:
                    case INIRuleMode::KEYNORMAL_EXCLUSIVE:
                    case INIRuleMode::KEYNORMAL_REMOVE:
                    case INIRuleMode::KEYNORMAL_ONCE:
                    case INIRuleMode::KEYNORMAL_EXCLUSIVE_ONCE:
                    case INIRuleMode::KEYNORMAL_REMOVE_ONCE:
                        matchingPresets = FindMatchingPresetsByFamily(masterPresetMap, ruleData.filterFragments, "KeyNormal", logFile, notFoundPresets);
                        break;
                        
                    case INIRuleMode::KEYUBE:
                    case INIRuleMode::KEYUBE_EXCLUSIVE:
                    case INIRuleMode::KEYUBE_REMOVE:
                    case INIRuleMode::KEYUBE_ONCE:
                    case INIRuleMode::KEYUBE_EXCLUSIVE_ONCE:
                    case INIRuleMode::KEYUBE_REMOVE_ONCE:
                        matchingPresets = FindMatchingPresetsByFamily(masterPresetMap, ruleData.filterFragments, "KeyUBE", logFile, notFoundPresets);
                        break;
                        
                    case INIRuleMode::KEYHIMBO:
                    case INIRuleMode::KEYHIMBO_EXCLUSIVE:
                    case INIRuleMode::KEYHIMBO_REMOVE:
                    case INIRuleMode::KEYHIMBO_ONCE:
                    case INIRuleMode::KEYHIMBO_EXCLUSIVE_ONCE:
                    case INIRuleMode::KEYHIMBO_REMOVE_ONCE:
                        matchingPresets = FindMatchingPresetsByFamily(masterPresetMap, ruleData.filterFragments, "KeyHIMBO", logFile, notFoundPresets);
                        break;
                        
                    default:
                        break;
                }
                
                for (const auto& notFound : notFoundPresets) {
                    logFile << "    WARNING: Preset '" << notFound << "' not found in specified family - SKIPPED" << std::endl;
                }
                
                if (!matchingPresets.empty()) {
                    if (IsExclusiveMode(ruleData.mode)) {
                        if (data.hasPlugin(ruleData.plugin)) {
                            data.removePlugin(ruleData.plugin);
                        }
                        for (const auto& preset : matchingPresets) {
                            data.addPreset(ruleData.plugin, preset);
                        }
                        logFile << "  [P" << ruleData.priority << "] EXCLUSIVE: " << ruleData.key 
                                << " -> " << ruleData.plugin << " -> Replaced with " 
                                << matchingPresets.size() << " presets";
                        if (IsOnceMode(ruleData.mode)) {
                            logFile << " (ONE TIME)";
                        }
                        logFile << std::endl;
                        
                    } else if (IsRemovalMode(ruleData.mode)) {
                        int removedCount = 0;
                        for (const auto& preset : matchingPresets) {
                            size_t beforeCount = data.getTotalPresetCount();
                            data.removePreset(ruleData.plugin, preset);
                            if (data.getTotalPresetCount() < beforeCount) {
                                removedCount++;
                            }
                        }
                        logFile << "  [P" << ruleData.priority << "] REMOVAL: " << ruleData.key 
                                << " -> " << ruleData.plugin << " -> Removed " 
                                << removedCount << " presets";
                        if (IsOnceMode(ruleData.mode)) {
                            logFile << " (ONE TIME)";
                        }
                        logFile << std::endl;
                        totalPresetsRemoved += removedCount;
                        
                    } else {
                        int addedCount = 0;
                        for (const auto& preset : matchingPresets) {
                            size_t beforeCount = data.getTotalPresetCount();
                            data.addPreset(ruleData.plugin, preset);
                            if (data.getTotalPresetCount() > beforeCount) {
                                addedCount++;
                            }
                        }
                        logFile << "  [P" << ruleData.priority << "] ADDITIVE: " << ruleData.key 
                                << " -> " << ruleData.plugin << " -> Added " 
                                << addedCount << " new presets";
                        if (IsOnceMode(ruleData.mode)) {
                            logFile << " (ONE TIME)";
                        }
                        logFile << std::endl;
                    }
                    
                    totalRulesApplied++;
                } else {
                    logFile << "  [P" << ruleData.priority << "] No matches: " << ruleData.key 
                            << " -> " << ruleData.plugin << std::endl;
                }
                
                if (needsUpdate) {
                    UpdateIniRuleCount(ruleData.iniPath, ruleData.originalLine, newCount);
                }
                
            } else if (ruleData.mode == INIRuleMode::ORGANIZE_REMOVE_ONCE) {
                shouldApply = true;
                needsUpdate = true;
                newCount = 0;
                
                auto& data = processedData[ruleData.key];
                int removedCount = 0;
                
                for (const auto& preset : ruleData.presets) {
                    size_t beforeCount = data.getTotalPresetCount();
                    data.removePreset(ruleData.plugin, preset);
                    if (data.getTotalPresetCount() < beforeCount) {
                        removedCount++;
                    }
                }
                
                logFile << "  [P" << ruleData.priority << "] ORGANIZE_REMOVE_ONCE: Removed " << removedCount 
                        << " presets (disabled)" << std::endl;
                totalPresetsRemoved += removedCount;
                totalRulesApplied++;
                UpdateIniRuleCount(ruleData.iniPath, ruleData.originalLine, newCount);
                
            } else if (ruleData.mode == INIRuleMode::ORGANIZE_EXCLUSIVE_ONCE) {
                shouldApply = true;
                needsUpdate = true;
                newCount = 0;
                
                auto& data = processedData[ruleData.key];
                
                if (ruleData.presets.empty()) {
                    if (data.hasPlugin(ruleData.plugin)) {
                        data.removePlugin(ruleData.plugin);
                        logFile << "  [P" << ruleData.priority << "] ORGANIZE_EXCLUSIVE_ONCE: Removed entire plugin (disabled)" << std::endl;
                        totalPluginsRemoved++;
                    }
                } else {
                    if (data.hasPlugin(ruleData.plugin)) {
                        data.removePlugin(ruleData.plugin);
                    }
                    int addedCount = 0;
                    for (const auto& preset : ruleData.presets) {
                        data.addPreset(ruleData.plugin, preset);
                        addedCount++;
                    }
                    logFile << "  [P" << ruleData.priority << "] ORGANIZE_EXCLUSIVE_ONCE: Replaced with " 
                            << addedCount << " presets (disabled)" << std::endl;
                }
                
                totalRulesApplied++;
                UpdateIniRuleCount(ruleData.iniPath, ruleData.originalLine, newCount);
                
            } else if (ruleData.mode == INIRuleMode::EXCLUSIVE_ALWAYS) {
                shouldApply = true;
                
                auto& data = processedData[ruleData.key];
                
                if (data.hasPlugin(ruleData.plugin)) {
                    data.removePlugin(ruleData.plugin);
                    totalPluginsRemoved++;
                }
                
                int addedCount = 0;
                for (const auto& preset : ruleData.presets) {
                    data.addPreset(ruleData.plugin, preset);
                    addedCount++;
                }
                
                logFile << "  [P" << ruleData.priority << "] EXCLUSIVE_ALWAYS: " << ruleData.key
                        << " -> " << ruleData.plugin << " -> Replaced with "
                        << addedCount << " presets (always active)" << std::endl;
                
                totalRulesApplied++;
                
            } else if (ruleData.applyCount == -1 || ruleData.applyCount > 0) {
                shouldApply = true;
                if (ruleData.applyCount > 0) {
                    needsUpdate = true;
                    newCount = ruleData.applyCount - 1;
                }

                auto& data = processedData[ruleData.key];

                if (ruleData.mode == INIRuleMode::REMOVE_ALWAYS || ruleData.mode == INIRuleMode::REMOVE_ONCE) {
                    int presetsRemovedCount = 0;
                    for (const auto& preset : ruleData.presets) {
                        std::string targetPreset = preset;
                        if (!targetPreset.empty() && targetPreset[0] == '!') {
                            targetPreset = targetPreset.substr(1);
                        }

                        size_t beforeCount = data.getTotalPresetCount();
                        data.removePreset(ruleData.plugin, targetPreset);
                        if (data.getTotalPresetCount() < beforeCount) {
                            presetsRemovedCount++;
                        }
                    }

                    if (presetsRemovedCount > 0) {
                        totalRulesApplied++;
                        totalPresetsRemoved += presetsRemovedCount;
                        logFile << "  [P" << ruleData.priority << "] REMOVAL: " << ruleData.key
                                << " -> " << ruleData.plugin << " -> Removed " << presetsRemovedCount << " presets";
                        if (ruleData.mode == INIRuleMode::REMOVE_ONCE) {
                            logFile << " (ONE TIME)";
                            needsUpdate = true;
                            newCount = 0;
                        }
                        logFile << std::endl;
                    } else {
                        logFile << "  [P" << ruleData.priority << "] No presets removed (not found): " 
                                << ruleData.key << " -> " << ruleData.plugin << std::endl;
                    }
                } else {
                    int presetsAdded = 0;
                    for (const auto& preset : ruleData.presets) {
                        size_t beforeCount = data.getTotalPresetCount();
                        data.addPreset(ruleData.plugin, preset);
                        if (data.getTotalPresetCount() > beforeCount) {
                            presetsAdded++;
                        }
                    }

                    if (presetsAdded > 0) {
                        totalRulesApplied++;
                        logFile << "  [P" << ruleData.priority << "] BASIC: " << ruleData.key
                                << " -> " << ruleData.plugin << " -> Added " << presetsAdded << " presets";
                        if (ruleData.applyCount > 0) {
                            logFile << " (remaining: " << newCount << ")";
                        }
                        logFile << std::endl;
                    } else {
                        logFile << "  [P" << ruleData.priority << "] No new presets (all exist): "
                                << ruleData.key << " -> " << ruleData.plugin << std::endl;
                    }
                }

                if (needsUpdate) {
                    UpdateIniRuleCount(ruleData.iniPath, ruleData.originalLine, newCount);
                }

            } else {
                shouldApply = false;
                totalRulesSkipped++;
                logFile << "  [P" << ruleData.priority << "] SKIPPED (count=0): " << ruleData.key
                        << " -> " << ruleData.plugin << std::endl;
            }
        }

        bool specialRulesApplied = ApplySpecialRules(processedData, specialRules, masterPresetMap, logFile);

        logFile << std::endl;
        logFile << "PHASE 4: Smart Cleaning (Final validation against Master Preset Map)..." << std::endl;
        logFile << "----------------------------------------------------" << std::endl;
        
        std::vector<std::string> missingPresetsFromIni;
        PerformSmartCleaning(processedData, config, bodySlidePresetsPath, logFile, missingPresetsFromIni);

        logFile << std::endl;
        logFile << "====================================================" << std::endl;
        logFile << "SUMMARY:" << std::endl;

        if (backupPerformed) {
            try {
                auto backupSize = fs::file_size(backupJsonPath);
                logFile << "Original JSON backup: SUCCESS (" << backupSize << " bytes)" << std::endl;
            } catch (...) {
                logFile << "Original JSON backup: SUCCESS (size verification failed)" << std::endl;
            }

        } else {
            logFile << "Original JSON backup: SKIPPED" << std::endl;
        }

        logFile << "Total .ini files processed: " << totalFilesProcessed << std::endl;
        logFile << "Total rules processed: " << totalRulesProcessed << std::endl;
        logFile << "Total npcFormID rules processed: " << totalNpcFormIDRulesProcessed << std::endl;
        logFile << "Total rules applied: " << totalRulesApplied << std::endl;
        logFile << "Total filtering rules applied: " << totalFilteringRulesApplied << std::endl;
        logFile << "Total special rules detected: " << specialRules.size() << std::endl;
        logFile << "Special rules applied: " << (specialRulesApplied ? "YES" : "NO") << std::endl;
        logFile << "Total rules skipped (count=0 or conflicts): " << totalRulesSkipped << std::endl;
        logFile << "Total presets removed (-): " << totalPresetsRemoved << std::endl;
        logFile << "Total plugins removed (*): " << totalPluginsRemoved << std::endl;
        logFile << "UBE XML presets found: " << allPresetsForBlacklist.size() << std::endl;
        logFile << "UBE presets added to female races: " << presetsForRaces.size() << std::endl;
        logFile << "UBE changes applied: " << (ubeChangesApplied ? "YES" : "NO") << std::endl;
        logFile << "HIMBO XML presets found: " << allHIMBOPresetsForBlacklist.size() << std::endl;
        logFile << "HIMBO presets added to male races: " << himboPresetsForRaces.size() << std::endl;
        logFile << "HIMBO changes applied: " << (himboChangesApplied ? "YES" : "NO") << std::endl;
        logFile << "Smart Cleaning enabled (any): " << ((config.presetsSmartCleaning || config.blacklistedPresetsSmartCleaningFromRandomDistribution || config.blacklistedPresetsSmartCleaningFromAll || config.outfitsForceReSmartCleaning) ? "YES" : "NO") << std::endl;
        logFile << "Smart Conflict Resolution enabled: " << (config.conflictSmartResolution ? "YES" : "NO") << std::endl;
        logFile << "Current blacklistedPresetsShowInOBodyMenu: " << (currentBlacklistedPresetsShow ? "true" : "false") << std::endl;
        logFile << "Target blacklistedPresetsShowInOBodyMenu (ModeUBE): " << (config.modeUBE ? "true" : "false") << std::endl;
        logFile << "ModeHIMBO enabled: " << (config.modeHIMBO ? "YES" : "NO") << std::endl;
        logFile << "Master Preset Map entries: " << masterPresetMap.exactMap.size() << std::endl;
        logFile << "Conflicts detected: " << (conflictTracker.exclusiveRules.empty() ? "NO" : "YES") << std::endl;
        if (config.conflictSmartResolution && !conflictTracker.exclusiveRules.empty()) {
            logFile << "Conflicts resolved: YES (most recent INI files took priority)" << std::endl;
        }
        logFile << std::endl << "Final data in JSON:" << std::endl;

        if (!npcFormIDProcessedData.isEmpty()) {
            logFile << "  npcFormID: " << npcFormIDProcessedData.getTotalPresetCount() << " total entries" << std::endl;
        }
        
        for (const auto& [key, data] : processedData) {
            size_t count = data.getTotalPresetCount();
            if (count > 0) {
                logFile << "  " << key << ": " << data.getPluginCount() << " plugins, " << count
                        << " total presets" << std::endl;
            }
        }

        logFile << "====================================================" << std::endl << std::endl;

        logFile << "Updating JSON at: " << jsonOutputPath.string() << std::endl;

        try {
            std::string updatedJsonContent =
                PreserveOriginalSections(originalJsonContent, processedData, npcFormIDProcessedData,
                                        currentBlacklistedPresetsShow, config.modeUBE, logFile);

            if (CheckIfChangesNeeded(originalJsonContent, processedData, npcFormIDProcessedData,
                                    currentBlacklistedPresetsShow, config.modeUBE)) {
                logFile << "Changes detected. Proceeding with atomic write..." << std::endl;

                if (WriteJsonAtomically(jsonOutputPath, updatedJsonContent, analysisDir, logFile)) {
                    logFile << "SUCCESS: JSON updated successfully with proper 4-space indentation hierarchy"
                            << std::endl;

                    logFile << std::endl;
                    if (CorrectJsonIndentation(jsonOutputPath, analysisDir, logFile)) {
                        logFile << "SUCCESS: JSON indentation verification and correction completed"
                                << std::endl;
                    } else {
                        logFile << "ERROR: JSON indentation correction failed" << std::endl;
                        logFile << "Attempting to restore from backup due to indentation failure..."
                                << std::endl;
                        if (fs::exists(backupJsonPath) &&
                            RestoreJsonFromBackup(backupJsonPath, jsonOutputPath, analysisDir, logFile)) {
                            logFile << "SUCCESS: JSON restored from backup after indentation failure"
                                    << std::endl;
                        } else {
                            logFile << "CRITICAL ERROR: Could not restore JSON from backup" << std::endl;
                        }
                    }
                } else {
                    logFile << "ERROR: Failed to write JSON safely" << std::endl;
                    logFile << "Attempting to restore from backup due to write failure..." << std::endl;
                    if (fs::exists(backupJsonPath) &&
                        RestoreJsonFromBackup(backupJsonPath, jsonOutputPath, analysisDir, logFile)) {
                        logFile << "SUCCESS: JSON restored from backup after write failure" << std::endl;
                    } else {
                        logFile << "CRITICAL ERROR: Could not restore JSON from backup" << std::endl;
                    }
                }
            } else {
                logFile << "No changes detected. Skipping redundant atomic write." << std::endl;

                if (CorrectJsonIndentation(jsonOutputPath, analysisDir, logFile)) {
                    logFile << "JSON indentation is already perfect or has been corrected." << std::endl;
                } else {
                    logFile << "ERROR: JSON indentation correction failed" << std::endl;
                    logFile << "Attempting to restore from backup due to indentation failure..."
                            << std::endl;
                    if (fs::exists(backupJsonPath) &&
                        RestoreJsonFromBackup(backupJsonPath, jsonOutputPath, analysisDir, logFile)) {
                        logFile << "SUCCESS: JSON restored from backup after indentation failure"
                                << std::endl;
                    } else {
                        logFile << "CRITICAL ERROR: Could not restore JSON from backup" << std::endl;
                    }
                }
            }

        } catch (const std::exception& e) {
            logFile << "ERROR in JSON update process: " << e.what() << std::endl;
            logFile << "Attempting to restore from backup due to update failure..." << std::endl;
            if (fs::exists(backupJsonPath) &&
                RestoreJsonFromBackup(backupJsonPath, jsonOutputPath, analysisDir, logFile)) {
                logFile << "SUCCESS: JSON restored from backup after update failure" << std::endl;
            } else {
                logFile << "CRITICAL ERROR: Could not restore JSON from backup" << std::endl;
            }

        } catch (...) {
            logFile << "ERROR in JSON update process: Unknown exception" << std::endl;
            logFile << "Attempting to restore from backup due to unknown failure..." << std::endl;
            if (fs::exists(backupJsonPath) &&
                RestoreJsonFromBackup(backupJsonPath, jsonOutputPath, analysisDir, logFile)) {
                logFile << "SUCCESS: JSON restored from backup after unknown failure" << std::endl;
            } else {
                logFile << "CRITICAL ERROR: Could not restore JSON from backup" << std::endl;
            }
        }

        logFile << std::endl;
        logFile << "====================================================" << std::endl;
        logFile << "PROCESS COMPLETED SUCCESSFULLY - v2.4.9" << std::endl;
        logFile << "====================================================" << std::endl;
        logFile << std::endl;
        logFile << "Plugin loaded at: " << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << std::endl;
        logFile << std::endl;
        logFile << "Installation Details:" << std::endl;
        logFile << "  Data path: " << dataPath.string() << std::endl;
        logFile << "  JSON output: " << jsonOutputPath.string() << std::endl;
        logFile << "  Backup folder: " << backupJsonPath.parent_path().string() << std::endl;
        logFile << std::endl;
        logFile << "For help and documentation, visit:" << std::endl;
        logFile << "  https://john95ac.github.io/website-documents-John95AC/" << std::endl;
        logFile << std::endl;
        logFile << "All systems operational. OBody NG is ready." << std::endl;
        logFile << "====================================================" << std::endl;
        logFile << std::endl;
        logFile.close();
        
        return true;

    } catch (const std::exception& e) {
        return false;
    } catch (...) {
        return false;
    }
}