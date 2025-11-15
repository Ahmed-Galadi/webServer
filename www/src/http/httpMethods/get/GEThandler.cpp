#include "./GEThandler.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>


std::string urlDecode(const std::string &str) {
    std::ostringstream decoded;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            std::string hex = str.substr(i + 1, 2);
            int value;
            std::istringstream(hex) >> std::hex >> value;
            decoded << static_cast<char>(value);
            i += 2;
        } else if (str[i] == '+') {
            decoded << ' ';
        } else {
            decoded << str[i];
        }
    }
    return decoded.str();
}

std::string urlEncode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else if (c == ' ') {
            escaped << "%20";
        } else {
            escaped << '%' << std::uppercase << std::setw(2) << int((unsigned char)c);
            escaped << std::nouppercase;
        }
    }
    return escaped.str();
}


std::map<std::string, std::string> parseQueryString(const std::string& query) {
    std::map<std::string, std::string> params;
    std::stringstream ss(query);
    std::string pair;

    while (std::getline(ss, pair, '&')) {
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = pair.substr(0, eq);
            std::string val = pair.substr(eq + 1);
            params[key] = urlDecode(val);
        }
    }

    return params;
}

GEThandler::GEThandler() {}
GEThandler::~GEThandler() {}

bool isDirectory(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0) return false;
    return S_ISDIR(statbuf.st_mode);
}

bool fileExists(const std::string& path) {
    struct stat statbuf;
    return stat(path.c_str(), &statbuf) == 0;
}

bool isFileReadable(const std::string& path) {
    return access(path.c_str(), R_OK) == 0;
}
Response* GEThandler::handler(const Request& req, const LocationConfig* location, const ServerConfig* /* serverConfig */) {
    Response* response = new Response();

    try {
        response->setVersion(req.getVersion());
        response->setServer("WebServer/1.0");
        response->setDate();
        response->setConnection("close");

        std::string uri = req.getURI();
        std::string queryString;

        // --------------------------------------------
        // Fix 1: Strip query string from URI properly
        // --------------------------------------------
        size_t queryPos = uri.find('?');
        if (queryPos != std::string::npos) {
            queryString = uri.substr(queryPos + 1);
            uri = uri.substr(0, queryPos);
        }

        uri = urlDecode(uri); // decode %20 -> space

        // --------------------------------------------
        // Fix 2: Pass cleaned URI to resolver
        // --------------------------------------------
        std::string path = FileHandler::resolveFilePath(uri, location);


        // Check if path exists (file or directory)
        if (!fileExists(path) && !isDirectory(path)) {
            delete response;
            return createErrorResponse(404, "Not Found");
        }

        // ============================================
        // Handle Directory Requests
        // ============================================
        if (isDirectory(path)) {

            // Fix 3: redirect /dir -> /dir/
            if (uri[uri.length() - 1] != '/') {
                std::string redirectUri = uri + "/";
                delete response;
                Response* redirect = new Response();
                redirect->setStatus(301);
                redirect->setVersion(req.getVersion());
                redirect->setServer("WebServer/1.0");
                redirect->setDate();
                redirect->setConnection("close");

                std::map<std::string, std::string> headers;
                headers["Location"] = redirectUri;
                redirect->setHeaders(headers);
                redirect->setBody("<html><body><h1>301 Moved Permanently</h1></body></html>");
                return redirect;
            }

            std::string dirPath = path;
            if (dirPath[dirPath.length() - 1] != '/')
                dirPath += "/";

            // ============================================
            // Nginx Behavior Matrix: index + autoindex
            // ============================================
            // Case 1-2: Index file exists → serve it (autoindex ignored)
            // Case 3: Index not found + autoindex ON → directory listing
            // Case 4: Index not found + autoindex OFF → 403 Forbidden
            // Case 11-12: No index directive → check autoindex
            // ============================================

            // Get index file from location config (empty string if not configured)
            std::string indexFile = location ? location->getIndex() : "";
            bool autoindex = location ? location->getAutoIndex() : false;

            
            // Check if index directive is configured and not empty
            if (!indexFile.empty()) {
                std::string indexPath = dirPath + indexFile;

                // Check if index path exists
                if (fileExists(indexPath)) {
                    // File exists - check if it's readable
                    if (!isFileReadable(indexPath)) {
                        // File exists but no read permission → 403 Forbidden
                        delete response;
                        return createErrorResponse(403, "Forbidden");
                    }
                    // Case 1-2: Index file exists and is readable → serve it (autoindex ignored)
                    path = indexPath;
                    // continue to serve below
                } else {
                    // Index configured but not found
                    
                    if (autoindex) {
                        // Case 3: Index not found + autoindex ON → directory listing
                        delete response;
                        return generateDirectoryListing(uri, dirPath, req);
                    } else {
                        // Case 4: Index not found + autoindex OFF → 403 Forbidden
                        delete response;
                        return createErrorResponse(403, "Forbidden");
                    }
                }
            } else {
                // No index directive configured (Cases 11-12)
                
                if (autoindex) {
                    // Case 11: No index + autoindex ON → directory listing
                    delete response;
                    return generateDirectoryListing(uri, dirPath, req);
                } else {
                    // Case 12: No index + autoindex OFF → 403 Forbidden
                    delete response;
                    return createErrorResponse(403, "Forbidden");
                }
            }
        }

        // ============================================
        // Serve Regular File
        // ============================================
        // At this point, path should be a file, not a directory

        // Security check: ensure path is not a directory
        if (isDirectory(path)) {
            delete response;
            return createErrorResponse(403, "Forbidden");
        }

        // Security: path traversal
        if (path.find("..") != std::string::npos) {
            delete response;
            return createErrorResponse(403, "Forbidden");
        }

        // Check if file is readable before trying to open it
        if (!isFileReadable(path)) {
            delete response;
            // File exists but no read permission
            if (fileExists(path)) {
                return createErrorResponse(403, "Forbidden");
            } else {
                return createErrorResponse(404, "Not Found");
            }
        }

        std::ifstream file(path.c_str(), std::ios::binary);
        if (!file.is_open()) {
            delete response;
            return createErrorResponse(404, "Not Found");
        }

        std::ostringstream bodyBuffer;
        bodyBuffer << file.rdbuf();
        std::string responseBody = bodyBuffer.str();
        file.close();

        std::string mimeType = MimeType::getMimeType(path);

        std::map<std::string, std::string> headers;
        headers["Content-Length"] = numberToString(responseBody.size());
        headers["Content-Type"] = mimeType;
        headers["Cache-Control"] = "public, max-age=3600";
        
        // If MIME type is unknown (application/octet-stream), add Content-Disposition
        // to trigger download behavior like NGINX
        if (mimeType == "application/octet-stream") {
            // Extract filename from path
            std::string filename = path;
            size_t lastSlash = path.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                filename = path.substr(lastSlash + 1);
            }
            headers["Content-Disposition"] = "attachment; filename=\"" + filename + "\"";
        }

        response->setStatus(200);
        response->setHeaders(headers);
        response->setBody(responseBody);

        return response;

    } catch (const std::exception& e) {
        delete response;
        return createErrorResponse(500, "Internal Server Error");
    }
}

