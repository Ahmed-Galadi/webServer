#pragma once

#include "../../include/webserv.hpp"
#include "ParseUtils.hpp"

class LocationConfig {
	private:
		std::string					path;
		std::string					root;
		std::string					index;
		std::vector<std::string>	methods;
		bool						cgi_enabled;
		size_t						client_max_body_size;
		bool						autoindex;
		// Return directive: first = status code (301/302/307), second = URL
		bool						has_return;
		int							return_code;
		std::string					return_url;
		// Upload store: directory where uploaded files should be saved
		std::string					upload_store;

	public:
		LocationConfig(); 
		void		setPath(std::string pathStr);
		void		setRoot(std::string rootStr);
		void		setMethods(std::string method);
		void		setCGI(bool state);
		void		setIndex(std::string indexStr);
		void 		setClientMaxBodySize(size_t size);
		void		setAutoIndex(bool autoindex);
		void		setReturn(int code, const std::string& url);
		void		setUploadStore(const std::string& path);

		std::string					getPath() const;
		std::string					getRoot() const;
		std::string					getIndex() const;
		std::vector<std::string>	getMethods() const;
		bool						isCGIEnabled()	const;
		bool						isMethodAllowed(const std::string& method) const;
		size_t						getClientMaxBodySize() const;
		bool						getAutoIndex() const;
		bool						hasReturn() const;
		int							getReturnCode() const;
		std::string					getReturnUrl() const;
		std::string					getUploadStore() const;
};