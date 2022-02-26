#include "../HttpServer.h"

using namespace muduo::net;
using namespace muduo::net::http;

int main(int argc, char** argv)
{
	int port = atoi(argv[1]);
	int nthreads = atoi(argv[2]);
	EventLoop loop;
	HttpServer server(&loop, InetAddress(port));
	server.setThreadNum(nthreads);

	server.Get("/hello", [](const HttpRequest& req, HttpResponse* res)
			{
				res->setBody("hello world");
			});
	server.Get("/wintersun", [](const HttpRequest& req, HttpResponse* res)
			{
				res->setBody("hello world, wintersun");
			});
	server.setHttpRequestCallback([](const HttpRequest& req, HttpResponse* res)
			{
				std::ostringstream oss;
				oss << "<html>"
						<< "<p>Method: " << req.getMethodString()
						<< "</p><p>Url: " << req.getUrl()
						<< "</p><p>Version: " << req.getVersionString()
						<< "</p><p>Headers: </p>";
				for ( const auto& header : req.getHeaders())
				{
					oss << "<p>\t" << header.first << " : " << header.second << "</p>";
				}
				oss << "<p>Body: " << req.body() << "</p></html>";

				res->setBody(oss.str());
				res->setStatus(HTTP_STATUS_OK);
				res->appendHeader("content-type", "text/html");
			});
	server.start();

	loop.loop();
	return 0;
}
