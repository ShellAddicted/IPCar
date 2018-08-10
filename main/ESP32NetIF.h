#ifndef ESP32_NET_IF_H
#define ESP32_NET_IF_H

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <map>
#include "esp_log.h"
#include "hwcrypto/sha.h"
#include "mbedtls/base64.h"

const char PAGE_BAD_REQUEST_400[] = "HTTP/1.1 400 BAD REQUEST\r\nContent-Length: 26\r\nContent-type: text/html\r\n\r\n\n<h1>400 - Bad Request</h1>";
const unsigned int PAGE_BAD_REQUEST_400_LEN = strlen(PAGE_BAD_REQUEST_400);

const char PAGE_NOT_FOUND_404[] = "HTTP/1.1 404 NOT FOUND\r\nContent-Length: 25\r\nContent-type: text/html\r\n\r\n\n<h1>404 - NOT FOUND </h1>";
const unsigned int PAGE_NOT_FOUND_404_LEN = strlen(PAGE_NOT_FOUND_404);

const char HEADERS_OK_HTML_200[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: ";
const unsigned int HEADERS_OK_HTML_200_LEN = strlen(HEADERS_OK_HTML_200);

const char HEADERS_OK_JS_200[] = "HTTP/1.1 200 OK\r\nContent-type: text/javascript\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: ";
const unsigned int HEADERS_OK_JS_200_LEN = strlen(HEADERS_OK_JS_200);

const char HEADERS_OK_CSS_200[] = "HTTP/1.1 200 OK\r\nContent-type: text/css\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: ";
const unsigned int HEADERS_OK_CSS_200_LEN = strlen(HEADERS_OK_CSS_200);

const char HEADERS_SW_PROTOCOLS_101[] = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ";
const unsigned int HEADERS_SW_PROTOCOLS_101_LEN = strlen(HEADERS_SW_PROTOCOLS_101);

class ESP32NetIF{
    protected:
    unsigned int port = 80;
	static const char *logTag;

	static void handleClient(void* clientSocket);
	static void serveTask(void* thisptr);
	
    public:
	ESP32NetIF(unsigned int port, void (*handler)(int client, char* data, unsigned int len));
	//static void *handler(int,char*,unsigned int);
	static void (*handler)(int client, char* data, unsigned int len);
	static void handleRequest(int client, char* data, unsigned int len);
	static void handleWS(int client, char* data, unsigned int len, std::string (*handler)(std::string dt, void* param), void* param4handler);
	
	void serveForever();

};

#endif