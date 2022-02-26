#ifndef MUDUO_NET_HTTP_CLIENT_H
#define MUDUO_NET_HTTP_CLIENT_H

#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include "HttpContext.h"

namespace muduo
{
namespace net
{
namespace http
{

class HttpClient
{
public:
	typedef std::function<void(const HttpRequest&, const HttpResponse&)> ResponseCallback;
	HttpClient(EventLoop* loop, const InetAddress& address)
		: client_(loop, address, "http-client")
	{
		client_.setConnectionCallback(std::bind(&HttpClient::onConnection, this, _1));
		client_.setMessageCallback(std::bind(&HttpClient::onMessage, this, _1, _2, _3));
	}

	void connect()
	{
		client_.connect();
	}

	void send(const HttpRequest& request, const ResponseCallback& responseCb)
	{
		TcpConnectionPtr conn = weakConn_.lock();
		HttpContextPtr context = boost::any_cast<HttpContextPtr>(conn->getContext());
		HttpRequest& req = context->getRequest();
		req = request;
		Buffer outBuff;
		context->parser(&outBuff);
		responseCb_ = responseCb;
		conn->send(&outBuff);
	}

private:
	void onConnection(const TcpConnectionPtr& conn);
	void onMessage(const TcpConnectionPtr& conn, Buffer* buff, Timestamp);

	TcpClient client_;
	std::weak_ptr<TcpConnection> weakConn_;
	ResponseCallback responseCb_;
};

} // namespace http
} // namespace net
} // namespace muduo

#endif
