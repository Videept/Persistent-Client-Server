#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <system_error>
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <cctype>
#include <stddef.h>

#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <errno.h>

namespace http
{
	inline int getLastError()
    {
        return errno;
    }

    class Socket final
    {
    public:
        Socket():
            endpoint(socket(PF_INET, SOCK_STREAM, IPPROTO_TCP))
        {

            if (endpoint == -1)
                throw std::system_error(errno, std::system_category(), "Failed to create socket");
        }

        Socket(int s):
            endpoint(s)
        {
        }
        ~Socket()
        {
            if (endpoint != -1) close(endpoint);
        }

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        Socket(Socket&& other):
            endpoint(other.endpoint)
        {

            other.endpoint = -1;
        }

        Socket& operator=(Socket&& other)
        {
            if (&other != this)
            {

                if (endpoint != -1) close(endpoint);


                endpoint = other.endpoint;


                other.endpoint = -1;

            }

            return *this;
        }

        operator int() const { return endpoint; }

    private:
        int endpoint = -1;
    };

    inline std::string urlEncode(const std::string& str)
    {
        static const char hexChars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

        std::string result;

        for (auto i = str.begin(); i != str.end(); ++i)
        {
            uint8_t cp = *i & 0xFF;

            if ((cp >= 0x30 && cp <= 0x39) || // 0-9
                (cp >= 0x41 && cp <= 0x5A) || // A-Z
                (cp >= 0x61 && cp <= 0x7A) || // a-z
                cp == 0x2D || cp == 0x2E || cp == 0x5F) // - . _
                result += static_cast<char>(cp);
            else if (cp <= 0x7F) // length = 1
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
            else if ((cp >> 5) == 0x6) // length = 2
            {
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
            }
            else if ((cp >> 4) == 0xe) // length = 3
            {
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
            }
            else if ((cp >> 3) == 0x1e) // length = 4
            {
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
            }
        }
        
        return result;
    }

    struct Response
    {
        int code = 0;
        std::vector<std::string> headers;
        std::vector<uint8_t> body;
    };

    class Request
    {
    public:
        Request(const std::string& url)
        {
            size_t protocolEndPosition = url.find("://");

            if (protocolEndPosition != std::string::npos)
            {
                protocol = url.substr(0, protocolEndPosition);
                std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);

                std::string::size_type pathPosition = url.find('/', protocolEndPosition + 3);

                if (pathPosition == std::string::npos)
                {
                    domain = url.substr(protocolEndPosition + 3);
                    path = "/";
                }
                else
                {
                    domain = url.substr(protocolEndPosition + 3, pathPosition - protocolEndPosition - 3);
                    path = url.substr(pathPosition);
                }

                std::string::size_type portPosition = domain.find(':');

                if (portPosition != std::string::npos)
                {
                    port = domain.substr(portPosition + 1);
                    domain.resize(portPosition);
                }
            }
        }

        Response send(const std::string& method,
                      const std::map<std::string, std::string>& parameters,
                      const std::vector<std::string>& headers = {})
        {
            std::string body;
            bool first = true;

            for (const auto& parameter : parameters)
            {
                if (!first) body += "&";
                first = false;

                body += urlEncode(parameter.first) + "=" + urlEncode(parameter.second);
            }

            return send(method, body, headers);
        }