// ============================================
// Generate Directory Listing (Autoindex)
// ============================================
Response* GEThandler::generateDirectoryListing(const std::string& uri, 
                                               const std::string& dirPath,
                                               const Request& req) {

    Response* response = new Response();
    response->setVersion(req.getVersion());
    response->setServer("WebServer/1.0");
    response->setDate();
    response->setConnection("close");

    // Open directory
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) {
        delete response;
        return createErrorResponse(500, "Cannot read directory");
    }

    // Read directory entries

    std::vector<DirectoryEntry> entries;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;

        // Skip hidden files and parent directory
        if (name[0] == '.') continue;

        DirectoryEntry dirEntry;
        dirEntry.name = name;

        // Get file info
        std::string fullPath = dirPath + name;
        struct stat statbuf;
        if (stat(fullPath.c_str(), &statbuf) == 0) {
            dirEntry.isDir = S_ISDIR(statbuf.st_mode);
            dirEntry.size = statbuf.st_size;
        } else {
            dirEntry.isDir = false;
            dirEntry.size = 0;
        }

        entries.push_back(dirEntry);
    }
    closedir(dir);

    // Sort entries: directories first, then files, alphabetically
    std::sort(entries.begin(), entries.end(), compareEntries);

    // Generate HTML
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"en\">\n";
    html << "<head>\n";
    html << "    <meta charset=\"UTF-8\">\n";
    html << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "    <title>Index of " << uri << "</title>\n";
    html << "    <style>\n";
    html << "        * { margin: 0; padding: 0; box-sizing: border-box; }\n";
    html << "        body { font-family: 'Segoe UI', Arial, sans-serif; background: #f5f5f5; padding: 20px; }\n";
    html << "        .container { max-width: 1000px; margin: 0 auto; background: white; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n";
    html << "        .header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; border-radius: 8px 8px 0 0; }\n";
    html << "        h1 { font-size: 1.8em; margin-bottom: 5px; }\n";
    html << "        .path { opacity: 0.9; font-size: 0.9em; }\n";
    html << "        table { width: 100%; border-collapse: collapse; }\n";
    html << "        th { background: #f8f9fa; padding: 15px; text-align: left; font-weight: 600; color: #333; border-bottom: 2px solid #dee2e6; }\n";
    html << "        td { padding: 12px 15px; border-bottom: 1px solid #e9ecef; }\n";
    html << "        tr:hover { background: #f8f9fa; }\n";
    html << "        a { color: #667eea; text-decoration: none; font-weight: 500; }\n";
    html << "        a:hover { text-decoration: underline; }\n";
    html << "        .icon { display: inline-block; width: 20px; margin-right: 8px; }\n";
    html << "        .dir { color: #ffc107; }\n";
    html << "        .file { color: #667eea; }\n";
    html << "        .size { color: #666; font-size: 0.9em; }\n";
    html << "        .footer { padding: 20px; text-align: center; color: #999; font-size: 0.9em; border-top: 1px solid #e9ecef; }\n";
    html << "    </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "    <div class=\"container\">\n";
    html << "        <div class=\"header\">\n";
    html << "            <h1>📁 Index of " << uri << "</h1>\n";
    html << "            <div class=\"path\">" << dirPath << "</div>\n";
    html << "        </div>\n";
    html << "        <table>\n";
    html << "            <thead>\n";
    html << "                <tr>\n";
    html << "                    <th>Name</th>\n";
    html << "                    <th>Size</th>\n";
    html << "                </tr>\n";
    html << "            </thead>\n";
    html << "            <tbody>\n";

    // Parent directory link
    if (uri != "/" && uri != "") {
        html << "                <tr>\n";
        html << "                    <td><span class=\"icon dir\">📁</span><a href=\"../\">Parent Directory</a></td>\n";
        html << "                    <td class=\"size\">-</td>\n";
        html << "                </tr>\n";
    }

    // List entries

    for (size_t i = 0; i < entries.size(); ++i) {
        const DirectoryEntry& e = entries[i];

        // Ensure URI ends with /
        std::string baseUri = uri;
        if (baseUri[baseUri.length() - 1] != '/') {
            baseUri += "/";
        }

        std::string href = baseUri + urlEncode(e.name);  // <-- encode here
        if (e.isDir) href += "/";

        html << "                <tr>\n";
        html << "                    <td>";

        if (e.isDir) {
            html << "<span class=\"icon dir\">📁</span>";
            html << "<a href=\"" << href << "\">" << e.name << "/</a>";
        } else {
            html << "<span class=\"icon file\">📄</span>";
            html << "<a href=\"" << href << "\">" << e.name << "</a>";
        }

        html << "</td>\n";
        html << "                    <td class=\"size\">";

        if (e.isDir) {
            html << "-";
        } else {
            html << formatFileSize(e.size);
        }

        html << "</td>\n";
        html << "                </tr>\n";
    }


    html << "            </tbody>\n";
    html << "        </table>\n";
    html << "        <div class=\"footer\">\n";
    html << "            WebServer/1.0\n";
    html << "        </div>\n";
    html << "    </div>\n";
    html << "</body>\n";
    html << "</html>\n";

    std::string body = html.str();

    // Set response
    response->setStatus(200);
    response->setBody(body);

    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "text/html; charset=utf-8";
    headers["Content-Length"] = numberToString(body.length());
    response->setHeaders(headers);


    return response;
}

