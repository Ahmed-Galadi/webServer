#pragma once

#include <string>
#include <vector>
#include <map>

class RequestBody {
	private:
		std::vector<uint8_t>				rawData;
		std::map<std::string, std::string>	encodData;
		std::string 						name;
		std::string 						fileName;
		std::string 						contentType;

	public:

		void					setName(const std::string &name);
		void					setFileName(const std::string &fileName);
		void					setContentType(const std::string &contentType);
		void					setRawData(const std::vector<uint8_t> &data);
		void					setEncodedData(const std::map<std::string, std::string> &encodedData);

		std::map<std::string, std::string>	getEncodedData() const;
		std::vector<uint8_t>				getRawData() const;
		std::string							getName() const;
		std::string							getFileName() const;
		std::string							getContentType() const;
};