#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace FileUtils {

    // Type alias for a map storing configuration key-value pairs
    using ConfigMap = std::unordered_map<std::string, std::string>;

    /**
     * Reads a configuration file and stores the key-value pairs in a ConfigMap.
     * The configuration file should have each key-value pair on a separate line in the format:
     * key=value. Lines beginning with '#' or ';' are treated as comments and ignored.
     * @param filename The path to the configuration file.
     * @return A ConfigMap containing all the key-value pairs from the file.
     */
    ConfigMap readConfigFile(const std::string& filename) {
        ConfigMap config;
        std::ifstream file(filename);   // Opens the file
        std::string line;

        while (std::getline(file, line)) {
            // Ignore comments and empty lines
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            std::istringstream lineStream(line);   // Process each line
            std::string key;
            if (std::getline(lineStream, key, '=')) {  // Extract the key until the '=' character
                std::string value;
                if (std::getline(lineStream, value)) { // Extract the value after '='
                    config[key] = value;   // Store the key-value pair in the config map
                }
            }
        }

        return config;   // Returns the populated map
    }

    /**
     * Retrieves the value associated with a specific key from a ConfigMap.
     * If the key is not found, the function returns a default value.
     * @param config The configuration map containing key-value pairs.
     * @param key The key whose value is to be retrieved.
     * @param defaultValue The default value to return if the key is not found.
     * @return The value associated with the key, or the default value if not found.
     */
    std::string getValue(const ConfigMap& config, const std::string& key, const std::string& defaultValue = "") {
        auto it = config.find(key);  // Searches for the key
        if (it != config.end()) {
            return it->second;       // Returns the value if key is found
        }
        return defaultValue;         // Returns the default value if key is not found
    }

    /**
     * Reads the entire contents of a file and returns it as a single string.
     * Each line in the file is concatenated with a newline character.
     * @param filename The path to the file.
     * @return A string containing the contents of the file, or an empty string if the file cannot be opened.
     */
    std::string readFile(const std::string& filename) {
        std::string result = "";
        std::string line = "";
        std::ifstream file(filename.c_str());

        if (file.is_open()) {
            while (std::getline(file, line)) {     // Read each line from the file
                result += line + '\n';             // Append the line and newline character
            }
            file.close();                          // Close the file
        }

        return result;   // Return the complete file content as a single string
    }
}
