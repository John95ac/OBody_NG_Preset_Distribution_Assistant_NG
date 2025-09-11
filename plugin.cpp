#include <shlobj.h>
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

// ✅ FUNCIÓN ULTRA-SEGURA PARA CONVERSIÓN WIDE STRING A STRING (SIN CODECVT DEPRECATED)
std::string SafeWideStringToString(const std::wstring& wstr) {
    if (wstr.empty()) {
        return std::string();
    }

    try {
        // Método principal: usar WideCharToMultiByte con CP_UTF8
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
        if (size_needed <= 0) {
            // Fallback: usar CP_ACP (página de códigos del sistema)
            size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
            if (size_needed <= 0) {
                return std::string();  // Conversión completamente fallida
            }

            std::string result(size_needed, 0);
            int converted =
                WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), (int)wstr.size(), &result[0], size_needed, NULL, NULL);
            if (converted <= 0) {
                return std::string();
            }
            return result;
        }

        std::string result(size_needed, 0);
        int converted =
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], size_needed, NULL, NULL);
        if (converted <= 0) {
            return std::string();
        }
        return result;

    } catch (...) {
        // Último recurso: conversión char por char (muy básico pero seguro)
        std::string result;
        result.reserve(wstr.size());
        for (wchar_t wc : wstr) {
            if (wc <= 127) {  // Solo caracteres ASCII seguros
                result.push_back(static_cast<char>(wc));
            } else {
                result.push_back('?');  // Reemplazar caracteres problemáticos
            }
        }
        return result;
    }
}

// Helper to get environment variable safely
std::string GetEnvVar(const std::string& key) {
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, key.c_str()) == 0 && buf != nullptr) {
        std::string value(buf);
        free(buf);
        return value;
    }
    return "";
}

// Estructura para almacenar una regla parseada
struct ParsedRule {
    std::string key;
    std::string plugin;
    std::vector<std::string> presets;
    std::string extra;
    int applyCount = -1;
};

// Estructura CORREGIDA para mantener el orden de los datos
struct OrderedPluginData {
    // Usamos vector de pares para GARANTIZAR el orden
    std::vector<std::pair<std::string, std::vector<std::string>>> orderedData;

    void addPreset(const std::string& plugin, const std::string& preset) {
        // Buscar si el plugin ya existe
        auto it = std::find_if(orderedData.begin(), orderedData.end(),
                               [&plugin](const auto& pair) { return pair.first == plugin; });

        if (it == orderedData.end()) {
            // Plugin nuevo, agregarlo
            orderedData.push_back({plugin, {preset}});
        } else {
            // Plugin existe, verificar si el preset ya está
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
            // Custom find with stripping "!" if present in preset names
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
                // Erase the original without stripping
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

// Helper function to extract substring using loop to avoid constructor issues
std::string extractSubstring(const char* str, size_t start, size_t end) {
    std::string res;
    for (size_t i = start; i < end; ++i) {
        res += str[i];
    }
    return res;
}

// Función para parsear plugins ordenados desde el contenido del objeto JSON
std::vector<std::pair<std::string, std::vector<std::string>>> parseOrderedPlugins(const std::string& content) {
    std::vector<std::pair<std::string, std::vector<std::string>>> result;
    const char* str = content.c_str();
    size_t len = 0;
    while (str[len] != '\0') ++len;
    size_t pos = 0;
    const size_t maxIters = 10000;  // Limit to avoid infinite loop
    size_t iter = 0;

    try {
        while (pos < len && iter++ < maxIters) {
            // Skip whitespace
            while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' ||
                                 str[pos] == '\f' || str[pos] == '\v'))
                ++pos;

            if (pos >= len) break;

            // Expect opening quote for key
            if (str[pos] != '"') {
                ++pos;
                continue;
            }

            size_t keyStart = pos + 1;
            // Find closing quote for key
            ++pos;
            while (pos < len && str[pos] != '"') ++pos;
            if (pos >= len) break;

            std::string plugin = extractSubstring(str, keyStart, pos);
            ++pos;  // skip closing "

            // Skip whitespace
            while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' ||
                                 str[pos] == '\f' || str[pos] == '\v'))
                ++pos;

            // Expect colon
            if (pos >= len || str[pos] != ':') {
                ++pos;
                continue;
            }

            ++pos;  // skip :

            // Skip whitespace
            while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' ||
                                 str[pos] == '\f' || str[pos] == '\v'))
                ++pos;

            // Expect opening bracket
            if (pos >= len || str[pos] != '[') {
                ++pos;
                continue;
            }

            ++pos;  // skip [

            std::vector<std::string> presets;
            size_t presetIter = 0;
            while (pos < len && presetIter++ < maxIters) {
                // Skip whitespace
                while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' ||
                                     str[pos] == '\f' || str[pos] == '\v'))
                    ++pos;

                if (pos >= len) break;

                if (str[pos] == ']') {
                    ++pos;  // skip ]
                    break;
                }

                // Expect quote for preset
                if (str[pos] != '"') {
                    ++pos;
                    continue;
                }

                size_t presetStart = pos + 1;
                // Find closing quote for preset
                ++pos;
                while (pos < len && str[pos] != '"') ++pos;
                if (pos >= len) break;

                std::string preset = extractSubstring(str, presetStart, pos);
                presets.push_back(preset);
                ++pos;  // skip closing "

                // Skip whitespace and expect comma or ]
                while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' ||
                                     str[pos] == '\f' || str[pos] == '\v'))
                    ++pos;

                if (pos < len && str[pos] == ',') {
                    ++pos;  // skip ,
                    // Skip whitespace after comma
                    while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' ||
                                         str[pos] == '\f' || str[pos] == '\v'))
                        ++pos;
                }
            }

            if (presetIter >= maxIters) {
                // Fallback: skip to ]
                while (pos < len && str[pos] != ']') ++pos;
                if (pos < len) ++pos;
            }

            // Skip to next entry (whitespace and comma)
            while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' ||
                                 str[pos] == '\f' || str[pos] == '\v'))
                ++pos;
            if (pos < len && str[pos] == ',') ++pos;

            result.push_back({plugin, presets});
        }

        if (iter >= maxIters) {
            // Log warning if logFile available
        }
    } catch (const std::exception&) {
        // Fallback to empty result on error
    }

    return result;
}

