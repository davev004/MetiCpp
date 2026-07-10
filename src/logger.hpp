#pragma once
#include <fstream>
#include <string>

namespace Logger {
    inline std::ofstream file;
    inline bool active = false;

    // Call this in main() or UCI::loop() if a debug flag is passed
    inline void init(const std::string& path) {
        file.open(path, std::ios::out | std::ios::app);
        if (file.is_open()) {
            active = true;
            file << "--- METIC++ LOG START ---" << std::endl;
        }
    }

    inline void write(const std::string& prefix, const std::string& msg) {
        if (active) {
            file << prefix << msg << std::endl;
        }
    }
}