// Helper: Format file size
std::string GEThandler::formatFileSize(off_t bytes) {
    std::ostringstream oss;
    
    if (bytes < 1024) {
        oss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        oss << (bytes / 1024.0) << " KB";
    } else if (bytes < 1024 * 1024 * 1024) {
        oss << (bytes / (1024.0 * 1024.0)) << " MB";
    } else {
        oss << (bytes / (1024.0 * 1024.0 * 1024.0)) << " GB";
    }
    
    return oss.str();
}

// Helper: Compare directory entries for sorting
bool GEThandler::compareEntries(const DirectoryEntry& a, const DirectoryEntry& b) {
    // Directories first
    if (a.isDir && !b.isDir) return true;
    if (!a.isDir && b.isDir) return false;
    
    // Then alphabetically
    return a.name < b.name;
}

Response* GEThandler::serveTextFile(Response* response, const std::string& filepath, const std::string& mimeType) {
    std::string content;
    if (!FileHandler::readFile(filepath, content)) {
        delete response;
        return createErrorResponse(500, "Cannot read file");
    }
    
    response->setStatus(200);
    response->setBody(content);
    
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = mimeType;
    headers["Content-Length"] = numberToString(content.length());
    response->setHeaders(headers);
    
    return response;
}

Response* GEThandler::serveBinaryFile(Response* response, const std::string& filepath, const std::string& mimeType) {
    std::vector<char> content;
    if (!FileHandler::readBinaryFile(filepath, content)) {
        delete response;
        return createErrorResponse(500, "Cannot read file");
    }
    
    std::string binaryData(content.begin(), content.end());
    
    response->setStatus(200);
    response->setBody(binaryData);
    
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = mimeType;
    headers["Content-Length"] = numberToString(content.size());
    headers["Cache-Control"] = "public, max-age=3600";
    
    response->setHeaders(headers);
    
    return response;
}

Response* GEThandler::createErrorResponse(int statusCode, const std::string& message) {
    Response* response = new Response();
    response->setStatus(statusCode);
    response->setVersion("HTTP/1.0");
    response->setServer("WebServer/1.0");
    response->setDate();
    response->setConnection("close");
    
    std::string body = "<html><body><h1>" + numberToString(statusCode) + " " + message + "</h1></body></html>";
    response->setBody(body);
    
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "text/html";
    headers["Content-Length"] = numberToString(body.length());
    response->setHeaders(headers);
    
    return response;
}

std::string GEThandler::numberToString(size_t number) {
    std::ostringstream oss;
    oss << number;
    return oss.str();
}
