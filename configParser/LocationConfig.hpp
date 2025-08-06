#pragma once

#include <string>
#include <vector>

class LocationConfig {
	private:
		std::string					path;
		std::vector<std::string>	methods;
		bool						cgi_enabled;
		std::string					index;
	public:
		std::string	getPath() const;
		bool		isCGIEnabled()	const;
};