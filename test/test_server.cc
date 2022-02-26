#include "../WebsocketServer.h"
#include <muduo/base/Timestamp.h>

using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::websocket;

void onConnection(const WebsocketConnectionPtr& wsConn)
{
	if (wsConn->connected())
	{
		LOG_INFO << "open a websocket connection";
		TimerId timer = wsConn->getLoop()->runEvery(1.0, [wsConn]
				{
					wsConn->sendText(Timestamp::now().toFormattedString());
				});
		wsConn->setContext(timer);
	}
	else
	{
		LOG_INFO << "close a websocket connection";
		TimerId timer = boost::any_cast<TimerId>(wsConn->getContext());
		wsConn->getLoop()->cancel(timer);
		wsConn->setContext(NULL);
	}
}

void onMessage(const WebsocketConnectionPtr& wsConn, const std::string& text)
{
	LOG_INFO << text;
	wsConn->sendText("hello world", false);
}

void onBinaryMessage(const WebsocketConnectionPtr& wsConn, const char* data, size_t len)
{
	wsConn->sendBinary("hello world", 11);
}

int main(int argc, char** argv)
{
	int port = atoi(argv[1]);
	int nthreads = atoi(argv[2]);

	EventLoop loop;
	WebsocketServer server(&loop, InetAddress(port), "websocket-server");
	server.setThreadNums(nthreads);
	server.setPingInterval(5);
	server.setWaitPongTime(30);
	server.setConnectionCallback(onConnection);
	server.setTextMessageCallback(onMessage);
	server.setBinaryMessageCallback(onBinaryMessage);
	server.start();

	loop.loop();
}
