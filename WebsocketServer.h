#ifndef MUDUO_NET_WEBSOCKET_SERVER_H
#define MUDUO_NET_WEBSOCKET_SERVER_H

#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include "TcpContext.h"
#include "http/HttpMessage.h"
#include "base64.h"
#include "sha1.h"

namespace muduo
{
namespace net
{
namespace websocket
{

class WebsocketServer
{
public:
	typedef std::function<void(const WebsocketConnectionPtr&)> ConnectionCallback;
	typedef std::function<void(const WebsocketConnectionPtr&,const std::string&)> TextMessageCallback;
	typedef std::function<void(const WebsocketConnectionPtr&,const char*,size_t)> BinaryMessageCallback;

	WebsocketServer(EventLoop* loop, const InetAddress& addr, const std::string name);

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

	void start()
	{
		tcpServer_.start();
	}

	void setThreadNums(int nthreads)
	{
		tcpServer_.setThreadNum(nthreads);
	}

	void setWaitSendConnReqTime(int time)
	{
		waitConnTime_ = time;
	}
	void setPingInterval(int time)
	{
		pingInterval_ = time;
	}
	void setWaitPongTime(int time)
	{
		waitPongTime_ = time;
	}

private:
	typedef std::shared_ptr<TcpConnContext> TcpConnContextPtr;
	void onConnection(const TcpConnectionPtr& conn);

	void onMessage(const TcpConnectionPtr& conn, Buffer* buff, Timestamp receiveTime);

	void responsePingTimer(const TcpConnectionPtr& conn)
	{
		conn->forceClose();
	}
	void sendPingTimer(const TcpConnectionPtr& conn);

	bool onHandleShake(const TcpConnectionPtr& conn, const http::HttpRequest& req);

	bool isHandShakeRequest(const http::HttpRequest& req);

	void responseHandShake(Buffer* buff, const std::string& websocketKey);

	TcpServer tcpServer_;
	std::map<std::string, WebsocketConnection> wsConn_;
	ConnectionCallback connCb_;
	TextMessageCallback textMsgCb_;
	BinaryMessageCallback binMsgCb_;

	int waitConnTime_ = 3;
	int pingInterval_ = 10;
	int waitPongTime_ = 4;
};

} // namespace websocket
} // namespace net
} // namespace muduo

#endif
