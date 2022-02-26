#include "WebsocketServer.h"

namespace muduo
{
namespace net
{
namespace websocket
{

WebsocketServer::WebsocketServer(EventLoop* loop, const InetAddress& addr, const std::string name)
		: tcpServer_(loop, addr, name)
{
	tcpServer_.setConnectionCallback(
			std::bind(&WebsocketServer::onConnection, this, _1));
	tcpServer_.setMessageCallback(
			std::bind(&WebsocketServer::onMessage, this, _1, _2, _3));
}
void WebsocketServer::onConnection(const TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		conn->setTcpNoDelay(true);
		// 设置waitConnTime_ s 内没发websocket握手请求自动断开连接
		TimerId timerId = conn->getLoop()->runAfter(waitConnTime_,
				std::bind(&TcpConnection::forceClose, conn.get()));
		conn->setContext(std::make_shared<TcpConnContext>(timerId, HTTP_REQUEST));
	}
	else
	{
		TcpConnContextPtr context = 
			boost::any_cast<TcpConnContextPtr>(conn->getContext());
		if (!context->handshaked())
		{
			LOG_INFO << waitConnTime_ << " s 之内没有收到websocket握手请求，自动断开该连接";
		}
		else
		{
			WebsocketConnectionPtr wsConn = context->getWebsocketConnection();
			if (wsConn)
			{
				wsConn->setConnected(false);
				if (connCb_)
					connCb_(wsConn);
			}
		}
		conn->getLoop()->cancel(context->timerId());
		conn->setContext(NULL);
	}
}

void WebsocketServer::onMessage(const TcpConnectionPtr& conn, Buffer* buff, Timestamp receiveTime)
{
	TcpConnContextPtr context = 
		boost::any_cast<TcpConnContextPtr>(conn->getContext());
	if (!context->handshaked())
	{
		std::shared_ptr<http::HttpContext> ctx = context->httpContext();
		if (!ctx->parser(buff))
		{
			conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
			conn->forceClose();
		}
		if (ctx->ready())
		{
			if (onHandleShake(conn, ctx->getRequest()))
			{
				conn->getLoop()->cancel(context->timerId());
				context->setHandshaked();
				// 关联websocketconnection, 设置pingInterval_ s后发ping
				TimerId timerId = conn->getLoop()->runAfter(pingInterval_,
						std::bind(&WebsocketServer::sendPingTimer, this, conn));
				context->setTimerId(timerId);
				context->setWebsocketConnection(conn);
				if (connCb_)
					connCb_(context->getWebsocketConnection());
			}
		}
	}
	else
	{
		//printRawData(buff);
		std::shared_ptr<websocket::WebsocketContext> wsCtx = context->wsContext();
		if (!wsCtx->decode(buff))
		{
			LOG_INFO << "无效的websocket message";
			conn->forceClose();
		}
		else
		{
			if (wsCtx->ready())
			{
				websocket::WebsocketData& reqData = wsCtx->requestData();
				uint8_t opcode = reqData.opcode();
				if (opcode == websocket::frame::opcode::PING)
				{
					Buffer buff;
					websocket::WebsocketContext::getPongFrame(&buff);
					conn->send(&buff);
				}
				else if (opcode == websocket::frame::opcode::PONG || 
								 opcode == websocket::frame::opcode::TEXT || 
								 opcode == websocket::frame::opcode::BINARY)
				{
					// 重置ping定时器
					conn->getLoop()->cancel(context->timerId());
					TimerId timerId = conn->getLoop()->runAfter(pingInterval_,
							std::bind(&WebsocketServer::sendPingTimer, this, conn));
					context->setTimerId(timerId);

					if (opcode == websocket::frame::opcode::TEXT && textMsgCb_) 
					{
						textMsgCb_(context->getWebsocketConnection(), 
											 std::string(reqData.payload().peek(), reqData.payload().readableBytes()));
					}

					if (opcode == websocket::frame::opcode::BINARY && binMsgCb_) 
					{
						binMsgCb_(context->getWebsocketConnection(),
											reqData.payload().peek(), reqData.payload().readableBytes());
					}
				}
				else if (opcode == websocket::frame::opcode::CLOSE)
				{
					conn->forceClose();
				}
				wsCtx->reset();
			}
		}
	}
	return;
}

void WebsocketServer::sendPingTimer(const TcpConnectionPtr& conn)
{
	TcpConnContextPtr context = 
		boost::any_cast<TcpConnContextPtr>(conn->getContext());

	Buffer buff;
	websocket::WebsocketContext::getPingFrame(&buff);
	// waitPongTime_ s 之内如果没有接收到pong，则断开连接
	TimerId timerId = conn->getLoop()->runAfter(waitPongTime_,
			std::bind(&WebsocketServer::responsePingTimer, this, conn));
	context->setTimerId(timerId);
	conn->send(&buff);
}

bool WebsocketServer::onHandleShake(const TcpConnectionPtr& conn, const http::HttpRequest& req)
{
	bool handshake = false;
	if (isHandShakeRequest(req))
	{
		Buffer buf;
		responseHandShake(&buf, req.getHeader("Sec-WebSocket-Key"));
		conn->send(&buf);
		handshake = true;
	}
	else
	{
		conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
		conn->forceClose();
	}
	return handshake;
}

bool WebsocketServer::isHandShakeRequest(const http::HttpRequest& req)
{
	return req.getMethod() == HTTP_GET &&
				req.getVersionString() == "HTTP/1.1" &&
				req.getHeader("Upgrade") == "websocket" &&
				req.getHeader("Connection") == "Upgrade" &&
				req.getHeader("Sec-WebSocket-Version") == "13" &&
				!req.getHeader("Sec-WebSocket-Key").empty();
}

void WebsocketServer::responseHandShake(Buffer* buff, const std::string& websocketKey)
{
	buff->append("HTTP/1.1 101 Switching Protocols\r\n"
							 "Upgrade: websocket\r\n"
							 "Connection: Upgrade\r\n"
							 "Sec-WebSocket-Version: 13\r\n");
	buff->append("Sec-WebSocket-Accept: " + getSecWebsocketAccept(websocketKey) + "\r\n\r\n");
}

} // namespace websocket
} // namespace net
} // namespace muduo
