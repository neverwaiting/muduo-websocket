#ifndef MUDUO_NET_WEBSOCKET_CLIENT_H
#define MUDUO_NET_WEBSOCKET_CLIENT_H

#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/base/Logging.h>
#include "TcpContext.h"
#include "base64.h"
#include "sha1.h"

namespace muduo
{
namespace net
{
namespace websocket
{

class Context
{
private:
	std::shared_ptr<WebsocketContext> wsContext_;
	WebsocketConnectionPtr wsConn_;
	bool handshaked_ = false;
};

class WebsocketClient
{
public:
	typedef std::function<void(const WebsocketConnectionPtr&)> ConnectionCallback;
	typedef std::function<void(const WebsocketConnectionPtr&,const std::string&)> TextMessageCallback;
	typedef std::function<void(const WebsocketConnectionPtr&,const char*,size_t)> BinaryMessageCallback;

	WebsocketClient(EventLoop* loop, const InetAddress& address, const std::string& name)
		: tcpClient_(loop, address, name)
	{
		tcpClient_.setConnectionCallback(
				std::bind(&WebsocketClient::onConnection, this, _1));
		tcpClient_.setMessageCallback(
				std::bind(&WebsocketClient::onMessage, this, _1, _2, _3));
	}

	void setConnectionCallback(const ConnectionCallback& cb)
	{
		connCb_ = cb;
	}
	void setTextMessageCallback(const TextMessageCallback& cb)
	{
		textMsgCb_ = cb;
	}
	void setBinaryMessageCallback(const BinaryMessageCallback& cb)
	{
		binMsgCb_ = cb;
	}

	void connect()
	{
		tcpClient_.connect();
	}

private:
	typedef std::shared_ptr<TcpConnContext> TcpConnContextPtr;

	void onConnection(const TcpConnectionPtr& conn);

	void onMessage(const TcpConnectionPtr& conn, Buffer* buff, Timestamp receiveTime);

	void sendHandshakeRequest(const TcpConnectionPtr& conn)
	{
		Buffer buff;
		buff.append("GET / HTTP/1.1\r\n"
								"Connection: Upgrade\r\n"
								"Upgrade: websocket\r\n"
								"Sec-WebSocket-Version: 13\r\n"
								"Sec-WebSocket-Key: ");
		key_ = getSecWebsocketKey();
		buff.append(key_ + "\r\n\r\n");
		conn->send(&buff);
	}

	bool validHandleShakeResponse(const http::HttpResponse& res)
	{
		return res.getVersionString() == "HTTP/1.1" &&
					 res.getStatus() == HTTP_STATUS_SWITCHING_PROTOCOLS &&
					 res.getHeader("Connection") == "Upgrade" &&
					 res.getHeader("Upgrade") == "websocket" &&
					 res.getHeader("Sec-WebSocket-Accept") == getSecWebsocketAccept(key_);
	}

	TcpClient tcpClient_;
	ConnectionCallback connCb_;
	TextMessageCallback textMsgCb_;
	BinaryMessageCallback binMsgCb_;
	std::string key_;

	int waitHandShakeResponseTime_ = 20;
};

} // namespace websocket
} // namespace net
} // namespace muduo

#endif
