#include "FileHandler.hpp"
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

std::string FileHandler::uploadDirectory = "./www/upload";

bool FileHandler::createDirectoryIfNotExists(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    
    // Try to create directory
    #ifdef _WIN32
        return _mkdir(path.c_str()) == 0;
    #else
        return mkdir(path.c_str(), 0755) == 0;
    #endif
}

void FileHandler::setUploadDirectory(const std::string& dir) {
    uploadDirectory = dir;
    
    // Create directory if it doesn't exist
    struct stat st;
    if (stat(dir.c_str(), &st) == -1) {
        mkdir(dir.c_str(), 0755);
    }
}

std::string FileHandler::getUploadDirectory() {
    return uploadDirectory;
}

std::string FileHandler::sanitizeFilename(const std::string& filename) {
    std::string sanitized = filename;
    
    // Remove or replace dangerous characters
    for (size_t i = 0; i < sanitized.length(); ++i) {
        char c = sanitized[i];
        if (c == '/' || c == '\\' || c == ':' || c == '*' || 
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            sanitized[i] = '_';
        }
    }
    
    // Remove leading dots and spaces
    while (!sanitized.empty() && (sanitized[0] == '.' || sanitized[0] == ' ')) {
        sanitized = sanitized.substr(1);
    }
    
    // If empty after sanitization, use default name
    if (sanitized.empty()) {
        sanitized = "unnamed_file";
    }
    
    return sanitized;
}

std::string FileHandler::generateUniqueFilename(const std::string& originalFilename) {
    std::string sanitized = sanitizeFilename(originalFilename);
    std::string fullPath = uploadDirectory + "/" + sanitized;
    struct stat buffer;
    
    // If file doesn't exist, use the original name
    if (stat(fullPath.c_str(), &buffer) != 0) {
        return sanitized;
    }
    
    // File exists, add counter
    size_t dotPos = sanitized.find_last_of(".");
    std::string extension = "";
    std::string baseName = sanitized;
    
    if (dotPos != std::string::npos) {
        extension = sanitized.substr(dotPos);
        baseName = sanitized.substr(0, dotPos);
    }
    
    int counter = 1;
    std::ostringstream oss;
    
    while (stat(fullPath.c_str(), &buffer) == 0) {
        oss.str("");
        oss << baseName << "_" << counter << extension;
        std::string uniqueName = oss.str();
        fullPath = uploadDirectory + "/" + uniqueName;
        counter++;
    }
    
    return oss.str();
}

bool FileHandler::fileExists(const std::string& filename) {
    std::string fullPath = uploadDirectory + "/" + filename;
    struct stat buffer;
    return (stat(fullPath.c_str(), &buffer) == 0);
}

