#include <iostream>

#include "httprequest.h"

int main() {
	// HTTP Requests
	auto response = http::get("http://www.example.com/");
	std::cout << response.data << std::endl;

	// HTTPS
	response = http::get("https://www.example.com/");
	std::cout << response.data << std::endl;

	// Custom Header
	response = http::get("https://www.example.com/", "User-Agent: libsimplehttp");
	std::cout << response.data << std::endl;

	return 0;
}

