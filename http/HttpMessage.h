
#ifndef MUDUO_NET_HTTP_MESSAGE_H
#define MUDUO_NET_HTTP_MESSAGE_H

#include <string>
#include <map>
#include "http_parser.h"

namespace muduo
{
namespace net
{
namespace http
{

class HttpMessage
{
public:
	void appendHeader(const std::string& field, const std::string& value)
	{
		headers_[field]= value;
	}
	bool findHeader(const std::string& field)
	{
		return headers_.find(field) != headers_.end();
	}
	std::string getHeader(const std::string& field) const
	{
		auto it = headers_.find(field);
		if (it != headers_.end())
		{
			return it->second;
		}
		else
		{
			return "";
		}
	}
	const std::map<std::string,std::string>& getHeaders() const
	{
		return headers_;
	}

	void setBody(const std::string& body)
	{
		body_ = body;
	}
	void setBody(const char* data, size_t len)
	{
		body_ = std::string(data, len);
	}
	const std::string& body() const
	{
		return body_;
	}

	void setVersion(unsigned short major, unsigned short minor)
	{
		httpVersionMajor_ = major;
		httpVersionMinor_ = minor;
	}
	std::pair<unsigned short,unsigned short> getVersion() const
	{
		return std::make_pair(httpVersionMajor_, httpVersionMinor_);
	}
	std::string getVersionString() const
	{
		char version[9] = {0};
		snprintf(version, 9, "HTTP/%u.%u", httpVersionMajor_, httpVersionMinor_);
		return std::string(version);
	}

	void reset()
	{
		headers_.clear();
		body_.clear();
	}

protected:
	std::map<std::string,std::string> headers_;
	std::string body_;
	unsigned short httpVersionMajor_;
	unsigned short httpVersionMinor_;
};

class HttpRequest : public HttpMessage
{
public:
	void setMethod(http_method method)
	{
		method_ = method;
	}
	http_method getMethod() const
	{
		return method_;
	}
	std::string getMethodString() const
	{
		return http_method_str(method_);
	}
	
	void setUrl(const std::string& url)
	{
		url_ = url;
	}
	void setUrl(const char* url, size_t len)
	{
		setUrl(std::string(url, len));
	}
	std::string getUrl() const
	{
		return url_;
	}
private:
	http_method method_;
	std::string url_;
};

class HttpResponse : public HttpMessage
{
public:
	void setStatus(http_status status)
	{
		status_ = status;
	}
	http_status getStatus() const
	{
		return status_;
	}
	std::string getStatusMessage()
	{
		return http_status_str(status_);
	}
	
private:
	http_status status_;
};

} // namespace http
} // namespace net
} // namespace muduo

#endif
