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
        std::cout << "[DEBUG] Created upload directory: " << dir << std::endl;
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
    // Extract file extension
    size_t dotPos = originalFilename.find_last_of(".");
    std::string extension = "";
    std::string baseName = originalFilename;
    
    if (dotPos != std::string::npos) {
        extension = originalFilename.substr(dotPos);
        baseName = originalFilename.substr(0, dotPos);
    }
    
    // Generate timestamp-based unique name
    time_t now = time(0);
    std::ostringstream oss;
    oss << baseName << "_" << now << extension;
    
    std::string uniqueName = oss.str();
    
    // Check if file already exists and modify if needed
    std::string fullPath = uploadDirectory + "/" + uniqueName;
    struct stat buffer;
    int counter = 1;
    
    while (stat(fullPath.c_str(), &buffer) == 0) {
        std::ostringstream ossCounter;
        ossCounter << baseName << "_" << now << "_" << counter << extension;
        uniqueName = ossCounter.str();
        fullPath = uploadDirectory + "/" + uniqueName;
        counter++;
    }
    
    std::cout << "[DEBUG] Generated unique filename: " << uniqueName << std::endl;
    return uniqueName;
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
    std::cout << "[DEBUG] FileHandler::saveUploadedBinaryFile: " << originalFilename 
              << " (" << binaryData.size() << " bytes)" << std::endl;
    
    if (binaryData.empty()) {
        std::cout << "[ERROR] No binary data to save" << std::endl;
        return "";
    }
    
    // Generate unique filename
    std::string savedFilename = generateUniqueFilename(originalFilename);
    std::string fullPath = uploadDirectory + "/" + savedFilename;
    
    std::cout << "[DEBUG] Saving binary file to: " << fullPath << std::endl;
    
    // Open file in binary mode
    std::ofstream file(fullPath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        std::cout << "[ERROR] Failed to open file for writing: " << fullPath << std::endl;
        return "";
    }
    
    // Write binary data
    file.write(&binaryData[0], binaryData.size());
    
    if (file.fail()) {
        std::cout << "[ERROR] Failed to write binary data to file" << std::endl;
        file.close();
        return "";
    }
    
    file.close();
    
    // Verify file was written correctly
    std::ifstream verifyFile(fullPath.c_str(), std::ios::binary | std::ios::ate);
    if (verifyFile.is_open()) {
        std::streamsize fileSize = verifyFile.tellg();
        verifyFile.close();
        
        std::cout << "[DEBUG] File saved successfully. Size on disk: " << fileSize << " bytes" << std::endl;
        
        if (static_cast<size_t>(fileSize) != binaryData.size()) {
            std::cout << "[ERROR] File size mismatch! Expected: " << binaryData.size() 
                      << ", Got: " << fileSize << std::endl;
            return "";
        }
    } else {
        std::cout << "[ERROR] Could not verify saved file" << std::endl;
        return "";
    }
    
    std::cout << "[SUCCESS] Binary file saved: " << savedFilename << std::endl;
    return savedFilename;
}

// Text file saving (existing method, but improved)
std::string FileHandler::saveUploadedFile(const std::string& originalFilename, 
                                         const std::string& data) {
    std::cout << "[DEBUG] FileHandler::saveUploadedFile: " << originalFilename 
              << " (" << data.size() << " bytes)" << std::endl;
    
    if (data.empty()) {
        std::cout << "[ERROR] No data to save" << std::endl;
        return "";
    }
    
    // Generate unique filename
    std::string savedFilename = generateUniqueFilename(originalFilename);
    std::string fullPath = uploadDirectory + "/" + savedFilename;
    
    std::cout << "[DEBUG] Saving text file to: " << fullPath << std::endl;
    
    // For text files, we still use binary mode to preserve exact data
    std::ofstream file(fullPath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        std::cout << "[ERROR] Failed to open file for writing: " << fullPath << std::endl;
        return "";
    }
    
    // Write data
    file.write(data.c_str(), data.size());
    
    if (file.fail()) {
        std::cout << "[ERROR] Failed to write data to file" << std::endl;
        file.close();
        return "";
    }
    
    file.close();
    
    std::cout << "[SUCCESS] Text file saved: " << savedFilename << std::endl;
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

// std::string FileHandler::resolveFilePath(const std::string& uri, const LocationConfig* location) {
//     // Default root
//     std::string root = "www";
//     std::string locationPath = "/";
//     std::string indexFile = "index.html";

//     if (location) {
//         if (!location->getRoot().empty())
//             root = location->getRoot();
//         if (!location->getPath().empty())
//             locationPath = location->getPath();
//         if (!location->getIndex().empty())
//             indexFile = location->getIndex();
//     }

//     // Remove the location prefix from the URI
//     std::string relativeUri = uri;
//     if (uri.find(locationPath) == 0)
//         relativeUri = uri.substr(locationPath.length());

//     // Avoid accidental double slashes
//     if (!root.empty() && root[root.length() - 1] == '/')
//         root.erase(root.length() - 1);

//     std::string fullPath = root + "/" + relativeUri;

//     // Handle directory requests (append index)
//     struct stat path_stat;
//     if (stat(fullPath.c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
//         if (fullPath[fullPath.length() - 1] != '/')
//             fullPath += "/";
//         fullPath += indexFile;
//     }

//     return fullPath;
// }

// Root directive behavior in this implementation: behaves like NGINX alias
// This means it REPLACES the location prefix with the root path
// Example: location /images/ { root /var/www/assets/pictures/; }
//          URI: /images/logo.png -> /var/www/assets/pictures/logo.png (not /var/www/assets/pictures/images/logo.png)
// This is what the subject requirements ask for: "root should behave like alias"
std::string FileHandler::resolveFilePath(const std::string &uri, const LocationConfig *location)
{
    std::string root = "www";
    std::string locationPath = "/";
    std::string indexFile = "index.html";

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
    std::cout << "{{{{{cleanLocationPath}}}}}" << cleanLocationPath << std::endl;

    // Remove the location prefix from URI (alias behavior)
    std::string relativeUri = uri;
    if (uri.find(cleanLocationPath) == 0)
    {
        relativeUri = uri.substr(cleanLocationPath.size());
    }
	std::cout << "{{{{{relativeUri}}}}}" << relativeUri << std::endl;
    
    // Remove leading slash from relative URI
    if (!relativeUri.empty() && relativeUri[0] == '/')
        relativeUri.erase(0, 1);

    // Build full path: root + relative URI
    std::string fullPath = root;
    if (!relativeUri.empty())
    {
        fullPath += "/" + relativeUri;
    }
	std::cout << "{{{{{fullPath}}}}}" << fullPath << std::endl;

    std::cout << "[DEBUG] resolveFilePath: URI=" << uri
              << " | Root=" << root
              << " | Location=" << locationPath
              << " | RelativeURI=" << relativeUri
              << " | Resolved=" << fullPath << std::endl;

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

    std::cout << "[DEBUG] Final resolved path: " << fullPath << std::endl;

    return fullPath;
}
