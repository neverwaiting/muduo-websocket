#include "../WebsocketClient.h"

using namespace muduo::net;
using namespace muduo::net::websocket;

void onConnection(const WebsocketConnectionPtr& wsConn)
{
	if (wsConn->connected())
	{
		LOG_INFO << "open websocket connection successfully";
		wsConn->sendText("hello world", true);
	}
	else
	{
		LOG_INFO << "close websocket connection";
	}
}

void onMessage(const WebsocketConnectionPtr& wsConn, const std::string& text)
{
	LOG_INFO << text;
}

int main()
{
	EventLoop loop;
	WebsocketClient client(&loop, InetAddress("192.168.235.10", 8889), "websocket-client");
	client.setConnectionCallback(onConnection);
	client.setTextMessageCallback(onMessage);
	client.connect();
	loop.loop();
}
