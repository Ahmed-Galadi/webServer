#include "FileHandler.hpp"
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

std::string FileHandler::uploadDirectory = "./www/upload";



void FileHandler::setUploadDirectory(const std::string& dir) {
    uploadDirectory = dir;
    
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
    
    for (size_t i = 0; i < sanitized.length(); ++i) {
        char c = sanitized[i];
        if (c == '/' || c == '\\' || c == ':' || c == '*' || 
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            sanitized[i] = '_';
        }
    }
    
    while (!sanitized.empty() && (sanitized[0] == '.' || sanitized[0] == ' ')) {
        sanitized = sanitized.substr(1);
    }
    
    if (sanitized.empty()) {
        sanitized = "unnamed_file";
    }
    
    return sanitized;
}

std::string FileHandler::generateUniqueFilename(const std::string& originalFilename) {
    std::string sanitized = sanitizeFilename(originalFilename);
    std::string fullPath = uploadDirectory + "/" + sanitized;
    struct stat buffer;
    
    if (stat(fullPath.c_str(), &buffer) != 0) {
        return sanitized;
    }
    
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
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    content.resize(size);
    file.read(&content[0], size);
    
    file.close();
    return true;
}




std::string FileHandler::saveUploadedBinaryFile(const std::string& originalFilename, 
                                               const std::vector<char>& binaryData) {
    
    if (binaryData.empty()) {
        return "";
    }
    
    std::string savedFilename = generateUniqueFilename(originalFilename);
    std::string fullPath = uploadDirectory + "/" + savedFilename;
    
    
    std::ofstream file(fullPath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    file.write(&binaryData[0], binaryData.size());
    
    if (file.fail()) {
        file.close();
        return "";
    }
    
    file.close();
    return savedFilename;
}

std::string FileHandler::saveUploadedFile(const std::string& originalFilename, 
                                         const std::string& data) {
    
    if (data.empty()) {
        return "";
    }
    
    std::string savedFilename = generateUniqueFilename(originalFilename);
    std::string fullPath = uploadDirectory + "/" + savedFilename;
    
    
    std::ofstream file(fullPath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
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
    }

    if (!root.empty() && root[root.size() - 1] == '/')
        root.erase(root.size() - 1);
    
    std::string cleanLocationPath = locationPath;
    if (cleanLocationPath.size() > 1 && cleanLocationPath[cleanLocationPath.size() - 1] == '/')
    {
        cleanLocationPath.erase(cleanLocationPath.size() - 1);
    }

    std::string relativeUri = uri;
    if (uri.find(cleanLocationPath) == 0)
    {
        relativeUri = uri.substr(cleanLocationPath.size());
    }
    
    if (!relativeUri.empty() && relativeUri[0] == '/')
        relativeUri.erase(0, 1);

    std::string fullPath = root;
    if (!relativeUri.empty())
    {
        fullPath += "/" + relativeUri;
    }

    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode))
    {
        if (fullPath[fullPath.size() - 1] != '/')
            fullPath += '/';
        
    }


    return fullPath;
}