// Función para parsear secciones top-level del JSON preservando orden de aparición
std::vector<std::pair<std::string, std::string>> ParseTopLevelSections(const std::string& json) {
    std::vector<std::pair<std::string, std::string>> sections;
    const char* str = json.c_str();
    size_t len = strlen(str);
    size_t pos = 0;
    const size_t maxIters = 10000;  // Limit to avoid infinite loop on malformed JSON
    size_t iter = 0;

    try {
        while (pos < len && iter++ < maxIters) {
            // Skip whitespace
            while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r')) ++pos;
            if (pos >= len || str[pos] == '}') break;

            if (str[pos] != '"') {
                ++pos;
                continue;
            }

            size_t keyStart = pos + 1;
            ++pos;
            while (pos < len && str[pos] != '"') ++pos;
            if (pos >= len) break;

            std::string key = extractSubstring(str, keyStart, pos);
            ++pos;  // skip "

            // Skip whitespace and :
            while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r')) ++pos;
            if (pos >= len || str[pos] != ':') {
                ++pos;
                continue;
            }
            ++pos;  // skip :

            // Skip whitespace
            while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r')) ++pos;

            // Find end of value (handle {} or [])
            size_t valueStart = pos;
            if (str[pos] == '{' || str[pos] == '[') {
                int braceCount = 1;
                ++pos;
                bool inString = false;
                bool escape = false;
                size_t braceIter = 0;
                while (pos < len && braceCount > 0 && braceIter++ < maxIters) {
                    if (str[pos] == '"' && !escape)
                        inString = !inString;
                    else if (!inString) {
                        if (str[pos] == '{' || str[pos] == '[')
                            ++braceCount;
                        else if (str[pos] == '}' || str[pos] == ']')
                            --braceCount;
                    }

                    escape = (str[pos] == '\\' && !escape);
                    ++pos;
                }

                if (braceIter >= maxIters) {
                    // Fallback: skip to next , or }
                    while (pos < len && str[pos] != ',' && str[pos] != '}') ++pos;
                }
            } else {
                // Simple value, find , or }
                while (pos < len && str[pos] != ',' && str[pos] != '}') ++pos;
            }

            std::string value = extractSubstring(str, valueStart, pos);
            // Inline rtrim for value to remove trailing whitespace
            while (!value.empty() &&
                   (value.back() == '\n' || value.back() == '\r' || value.back() == ' ' || value.back() == '\t')) {
                value.pop_back();
            }

            sections.emplace_back(key, value);

            // Skip to next , or }
            while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r')) ++pos;
            if (pos < len && str[pos] == ',') ++pos;
        }

        if (iter >= maxIters) {
            // Log warning for malformed JSON (logFile not here, but could pass)
        }
    } catch (const std::exception&) {
        // Fallback to empty sections on error
    }

    return sections;
}

// ✅ FUNCIÓN ULTRA-SEGURA para obtener la ruta de la carpeta "Documentos" del usuario
std::string GetDocumentsPath() {
    try {
        wchar_t path[MAX_PATH] = {0};
        HRESULT result = SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);

        if (SUCCEEDED(result)) {
            std::wstring ws(path);
            std::string converted = SafeWideStringToString(ws);
            if (!converted.empty()) {
                return converted;
            }
        }

        // Fallback 1: Usar variables de entorno
        std::string userProfile = GetEnvVar("USERPROFILE");
        if (!userProfile.empty()) {
            return userProfile + "\\Documents";
        }

        // Fallback 2: Ruta por defecto
        std::string homedrive = GetEnvVar("HOMEDRIVE");
        std::string homepath = GetEnvVar("HOMEPATH");
        if (!homedrive.empty() && !homepath.empty()) {
            return homedrive + homepath + "\\Documents";
        }

        // Fallback 3: Ruta hardcoded (último recurso)
        return "C:\\Users\\Default\\Documents";

    } catch (const std::exception&) {
        // Último recurso absoluto
        return "C:\\Users\\Default\\Documents";
    }
}