        Response send(const std::string& method = "GET",
                      const std::string& body = "",
                      const std::vector<std::string>& headers = {})
        {
            Response response;

            if (protocol != "http")
                throw std::runtime_error("Only HTTP protocol is supported");

            Socket socket;

            addrinfo* info;
            if (getaddrinfo(domain.c_str(), port.empty() ? "80" : port.c_str(), nullptr, &info) != 0)
                throw std::system_error(getLastError(), std::system_category(), "Failed to get address info of " + domain);

            sockaddr addr = *info->ai_addr;

            freeaddrinfo(info);

            if (::connect(socket, &addr, sizeof(addr)) < 0)
                throw std::system_error(getLastError(), std::system_category(), "Failed to connect to " + domain + ":" + port);

            std::string requestData = method + " " + path + " HTTP/1.1\r\n";

            for (const std::string& header : headers)
                requestData += header + "\r\n";

            requestData += "Host: " + domain + "\r\n";
            requestData += "Content-Length: " + std::to_string(body.size()) + "\r\n";

            requestData += "\r\n";
            requestData += body;

            int flags = MSG_NOSIGNAL;

            ssize_t remaining = static_cast<ssize_t>(requestData.size());
            ssize_t sent = 0;
            ssize_t size;

            do
            {
                size = ::send(socket, requestData.data() + sent, static_cast<size_t>(remaining), flags);

                if (size < 0)
                    throw std::system_error(getLastError(), std::system_category(), "Failed to send data to " + domain + ":" + port);

                remaining -= size;
                sent += size;
            }
            while (remaining > 0);

            uint8_t TEMP_BUFFER[65536];
            const std::vector<uint8_t> clrf = {'\r', '\n'};
            std::vector<uint8_t> responseData;
            bool firstLine = true;
            bool parsedHeaders = false;
            int contentSize = -1;
            bool chunkedResponse = false;
            size_t expectedChunkSize = 0;
            bool removeCLRFAfterChunk = false;

            do
            {
                size = recv(socket, reinterpret_cast<char*>(TEMP_BUFFER), sizeof(TEMP_BUFFER), flags);

                if (size < 0)
                    throw std::system_error(getLastError(), std::system_category(), "Failed to read data from " + domain + ":" + port);
                else if (size == 0)
                {


                    // disconnected
                    break;
                }

                responseData.insert(responseData.end(), std::begin(TEMP_BUFFER), std::begin(TEMP_BUFFER) + size);

                if (!parsedHeaders)
                {
                    for (;;)
                    {
                        auto i = std::search(responseData.begin(), responseData.end(), clrf.begin(), clrf.end());

                        // didn't find a newline
                        if (i == responseData.end()) break;

                        std::string line(responseData.begin(), i);
                        responseData.erase(responseData.begin(), i + 2);

                        // empty line indicates the end of the header section
                        if (line.empty())
                        {
                            parsedHeaders = true;
                            break;
                        }
                        else if (firstLine) // first line
                        {
                            firstLine = false;

                            std::string::size_type pos, lastPos = 0, length = line.length();
                            std::vector<std::string> parts;

                            // tokenize first line
                            while (lastPos < length + 1)
                            {
                                pos = line.find(' ', lastPos);
                                if (pos == std::string::npos) pos = length;

                                if (pos != lastPos)
                                    parts.push_back(std::string(line.data() + lastPos,
                                                                static_cast<std::vector<std::string>::size_type>(pos) - lastPos));
                                
                                lastPos = pos + 1;
                            }

                            if (parts.size() >= 2)
                                response.code = std::stoi(parts[1]);
                        }
                        else // headers
                        {
                            response.headers.push_back(line);

                            std::string::size_type pos = line.find(':');

                            if (pos != std::string::npos)
                            {
                                std::string headerName = line.substr(0, pos);
                                std::string headerValue = line.substr(pos + 1);

                                // ltrim
                                headerValue.erase(headerValue.begin(),
                                                  std::find_if(headerValue.begin(), headerValue.end(),
                                                               std::not1(std::ptr_fun<int, int>(std::isspace))));

                                // rtrim
                                headerValue.erase(std::find_if(headerValue.rbegin(), headerValue.rend(),
                                                               std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
                                                  headerValue.end());

                                if (headerName == "Content-Length")
                                    contentSize = std::stoi(headerValue);
                                else if (headerName == "Transfer-Encoding" && headerValue == "chunked")
                                    chunkedResponse = true;
                            }
                        }
                    }
                }

                if (parsedHeaders)
                {
                    if (chunkedResponse)
                    {
                        bool dataReceived = false;
                        for (;;)
                        {
                            if (expectedChunkSize > 0)
                            {
                                auto toWrite = std::min(expectedChunkSize, responseData.size());
                                response.body.insert(response.body.end(), responseData.begin(), responseData.begin() + static_cast<ptrdiff_t>(toWrite));
                                responseData.erase(responseData.begin(), responseData.begin() + static_cast<ptrdiff_t>(toWrite));
                                expectedChunkSize -= toWrite;

                                if (expectedChunkSize == 0) removeCLRFAfterChunk = true;
                                if (responseData.empty()) break;
                            }
                            else
                            {
                                if (removeCLRFAfterChunk)
                                {
                                    if (responseData.size() >= 2)
                                    {
                                        removeCLRFAfterChunk = false;
                                        responseData.erase(responseData.begin(), responseData.begin() + 2);
                                    }
                                    else break;
                                }

                                auto i = std::search(responseData.begin(), responseData.end(), clrf.begin(), clrf.end());

                                if (i == responseData.end()) break;

                                std::string line(responseData.begin(), i);
                                responseData.erase(responseData.begin(), i + 2);

                                expectedChunkSize = std::stoul(line, 0, 16);

                                if (expectedChunkSize == 0)
                                {
                                    dataReceived = true;
                                    break;
                                }
                            }
                        }

                        if (dataReceived)
                            break;
                    }
                    else
                    {
                        response.body.insert(response.body.end(), responseData.begin(), responseData.end());
                        responseData.clear();

                        // got the whole content
                        if (contentSize == -1 || response.body.size() >= static_cast<size_t>(contentSize))
                            break;
                    }
                }
            }
            while (size > 0);

            return response;
        }

    private:
        std::string protocol;
        std::string domain;
        std::string port;
        std::string path;
    };
}

#endif
