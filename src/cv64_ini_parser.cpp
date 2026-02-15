/**
 * @file cv64_ini_parser.cpp
 * @brief Castlevania 64 PC Recomp - INI File Parser Implementation
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#define _CRT_SECURE_NO_WARNINGS

#include "../include/cv64_ini_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <Windows.h>

/*===========================================================================
 * CV64_IniParser Implementation
 *===========================================================================*/

CV64_IniParser::CV64_IniParser() {
}

CV64_IniParser::~CV64_IniParser() {
    Clear();
}

std::string CV64_IniParser::Trim(const std::string& str) const {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string CV64_IniParser::ToLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool CV64_IniParser::Load(const char* filepath) {
    if (!filepath) {
        m_lastError = "NULL filepath provided";
        return false;
    }

    std::ifstream file(filepath);
    if (!file.is_open()) {
        m_lastError = "Failed to open file: " + std::string(filepath);
        OutputDebugStringA("[CV64_INI] ");
        OutputDebugStringA(m_lastError.c_str());
        OutputDebugStringA("\n");
        return false;
    }

    m_filepath = filepath;
    Clear();

    std::string currentSection;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;
        line = Trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        // Check for section header [SectionName]
        if (line[0] == '[') {
            size_t endBracket = line.find(']');
            if (endBracket != std::string::npos) {
                currentSection = Trim(line.substr(1, endBracket - 1));
                // Create section if it doesn't exist
                if (m_data.find(currentSection) == m_data.end()) {
                    m_data[currentSection] = std::unordered_map<std::string, std::string>();
                }
            }
            continue;
        }

        // Parse key=value pair
        size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos) {
            std::string key = Trim(line.substr(0, equalsPos));
            std::string value = Trim(line.substr(equalsPos + 1));

            // Remove inline comments
            size_t commentPos = value.find(';');
            if (commentPos != std::string::npos) {
                value = Trim(value.substr(0, commentPos));
            }
            commentPos = value.find('#');
            if (commentPos != std::string::npos) {
                value = Trim(value.substr(0, commentPos));
            }

            // Store the value
            if (!currentSection.empty() && !key.empty()) {
                m_data[currentSection][key] = value;
            }
        }
    }

    file.close();

    char msg[512];
    sprintf_s(msg, sizeof(msg), "[CV64_INI] Loaded %s: %zu sections\n", filepath, m_data.size());
    OutputDebugStringA(msg);

    return true;
}

bool CV64_IniParser::Save(const char* filepath) {
    const char* savePath = filepath ? filepath : m_filepath.c_str();
    if (!savePath || strlen(savePath) == 0) {
        m_lastError = "No filepath specified for save";
        return false;
    }

    std::ofstream file(savePath);
    if (!file.is_open()) {
        m_lastError = "Failed to open file for writing: " + std::string(savePath);
        return false;
    }

    for (const auto& section : m_data) {
        file << "[" << section.first << "]\n";
        for (const auto& kv : section.second) {
            file << kv.first << "=" << kv.second << "\n";
        }
        file << "\n";
    }

    file.close();
    return true;
}

bool CV64_IniParser::HasSection(const char* section) const {
    return m_data.find(section) != m_data.end();
}

bool CV64_IniParser::HasKey(const char* section, const char* key) const {
    auto sectionIt = m_data.find(section);
    if (sectionIt == m_data.end()) return false;
    return sectionIt->second.find(key) != sectionIt->second.end();
}

std::string CV64_IniParser::GetString(const char* section, const char* key, const char* defaultValue) const {
    auto sectionIt = m_data.find(section);
    if (sectionIt == m_data.end()) {
        return defaultValue ? defaultValue : "";
    }
    
    auto keyIt = sectionIt->second.find(key);
    if (keyIt == sectionIt->second.end()) {
        return defaultValue ? defaultValue : "";
    }
    
    return keyIt->second;
}

int CV64_IniParser::GetInt(const char* section, const char* key, int defaultValue) const {
    std::string value = GetString(section, key, "");
    if (value.empty()) return defaultValue;
    
    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

float CV64_IniParser::GetFloat(const char* section, const char* key, float defaultValue) const {
    std::string value = GetString(section, key, "");
    if (value.empty()) return defaultValue;
    
    try {
        return std::stof(value);
    } catch (...) {
        return defaultValue;
    }
}

bool CV64_IniParser::GetBool(const char* section, const char* key, bool defaultValue) const {
    std::string value = ToLower(GetString(section, key, ""));
    if (value.empty()) return defaultValue;
    
    if (value == "true" || value == "yes" || value == "1" || value == "on") {
        return true;
    }
    if (value == "false" || value == "no" || value == "0" || value == "off") {
        return false;
    }
    
    return defaultValue;
}

void CV64_IniParser::SetString(const char* section, const char* key, const char* value) {
    m_data[section][key] = value ? value : "";
}

void CV64_IniParser::SetInt(const char* section, const char* key, int value) {
    m_data[section][key] = std::to_string(value);
}

void CV64_IniParser::SetFloat(const char* section, const char* key, float value) {
    m_data[section][key] = std::to_string(value);
}

void CV64_IniParser::SetBool(const char* section, const char* key, bool value) {
    m_data[section][key] = value ? "true" : "false";
}

void CV64_IniParser::Clear() {
    m_data.clear();
}

/*===========================================================================
 * C API Implementation
 *===========================================================================*/

CV64_IniHandle CV64_Ini_Create(void) {
    return new CV64_IniParser();
}

void CV64_Ini_Destroy(CV64_IniHandle handle) {
    if (handle) {
        delete static_cast<CV64_IniParser*>(handle);
    }
}

bool CV64_Ini_Load(CV64_IniHandle handle, const char* filepath) {
    if (!handle) return false;
    return static_cast<CV64_IniParser*>(handle)->Load(filepath);
}

bool CV64_Ini_Save(CV64_IniHandle handle, const char* filepath) {
    if (!handle) return false;
    return static_cast<CV64_IniParser*>(handle)->Save(filepath);
}

const char* CV64_Ini_GetString(CV64_IniHandle handle, const char* section, const char* key, const char* defaultValue) {
    if (!handle) return defaultValue;
    // Note: This returns a temporary - in real usage you'd want to cache this
    static thread_local std::string result;
    result = static_cast<CV64_IniParser*>(handle)->GetString(section, key, defaultValue);
    return result.c_str();
}

int CV64_Ini_GetInt(CV64_IniHandle handle, const char* section, const char* key, int defaultValue) {
    if (!handle) return defaultValue;
    return static_cast<CV64_IniParser*>(handle)->GetInt(section, key, defaultValue);
}

float CV64_Ini_GetFloat(CV64_IniHandle handle, const char* section, const char* key, float defaultValue) {
    if (!handle) return defaultValue;
    return static_cast<CV64_IniParser*>(handle)->GetFloat(section, key, defaultValue);
}

bool CV64_Ini_GetBool(CV64_IniHandle handle, const char* section, const char* key, bool defaultValue) {
    if (!handle) return defaultValue;
    return static_cast<CV64_IniParser*>(handle)->GetBool(section, key, defaultValue);
}
