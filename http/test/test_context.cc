#include "../HttpContext.h"
#include <iostream>

int main()
{
	muduo::net::http::HttpContext context(HTTP_REQUEST);
	
	muduo::net::Buffer buff;
	buff.append("GET / HTTP/1.1\r\n\r\n");
	assert(context.parser(&buff));
	assert(context.ready());

	muduo::net::http::HttpRequest& request = context.getRequest();
	std::cout << request.getMethodString() << std::endl;
	std::cout << request.getUrl() << std::endl;
	std::cout << request.getVersionString() << std::endl;

	muduo::net::Buffer outBuff;
	muduo::net::http::HttpResponse& response = context.getResponse();
	response.setStatus(HTTP_STATUS_OK);
	response.appendHeader("Connection", "Close");
	response.appendHeader("Server", "muduo-http-server");
	response.setVersion(1, 1);
	response.setBody("hello world", 11);
	context.package(&outBuff);
	std::cout << std::string(outBuff.peek(), outBuff.readableBytes()) << std::endl;

	
	muduo::net::http::HttpContext context1(HTTP_RESPONSE);
	assert(context1.parser(&outBuff));
	assert(context1.ready());

	muduo::net::http::HttpResponse& response1 = context.getResponse();
	std::cout << response1.getVersionString() << std::endl;
	std::cout << response1.getStatus() << std::endl;
	std::cout << response1.getStatusMessage() << std::endl;
	for (const auto& header : response1.getHeaders())
		std::cout << header.first << ": " << header.second << std::endl;

	std::cout << response1.body() << std::endl;
}
