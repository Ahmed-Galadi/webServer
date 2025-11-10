#include "MimeType.hpp"
#include <algorithm>
#include <iostream>

std::map<std::string, std::string> MimeType::mimeTypes;

void MimeType::initializeMimeTypes() {
    if (!mimeTypes.empty()) return; // Already initialized
    
    // Images
    mimeTypes[".jpg"] = "image/jpeg";
    mimeTypes[".jpeg"] = "image/jpeg";
    mimeTypes[".png"] = "image/png";
    mimeTypes[".gif"] = "image/gif";
    mimeTypes[".bmp"] = "image/bmp";
    mimeTypes[".svg"] = "image/svg+xml";
    mimeTypes[".webp"] = "image/webp";
    mimeTypes[".tiff"] = "image/tiff";
    mimeTypes[".tif"] = "image/tiff";
    mimeTypes[".ico"] = "image/x-icon";
    
    // Audio
    mimeTypes[".mp3"] = "audio/mpeg";
    mimeTypes[".wav"] = "audio/wav";
    mimeTypes[".ogg"] = "audio/ogg";
    mimeTypes[".m4a"] = "audio/mp4";
    mimeTypes[".flac"] = "audio/flac";
    mimeTypes[".aac"] = "audio/aac";
    
    // Video
    mimeTypes[".mp4"] = "video/mp4";
    mimeTypes[".avi"] = "video/avi";
    mimeTypes[".mov"] = "video/quicktime";
    mimeTypes[".wmv"] = "video/x-ms-wmv";
    mimeTypes[".webm"] = "video/webm";
    mimeTypes[".mkv"] = "video/x-matroska";
    
    // Documents
    mimeTypes[".pdf"] = "application/pdf";
    mimeTypes[".doc"] = "application/msword";
    mimeTypes[".docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    mimeTypes[".xls"] = "application/vnd.ms-excel";
    mimeTypes[".xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    mimeTypes[".ppt"] = "application/vnd.ms-powerpoint";
    mimeTypes[".pptx"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    
    // Text
    mimeTypes[".txt"] = "text/plain";
    mimeTypes[".html"] = "text/html";
    mimeTypes[".htm"] = "text/html";
    mimeTypes[".css"] = "text/css";
    mimeTypes[".js"] = "application/javascript";
    mimeTypes[".json"] = "application/json";
    mimeTypes[".xml"] = "application/xml";
    mimeTypes[".csv"] = "text/csv";
    
    // Archives
    mimeTypes[".zip"] = "application/zip";
    mimeTypes[".rar"] = "application/vnd.rar";
    mimeTypes[".tar"] = "application/x-tar";
    mimeTypes[".gz"] = "application/gzip";
    mimeTypes[".7z"] = "application/x-7z-compressed";
    
    // Others
    mimeTypes[".bin"] = "application/octet-stream";
    mimeTypes[".exe"] = "application/octet-stream";
    mimeTypes[".dmg"] = "application/octet-stream";
    
    std::cout << "[DEBUG] MimeType: Initialized " << mimeTypes.size() << " MIME types" << std::endl;
}

std::string MimeType::toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string MimeType::getFileExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of(".");
    if (dotPos != std::string::npos) {
        return toLowerCase(filename.substr(dotPos));
    }
    return "";
}

std::string MimeType::getMimeType(const std::string& filename) {
    initializeMimeTypes();
    
    std::string extension = getFileExtension(filename);
    if (extension.empty()) {
        return "application/octet-stream"; // Default for unknown types
    }
    
    std::map<std::string, std::string>::const_iterator it = mimeTypes.find(extension);
    if (it != mimeTypes.end()) {
        return it->second;
    }
    
    return "application/octet-stream"; // Default for unknown extensions
}

std::string MimeType::getMimeTypeFromExtension(const std::string& extension) {
    initializeMimeTypes();
    
    std::string lowerExt = toLowerCase(extension);
    if (!lowerExt.empty() && lowerExt[0] != '.') {
        lowerExt = "." + lowerExt;
    }
    
    std::map<std::string, std::string>::const_iterator it = mimeTypes.find(lowerExt);
    if (it != mimeTypes.end()) {
        return it->second;
    }
    
    return "application/octet-stream";
}

bool MimeType::isImageType(const std::string& mimeType) {
    return mimeType.find("image/") == 0;
}

bool MimeType::isVideoType(const std::string& mimeType) {
    return mimeType.find("video/") == 0;
}

bool MimeType::isAudioType(const std::string& mimeType) {
    return mimeType.find("audio/") == 0;
}

bool MimeType::isTextType(const std::string& mimeType) {
    return mimeType.find("text/") == 0 || 
           mimeType == "application/json" ||
           mimeType == "application/xml" ||
           mimeType == "application/javascript";
}

bool MimeType::isBinaryType(const std::string& mimeType) {
    // Consider binary types as those that are not text-based
    return !isTextType(mimeType) || 
           mimeType == "application/octet-stream" ||
           isImageType(mimeType) ||
           isVideoType(mimeType) ||
           isAudioType(mimeType) ||
           isArchiveType(mimeType) ||
           mimeType.find("application/") == 0; // Most application types are binary
}

bool MimeType::isArchiveType(const std::string& mimeType) {
    return mimeType == "application/zip" ||
           mimeType == "application/vnd.rar" ||
           mimeType == "application/x-tar" ||
           mimeType == "application/gzip" ||
           mimeType == "application/x-7z-compressed";
}

bool MimeType::isDocumentType(const std::string& mimeType) {
    return mimeType == "application/pdf" ||
           mimeType == "application/msword" ||
           mimeType == "application/vnd.openxmlformats-officedocument.wordprocessingml.document" ||
           mimeType == "application/vnd.ms-excel" ||
           mimeType == "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" ||
           mimeType == "application/vnd.ms-powerpoint" ||
           mimeType == "application/vnd.openxmlformats-officedocument.presentationml.presentation";
}