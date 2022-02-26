#include "HttpServer.h"

namespace muduo
{
namespace net
{
namespace http
{

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		conn->setContext(std::make_shared<HttpContext>(HTTP_REQUEST));
	}
	else
	{
		conn->setContext(NULL);
	}
}

void HttpServer::onMessage(const TcpConnectionPtr& conn, Buffer* buff, Timestamp)
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
			HttpResponse& res = context->getResponse();
			if (!req.getHeader("Connection").empty())
				res.appendHeader("Connection", req.getHeader("Connection"));
			else
				res.appendHeader("Connection", "Keep-Alive");
			if (req.getMethod() == HTTP_GET || req.getMethod() == HTTP_POST)
			{
				RequestCallbackList* cbs = 
					req.getMethod() == HTTP_GET ? &getRequestCbs_ : &postRequestCbs_;
				auto it = cbs->find(req.getUrl());
				if (it != cbs->end())
				{
					HttpResponse res;
					res.setStatus(HTTP_STATUS_OK);
					(*cbs)[req.getUrl()](req, &res);
				}
				else if (cb_)
				{
					cb_(req, &res);
				}
				else
				{
					res.setStatus(HTTP_STATUS_NOT_FOUND);
					res.appendHeader("Connection", "Close");
				}
			}
			else
			{
				res.setStatus(HTTP_STATUS_NOT_IMPLEMENTED);
				res.appendHeader("Connection", "Close");
			}

			Buffer outBuff;
			context->package(&outBuff);
			conn->send(&outBuff);
			context->reset();
		}
	}
}

} // namespace http
} // namespace net
} // namespace muduo
