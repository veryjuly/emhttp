//
#include <stdio.h>
#include <string.h>
#include "emhttp.h"
#include <time.h>
#ifdef _WIN32
#include <WinSock2.h>
#pragma warning(disable:4996)
#else
#include <sys/socket.h>

#endif


int webapi_get_html(void * arg, int s, char * method, char * url, char * query)
{

	char * html = 
		"<HTML><HEAD><TITLE>Get html demo\r\n "
		"</TITLE></HEAD>\r\n"
		"<BODY><P>This is return from C langvage.\r\n"
		"</BODY></HTML>\r\n";;
	discardheaders(s);
	headers(s, "text/html");
	send(s, html, strlen(html), 0);

	return 0;
}


int webapi_http_exit(void * arg, int s, char * method, char * url, char * query)
{
	char * html =
		"<HTML><HEAD><TITLE>http exited\r\n "
		"</TITLE></HEAD>\r\n"
		"<BODY><P>the http server is shutdown.\r\n"
		"</BODY></HTML>\r\n";;
	discardheaders(s);
	headers(s, "text/html");
	send(s, html, strlen(html), 0);
	http_exit();//ÍË³öhttpserver;
	return 0;
}

int webapi_get_json(void * arg, int s, char * method, char * url, char * query)
{
	char strtime[64]="";
	time_t timer = time(NULL);
	strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", localtime(&timer));
	char buf[1024];
	char * json = "{\"app\":\"emhttp\",\"ver\":0.1,\"time\":\"%s\"}";
	sprintf(buf, json, strtime);
	discardheaders(s);
	headers(s, "application/json");
	send(s, buf, strlen(buf), 0);

	return 0;
}


int main(int argc , char ** argv)
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);// ³õÊ¼»¯socket
#endif
	set_router("/gethtml.wpi", webapi_get_html, NULL);
	set_router("/getjson.wpi", webapi_get_json, NULL);
	set_router("/httpexit.wpi", webapi_http_exit, NULL);

	int unsigned short port = 7077;
	http_main(&port);

#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}



