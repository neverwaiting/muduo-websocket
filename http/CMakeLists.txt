add_library(wsun_http http_parser.c HttpParser.cc HttpClient.cc HttpServer.cc)

add_executable(test_http_context test/test_context.cc)
target_link_libraries(test_http_context wsun_http muduo_net muduo_base)

add_executable(test_http_server test/test_http_server.cc)
target_link_libraries(test_http_server wsun_http muduo_net muduo_base)
