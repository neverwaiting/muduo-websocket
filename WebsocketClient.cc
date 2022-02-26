#include "WebsocketClient.h"

namespace muduo
{
namespace net
{
namespace websocket
{

void WebsocketClient::onConnection(const TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		conn->setTcpNoDelay(true);
		sendHandshakeRequest(conn);
		// 设置waitHandShakeResponseTime_ s 内必须接收到握手响应
		TimerId timerId = conn->getLoop()->runAfter(waitHandShakeResponseTime_,
																								std::bind(&TcpConnection::forceClose, conn.get()));
		conn->setContext(std::make_shared<TcpConnContext>(timerId, HTTP_RESPONSE));
	}
	else
	{
		TcpConnContextPtr context = 
			boost::any_cast<TcpConnContextPtr>(conn->getContext());
		if (!context->handshaked())
		{
			LOG_INFO << waitHandShakeResponseTime_ 
							 << " s 之内没有收到websocket握手响应，自动断开该连接";
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

void WebsocketClient::onMessage(const TcpConnectionPtr& conn, Buffer* buff, Timestamp receiveTime)
{
	TcpConnContextPtr context = 
		boost::any_cast<TcpConnContextPtr>(conn->getContext());
	if (!context->handshaked())
	{
		std::shared_ptr<http::HttpContext> ctx = context->httpContext();
		if (!ctx->parser(buff))
		{
			conn->forceClose();
		}
		if (ctx->ready())
		{
			if (validHandleShakeResponse(ctx->getResponse()))
			{
				conn->getLoop()->cancel(context->timerId());
				context->setHandshaked();
				// 关联websocketconnection 
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
				else if (opcode == websocket::frame::opcode::PONG)
				{
					// do nothing
				}
				else if (opcode == websocket::frame::opcode::CLOSE)
				{
					conn->forceClose();
				}
				else if (opcode == websocket::frame::opcode::TEXT)
				{
					if (textMsgCb_) 
					{
						textMsgCb_(context->getWebsocketConnection(), 
											 std::string(reqData.payload().peek(), reqData.payload().readableBytes()));
					}
				}
				else if (opcode == websocket::frame::opcode::BINARY)
				{
					if (binMsgCb_)
					{
						binMsgCb_(context->getWebsocketConnection(),
											reqData.payload().peek(), reqData.payload().readableBytes());
					}
				}
				wsCtx->reset();
			}
		}
	}
	return;
}

} // namespace websocket
} // namespace net
} // namespace muduo
