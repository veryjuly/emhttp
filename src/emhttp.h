



//****************************************************************************//
//*http主函数,本函数不会反回,如果程序还要做别的事情,请在新线程中调用
//****************************************************************************//
int http_main(unsigned short * port);

//****************************************************************************//
//*处理完当前请求后不再接受请求,退出主循环
//****************************************************************************//
void http_exit();

//****************************************************************************//
//*处理某一个url的请求的函数定义
//*参数: arg		:由路由设置函数传入的参数
//*		 s			:客户端套接字
//*		 method		:请求方法
//*		 url		:要处理的url,如 /test.wpi
//*		 query		:地址栏?号之后的内容
//****************************************************************************//
typedef int(*ROUTER_FUNC)(void * arg, int s, char * method, char * url, char * query);

//****************************************************************************//
//*设置一个函数与处理某一个url的请求
//*参数: url		:要处理的url,如 /test.wpi
//		 router_func:处理函数,参数所返回值一定要同ROUTER_FUNC的定义
//		 arg		:要传给处理函数的参数,处理函数被调用时会由调用方传入
//****************************************************************************//
void set_router(char * url, ROUTER_FUNC router_func, void * arg);

/**********************************************************************/
/* 发送HTTP headers,并指定mime类型. */
/* 参数: client 客户套接字 */
/* 参数: mime  mime类型 */
/**********************************************************************/
void headers(int client, const char * mime);

/**********************************************************************/
/*给客户端发送数据前需要先读取完客户发送的数据,这样客户端才能开始接收数据*/
/**********************************************************************/
void discardheaders(int client);