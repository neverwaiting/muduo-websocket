#ifndef MUDUO_NET_TCP_CONTEXT_H
#define MUDUO_NET_TCP_CONTEXT_H

#include <muduo/net/TcpConnection.h>
#include <muduo/net/TimerId.h>
#include "WebsocketConnection.h"
#include "http/HttpContext.h"

namespace muduo
{
namespace net
{
namespace websocket
{

class TcpConnContext
{
public:
	TcpConnContext(TimerId timerId, http_parser_type type)
		: timerId_(timerId), 
			handshaked_(false),
			httpContext_(std::make_shared<http::HttpContext>(type))
	{
	}
	
	TimerId timerId() const
	{
		return timerId_;
	}
	void setTimerId(TimerId timerId)
	{
		timerId_ = timerId;
	}
	void setHandshaked()
	{ 
		handshaked_ = true; 
		httpContext_.reset();
		wsContext_ = std::make_shared<WebsocketContext>();
	}
	bool handshaked() const { return handshaked_; }
	std::shared_ptr<http::HttpContext> httpContext() const
	{
		return httpContext_;
	}
	std::shared_ptr<websocket::WebsocketContext> wsContext() const
	{
		return wsContext_;
	}
	void setWebsocketConnection(const TcpConnectionPtr& conn)
	{
		wsConn_.reset(new WebsocketConnection(conn));
	}

	WebsocketConnectionPtr getWebsocketConnection() const
	{
		return wsConn_;
	}

private:
	TimerId timerId_;
	bool handshaked_;
	std::shared_ptr<http::HttpContext> httpContext_;
	std::shared_ptr<WebsocketContext> wsContext_;
	WebsocketConnectionPtr wsConn_;
};

} // namespace websocket
} // namespace net
} // namespace muduo

#endif
