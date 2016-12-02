/*
Copyright(c) 2016, veryjuly
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

*Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and / or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <direct.h>
#pragma warning(disable : 4267)
#pragma comment(lib,"Ws2_32.lib") 
#pragma warning(disable:4996)
#else
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <strings.h> //for strcasecmp
#define closesocket close
#define stricmp strcasecmp
#endif

#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: emhttpd 0.1.0\r\n"

void accept_request(int);
void bad_request(int);
void cannot_execute(int);
void execute_cgi(int, const char *, const char *, const char *);
int  get_line(int, char *, int);
void headers(int, const char *);
void not_found(int,const char * url);
void serve_file(int, const char *);
void unimplemented(int);
void discardheaders(int);
char * get_mime(const char * filename);

/**********************************************************************/
/* 路由结构定义*/
/**********************************************************************/

typedef struct router_t{
	char url[1024];
	int(*router_enter)(void * arg,int s,char * method, char * url, char * query);
	void * arg;
	struct router_t * next;
} ROUTER_T;

static struct sockaddr_in	int_addr;	//服务器端地址端口
static int		server_sock		= -1;	//服务套接字
static char			root_path[1024] = "";	//
static ROUTER_T *	routers			= NULL;	//路由链表指针指
static	int			b_exit			= 0;




void set_router(char * url,void * enter,void * arg)
{
	ROUTER_T * rut = malloc(sizeof(ROUTER_T));
	if (rut != NULL){
		rut->next = routers;
		routers = rut;
		routers->arg = arg;
		strncpy(routers->url, url, 1024);
		routers->router_enter = enter;
	}
}

/**********************************************************************/
/* 接收一个请求时调用此函数
* 目前为单线程处理,此时不能再接收别的请求
* 参数为客户套接字*/
/**********************************************************************/
void accept_request(int client)
{
	char buf[1024*3];
	int  numchars;
	char method[64];
	char url[255];
	char path[1024];

	int i, j;
	struct stat st;
	char *query_string = NULL;
	numchars = get_line(client, buf, sizeof(buf));
	i = 0; j = 0;
	while (j < numchars && !ISspace(buf[j]) && (i < sizeof(method) - 1))
	{
		method[i] = buf[j];
		i++; j++;
	}
	method[i] = '\0';
	if (stricmp(method, "GET") != 0 && stricmp(method, "POST") != 0)
	{
		if (numchars > 0)
			discardheaders(client);
		unimplemented(client);
		closesocket(client);
		return;
	}
	i = 0;
	while (ISspace(buf[j]) && (j < sizeof(buf)))
		j++;
	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))){
		url[i] = buf[j];
		i++; j++;
	}
	url[i] = '\0';
	if (stricmp(method, "GET") == 0)
	{
		query_string = url;
		while ((*query_string != '?') && (*query_string != '\0'))
			query_string++;
		if (*query_string == '?')
		{
			*query_string = '\0';
			query_string++;
		}
	}
	strcpy(path, root_path);
	strcat(path, url);
	if (path[strlen(path) - 1] == '/')
		strcat(path, "index.html");

	ROUTER_T * router = routers;
	while (router != NULL){
		if (strcmp(url, router->url) == 0)
			break;
		router = router->next;
	}
	if (router != NULL){
		router->router_enter(router->arg,client,method,url,query_string);
	}
	else
	{
		//静态文件
		if (stat(path, &st) == -1) {
			not_found(client, url);
		}
		else{
	#ifdef _WIN32
			if ((st.st_mode & S_IFMT) == S_IFDIR)
#else
			if (S_ISDIR(st.st_mode))
#endif
				strncat(path, "/index.html", 1024);	
			serve_file(client, path);
		}
	}
	closesocket(client);
}

/**********************************************************************/
/* 错误的请求 */
/**********************************************************************/
void bad_request(int client)
{
	char * txt = "HTTP/1.0 400 BAD REQUEST\r\n"
		"Content-type: text/html\r\n"
		"\r\n"
		"<P>Your browser sent a bad request, "
		"such as a POST without a Content-Length.\r\n";
	discardheaders(client);
	send(client, txt, strlen(txt), 0);	
}



/**********************************************************************/
/* 从套接字读一行,读到行尾时返回 ,行尾以空结束.
*需要多次读取才能读完客户发送的内容.
* Parameters: the int descriptor
*             the buffer to save the data in
*             the size of the buffer
* 返回值: 读取的字节数 */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;

	while ((i < size - 1) && (c != '\n'))
	{
		n = recv(sock, &c, 1, 0);
		/* DEBUG printf("%02X\n", c); */
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				/* DEBUG printf("%02X\n", c); */
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			buf[i] = c;
			i++;
		}
		else
			c = '\n';
	}
	buf[i] = '\0';
	return(i);
}

