#ifndef MUDUO_NET_HTTP_PARSER_H
#define MUDUO_NET_HTTP_PARSER_H

#include "HttpMessage.h"
#include <memory>
#include <sstream>
#include <muduo/net/Buffer.h>

namespace muduo
{
namespace net
{
namespace http
{

int on_url(http_parser* p, const char* at, size_t length);
int on_header_field(http_parser* p, const char* at, size_t length);
int on_header_value(http_parser* p, const char* at, size_t length);
int on_body(http_parser* p, const char* at, size_t length);
int on_message_complete(http_parser* p);

class HttpParser
{
public:
	HttpParser()
		: parser_(new http_parser), 
			settings_(new http_parser_settings)
	{
		http_parser_settings_init(settings_.get());
		settings_->on_header_field = on_header_field;
		settings_->on_header_value = on_header_value;
		settings_->on_body = on_body;
		settings_->on_message_complete = on_message_complete;
	}

	void onHeaderField(const char* at, size_t length)
	{
		headFieldTemp_.reset(new std::string(at, length));
	}
	void onHeaderValue(const char* at, size_t length)
	{
		if (headFieldTemp_)
		{
			message_->appendHeader(*headFieldTemp_, std::string(at, length));
			headFieldTemp_.reset();
		}
	}
	void onBody(const char* at, size_t length)
	{
		message_->setBody(at, length);
	}
	void onMessageComplete()
	{
		message_->setVersion(parser_->http_major, parser_->http_minor);
		complete_ = true;
		setLeftInfomations();
	}

	bool excuteParser(Buffer* buff)
	{
		size_t parsed = http_parser_execute(parser_.get(), 
																				settings_.get(), 
																				buff->peek(), 
																				buff->readableBytes());
		if (HTTP_PARSER_ERRNO(parser_.get()) == HPE_OK)
		{
			buff->retrieve(parsed);
			return true;
		}
		return false;
	}

	void package(Buffer* buff)
	{
		packageBeginLine(buff);
		packageHeaderAndBody(buff);
	}

	void reset()
	{
		complete_ = false;
		message_->reset();
		headFieldTemp_.reset();
	}
	bool complete()
	{
		return complete_;
	}

protected:
	void packageHeaderAndBody(Buffer* buff)
	{
		std::ostringstream oss;
		for (const auto& header : message_->getHeaders())
		{
			oss << header.first << ": " << header.second << "\r\n";
		}
		std::string body = message_->body();
		if (!body.empty())
		{
			oss << "Content-Length: " << body.size() << "\r\n";
		}
		oss << "\r\n";
		oss << body;
		buff->append(oss.str());
	}
	virtual void packageBeginLine(Buffer* buff) = 0;
	virtual void setLeftInfomations() = 0;

	std::unique_ptr<http_parser> parser_;
	std::unique_ptr<http_parser_settings> settings_;
	std::unique_ptr<HttpMessage> message_;
	std::unique_ptr<std::string> headFieldTemp_;
	bool complete_ = false;
};

class HttpRequestParser : public HttpParser
{
public:
	HttpRequestParser()
	{
		http_parser_init(parser_.get(), HTTP_REQUEST);
		parser_->data = this;
		settings_->on_url = on_url;
		message_.reset(new HttpRequest);
	}

	virtual void packageBeginLine(Buffer* buff) override
	{
		HttpRequest* request = static_cast<HttpRequest*>(message_.get());
		std::ostringstream oss;
		oss << request->getMethodString() << " " 
				<< request->getUrl() << " "
				<< request->getVersionString() << "\r\n";
		buff->append(oss.str());
	}

	void onUrl(const char* at, size_t length)
	{
		HttpRequest* request = static_cast<HttpRequest*>(message_.get());
		request->setUrl(at, length);
	}

	virtual void setLeftInfomations() override
	{
		HttpRequest* request = static_cast<HttpRequest*>(message_.get());
		request->setMethod(static_cast<http_method>(parser_->method));
	}

	HttpRequest& getRequest()
	{
		return *static_cast<HttpRequest*>(message_.get());
	}

};

class HttpResponseParser : public HttpParser
{
public:
	HttpResponseParser()
	{
		http_parser_init(parser_.get(), HTTP_RESPONSE);
		parser_->data = this;
		message_.reset(new HttpResponse);
	}

	virtual void packageBeginLine(Buffer* buff) override
	{
		HttpResponse* response = static_cast<HttpResponse*>(message_.get());
		std::ostringstream oss;
		oss << response->getVersionString() << " " 
				<< response->getStatus() << " "
				<< response->getStatusMessage() << "\r\n";
		buff->append(oss.str());
	}

	virtual void setLeftInfomations() override
	{
		HttpResponse* response = static_cast<HttpResponse*>(message_.get());
		response->setStatus(static_cast<http_status>(parser_->status_code));
	}

	HttpResponse& getResponse()
	{
		return *static_cast<HttpResponse*>(message_.get());
	}
};

} // namespace http
} // namespace net
} // namespace muduo

#endif
