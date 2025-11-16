#ifndef MIMETYPE_HPP
#define MIMETYPE_HPP

#include "../../../../include/webserv.hpp"

class MimeType {
private:
    static std::map<std::string, std::string> mimeTypes;
    static void initializeMimeTypes();
    
public:
    // Get MIME type from file extension
    static std::string getMimeType(const std::string& filename);
    static std::string getMimeTypeFromExtension(const std::string& extension);
    
    // Type checking functions
    static bool isImageType(const std::string& mimeType);
    static bool isVideoType(const std::string& mimeType);
    static bool isAudioType(const std::string& mimeType);
    static bool isTextType(const std::string& mimeType);
    static bool isArchiveType(const std::string& mimeType);
    
    // Utility functions
    static std::string getFileExtension(const std::string& filename);
    static std::string toLowerCase(const std::string& str);
};

#endif