//根据文件扩展名返回Content-Type
char * get_mime(const char * filename)
{
	const char * ext_name = "";
	int len = strlen(filename);
	for (int i = len; i > len - 6; i--){
		if (filename[i] == '.'){
			ext_name = filename + i;
			break;
		}
	}
	if (strcmp(ext_name, ".png") == 0){
		return "image/png";
	}
	else if ((strcmp(ext_name, ".jpg") == 0) ||
		(strcmp(ext_name, ".jpeg") == 0))
	{
		return  "image/jpeg";
	}
	else if (strcmp(ext_name, ".gif") == 0){
		return "image/gif";
	}
	else if (strcmp(ext_name, ".js") == 0){
		return "text/javascript";
	}
	else if (strcmp(ext_name, ".json") == 0){
		return "application/Json";
	}
	else if (strcmp(ext_name, ".css") == 0){
		return "text/css";
	}
	else if (strcmp(ext_name, ".zip") == 0){
		return "application/zip";
	}
	else if (strcmp(ext_name, ".txt") == 0){
		return "text/plain";
	}
	else if (strcmp(ext_name, ".html") == 0){
		return "text/html";
	}
	else if (strcmp(ext_name, ".htm") == 0){
		return "text/html";
	}
	else if (strcmp(ext_name, ".ico") == 0){
		return "image/x-icon";
	}
	else{
		return "application/octet-stream";
	}

}

/**********************************************************************/
/* 发送HTTP headers,并指定mime类型. */
/* 参数: client 客户套接字 */
/* 参数: mime  mime类型 */
/**********************************************************************/
void headers(int client, const char * mime)
{
	char buf[1024];
	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	strcat(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: %s\r\n", mime);
	strcat(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* 给客户端返回 404 not found 消息. */
/**********************************************************************/
void not_found(int client,  const char * url)
{
	char buf[1024] = "";
	char * txt =
		"HTTP/1.0 404 NOT FOUND\r\n"
		SERVER_STRING
		"Content-Type: text/html\r\n"
		"\r\n"
		"<HTML><TITLE>Not Found</TITLE>\r\n"
		"<BODY><P>404 NOT FOUND</p>\r\n"
		"your request because the resource specified\r\n"
		"is unavailable or nonexistent.\r\n";
	discardheaders(client);
	send(client, txt, strlen(txt), 0);
	sprintf(buf, "<br>url:%s", url);
	char * txt2 = "</BODY></HTML>\r\n";
	send(client, buf, strlen(buf), 0);
	send(client, txt2, strlen(txt2), 0);
}

/**********************************************************************/
/* 给客户返回一个静态文件.包括头和mime类型,mime类型有扩展名决定.
* Parameters: client	套接字
*             filename	文件名为全路径 */
/**********************************************************************/
void serve_file(int client, const char *filename)
{
	FILE * pf = NULL;
	char buf[1024];
	discardheaders(client);	
	pf = fopen(filename, "rb");
	if (pf == NULL){
		not_found(client,filename);
	}
	else
	{
		char * mime = get_mime(filename);
		headers(client, mime);
		while (1)
		{
			int rsz = fread(buf, 1, 1024, pf);
			if (rsz <= 0)
				break;
			send(client, buf, rsz, 0); //cat(client, resource);
		}
	}
	if (pf != NULL)
		fclose(pf);
}


/**********************************************************************/
/*给客户端发送数据前需要先读取完客户发送的数据,这样客户端才能开始接收数据*/
/**********************************************************************/
void discardheaders(int client)
{
	char buf[1024];
	int numchars = 1;
	while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
		numchars = get_line(client, buf, sizeof(buf));
}



/**********************************************************************/
/*当收到客户请求的方法不支持时 */
/**********************************************************************/
void unimplemented(int client)
{
	char * respone =
		"HTTP/1.0 501 Method Not Implemented\r\n"
		SERVER_STRING
		"Content-Type: text/html\r\n"
		"\r\n"
		"<HTML><HEAD><TITLE>Method Not Implemented\r\n "
		"</TITLE></HEAD>\r\n"
		"<BODY><P>HTTP request method not supported.\r\n"
		"</BODY></HTML>\r\n";
	discardheaders(client);
	send(client, respone, strlen(respone), 0);
}



/**********************************************************************/

int http_main(unsigned short * port)
{
	b_exit = 0;
	if (NULL == getcwd(root_path, 1024)){
		printf("getcwd Error!!!\n");
	}
	strcat(root_path, "/htdocs");

	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock == -1){
		printf("http_sever int create error;\n");
		return -1;
	}
	memset(&int_addr, 0, sizeof(int_addr));
	int_addr.sin_family = AF_INET;
	int_addr.sin_port = htons(*port);
	int_addr.sin_addr.s_addr = INADDR_ANY;// inet_addr("127.0.0.1");
	if (bind(server_sock, (struct sockaddr *)&int_addr, sizeof(int_addr)) < 0){
		printf("http_sever bind error;\n");
		return -1;
	}
	if (listen(server_sock, 5) < 0){
		printf("http_sever listen error;\n");
		return -1;
	}
	int iport = *port;
	printf("http_sever listen port : %d;\n",iport);
	int client_sock = -1;
	struct sockaddr_in client_name;
	int client_name_len = sizeof(client_name);
	while (b_exit == 0)
	{
		client_sock = accept(server_sock,(struct sockaddr *)&client_name,&client_name_len);
		if (client_sock == -1){
			printf("web ui is shutdown;\n");
			break;
		}
		accept_request(client_sock);		// 单线程模式
	}
	closesocket(server_sock);
	return(0);
}

//处理完当前请求后不再接受请求,退出主循环
void http_exit()
{
	b_exit = 1;
	closesocket(server_sock);
}




