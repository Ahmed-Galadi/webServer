#include "RequestParser.hpp"
#include "../configParser/ParseUtils.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <sstream>

// ----- Split body by boundary -----
std::vector<std::string> RequestParser::splitBody(const std::string &rawBody, const std::string &boundary) {
    std::vector<std::string> result;
    if (rawBody.empty() || boundary.empty()) {
        result.push_back(rawBody);
        return (result);
    }

    size_t start = 0;
    size_t pos = rawBody.find(boundary);

    while (pos != std::string::npos) {
        result.push_back(rawBody.substr(start, pos - start));
        start = pos + boundary.length();
        pos = rawBody.find(boundary, start);
    }

    // Add the final chunk
    result.push_back(rawBody.substr(start));
    return (result);
}

// ------------- [Parse Hexadecimal values in [application/x-www-form-urlencoded] ]

void	RequestParser::parseHexa(std::string &hexString) {
	std::stringstream output;

	for (int i = 0; i < hexString.size(); i++) {
		
		if (hexString[i] == '%' && (i + 2) < hexString.size()) {
			std::string tmp;
			i++;
			tmp += hexString[i++];
			tmp += hexString[i];
			output << static_cast<char>(ParseUtils::htoi(tmp));
		} else if (hexString[i] == '+') {
			output << ' ';
		} else
			output << hexString[i];
	}
	hexString = output.str();
}

// ----- [Extract application/x-www-form-urlencoded or octet-stream] -----
void RequestParser::extractEncodedData(RequestBody &rb, const std::string &bodyRawStr, const std::string &type) {
    std::string rawBody = ParseUtils::trim(bodyRawStr);

    if (type == "x-www-form-urlencoded") {
        std::map<std::string, std::string> output;
        std::vector<std::string> pairs = ParseUtils::splitString(rawBody, '&');

		for (size_t i = 0; i < pairs.size(); ++i) {
    		std::vector<std::string> kv = ParseUtils::splitString(pairs[i], '=');
   			if (kv.size() != 2) {
        		std::cerr << "Error: Not valid URL-encoded data!" << std::endl;
       			throw (Request::InvalidRequest);
    		}
			parseHexa(kv[0]);
			parseHexa(kv[1]);
    		output[kv[0]] = kv[1];
		}
    	rb.setEncodedData(output);
    } else if (type == "octet-stream")
        extractOctetStream(rb, rawBody);
}


// ----- [Extract a single multipart body part] -----
RequestBody RequestParser::extractBodyPart(const std::string &rawBodyPart) {
    RequestBody output;

    std::vector<std::string> splitHeaderFromData = splitBody(rawBodyPart, "\r\n\r\n");
    if (splitHeaderFromData.size() < 2)
		throw (Request::InvalidRequest);
    std::string headerBlock = splitHeaderFromData[0];
    std::string body = splitHeaderFromData[1];

    std::vector<std::string> headers = splitBody(headerBlock, "\r\n");
    for (size_t i = 0; i < headers.size(); ++i) {
    	std::string &line = headers[i];
        std::string trimmed = ParseUtils::trim(line);

        if (trimmed.find("Content-Type:") == 0) {
            output.setContentType(ParseUtils::trim(trimmed.substr(13)));
        }
        else if (trimmed.find("Content-Disposition:") == 0) {
            std::string disp = trimmed.substr(20); // skip "Content-Disposition:"
            std::vector<std::string> params = ParseUtils::splitString(disp, ';');

            for (size_t j = 0; j < params.size(); ++j) {
    			std::string &param = params[j];
                param = ParseUtils::trim(param);
                if (param.find("name=") == 0) {
					std::string clean = param.substr(5);
					if (!clean.empty() && clean.front() == '"' && clean.back() == '"') {
    					clean = clean.substr(1, clean.size() - 2);
					}
					output.setName(clean);
				} else if (param.find("filename=") == 0) {
					std::string clean = param.substr(9);
					if (!clean.empty() && clean.front() == '"' && clean.back() == '"') {
    					clean = clean.substr(1, clean.size() - 2);
					}
                    output.setFileName(clean);
				}
            }
        }
    }
	std::string tmp = ParseUtils::splitString(body, '\r')[0];
    extractOctetStream(output, tmp);

    return (output);
}

// parse function helper
bool	findStr(const std::string &hayStack, const std::string &needle) {
	for (int i = 0; i < hayStack.size(); i++) {
		for (int j = 0; j < needle.size(); j++) {
			if (hayStack[i + j] != needle[j])
				break;
			if (j + 1 == needle.size())
				return (true);
		}
	}
	return (false);
}

// *******[ Hepler Function to check boundaries ]***********
bool	RequestParser::boundariesError(std::string &rawBody, const std::string &boundary) {
	std::vector<std::string> boundaries;
	std::vector<std::string> splitedBody = splitBody(rawBody, "\r\n");

	for (int i = 0; i < splitedBody.size(); i++) {
		if (findStr(splitedBody[i], boundary))
			boundaries.push_back(splitedBody[i]);
	}
	if (boundaries.front() == boundaries.back()) {
		std::cout << "Error: multipart boundary incorrect!" << std::endl;
		throw (Request::InvalidRequest);
	}

	std::string tmpLastBoundary = boundary + "--";
	if (boundaries.back() != tmpLastBoundary) {
		std::cout << "Error: multipart boundary incorrect!" << std::endl;
		throw (Request::InvalidRequest);
	}
	
}

