#ifndef FILEHANDLER_HPP
#define FILEHANDLER_HPP

#include <string>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <sys/stat.h>
#include "../../../config/LocationConfig.hpp"

class FileHandler {
private:
    static std::string uploadDirectory;
    
public:
    // Directory management
    static void setUploadDirectory(const std::string& dir);
    static std::string getUploadDirectory();
    static bool createDirectoryIfNotExists(const std::string& path);
    
    // File saving - BINARY-SAFE versions
    static std::string saveUploadedBinaryFile(const std::string& originalFilename, 
                                             const std::vector<char>& binaryData);
    static std::string saveUploadedFile(const std::string& originalFilename, 
                                       const std::string& data);
    
    // File operations
    static bool fileExists(const std::string& filename);
    static bool readFile(const std::string& filepath, std::string& content);
    static bool readBinaryFile(const std::string& filepath, std::vector<char>& content);
    static bool writeFile(const std::string& filepath, const std::string& content);
    static bool writeBinaryFile(const std::string& filepath, const std::vector<char>& content);
    
    // Utility functions
    static std::string generateUniqueFilename(const std::string& originalFilename);
    static std::string sanitizeFilename(const std::string& filename);
    static size_t getFileSize(const std::string& filename);
    static std::string getFileName(const std::string& filepath);
    static std::string getDirectoryPath(const std::string& filepath);
    static bool isValidPath(const std::string& path);

    static std::string resolveFilePath(const std::string& uri, const LocationConfig* location);
};

#endif