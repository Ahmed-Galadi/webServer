#pragma once

#include <string>
#include <vector>

class LocationConfig {
	private:
		std::string					path;
		std::string					root;
		std::string					index;
		std::vector<std::string>	methods;
		bool						cgi_enabled;

	public:
		void		setPath(std::string pathStr);
		void		setRoot(std::string rootStr);
		void		setMethods(std::string method);
		void		setCGI(bool state);
		void		setIndex(std::string indexStr);

		std::string					getPath() const;
		std::string					getRoot() const;
		std::string					getIndex() const;
		std::vector<std::string>	getMethods() const;
		bool						isCGIEnabled()	const;
		bool						isMethodAllowed(const std::string& method) const;
};