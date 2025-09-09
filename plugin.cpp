#include <RE/Skyrim.h>
#include <SKSE/API.h>
#include <SKSE/SKSE.h>
#include <shlobj.h>
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <map>
#include <regex>
#include <cctype>
#include <utility>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

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
    while (pos < len) {
        // Skip whitespace
        while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' || str[pos] == '\f' || str[pos] == '\v')) ++pos;
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
        ++pos; // skip closing "
        // Skip whitespace
        while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' || str[pos] == '\f' || str[pos] == '\v')) ++pos;
        // Expect colon
        if (pos >= len || str[pos] != ':') {
            ++pos;
            continue;
        }
        ++pos; // skip :
        // Skip whitespace
        while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' || str[pos] == '\f' || str[pos] == '\v')) ++pos;
        // Expect opening bracket
        if (pos >= len || str[pos] != '[') {
            ++pos;
            continue;
        }
        ++pos; // skip [
        std::vector<std::string> presets;
        while (pos < len) {
            // Skip whitespace
            while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' || str[pos] == '\f' || str[pos] == '\v')) ++pos;
            if (pos >= len) break;
            if (str[pos] == ']') {
                ++pos; // skip ]
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
            ++pos; // skip closing "
            // Skip whitespace and expect comma or ]
            while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' || str[pos] == '\f' || str[pos] == '\v')) ++pos;
            if (pos < len && str[pos] == ',') {
                ++pos; // skip ,
                // Skip whitespace after comma
                while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' || str[pos] == '\f' || str[pos] == '\v')) ++pos;
            }
        }
        // Skip to next entry (whitespace and comma)
        while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r' || str[pos] == '\f' || str[pos] == '\v')) ++pos;
        if (pos < len && str[pos] == ',') ++pos;
        result.push_back({plugin, presets});
    }
    return result;
}

// Función para parsear secciones top-level del JSON preservando orden de aparición
std::vector<std::pair<std::string, std::string>> ParseTopLevelSections(const std::string& json) {
    std::vector<std::pair<std::string, std::string>> sections;
    const char* str = json.c_str();
    size_t len = strlen(str);
    size_t pos = 0;
    while (pos < len) {
        // Skip whitespace
        while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r')) ++pos;
        if (pos >= len || str[pos] == '}') break;
        if (str[pos] != '"') { ++pos; continue; }
        size_t keyStart = pos + 1;
        ++pos;
        while (pos < len && str[pos] != '"') ++pos;
        if (pos >= len) break;
        std::string key = extractSubstring(str, keyStart, pos);
        ++pos; // skip "
        // Skip whitespace and :
        while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r')) ++pos;
        if (pos >= len || str[pos] != ':') { ++pos; continue; }
        ++pos; // skip :
        // Skip whitespace
        while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r')) ++pos;
        // Find end of value (handle {} or [])
        size_t valueStart = pos;
        if (str[pos] == '{' || str[pos] == '[') {
            int braceCount = 1;
            ++pos;
            bool inString = false;
            bool escape = false;
            while (pos < len && braceCount > 0) {
                if (str[pos] == '"' && !escape) inString = !inString;
                else if (!inString) {
                    if (str[pos] == '{' || str[pos] == '[') ++braceCount;
                    else if (str[pos] == '}' || str[pos] == ']') --braceCount;
                }
                escape = (str[pos] == '\\' && !escape);
                ++pos;
            }
        } else {
            // Simple value, find , or }
            while (pos < len && str[pos] != ',' && str[pos] != '}') ++pos;
        }
        std::string value = extractSubstring(str, valueStart, pos);
        sections.emplace_back(key, value);
        // Skip to next , or }
        while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r')) ++pos;
        if (pos < len && str[pos] == ',') ++pos;
    }
    return sections;
}

// Función para obtener la ruta de la carpeta "Documentos" del usuario
std::string GetDocumentsPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path))) {
        std::wstring ws(path);
        std::string str(ws.begin(), ws.end());
        return str;
    }
    return "";
}

// Función para obtener la ruta de instalación de Skyrim desde el registro de Windows
std::string GetGamePath() {
    std::vector<std::string> registryKeys = {
        "SOFTWARE\\WOW6432Node\\Bethesda Softworks\\Skyrim Special Edition",
        "SOFTWARE\\WOW6432Node\\GOG.com\\Games\\1457087920",
        "SOFTWARE\\WOW6432Node\\Valve\\Steam\\Apps\\489830"
    };
    HKEY hKey;
    char path[MAX_PATH];
    DWORD pathSize = sizeof(path);

    for (const auto& key : registryKeys) {
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExA(hKey, "Installed Path", NULL, NULL, (LPBYTE)path, &pathSize) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return std::string(path);
            }
            RegCloseKey(hKey);
        }
    }
    return "";
}