// ----- [Extract multipart body] -----
void RequestParser::extractMultiPart(std::vector<RequestBody> &output, const std::string &bodyRawStr, const std::string &boundary, const std::string &type) {
	// boundary check
    std::vector<std::string> bodyParts = splitBody(bodyRawStr, boundary);

	for (size_t i = 0; i < bodyParts.size(); ++i) {
    	std::string trimmed = ParseUtils::trim(bodyParts[i]);
    	if (trimmed.empty() || trimmed == "--")
        	continue;
    	output.push_back(extractBodyPart(trimmed));
	}
}

// ----- Convert raw string to raw binary -----
void RequestParser::extractOctetStream(RequestBody &rb, const std::string &rawData) {
    std::vector<uint8_t> output;
    output.reserve(rawData.size());

    for (size_t i = 0; i < rawData.size(); i++)
        output.push_back(static_cast<uint8_t>(rawData[i]));

    rb.setRawData(output);
}

// ----- Main body parser -----
std::vector<RequestBody> RequestParser::ParseBody(const Request &req) {
    std::vector<RequestBody> output;
    RequestBody RBHolder;
    std::string boundary;

    // --- Extract Content-Type ---
    std::map<std::string, std::string> headers = req.getHeaders();
    if (headers.find("Content-Type") == headers.end()) {
        std::cerr << "ERROR: Cannot parse body without Content-Type" << std::endl;
        throw (RequestBody::InvalidRequest);
    }

    std::string contentType = headers["Content-Type"];
    if (contentType == "application/x-www-form-urlencoded" ||
        contentType == "application/json" ||
        contentType == "text/plain") {
        RBHolder.setContentType(contentType);
        extractEncodedData(RBHolder, req.getRawBody(), ParseUtils::splitString(contentType, '/')[1]);
        output.push_back(RBHolder);
        return (output);
    }

    // --- Handle multipart/form-data ---
    std::vector<std::string> parts = ParseUtils::splitString(contentType, ';');
    if (parts.size() < 2) {
        std::cerr << "ERROR: Invalid multipart Content-Type" << std::endl;
        throw (RequestBody::InvalidRequest);
    }

    std::string typePart = ParseUtils::trim(parts[0]);
	std::string typePart2 = ParseUtils::trim(parts[1]);
    std::vector<std::string> boundaryPart = ParseUtils::splitString(typePart2, '=');

    if (typePart == "multipart/form-data" &&
        boundaryPart.size() == 2 &&
        ParseUtils::trim(boundaryPart[0]) == "boundary") {
        RBHolder.setContentType("multipart/form-data");
        boundary = boundaryPart[1];
    } else {
        std::cerr << "ERROR: Invalid multipart Content-Type header" << std::endl;
        throw (RequestBody::InvalidRequest);
    }

    extractMultiPart(output, req.getRawBody(), boundary, "form-data");
    return (output);
}



// #include "RequestParser.hpp"
// #include "ParseUtils.hpp"
// #include <map>

// std::vector<std::string> RequestParser::splitBody(const std::string &rawBody, const std::string &boundary) {
//     std::vector<std::string> result;
// 	if (rawBody.empty() || boundary.empty()) {
// 		result.push_back(rawBody);
// 		return (result);
// 	}
//     size_t start = 0;
//     size_t pos = rawBody.find(boundary);

//     while (pos != std::string::npos) {
//         result.push_back(rawBody.substr(start, pos - start));
//         start = pos + boundary.length();
//         pos = rawBody.find(boundary, start);
//     }

//     result.push_back(rawBody.substr(start));
//     return result;
// }

// void	RequestParser::extractEncodedData(RequestBody &rb ,const std::string &bodyRawStr, const std::string &type) {
// 	std::string	rawBody = ParseUtils::trim(bodyRawStr);
// 	std::map<std::string, std::string> output;

// 	if (type == "x-www-form-urlencoded") {
// 		std::vector<std::string> splitedRawEncodedData  = ParseUtils::splitString(rawBody, '&');
// 		for (int i = 0; i < splitedRawEncodedData.size(); i++) {
// 			std::vector<std::string> holder = ParseUtils::splitString(splitedRawEncodedData[i], '=');
// 			if (holder.size() != 2) {
// 				std::cout << "Error: Not Valid Encoded data!" << std::endl;
// 				exit(1);
// 			}
// 			std::pair<std::string, std::string> tmp = std::make_pair(holder[0], holder[1]);
// 			output.insert(tmp);
// 		}
// 		rb.setEncodedData(output);
// 	} else if (type == "octet-stream")
// 		extractOctetStream(rb, rawBody);
// }

