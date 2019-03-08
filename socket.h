#ifndef QSOCKET_H
#define QSOCKET_H


typedef struct Header
{
	char *content_type;
	int status_num;
} Header;//socket用于通信简化的头

typedef struct Response
{
	Header *header;
	char *body;
	int body_len;
	struct Url *url;//请求页面相关的url
} Response;


//对外的接口
extern int build_connect(int *fd, char *ip, int port);
//向socket里面写数据就代表请求服务器的过程
extern int send_request(int fd, void *arg);
//设置非堵塞
extern void set_nonblocking(int fd);
//得到返回的数据
extern void * recv_response(void *arg);




#endif