// Función para crear un directorio si no existe
void CreateDirectoryIfNotExists(const fs::path& path) {
    if (!fs::exists(path)) {
        fs::create_directories(path);
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
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
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

// Función para pretty-print el JSON con indentación consistente
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
                currentIndent += 2;
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
                rule.applyCount = -1;  // Ilimitado
            } else {
                try {
                    rule.applyCount = std::stoi(rule.extra);
                } catch (...) {
                    rule.applyCount = -1;  // Si no es un número, tratar como ilimitado
                }
            }
        } else {
            rule.applyCount = -1;  // Por defecto, ilimitado
        }
    }

    return rule;
}

// Función MEJORADA para leer TODO el JSON existente preservando orden
std::string ReadCompleteJson(const fs::path& jsonPath,
                             std::map<std::string, OrderedPluginData>& processedData,
                             std::ofstream& logFile) {
    if (!fs::exists(jsonPath)) {
        logFile << "No existing JSON found at: " << jsonPath.string() << std::endl;
        // Retornar JSON con estructura mínima por defecto
        return R"({
  "npcFormID": {},
  "npc": {},
  "factionFemale": {},
  "factionMale": {},
  "npcPluginFemale": {},
  "npcPluginMale": {},
  "raceFemale": {},
  "raceMale": {},
  "blacklistedNpcs": [],
  "blacklistedNpcsFormID": {},
  "blacklistedNpcsPluginFemale": [],
  "blacklistedNpcsPluginMale": [],
  "blacklistedRacesFemale": [],
  "blacklistedRacesMale": [],
  "blacklistedOutfitsFromORefitFormID": {},
  "blacklistedOutfitsFromORefit": [],
  "blacklistedOutfitsFromORefitPlugin": [],
  "outfitsForceRefitFormID": {},
  "outfitsForceRefit": [],
  "blacklistedPresetsFromRandomDistribution": [],
  "blacklistedPresetsShowInOBodyMenu": true
})";
    }

    std::ifstream jsonFile(jsonPath);
    if (!jsonFile.is_open()) {
        logFile << "Could not open existing JSON file for reading." << std::endl;
        return "{}";
    }

    logFile << "Reading existing JSON from: " << jsonPath.string() << std::endl;

    std::string jsonContent((std::istreambuf_iterator<char>(jsonFile)), std::istreambuf_iterator<char>());
    jsonFile.close();

    // Parsear los datos que necesitamos modificar preservando el orden
    const std::vector<std::string> validKeys = {
        "npcFormID", "npc", "factionFemale", "factionMale",
        "npcPluginFemale", "npcPluginMale", "raceFemale", "raceMale"
    };

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
                    
                    while (pos < jsonContent.length() && braceCount > 0) {
                        if (jsonContent[pos] == '{') braceCount++;
                        else if (jsonContent[pos] == '}') {
                            braceCount--;
                            if (braceCount == 0) {
                                closeBrace = pos;
                                break;
                            }
                        }
                        pos++;
                    }
                    
                    if (closeBrace != std::string::npos) {
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
    }

    // Log what was loaded
    logFile << "Loaded existing data from JSON:" << std::endl;
    for (const auto& [key, data] : processedData) {
        size_t count = data.getTotalPresetCount();
        if (count > 0) {
            logFile << "  " << key << ": " << data.getPluginCount() 
                    << " plugins, " << count << " presets" << std::endl;
        }
    }
    logFile << std::endl;

    return jsonContent;  // Retornar el JSON completo original
}

// Función MEJORADA para actualizar selectivamente el JSON existente preservando orden
std::string UpdateJsonSelectively(const std::string& originalJson,
                                  const std::map<std::string, OrderedPluginData>& processedData) {
    auto topSections = ParseTopLevelSections(originalJson);
    std::stringstream result;
    result << "{\n";
    bool firstSection = true;
    const std::vector<std::string> validKeys = {
        "npcFormID", "npc", "factionFemale", "factionMale",
        "npcPluginFemale", "npcPluginMale", "raceFemale", "raceMale"
    };
    for (const auto& [key, originalValue] : topSections) {
        if (!firstSection) {
            result << ",\n";
        }
        firstSection = false;
        result << "  \"" << EscapeJson(key) << "\": ";
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
                result << "    \"" << EscapeJson(plugin) << "\": [\n";
                bool firstPreset = true;
                for (const auto& preset : presets) {
                    if (!firstPreset) {
                        result << ",\n";
                    }
                    firstPreset = false;
                    result << "      \"" << EscapeJson(preset) << "\"";
                }
                result << "\n    ]";
            }
            result << "\n  }";
        } else {
            result << originalValue;
        }
    }
    result << "\n}";
    return result.str();
}

// Función para actualizar el archivo INI con el nuevo conteo
void UpdateIniRuleCount(const fs::path& iniPath, const std::string& originalLine, int newCount) {
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
}