// RequestBody	RequestParser::extractBodyPart(const std::string &rawBodyPart) {
// 	RequestBody		output = RequestBody();
// 	std::vector<std::string> splitHeaderFromData = splitBody(rawBodyPart, "\r\n\r\n");
// 	std::vector<std::string> headers = splitBody(splitHeaderFromData[0], "\r\n");
// 	std::vector<std::string> headersTokens = ParseUtils::splitAndAccumulate(headers);
// 	for (int i = 0; i < headersTokens.size(); i++) {
// 		if (headersTokens[i] == "Content-Type:" && (i++)) {
// 			if (headersTokens[i] == "image/png")
// 				output.setContentType("image/png");
// 			else if (headersTokens[i] == "text/plain")
// 				output.setContentType("text/plain");
// 			else {
// 				std::cout << "Error: Content-Type: invalid body part!" << std::endl;
// 				exit(1);
// 			}
// 		} else if (headersTokens[i] == "Content-Disposition:" && (i++)) {
// 			if (headersTokens[i] == "form-data;" && (i++)) {
// 				for (;headersTokens[i] != "Content-Type:"; i++) {
// 					std::vector<std::string> splitkeyVal = ParseUtils::splitString(headersTokens[i], '=');
// 					if (splitkeyVal[0] == "name")
// 						output.setName(splitkeyVal[1]);
// 					else if (splitkeyVal[0] == "filename")
// 						output.setFileName(splitkeyVal[1]);
// 					else {
// 						std::cout << "Error: Content-Disposition: invalid body part!" << std::endl;
// 						exit(1);
// 					}
// 				}
// 			} else {
// 				std::cout << "Error: invalid body part!" << std::endl;
// 				exit(1);
// 			}
// 		}
// 		i++;
// 	}
// 	std::string body = headers[1];
// 	extractOctetStream(output, body);
// }

// void	RequestParser::extractMultiPart(std::vector<RequestBody> &output,const std::string &bodyRawStr, const std::string &boundary, const std::string &type) {
// 	std::vector<std::string> bodyParts = splitBody(bodyRawStr, boundary);

// 	for (int i = 0; (i < bodyParts.size()) && (bodyParts[i] != "--"); i++)
// 		output.push_back(extractBodyPart(bodyParts[i]));
// }

// void	RequestParser::extractOctetStream(RequestBody &rb, const std::string &rawData) {
// 	std::vector<uint8_t> output;

// 	for (int i = 0; i < rawData.size(); i++)
// 		output.push_back(static_cast<uint8_t>(rawData[i]));
// 	rb.setRawData(output);
// }

// std::vector<RequestBody> RequestParser::ParseBody(const Request &req) {
// 	std::vector<RequestBody>				output;
// 	RequestBody								RBHolder = RequestBody();
// 	std::string								boundary;
// 	// ----- extract meta data --------
// 		// cehck if [Content-Type] exists first
// 	std::map<std::string, std::string>		headers = req.getHeaders();
// 	if (headers.find("Content-Type") == headers.end()) {
// 		std::cout << "ERROR: can't Parse body without [Content-Type]" << std::endl;
// 		exit(1);
// 	}
// 		// check if [Content-Type] is not multipart and assign it else split [Content-Type] and extract the boundary
// 	if (headers["Content-Type"] == "application/x-www-form-urlencoded"
// 		|| headers["Content-Type"] == "application/json"
// 		|| headers["Content-Type"] == "text/plain")
// 		RBHolder.setContentType(headers["Content-Type"]);
// 	else {
// 		std::vector<std::string> spllitedContentType = ParseUtils::splitString(headers["Content-Type"], ';');
// 		if (spllitedContentType.size() != 2) {
// 			std::cout << "ERROR: invalid two args content type" << std::endl;
// 			exit(1);
// 		}
		
// 		std::vector<std::string> splitBoundary = ParseUtils::splitString(spllitedContentType[0], '=');
// 		if (splitBoundary.size() != 2) {
// 			std::cout << "ERROR: [Content-Type] boundary is not set" << std::endl;
// 			exit(0);
// 		}
// 		if (ParseUtils::trim(spllitedContentType[0]) == "multipart/form-data;" && ParseUtils::trim(splitBoundary[0]) == "Boundary") {
// 			RBHolder.setContentType("multipart/form-data");
// 			boundary = splitBoundary[1];
// 		} else {
// 			std::cout << "ERROR: [Content-Type] is not valid" << std::endl;
// 			exit(1);
// 		}
// 	}
// 	// --------------------------------
// 	// ----------- extract body data --------------------
// 	std::string tmp = RBHolder.getContentType();
// 	std::string type1 = ParseUtils::splitString(tmp, '/')[0];
// 	std::string type2 = ParseUtils::splitString(tmp, '/')[1];
// 	if (type1 == "application") {
// 		RequestParser::extractEncodedData(RBHolder ,req.getRawBody(), type2);
// 		output.push_back(RBHolder);
// 		return (output);
// 	}
// 	else if (type1 == "multipart" && !boundary.empty())
// 		RequestParser::extractMultiPart(output,req.getRawBody(), boundary, type2);
// 	// --------------------------------
// 	// ------------ [ Final Return ] ---------------
// 	return ;
// }
