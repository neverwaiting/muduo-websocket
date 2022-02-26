#ifndef MUDUO_NET_HTTP_CONTEXT_H
#define MUDUO_NET_HTTP_CONTEXT_H

#include "HttpParser.h"

namespace muduo
{
namespace net
{
namespace http
{

class HttpContext
{
public:
	HttpContext(http_parser_type type)
		: type_(type),
			requestParser_(new HttpRequestParser),
			responseParser_(new HttpResponseParser)
	{
		parser_ = (type == HTTP_REQUEST ? 
				static_cast<HttpParser*>(requestParser_.get()) : static_cast<HttpParser*>(responseParser_.get()));
	}
	bool parser(Buffer* buff)
	{
		return parser_->excuteParser(buff);
	}
	bool ready()
	{
		return parser_->complete();
	}

	void reset()
	{
		requestParser_->reset();
		responseParser_->reset();
	}

	HttpRequest& getRequest()
	{
		return requestParser_->getRequest();
	}

	HttpResponse& getResponse()
	{
		return responseParser_->getResponse();
	}

	void package(Buffer* buff)
	{
		HttpParser* parser = (type_ == HTTP_REQUEST ? 
				static_cast<HttpParser*>(responseParser_.get()) : static_cast<HttpParser*>(requestParser_.get()));
		parser->package(buff);
	}

private:
	http_parser_type type_;
	std::shared_ptr<HttpRequestParser> requestParser_;
	std::shared_ptr<HttpResponseParser> responseParser_;
	HttpParser* parser_;
};

typedef std::shared_ptr<HttpContext> HttpContextPtr;

} // namespace http
} // namespace net
} // namespace muduo

#endif
