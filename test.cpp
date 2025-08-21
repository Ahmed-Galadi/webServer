#include <iostream>
#include <string>
#include <vector>
#include <sstream>

int htoi(const std::string &hexStr) {
    int result = 0;
    for (size_t i = 0; i < hexStr.size(); ++i) {
        char c = std::toupper(hexStr[i]);
        int value;

        if (std::isdigit(c))
            value = c - '0';
        else if (c >= 'A' && c <= 'F')
            value = 10 + (c - 'A');
        else
            break;
        result = result * 16 + value;
    }
    return result;
}

void parseHexa(std::string &hexString) {
	std::stringstream output;

	for (int i = 0; i < hexString.size(); i++) {
		
		if (hexString[i] == '%' && (i + 2) < hexString.size()) {
			std::string tmp;
			i++;
			tmp += hexString[i++];
			tmp += hexString[i];
			output << static_cast<char>(htoi(tmp));
		} else if (hexString[i] == '+') {
			output << ' ';
		} else
			output << hexString[i];
	}
	hexString = output.str();
}

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

int main() {
	// std::string str = "C%2B%2B+programming";

	// std::cout << "before: " << str << std::endl;
	// parseHexa(str);
	// std::cout << "after: " << str << std::endl;
    std::string line = "dgfiabsfvgsdhjvbajksbv-----fvdfvdfv--sdvdfbdfb-dsbdfbdf--boundary123dfbdfbdfb--asdghfagsdfhisudhfsdgvadshvadgsjkgvsgdvgskgvgv\r\n";
	if (findStr(line, "boundary123"))
		std::cout << "found it!" << std::endl;
	else
		std::cout << "(T_T)" << std::endl;
	return 0;
}