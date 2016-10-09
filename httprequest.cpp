#include "httprequest.h"

#include <algorithm>
#include <iostream>
#include <regex>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

namespace http {

struct url_t {
	string host;
	string path;
	string query;
	int port;
	string protocol;
	bool valid = false;

	operator string() const {
		return protocol + "://" + host + ":" + boost::lexical_cast<std::string>(port) + path + "?" + query;
	}
};

template<typename Stream>
static void read_all(Stream &socket, response_t &response)
{
	// Read all
	boost::asio::streambuf buf;
	boost::system::error_code error;

	boost::asio::read(socket, buf, boost::asio::transfer_all(), error);

	auto bufs = buf.data();
	string str(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + buf.size());

	response.data = str;
}

/**
 * @brief parse_url Extract the components of an URL supplied at string.
 */
static url_t parse_url(const string& urlstr)
{
	url_t result;

	// Create regex
	regex ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
	cmatch what;

	// Match
	if(regex_match(urlstr.c_str(), what, ex)) {
		result.valid = true;

		// Extract protocol, store as lowercase
		result.protocol = string(what[1].first, what[1].second);
		boost::algorithm::to_lower(result.protocol);

		// Extract other components.
		result.host  = string(what[2].first, what[2].second);
		result.port = std::atoi(string(what[3].first, what[3].second).c_str());
		result.path = string(what[4].first, what[4].second);
		result.query = string(what[5].first, what[5].second);
	}
	return result;
}

static bool seperate_header_and_data(response_t &response)
{
	size_t pos = response.data.find("\r\n\r\n");
	if(pos == string::npos) return false;

	response.header = response.data.substr(0, pos);
	response.data = response.data.substr(pos + 4);

	return true;
}

static response_t do_request(const url_t &url, boost::asio::streambuf &buf)
{
	// Create TCP resolver & endpoint
	boost::asio::io_service io;
	tcp::resolver resolver(io);
	tcp::resolver::query q(url.host, url.protocol);
	tcp::resolver::iterator endpoint = resolver.resolve(q);

	response_t response;
	if(url.protocol == "http") {
		tcp::socket socket(io);
		boost::asio::connect(socket, endpoint);

		// Transmit
		boost::asio::write(socket, buf);

		// Read the response & return
		read_all(socket, response);
	} else if(url.protocol == "https") {
		// Init SSL
		ssl::context ctx(ssl::context::sslv23);
		ctx.set_default_verify_paths();

		// Create socket & endpoint
		ssl::stream<tcp::socket> ssl_socket(io, ctx);
		boost::asio::connect(ssl_socket.lowest_layer(), endpoint);
		ssl_socket.lowest_layer().set_option(tcp::no_delay(true));

		// Verify certificate
		ssl_socket.set_verify_mode(ssl::verify_peer);
		ssl_socket.set_verify_callback(ssl::rfc2818_verification(url.host));
		try {
			ssl_socket.handshake(ssl::stream<tcp::socket>::client);
		} catch (boost::system::system_error &err) {
			response.error = "Could not perform SSL handshake: " + string(err.what());
			return response;
		}

		// Transmit
		boost::asio::write(ssl_socket, buf);

		// Read the response & return
		read_all(ssl_socket, response);
	} else {
		// Unsupported protocol!
		response.error = "Unsupported protocol '" + url.protocol + "'";
	}
	seperate_header_and_data(response);
	return response;
}

response_t get(const string &url_str, const string &custom_header)
{
	// Parse URL
	url_t url = parse_url(url_str);

	// Create the buffer for our HTTP request.
	boost::asio::streambuf buf;
	ostream request(&buf);

	// Build the request
	request << "GET " << url.path << "?" << url.query << " HTTP/1.0\r\n";
	request << "Host: " << url.host << "\r\n";
	request << "Accept: */*\r\n";

	// Add custom headers
	if(!custom_header.empty()) {
		request << custom_header;
		if(!boost::ends_with(custom_header, "\r\n")) {
			request << "\r\n";
		}
	}
	request << "Connection: close\r\n\r\n";

	return do_request(url, buf);
}

response_t post(const std::string &url_str, const std::string post_data,
		const std::string &custom_header)
{
	// Parse URL
	url_t url = parse_url(url_str);

	// Create the buffer for our HTTP request.
	boost::asio::streambuf buf;
	ostream request(&buf);

	// Build the request
	request << "POST " << url.path + url.query << " HTTP/1.1\r\n";
	request << "Host: " + url.host + "\r\n";
	request << "Content-Length: " << post_data.size() << "\r\n";
	request << "Accept: */*\r\n";

	// Add custom headers
	if(!custom_header.empty()) {
		request << custom_header;
		if(!boost::ends_with(custom_header, "\r\n")) {
			request << "\r\n";
		}
	}
	request << "Connection: close\r\n\r\n";

	// Add Post data
	request << post_data;

	return do_request(url, buf);
}

}
