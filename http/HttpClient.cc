#include "HttpClient.h"

namespace muduo
{
namespace net
{
namespace http
{

void HttpClient::onConnection(const TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		conn->setContext(std::make_shared<HttpContext>(HTTP_RESPONSE));
		weakConn_ = conn;
	}
	else
	{
		conn->setContext(NULL);
	}
}

void HttpClient::onMessage(const TcpConnectionPtr& conn, Buffer* buff, Timestamp)
{
	HttpContextPtr context = boost::any_cast<HttpContextPtr>(conn->getContext());
	if (!context->parser(buff))
	{
		printf("close connection\n");
		conn->forceClose();
	}
	else
	{
		if (context->ready())
		{
			const HttpRequest& req = context->getRequest();
			const HttpResponse& res = context->getResponse();
			if (responseCb_)
			{
				responseCb_(req, res);
			}
			if (res.getHeader("Connection") == "Close")
			{
				conn->forceClose();
			}
			context->reset();
		}
	}
}

} // namespace http
} // namespace net
} // namespace muduo