bool FileHandler::readFile(const std::string& filepath, std::string& content) {
    std::ifstream file(filepath.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    
    file.close();
    return true;
}

bool FileHandler::readBinaryFile(const std::string& filepath, std::vector<char>& content) {
    std::ifstream file(filepath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read file content
    content.resize(size);
    file.read(&content[0], size);
    
    file.close();
    return true;
}

bool FileHandler::writeFile(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    file.close();
    
    return true;
}

bool FileHandler::writeBinaryFile(const std::string& filepath, const std::vector<char>& content) {
    std::ofstream file(filepath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(&content[0], content.size());
    file.close();
    
    return true;
}

size_t FileHandler::getFileSize(const std::string& filename) {
    std::string fullPath = uploadDirectory + "/" + filename;
    std::ifstream file(fullPath.c_str(), std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        size_t size = static_cast<size_t>(file.tellg());
        file.close();
        return size;
    }
    return 0;
}

// BINARY-SAFE: Save binary file from vector<char>
std::string FileHandler::saveUploadedBinaryFile(const std::string& originalFilename, 
                                               const std::vector<char>& binaryData) {
    
    if (binaryData.empty()) {
        return "";
    }
    
    // Generate unique filename
    std::string savedFilename = generateUniqueFilename(originalFilename);
    std::string fullPath = uploadDirectory + "/" + savedFilename;
    
    
    // Open file in binary mode
    std::ofstream file(fullPath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    // Write binary data
    file.write(&binaryData[0], binaryData.size());
    
    if (file.fail()) {
        file.close();
        return "";
    }
    
    file.close();
    
    // Verify file was written correctly
    std::ifstream verifyFile(fullPath.c_str(), std::ios::binary | std::ios::ate);
    if (verifyFile.is_open()) {
        std::streamsize fileSize = verifyFile.tellg();
        verifyFile.close();
        
        
        if (static_cast<size_t>(fileSize) != binaryData.size()) {
            return "";
        }
    } else {
        return "";
    }
    
    return savedFilename;
}

// Text file saving (existing method, but improved)
std::string FileHandler::saveUploadedFile(const std::string& originalFilename, 
                                         const std::string& data) {
    
    if (data.empty()) {
        return "";
    }
    
    // Generate unique filename
    std::string savedFilename = generateUniqueFilename(originalFilename);
    std::string fullPath = uploadDirectory + "/" + savedFilename;
    
    
    // For text files, we still use binary mode to preserve exact data
    std::ofstream file(fullPath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    // Write data
    file.write(data.c_str(), data.size());
    
    if (file.fail()) {
        file.close();
        return "";
    }
    
    file.close();
    
    return savedFilename;
}

std::string FileHandler::getFileName(const std::string& filepath) {
    size_t pos = filepath.find_last_of("/\\");
    if (pos != std::string::npos) {
        return filepath.substr(pos + 1);
    }
    return filepath;
}

std::string FileHandler::getDirectoryPath(const std::string& filepath) {
    size_t pos = filepath.find_last_of("/\\");
    if (pos != std::string::npos) {
        return filepath.substr(0, pos);
    }
    return "";
}

bool FileHandler::isValidPath(const std::string& path) {
    // Basic security check - prevent directory traversal
    return path.find("..") == std::string::npos &&
           path.find("//") == std::string::npos &&
           !path.empty() &&
           path[0] != '/';
}

// before:

// Root directive behavior in this implementation: behaves like NGINX alias
// This means it REPLACES the location prefix with the root path
// Example: location /images/ { root /var/www/assets/pictures/; }
//          URI: /images/logo.png -> /var/www/assets/pictures/logo.png (not /var/www/assets/pictures/images/logo.png)
// This is what the subject requirements ask for: "root should behave like alias"
std::string FileHandler::resolveFilePath(const std::string &uri, const LocationConfig *location)
{
    std::string root = "www";
    std::string locationPath = "/";
    std::string indexFile = "";

    if (location)
    {
        if (!location->getRoot().empty())
            root = location->getRoot();
        if (!location->getPath().empty())
            locationPath = location->getPath();
        if (!location->getIndex().empty())
            indexFile = location->getIndex();
        // autoindex = location->getAutoIndex(); // Not used in this function
    }

    // Clean root: remove trailing slash if present
    if (!root.empty() && root[root.size() - 1] == '/')
        root.erase(root.size() - 1);
    
    // Clean location path: remove trailing slash for matching
    std::string cleanLocationPath = locationPath;
    if (cleanLocationPath.size() > 1 && cleanLocationPath[cleanLocationPath.size() - 1] == '/')
    {
        cleanLocationPath.erase(cleanLocationPath.size() - 1);
    }

    // Remove the location prefix from URI (alias behavior)
    std::string relativeUri = uri;
    if (uri.find(cleanLocationPath) == 0)
    {
        relativeUri = uri.substr(cleanLocationPath.size());
    }
    
    // Remove leading slash from relative URI
    if (!relativeUri.empty() && relativeUri[0] == '/')
        relativeUri.erase(0, 1);

    // Build full path: root + relative URI
    std::string fullPath = root;
    if (!relativeUri.empty())
    {
        fullPath += "/" + relativeUri;
    }

    // Check if path is a directory
    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode))
    {
        // Ensure trailing slash for directories
        if (fullPath[fullPath.size() - 1] != '/')
            fullPath += '/';
        
        // DON'T append index here - let the handler decide based on autoindex setting
        // The GET handler will check for index file and autoindex setting
    }


    return fullPath;
}
