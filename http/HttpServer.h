#ifndef MUDUO_NET_HTTP_SERVER_H
#define MUDUO_NET_HTTP_SERVER_H

#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include "HttpContext.h"

namespace muduo
{
namespace net
{
namespace http
{

class HttpServer
{
public:
	typedef std::function<void(const HttpRequest&, HttpResponse*)> RequestCallback;
	typedef std::map<std::string, RequestCallback> RequestCallbackList;
	HttpServer(EventLoop* loop, const InetAddress& address)
		: server_(loop, address, "http-server")
	{
		server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this, _1));
		server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, _1, _2, _3));
	}

	void setThreadNum(int nthreads)
	{
		server_.setThreadNum(nthreads);
	}
	void start()
	{
		server_.start();
	}
	
	void setHttpRequestCallback(const RequestCallback& cb)
	{
		cb_ = cb;
	}

	void Get(const std::string& url, const RequestCallback& cb)
	{
		getRequestCbs_[url] = cb;
	}
	void Post(const std::string& url, const RequestCallback& cb)
	{
		postRequestCbs_[url] = cb;
	}
	//void Put(const RequestCallback& cb);
	//void Delete(const RequestCallback& cb);
	//void Head(const RequestCallback& cb);

private:
	void onConnection(const TcpConnectionPtr& conn);
	void onMessage(const TcpConnectionPtr& conn, Buffer* buff, Timestamp);

	TcpServer server_;
	RequestCallback cb_;
	RequestCallbackList getRequestCbs_;
	RequestCallbackList postRequestCbs_;
};

} // namespace http
} // namespace net
} // namespace muduo

#endif