// ✅ FUNCIÓN ULTRA-SEGURA para obtener la ruta de instalación de Skyrim sea MO2 o Vortex o Steam/GOG/AE/VR
std::string GetGamePath() {
    try {
        // Check mod managers env vars for virtual paths (MO2/Vortex)
        std::string mo2Path = GetEnvVar("MO2_MODS_PATH");
        if (!mo2Path.empty()) {
            return mo2Path;
        }

        std::string vortexPath = GetEnvVar("VORTEX_MODS_PATH");
        if (!vortexPath.empty()) {
            return vortexPath;
        }

        std::string skyrimMods = GetEnvVar("SKYRIM_MODS_FOLDER");
        if (!skyrimMods.empty()) {
            return skyrimMods;
        }

        // Fallback to registry for vanilla/Steam/GOG/AE/VR
        std::vector<std::string> registryKeys = {
            "SOFTWARE\\WOW6432Node\\Bethesda Softworks\\Skyrim Special Edition",
            "SOFTWARE\\WOW6432Node\\GOG.com\\Games\\1457087920",
            "SOFTWARE\\WOW6432Node\\Valve\\Steam\\Apps\\489830",  // AE
            "SOFTWARE\\WOW6432Node\\Valve\\Steam\\Apps\\611670"   // VR
        };

        HKEY hKey;
        char pathBuffer[MAX_PATH] = {0};
        DWORD pathSize = sizeof(pathBuffer);

        for (const auto& key : registryKeys) {
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                if (RegQueryValueExA(hKey, "Installed Path", NULL, NULL, (LPBYTE)pathBuffer, &pathSize) ==
                    ERROR_SUCCESS) {
                    RegCloseKey(hKey);
                    std::string result(pathBuffer);
                    if (!result.empty()) {
                        return result;
                    }
                }
                RegCloseKey(hKey);
            }
        }

        // Fallback: rutas comunes de instalación
        std::vector<std::string> commonPaths = {
            "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special Edition",
            "C:\\Program Files\\Steam\\steamapps\\common\\Skyrim Special Edition",
            "D:\\Steam\\steamapps\\common\\Skyrim Special Edition",
            "E:\\Steam\\steamapps\\common\\Skyrim Special Edition",
            "F:\\Steam\\steamapps\\common\\Skyrim Special Edition"};

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

    } catch (const std::exception&) {
        return "";
    }
}

// ✅ FUNCIÓN SEGURA para crear un directorio si no existe
void CreateDirectoryIfNotExists(const fs::path& path) {
    try {
        if (!fs::exists(path)) {
            fs::create_directories(path);
        }
    } catch (const std::exception&) {
        // Silent fail, log if possible
    } catch (...) {
        // Catch any other type of exception
    }
}

// Función para eliminar espacios al inicio y final
std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Función para dividir una cadena por un delimitador
std::vector<std::string> Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        std::string trimmed = Trim(token);
        if (!trimmed.empty()) {
            tokens.push_back(trimmed);
        }
    }

    return tokens;
}

// Función para escapar caracteres especiales en JSON
std::string EscapeJson(const std::string& str) {
    std::string result;
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
                    snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                    result += buf;
                }
        }
    }
    return result;
}

// ✅ FUNCIÓN CORREGIDA para pretty-print el JSON con indentación consistente de 4 espacios
std::string PrettyPrintJson(const std::string& json) {
    std::string result;
    std::vector<int> indentStack;
    bool inString = false;
    bool escape = false;
    bool afterComma = true;
    bool afterColon = false;
    int currentIndent = 0;
    size_t i = 0;

    indentStack.push_back(0);

    while (i < json.length()) {
        char c = json[i];

        if (inString) {
            result += c;
            if (c == '\\') {
                escape = true;
            } else if (c == '"' && !escape) {
                inString = false;
            }
            escape = false;
        } else {
            if (c == '{' || c == '[') {
                indentStack.push_back(currentIndent);
                currentIndent += 4;  // ✅ 4 espacios de indentación
                if (afterComma) {
                    result += "\n";
                    result += std::string(currentIndent, ' ');
                } else {
                    result += " ";
                }
                result += c;
                afterComma = false;
                afterColon = false;
            } else if (c == '}' || c == ']') {
                currentIndent = indentStack.back();
                indentStack.pop_back();
                result += "\n";
                result += std::string(currentIndent, ' ');
                result += c;
                afterComma = true;
                afterColon = false;
            } else if (c == ',') {
                result += c;
                afterComma = true;
                afterColon = false;
            } else if (c == ':') {
                result += ": ";
                afterColon = true;
                afterComma = false;
            } else if (c == '"') {
                inString = true;
                result += c;
                afterComma = false;
                afterColon = false;
            } else if (std::isspace(static_cast<unsigned char>(c))) {
                if (afterColon && !inString) {
                    // Skip space after colon
                } else if (afterComma && !inString) {
                    result += "\n";
                    result += std::string(currentIndent, ' ');
                }
            } else {
                result += c;
                afterComma = false;
                afterColon = false;
            }
        }

        ++i;
    }

    return result;
}

// Función para parsear una línea del formato: key = textA|text1,text2|x
ParsedRule ParseRuleLine(const std::string& key, const std::string& value) {
    ParsedRule rule;
    rule.key = key;

    // Dividir por |
    std::vector<std::string> parts = Split(value, '|');

    if (parts.size() >= 2) {
        // Primera parte es el plugin/mod
        rule.plugin = parts[0];

        // Segunda parte son los presets separados por comas
        rule.presets = Split(parts[1], ',');

        // Si hay una tercera parte, es el extra (x, 1, 0)
        if (parts.size() >= 3) {
            rule.extra = parts[2];

            // Determinar el conteo de aplicación
            if (rule.extra == "x" || rule.extra == "X") {
                rule.applyCount = -1;  // Ilimitado agregar
            } else if (rule.extra == "x-" || rule.extra == "X-") {
                rule.applyCount = -4;  // Ilimitado remover preset específico
            } else if (rule.extra == "x*" || rule.extra == "X*") {
                rule.applyCount = -5;  // Ilimitado remover plugin completo
            } else if (rule.extra == "-") {
                rule.applyCount = -2;  // Remover preset específico
            } else if (rule.extra == "*") {
                rule.applyCount = -3;  // Remover plugin completo
            } else {
                try {
                    rule.applyCount = std::stoi(rule.extra);
                    if (rule.applyCount < 0) {
                        rule.applyCount = 0;  // Invalidar si negativo y no reconocido
                    }
                } catch (...) {
                    rule.applyCount =
                        1;  // Si no es un número válido, tratar como "1" (aplicar una vez y actualizar a |0)
                }
            }
        } else {
            rule.applyCount = -1;  // Por defecto, ilimitado
        }
    }

    return rule;
}

