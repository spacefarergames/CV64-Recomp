/**
 * @file cv64_ini_parser.h
 * @brief Castlevania 64 PC Recomp - INI File Parser
 * 
 * Simple INI file parser for loading configuration settings.
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_INI_PARSER_H
#define CV64_INI_PARSER_H

#include "cv64_types.h"
#include <string>
#include <unordered_map>

#ifdef __cplusplus

/**
 * @brief Simple INI file parser class
 */
class CV64_IniParser {
public:
    CV64_IniParser();
    ~CV64_IniParser();

    /**
     * @brief Load an INI file
     * @param filepath Path to the INI file
     * @return true if successfully loaded, false otherwise
     */
    bool Load(const char* filepath);

    /**
     * @brief Save current settings to an INI file
     * @param filepath Path to save to (NULL = use loaded path)
     * @return true if successfully saved, false otherwise
     */
    bool Save(const char* filepath = nullptr);

    /**
     * @brief Check if a section exists
     */
    bool HasSection(const char* section) const;

    /**
     * @brief Check if a key exists in a section
     */
    bool HasKey(const char* section, const char* key) const;

    /**
     * @brief Get a string value
     * @param section Section name
     * @param key Key name
     * @param defaultValue Default value if not found
     * @return The value or default
     */
    std::string GetString(const char* section, const char* key, const char* defaultValue = "") const;

    /**
     * @brief Get an integer value
     */
    int GetInt(const char* section, const char* key, int defaultValue = 0) const;

    /**
     * @brief Get a float value
     */
    float GetFloat(const char* section, const char* key, float defaultValue = 0.0f) const;

    /**
     * @brief Get a boolean value (accepts true/false, yes/no, 1/0)
     */
    bool GetBool(const char* section, const char* key, bool defaultValue = false) const;

    /**
     * @brief Set a string value
     */
    void SetString(const char* section, const char* key, const char* value);

    /**
     * @brief Set an integer value
     */
    void SetInt(const char* section, const char* key, int value);

    /**
     * @brief Set a float value
     */
    void SetFloat(const char* section, const char* key, float value);

    /**
     * @brief Set a boolean value
     */
    void SetBool(const char* section, const char* key, bool value);

    /**
     * @brief Clear all loaded data
     */
    void Clear();

    /**
     * @brief Get the last error message
     */
    const char* GetLastError() const { return m_lastError.c_str(); }

private:
    std::string m_filepath;
    std::string m_lastError;
    
    // Section -> (Key -> Value) mapping
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_data;

    std::string Trim(const std::string& str) const;
    std::string ToLower(const std::string& str) const;
};

extern "C" {
#endif

/*===========================================================================
 * C API for INI Parser
 *===========================================================================*/

typedef void* CV64_IniHandle;

CV64_IniHandle CV64_Ini_Create(void);
void CV64_Ini_Destroy(CV64_IniHandle handle);
bool CV64_Ini_Load(CV64_IniHandle handle, const char* filepath);
bool CV64_Ini_Save(CV64_IniHandle handle, const char* filepath);
const char* CV64_Ini_GetString(CV64_IniHandle handle, const char* section, const char* key, const char* defaultValue);
int CV64_Ini_GetInt(CV64_IniHandle handle, const char* section, const char* key, int defaultValue);
float CV64_Ini_GetFloat(CV64_IniHandle handle, const char* section, const char* key, float defaultValue);
bool CV64_Ini_GetBool(CV64_IniHandle handle, const char* section, const char* key, bool defaultValue);

#ifdef __cplusplus
}
#endif

#endif /* CV64_INI_PARSER_H */
