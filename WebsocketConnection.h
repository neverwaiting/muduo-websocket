#ifndef MUDUO_NET_WEBSOCKET_CONNECTION_H
#define MUDUO_NET_WEBSOCKET_CONNECTION_H

#include <muduo/net/TcpConnection.h>
#include "WebsocketContext.h"

namespace muduo
{
namespace net
{
namespace websocket
{

class WebsocketConnection
{
public:
	WebsocketConnection(const TcpConnectionPtr& conn)
		: tcpConn_(conn), connected_(true)
	{
	}

	void sendText(const std::string& data, bool mask = false)
	{
		Buffer payload;
		payload.append(&*data.begin(), data.size());
		Buffer buff;
		WebsocketContext::getFrame(&buff, websocket::frame::opcode::TEXT, mask, &payload);
		tcpConn_->send(&buff);
	}

	void sendBinary(const char* data, size_t len, bool mask = false)
	{
		Buffer payload;
		payload.append(data, len);
		Buffer buff;
		websocket::WebsocketContext::getFrame(&buff, websocket::frame::opcode::BINARY, mask, &payload);
		tcpConn_->send(&buff);
	}

	void forceClose()
	{
		tcpConn_->forceClose();
	}

	void setConnected(bool connected)
	{
		connected_ = connected;
	}

	bool connected() const
	{
		return connected_;
	}

	EventLoop* getLoop() const
	{
		return tcpConn_->getLoop();
	}

  void setContext(const boost::any& context)
  { context_ = context; }

  const boost::any& getContext() const
  { return context_; }

  boost::any* getMutableContext()
  { return &context_; }

private:
	TcpConnectionPtr tcpConn_;
	bool connected_;
	boost::any context_;
};

typedef std::shared_ptr<WebsocketConnection> WebsocketConnectionPtr;

} // namespace websocket
} // namespace net
} // namespace muduo

#endif
