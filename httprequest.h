#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>

namespace http {

struct response_t {
	std::string data;
	std::string header;
	std::string error;
	unsigned short code;
};

response_t get(const std::string &url, const std::string &custom_header = "");
response_t post(const std::string &url_str, const std::string post_data,
		const std::string &custom_header = "");

}

#endif // HTTPREQUEST_H