// ✅ FUNCIÓN ULTRA-SEGURA para leer TODO el JSON existente preservando orden
std::pair<bool, std::string> ReadCompleteJson(const fs::path& jsonPath,
                                              std::map<std::string, OrderedPluginData>& processedData,
                                              std::ofstream& logFile) {
    bool success = false;
    std::string jsonContent = "{}";

    try {
        if (!fs::exists(jsonPath)) {
            logFile
                << "ERROR: JSON file does not exist at: " << jsonPath.string()
                << ". The JSON path does not allow reading the JSON. Contact the modder or reinstall. What happened?"
                << std::endl;
            logFile.flush();
            return {false, ""};
        }

        std::ifstream jsonFile(jsonPath);
        if (!jsonFile.is_open()) {
            logFile
                << "ERROR: Could not open JSON file at: " << jsonPath.string()
                << ". The JSON path does not allow reading the JSON. Contact the modder or reinstall. What happened?"
                << std::endl;
            logFile.flush();
            return {false, ""};
        }

        logFile << "Reading existing JSON from: " << jsonPath.string() << std::endl;
        logFile.flush();

        jsonContent = std::string((std::istreambuf_iterator<char>(jsonFile)), std::istreambuf_iterator<char>());
        jsonFile.close();

        if (jsonContent.empty() || jsonContent == "{}") {
            logFile
                << "ERROR: JSON file is empty or invalid at: " << jsonPath.string()
                << ". The JSON path does not allow reading the JSON. Contact the modder or reinstall. What happened?"
                << std::endl;
            logFile.flush();
            return {false, ""};
        }

        // Limit size for memory safety
        if (jsonContent.size() > 1048576) {  // 1MB limit
            logFile << "Warning: JSON too large (>1MB), using subset." << std::endl;
            jsonContent = jsonContent.substr(0, 1048576);
        }

        jsonContent.reserve(1024 * 1024);  // Pre-reserve for safety

        // Parsear los datos que necesitamos modificar preservando el orden
        const std::vector<std::string> validKeys = {"npcFormID",       "npc",           "factionFemale", "factionMale",
                                                    "npcPluginFemale", "npcPluginMale", "raceFemale",    "raceMale"};

        for (const auto& key : validKeys) {
            processedData[key] = OrderedPluginData();

            // Buscar el patrón: "key": { ... }
            std::string pattern = "\"" + key + "\"\\s*:\\s*\\{([^{}]*)\\}";
            // Necesitamos manejar posibles objetos anidados

            size_t keyPos = jsonContent.find("\"" + key + "\"");
            if (keyPos != std::string::npos) {
                size_t colonPos = jsonContent.find(":", keyPos);
                if (colonPos != std::string::npos) {
                    size_t openBrace = jsonContent.find("{", colonPos);
                    if (openBrace != std::string::npos) {
                        // Encontrar el cierre correspondiente
                        int braceCount = 1;
                        size_t pos = openBrace + 1;
                        size_t closeBrace = std::string::npos;
                        size_t braceIter = 0;
                        const size_t maxBraceIters = 10000;

                        while (pos < jsonContent.length() && braceCount > 0 && braceIter++ < maxBraceIters) {
                            if (jsonContent[pos] == '{')
                                braceCount++;
                            else if (jsonContent[pos] == '}') {
                                braceCount--;
                                if (braceCount == 0) {
                                    closeBrace = pos;
                                    break;
                                }
                            }
                            pos++;
                        }

                        if (braceIter >= maxBraceIters || closeBrace == std::string::npos) {
                            logFile << "Warning: Malformed JSON for key '" << key << "', skipping." << std::endl;
                            continue;
                        }

                        std::string keyContent = jsonContent.substr(openBrace + 1, closeBrace - openBrace - 1);

                        // Buscar todos los pares plugin: [presets] preservando orden de aparición
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

        // Check if any data was loaded
        bool hasData = false;
        for (const auto& [key, data] : processedData) {
            if (data.getTotalPresetCount() > 0) {
                hasData = true;
                break;
            }
        }

        if (!hasData) {
            logFile
                << "ERROR: No valid data loaded from JSON at: " << jsonPath.string()
                << ". The JSON path does not allow reading the JSON. Contact the modder or reinstall. What happened?"
                << std::endl;
            logFile.flush();
            return {false, jsonContent};
        }

        // Log what was loaded
        logFile << "Loaded existing data from JSON:" << std::endl;
        for (const auto& [key, data] : processedData) {
            size_t count = data.getTotalPresetCount();
            if (count > 0) {
                logFile << "  " << key << ": " << data.getPluginCount() << " plugins, " << count << " presets"
                        << std::endl;
            }
        }

        logFile << std::endl;
        logFile.flush();

        success = true;
        return {true, jsonContent};

    } catch (const std::exception& e) {
        logFile << "ERROR in ReadCompleteJson: " << e.what()
                << ". The JSON path does not allow reading the JSON. Contact the modder or reinstall. What happened?"
                << std::endl;
        logFile.flush();
        return {false, ""};
    } catch (...) {
        logFile << "ERROR in ReadCompleteJson: Unknown exception occurred. The JSON path does not allow reading the "
                   "JSON. Contact the modder or reinstall. What happened?"
                << std::endl;
        logFile.flush();
        return {false, ""};
    }
}

// ✅ FUNCIÓN CORREGIDA para actualizar selectivamente el JSON existente preservando orden CON INDENTACIÓN DE 4 ESPACIOS
std::string UpdateJsonSelectively(const std::string& originalJson,
                                  const std::map<std::string, OrderedPluginData>& processedData) {
    try {
        auto topSections = ParseTopLevelSections(originalJson);

        std::stringstream result;
        result << "{\n";

        bool firstSection = true;
        const std::vector<std::string> validKeys = {"npcFormID",       "npc",           "factionFemale", "factionMale",
                                                    "npcPluginFemale", "npcPluginMale", "raceFemale",    "raceMale"};

        for (const auto& [key, originalValue] : topSections) {
            if (!firstSection) {
                result << ",\n";
            }

            firstSection = false;
            result << "    \"" << EscapeJson(key) << "\": ";  // ✅ 4 espacios de indentación

            bool isValidKeyWithData = (std::find(validKeys.begin(), validKeys.end(), key) != validKeys.end() &&
                                       processedData.count(key) && !processedData.at(key).orderedData.empty());

            if (isValidKeyWithData) {
                const auto& data = processedData.at(key);
                result << "{\n";

                bool first = true;
                for (const auto& [plugin, presets] : data.orderedData) {
                    if (!first) {
                        result << ",\n";
                    }

                    first = false;
                    result << "        \"" << EscapeJson(plugin) << "\": [\n";  // ✅ 8 espacios de indentación

                    bool firstPreset = true;
                    for (const auto& preset : presets) {
                        if (!firstPreset) {
                            result << ",\n";
                        }

                        firstPreset = false;
                        result << "            \"" << EscapeJson(preset) << "\"";  // ✅ 12 espacios de indentación
                    }

                    result << "\n        ]";  // ✅ 8 espacios de indentación
                }

                result << "\n    }";  // ✅ 4 espacios de indentación
            } else {
                std::string trimmedValue = originalValue;
                // Inline rtrim for originalValue to remove trailing whitespace
                while (!trimmedValue.empty() && (trimmedValue.back() == '\n' || trimmedValue.back() == '\r' ||
                                                 trimmedValue.back() == ' ' || trimmedValue.back() == '\t')) {
                    trimmedValue.pop_back();
                }

                result << trimmedValue;
            }
        }

        std::string content = result.str();
        content.reserve(1024 * 1024);  // Pre-reserve for memory safety

        // Inline rtrim for content to remove trailing whitespace before adding final \n}
        while (!content.empty() && isspace(static_cast<unsigned char>(content.back()))) {
            content.pop_back();
        }

        return content + "\n}";

    } catch (const std::exception&) {
        // Fallback to original on error
        return originalJson;
    } catch (...) {
        // Fallback to original on any unknown error
        return originalJson;
    }
}

// Función para actualizar el archivo INI con el nuevo conteo
void UpdateIniRuleCount(const fs::path& iniPath, const std::string& originalLine, int newCount) {
    try {
        std::ifstream iniFile(iniPath);
        if (!iniFile.is_open()) return;

        std::vector<std::string> lines;
        std::string line;

        while (std::getline(iniFile, line)) {
            lines.push_back(line);
        }

        iniFile.close();

        // Buscar y actualizar la línea
        for (auto& fileLine : lines) {
            // Remover comentarios para comparación
            std::string cleanLine = fileLine;
            size_t commentPos = cleanLine.find(';');
            if (commentPos != std::string::npos) {
                cleanLine = cleanLine.substr(0, commentPos);
            }

            commentPos = cleanLine.find('#');
            if (commentPos != std::string::npos) {
                cleanLine = cleanLine.substr(0, commentPos);
            }

            // Si es la línea que buscamos
            if (cleanLine == originalLine) {
                // Reemplazar el último |x o |1 con |0
                size_t lastPipe = fileLine.rfind('|');
                if (lastPipe != std::string::npos) {
                    std::string beforePipe = fileLine.substr(0, lastPipe + 1);
                    fileLine = beforePipe + std::to_string(newCount);
                }
                break;
            }
        }

        // Escribir de vuelta al archivo
        std::ofstream outFile(iniPath);
        for (const auto& outputLine : lines) {
            outFile << outputLine << std::endl;
        }

        outFile.close();

    } catch (const std::exception&) {
        // Silent fail on error
    } catch (...) {
        // Silent fail on any unknown error
    }
}

// ✅ FUNCIÓN PRINCIPAL ULTRA-SEGURA que se ejecuta al cargar el plugin de SKSE
extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* skse) {
    try {
        SKSE::Init(skse);

        SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
            try {
                if (message->type == SKSE::MessagingInterface::kDataLoaded) {
                    std::string documentsPath;
                    std::string gamePath;

                    // ✅ Obtener rutas de manera ultra-segura
                    try {
                        documentsPath = GetDocumentsPath();
                        gamePath = GetGamePath();
                    } catch (const std::exception&) {
                        RE::ConsoleLog::GetSingleton()->Print("OBody Assistant: Error getting paths - using defaults");
                        documentsPath = "C:\\Users\\Default\\Documents";
                        gamePath = "";
                    } catch (...) {
                        RE::ConsoleLog::GetSingleton()->Print(
                            "OBody Assistant: Unknown error getting paths - using defaults");
                        documentsPath = "C:\\Users\\Default\\Documents";
                        gamePath = "";
                    }

                    if (gamePath.empty() || documentsPath.empty()) {
                        RE::ConsoleLog::GetSingleton()->Print(
                            "OBody Assistant: Could not find Game or Documents path.");
                        return;
                    }

                    // --- 1. Configuración de rutas y logging ---
                    fs::path dataPath = fs::path(gamePath) / "Data";
                    fs::path sksePluginsPath = dataPath / "SKSE" / "Plugins";
                    CreateDirectoryIfNotExists(sksePluginsPath);

                    fs::path logFilePath = fs::path(documentsPath) / "My Games" / "Skyrim Special Edition" / "SKSE" /
                                           "OBody_NG_Preset_Distribution_Assistant-NG.log";
                    CreateDirectoryIfNotExists(logFilePath.parent_path());

                    std::ofstream logFile(logFilePath);

                    auto now = std::chrono::system_clock::now();
                    std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);
                    std::tm buf;
                    localtime_s(&buf, &in_time_t);

                    logFile << "====================================================" << std::endl;
                    logFile << "OBody NG Preset Distribution Assistant NG" << std::endl;
                    logFile << "Log created on: " << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << std::endl;
                    logFile << "====================================================" << std::endl << std::endl;

                    // --- 2. Inicializar estructuras de datos ---
                    const std::set<std::string> validKeys = {
                        "npcFormID",       "npc",           "factionFemale", "factionMale",
                        "npcPluginFemale", "npcPluginMale", "raceFemale",    "raceMale"};

                    // Usar std::map en lugar de unordered_map para preservar orden alfabético
                    std::map<std::string, OrderedPluginData> processedData;
                    for (const auto& key : validKeys) {
                        processedData[key] = OrderedPluginData();
                    }

                    // --- 3. Leer el JSON existente COMPLETO ---
                    fs::path jsonOutputPath = sksePluginsPath / "OBody_presetDistributionConfig.json";

                    auto readResult = ReadCompleteJson(jsonOutputPath, processedData, logFile);
                    bool readSuccess = readResult.first;
                    std::string originalJsonContent = readResult.second;

                    if (!readSuccess) {
                        logFile << "Process truncated due to JSON read error. No INI processing or updates performed."
                                << std::endl;
                        logFile << "====================================================" << std::endl;
                        logFile.close();
                        RE::ConsoleLog::GetSingleton()->Print("ERROR: JSON READ FAILED - CONTACT MODDER OR REINSTALL!");
                        return;
                    }

                    int totalRulesProcessed = 0;
                    int totalRulesApplied = 0;
                    int totalRulesSkipped = 0;
                    int totalPresetsRemoved = 0;
                    int totalPluginsRemoved = 0;
                    int totalFilesProcessed = 0;

                    logFile << "Scanning for OBodyNG_PDA_*.ini files..." << std::endl;
                    logFile << "----------------------------------------------------" << std::endl;

                    // --- 4. Procesar archivos .ini de manera segura ---
                    try {
                        for (const auto& entry : fs::directory_iterator(dataPath)) {
                            if (entry.is_regular_file()) {
                                std::string filename = entry.path().filename().string();

                                // Verificar si el archivo empieza con "OBodyNG_PDA_" y termina con ".ini"
                                if (filename.rfind("OBodyNG_PDA_", 0) == 0 && filename.size() > 4 &&
                                    filename.substr(filename.length() - 4) == ".ini") {
                                    logFile << std::endl << "Processing file: " << filename << std::endl;
                                    totalFilesProcessed++;

                                    std::ifstream iniFile(entry.path());
                                    if (!iniFile.is_open()) {
                                        logFile << "  ERROR: Could not open file!" << std::endl;
                                        continue;
                                    }

                                    std::vector<std::pair<std::string, ParsedRule>> fileLinesAndRules;
                                    std::string line;
                                    int rulesInFile = 0;
                                    int rulesAppliedInFile = 0;
                                    int rulesSkippedInFile = 0;
                                    int presetsRemovedInFile = 0;
                                    int pluginsRemovedInFile = 0;

                                    while (std::getline(iniFile, line)) {
                                        std::string originalLine = line;

                                        // Eliminar comentarios
                                        size_t commentPos = line.find(';');
                                        if (commentPos != std::string::npos) {
                                            line = line.substr(0, commentPos);
                                        }

                                        commentPos = line.find('#');
                                        if (commentPos != std::string::npos) {
                                            line = line.substr(0, commentPos);
                                        }

                                        // Buscar el signo =
                                        size_t equalPos = line.find('=');
                                        if (equalPos != std::string::npos) {
                                            std::string key = Trim(line.substr(0, equalPos));
                                            std::string value = Trim(line.substr(equalPos + 1));

                                            // Verificar si es una clave válida y tiene contenido
                                            if (validKeys.count(key) && !value.empty()) {
                                                ParsedRule rule = ParseRuleLine(key, value);
                                                if (!rule.plugin.empty() && !rule.presets.empty()) {
                                                    rulesInFile++;
                                                    totalRulesProcessed++;

                                                    // Verificar si debe aplicarse según el conteo
                                                    bool shouldApply = false;
                                                    bool needsUpdate = false;
                                                    int newCount = rule.applyCount;

                                                    if (rule.applyCount == -1 || rule.applyCount == -2 ||
                                                        rule.applyCount == -3 || rule.applyCount == -4 ||
                                                        rule.applyCount == -5 || rule.applyCount > 0) {
                                                        shouldApply = true;
                                                        if (rule.applyCount > 0) {
                                                            needsUpdate = true;
                                                            newCount = rule.applyCount - 1;
                                                        } else if (rule.applyCount == -2 || rule.applyCount == -3) {
                                                            needsUpdate = true;
                                                            newCount = 0;  // Actualizar a 0 después de remoción
                                                        }
                                                        // Para -4 y -5 (x- y x*), no actualizar INI (needsUpdate =
                                                        // false)
                                                    } else {
                                                        // applyCount es 0 - no aplicar
                                                        shouldApply = false;
                                                        rulesSkippedInFile++;
                                                        totalRulesSkipped++;
                                                        logFile << "  Skipped (count=0): " << key
                                                                << " -> Plugin: " << rule.plugin << std::endl;
                                                    }

                                                    if (shouldApply) {
                                                        auto& data = processedData[key];

                                                        if (rule.applyCount == -1) {
                                                            // Agregar presets (ilimitado)
                                                            int presetsAdded = 0;
                                                            for (const auto& preset : rule.presets) {
                                                                size_t beforeCount = data.getTotalPresetCount();
                                                                data.addPreset(rule.plugin, preset);
                                                                if (data.getTotalPresetCount() > beforeCount) {
                                                                    presetsAdded++;
                                                                }
                                                            }

                                                            if (presetsAdded > 0) {
                                                                rulesAppliedInFile++;
                                                                totalRulesApplied++;
                                                                logFile << "  Applied: " << key
                                                                        << " -> Plugin: " << rule.plugin << " -> Added "
                                                                        << presetsAdded << " new presets";
                                                                if (!rule.extra.empty()) {
                                                                    logFile << " (mode: " << rule.extra << ")";
                                                                }
                                                                logFile << std::endl;
                                                            } else {
                                                                logFile
                                                                    << "  No new presets added (all already exist): "
                                                                    << key << " -> Plugin: " << rule.plugin
                                                                    << std::endl;
                                                            }

                                                        } else if (rule.applyCount == -4 || rule.applyCount == -2) {
                                                            // Remover todos los presets listados (strip ! si presente
                                                            // en rule)
                                                            int presetsRemoved = 0;
                                                            for (const auto& preset : rule.presets) {
                                                                std::string targetPreset = preset;
                                                                if (!targetPreset.empty() && targetPreset[0] == '!') {
                                                                    targetPreset = targetPreset.substr(
                                                                        1);  // Strip ! from rule preset
                                                                }

                                                                size_t beforeCount = data.getTotalPresetCount();
                                                                data.removePreset(rule.plugin, targetPreset);
                                                                if (data.getTotalPresetCount() < beforeCount) {
                                                                    presetsRemoved++;
                                                                }
                                                            }

                                                            if (presetsRemoved > 0) {
                                                                rulesAppliedInFile++;
                                                                totalRulesApplied++;
                                                                totalPresetsRemoved += presetsRemoved;
                                                                presetsRemovedInFile += presetsRemoved;
                                                                std::string mode = (rule.applyCount == -4) ? "x-" : "-";
                                                                logFile << "  Applied: " << key
                                                                        << " -> Plugin: " << rule.plugin
                                                                        << " -> Removed " << presetsRemoved
                                                                        << " presets (mode: " << mode << ")"
                                                                        << std::endl;
                                                            } else {
                                                                std::string mode = (rule.applyCount == -4) ? "x-" : "-";
                                                                logFile << "  No changes (already removed): " << key
                                                                        << " -> Plugin: " << rule.plugin
                                                                        << " (mode: " << mode << ")" << std::endl;
                                                            }

                                                        } else if (rule.applyCount == -5 || rule.applyCount == -3) {
                                                            // Remover plugin completo
                                                            size_t beforePlugins = data.getPluginCount();
                                                            data.removePlugin(rule.plugin);
                                                            size_t afterPlugins = data.getPluginCount();

                                                            if (afterPlugins < beforePlugins) {
                                                                rulesAppliedInFile++;
                                                                totalRulesApplied++;
                                                                totalPluginsRemoved++;
                                                                pluginsRemovedInFile++;
                                                                std::string mode = (rule.applyCount == -5) ? "x*" : "*";
                                                                logFile << "  Applied: " << key
                                                                        << " -> Removed entire plugin: " << rule.plugin
                                                                        << " (mode: " << mode << ")" << std::endl;
                                                            } else {
                                                                std::string mode = (rule.applyCount == -5) ? "x*" : "*";
                                                                logFile << "  No changes (already removed): " << key
                                                                        << " -> Plugin: " << rule.plugin
                                                                        << " (mode: " << mode << ")" << std::endl;
                                                            }

                                                        } else if (rule.applyCount > 0) {
                                                            // Agregar presets (limitado)
                                                            int presetsAdded = 0;
                                                            for (const auto& preset : rule.presets) {
                                                                size_t beforeCount = data.getTotalPresetCount();
                                                                data.addPreset(rule.plugin, preset);
                                                                if (data.getTotalPresetCount() > beforeCount) {
                                                                    presetsAdded++;
                                                                }
                                                            }

                                                            if (presetsAdded > 0) {
                                                                rulesAppliedInFile++;
                                                                totalRulesApplied++;
                                                                logFile << "  Applied: " << key
                                                                        << " -> Plugin: " << rule.plugin << " -> Added "
                                                                        << presetsAdded << " new presets";
                                                                if (!rule.extra.empty()) {
                                                                    logFile << " (mode: " << rule.extra << ")";
                                                                }
                                                                logFile << std::endl;
                                                            } else {
                                                                logFile
                                                                    << "  No new presets added (all already exist): "
                                                                    << key << " -> Plugin: " << rule.plugin
                                                                    << std::endl;
                                                            }
                                                        }

                                                        // Actualizar el INI si es necesario (cambiar de |1 a |0, o
                                                        // |-/|* a |0)
                                                        if (needsUpdate) {
                                                            fileLinesAndRules.push_back({line, rule});
                                                            fileLinesAndRules.back().second.applyCount = newCount;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    iniFile.close();

                                    // Actualizar el archivo INI si hay reglas con conteo que necesitan actualización
                                    if (!fileLinesAndRules.empty()) {
                                        for (const auto& [origLine, updatedRule] : fileLinesAndRules) {
                                            UpdateIniRuleCount(entry.path(), origLine, updatedRule.applyCount);
                                        }

                                        logFile << "  Updated " << fileLinesAndRules.size()
                                                << " rule counts in INI file" << std::endl;
                                    }

                                    logFile << "  Rules in file: " << rulesInFile
                                            << " | Applied: " << rulesAppliedInFile
                                            << " | Skipped: " << rulesSkippedInFile
                                            << " | Presets removed: " << presetsRemovedInFile
                                            << " | Plugins removed: " << pluginsRemovedInFile << std::endl;
                                }
                            }
                        }
                    } catch (const std::exception& e) {
                        logFile << "ERROR during INI processing: " << e.what() << std::endl;
                        RE::ConsoleLog::GetSingleton()->Print("OBody Assistant: ERROR during INI processing");
                    } catch (...) {
                        logFile << "ERROR during INI processing: Unknown exception" << std::endl;
                        RE::ConsoleLog::GetSingleton()->Print("OBody Assistant: ERROR during INI processing - unknown");
                    }

                    logFile << std::endl << "====================================================" << std::endl;
                    logFile << "SUMMARY:" << std::endl;
                    logFile << "Total .ini files processed: " << totalFilesProcessed << std::endl;
                    logFile << "Total rules processed: " << totalRulesProcessed << std::endl;
                    logFile << "Total rules applied: " << totalRulesApplied << std::endl;
                    logFile << "Total rules skipped (count=0): " << totalRulesSkipped << std::endl;
                    logFile << "Total presets removed (-): " << totalPresetsRemoved << std::endl;
                    logFile << "Total plugins removed (*): " << totalPluginsRemoved << std::endl;
                    logFile << std::endl << "Final data in JSON:" << std::endl;

                    for (const auto& [key, data] : processedData) {
                        size_t count = data.getTotalPresetCount();
                        if (count > 0) {
                            logFile << "  " << key << ": " << data.getPluginCount() << " plugins, " << count
                                    << " total presets" << std::endl;
                        }
                    }

                    logFile << "====================================================" << std::endl << std::endl;

                    // --- 5. Actualizar selectivamente el JSON preservando el resto ---
                    logFile << "Updating JSON at: " << jsonOutputPath.string() << std::endl;

                    std::string updatedJson = UpdateJsonSelectively(originalJsonContent, processedData);

                    std::ofstream jsonFile(jsonOutputPath);
                    if (!jsonFile.is_open()) {
                        logFile << "ERROR: Could not create/open JSON file for writing!" << std::endl;
                        RE::ConsoleLog::GetSingleton()->Print("OBody Assistant: ERROR - Could not write JSON file.");
                        logFile.close();
                        return;
                    }

                    // Escribir el JSON actualizado (preservando todo lo demás)
                    jsonFile << updatedJson;
                    jsonFile.close();

                    if (jsonFile.fail()) {
                        logFile << "ERROR: Failed to write complete JSON content!" << std::endl;
                        RE::ConsoleLog::GetSingleton()->Print("OBody Assistant: ERROR writing JSON.");
                    } else {
                        logFile << "SUCCESS: JSON file updated successfully (preserving existing data and order)."
                                << std::endl;

                        // Mensaje de éxito en la consola del juego con Full SUMMARY
                        std::string consoleMsg =
                            "OBody Assistant SUMMARY: Total .ini files processed: " +
                            std::to_string(totalFilesProcessed) +
                            ", Total rules processed: " + std::to_string(totalRulesProcessed) +
                            ", Total rules applied: " + std::to_string(totalRulesApplied) +
                            ", Total rules skipped (count=0): " + std::to_string(totalRulesSkipped) +
                            ", Total presets removed (-): " + std::to_string(totalPresetsRemoved) +
                            ", Total plugins removed (*): " + std::to_string(totalPluginsRemoved) +
                            ". JSON updated. Final data: ";

                        bool firstData = true;
                        for (const auto& [key, data] : processedData) {
                            size_t count = data.getTotalPresetCount();
                            if (count > 0) {
                                if (!firstData) consoleMsg += ", ";
                                firstData = false;
                                consoleMsg += key + ": " + std::to_string(data.getPluginCount()) + " plugins, " +
                                              std::to_string(count) + " total presets";
                            }
                        }

                        RE::ConsoleLog::GetSingleton()->Print(consoleMsg.c_str());

                        std::string logMsg =
                            "All the process and elements analysis can be read in the "
                            "OBody_NG_Preset_Distribution_Assistant-NG.log inside SKSE.";
                        RE::ConsoleLog::GetSingleton()->Print(logMsg.c_str());
                    }

                    logFile << std::endl << "Process completed successfully." << std::endl;
                    logFile.close();
                }

            } catch (const std::exception& e) {
                RE::ConsoleLog::GetSingleton()->Print("OBody Assistant: Fatal error occurred");
            } catch (...) {
                RE::ConsoleLog::GetSingleton()->Print("OBody Assistant: Unknown fatal error");
            }
        });

        return true;

    } catch (const std::exception& e) {
        return false;
    } catch (...) {
        return false;
    }
}
