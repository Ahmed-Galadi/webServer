#pragma once

#include "../../../include/webserv.hpp"

class RequestBody {
	private:
		std::string							rawData;
		 
		std::vector<char> binaryData;  // Binary-safe version (authoritative)
		std::map<std::string, std::string>	encodData;
		std::string 						name;
		std::string 						fileName;
		std::string 						contentType;

	public:

		void					setName(const std::string &name);
		void					setFileName(const std::string &fileName);
		void					setContentType(const std::string &contentType);
		void					setRawData(const std::string &data);
		void					setEncodedData(const std::map<std::string, std::string> &encodedData);

		std::map<std::string, std::string>	getEncodedData() const;
		std::string							getRawData() const;
		std::string							getName() const;
		std::string							getFileName() const;
		std::string							getContentType() const;

		void setBinaryData(const std::vector<char>& data);
    	void setBinaryData(const char* data, size_t size);

	    // BINARY-SAFE: New getters
    	const std::vector<char>& getBinaryData() const;
    	std::string getBinaryDataAsString() const;
    	bool isBinaryData() const;
    	size_t getDataSize() const;
};