// Función principal que se ejecuta al cargar el plugin de SKSE
extern "C" bool SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            std::string documentsPath = GetDocumentsPath();
            std::string gamePath = GetGamePath();

            if (gamePath.empty() || documentsPath.empty()) {
                RE::ConsoleLog::GetSingleton()->Print("OBody Assistant: Could not find Game or Documents path.");
                return;
            }

            // --- 1. Configuración de rutas y logging ---
            fs::path dataPath = fs::path(gamePath) / "Data";
            fs::path sksePluginsPath = dataPath / "SKSE" / "Plugins";
            CreateDirectoryIfNotExists(sksePluginsPath);

            fs::path logFilePath = fs::path(documentsPath) / "My Games" / "Skyrim Special Edition" / "SKSE" /
                                   "OBody_preset_Distribution_Config_Assistant-NG.log";
            CreateDirectoryIfNotExists(logFilePath.parent_path());
            std::ofstream logFile(logFilePath);

            auto now = std::chrono::system_clock::now();
            std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);
            std::tm buf;
            localtime_s(&buf, &in_time_t);
            logFile << "====================================================" << std::endl;
            logFile << "OBody Preset Distribution Config Assistant NG" << std::endl;
            logFile << "Log created on: " << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << std::endl;
            logFile << "====================================================" << std::endl << std::endl;

            // --- 2. Inicializar estructuras de datos ---
            const std::set<std::string> validKeys = {
                "npcFormID", "npc", "factionFemale", "factionMale",
                "npcPluginFemale", "npcPluginMale", "raceFemale", "raceMale"
            };

            // Usar std::map en lugar de unordered_map para preservar orden alfabético
            std::map<std::string, OrderedPluginData> processedData;
            for (const auto& key : validKeys) {
                processedData[key] = OrderedPluginData();
            }

            // --- 3. Leer el JSON existente COMPLETO ---
            fs::path jsonOutputPath = sksePluginsPath / "OBody_presetDistributionConfig.json";
            std::string originalJsonContent = ReadCompleteJson(jsonOutputPath, processedData, logFile);

            int totalRulesProcessed = 0;
            int totalRulesApplied = 0;
            int totalRulesSkipped = 0;
            int totalFilesProcessed = 0;

            logFile << "Scanning for OBody_PD_*.ini files in: " << dataPath.string() << std::endl;
            logFile << "----------------------------------------------------" << std::endl;

            // --- 4. Procesar archivos .ini ---
            for (const auto& entry : fs::directory_iterator(dataPath)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();

                    // Verificar si el archivo empieza con "OBody_PD_" y termina con ".ini"
                    if (filename.rfind("OBody_PD_", 0) == 0 && filename.size() > 4 &&
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

                                        if (rule.applyCount == -1) {
                                            // Ilimitado (x) - siempre aplicar
                                            shouldApply = true;
                                        } else if (rule.applyCount > 0) {
                                            // Tiene aplicaciones restantes
                                            shouldApply = true;
                                            needsUpdate = true;
                                            newCount = rule.applyCount - 1;
                                        } else {
                                            // applyCount es 0 - no aplicar
                                            shouldApply = false;
                                            rulesSkippedInFile++;
                                            totalRulesSkipped++;
                                            logFile << "  Skipped (count=0): " << key << " -> Plugin: " << rule.plugin
                                                    << std::endl;
                                        }

                                        if (shouldApply) {
                                            // Agregar los presets preservando orden
                                            int presetsAdded = 0;
                                            auto& data = processedData[key];

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

                                                logFile << "  Applied: " << key << " -> Plugin: " << rule.plugin
                                                        << " -> Added " << presetsAdded << " new presets";
                                                if (!rule.extra.empty()) {
                                                    logFile << " (mode: " << rule.extra << ")";
                                                }
                                                logFile << std::endl;
                                            } else {
                                                logFile << "  No new presets added (all already exist): " << key
                                                        << " -> Plugin: " << rule.plugin << std::endl;
                                            }

                                            // Actualizar el INI si es necesario (cambiar de |1 a |0)
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
                            logFile << "  Updated " << fileLinesAndRules.size() << " rule counts in INI file"
                                    << std::endl;
                        }

                        logFile << "  Rules in file: " << rulesInFile << " | Applied: " << rulesAppliedInFile
                                << " | Skipped: " << rulesSkippedInFile << std::endl;
                    }
                }
            }

            logFile << std::endl << "====================================================" << std::endl;
            logFile << "SUMMARY:" << std::endl;
            logFile << "Total .ini files processed: " << totalFilesProcessed << std::endl;
            logFile << "Total rules processed: " << totalRulesProcessed << std::endl;
            logFile << "Total rules applied: " << totalRulesApplied << std::endl;
            logFile << "Total rules skipped (count=0): " << totalRulesSkipped << std::endl;
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
                logFile << "SUCCESS: JSON file updated successfully (preserving existing data and order)." << std::endl;

                // Mensaje de éxito en la consola del juego
                std::string consoleMsg = "OBody Assistant: Processed " + std::to_string(totalFilesProcessed) +
                                         " files, applied " + std::to_string(totalRulesApplied) + " rules, skipped " +
                                         std::to_string(totalRulesSkipped) + ". JSON updated.";
                RE::ConsoleLog::GetSingleton()->Print(consoleMsg.c_str());
            }

            logFile << std::endl << "Process completed successfully." << std::endl;
            logFile.close();
        }
    });

    return true;
}
