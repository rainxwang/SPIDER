#include <fcntl.h>
#include "url.h"
#include "socket.h"
#include "threads.h"
#include "qstring.h"
#include "dso.h"


//正则表达式的pattern
static const char * HREF_PATTERN = "href=\"\\s*\\([^ >\"]*\\)\\s*\"";
//把header转化为headerobject
static Header * parse_header(char *header);

//调用模块检查header
static int header_postcheck(Header *header);

//链接：ip,port传入
int build_connect(int *fd, char *ip, int port)
{
    struct sockaddr_in serv_addr;//用来存放ip和端口号，
    bezero(&server_addr, sizeof(struct sockaddr_in));

    serv_addr.sin_family= AF_INET;
    serv_addr.sin_port = htnos(port);//主机字节序转为网络字节序

    if(!inet_aton(ip, &(serv_addr.sin_addr)))
        return 1;//吧ip放在sin_addr中
    //创建socket进行通信
    if((*fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    
    if(connect(*fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0)
    {
        close(*fd);
        return -1;
    }
	
	SPIDER_LOG(SPIDER_LEVEL_DEBUG, "connect success %s", ip);
    return 0;
}
//向socket里面写数据就代表请求服务器的过程
//也就是socket用于套接字通信
int send_request(int fd, void *arg)
{
    int need, begin, n;
    chat request[1024] = {0};
    Url *url = (Url *)arg;

    sprintf(request, "GET /%s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Accept: */*\r\n"
        "Connection: Keep-Alive\r\n"
        "User-Agent: Mozilla/5.0 (compatible; Qteqpidspider/1.0;)\r\n"
        "Referer: %s\r\n\r\n", url->path, url->domain, url->domain);
    
    need = strlen(request);
    begin = 0;

    //循环发送头
    while(need)
    {
        n = write(fd, request+begin, need);//n返回写入的长度
        //fd是非阻塞的，这样容易在读的时候没数据直接返回EAGAIN，这个时候可以稍微等一下，重试一次，
        if (n <= 0)
        {
            if(errno == EAGAIN)
            {
                usleep(1000);
                continue;
            }
            SPIDER_LOG(SPIDER_LEVEL_WARN, "Thread %lu send ERROR: %d", pthread_self(), n);
            free_url(url);
            close(fd);
            return -1;
        }
        begin += n;//下次write的起始点
        need -=n;//直到发送完了
    }

    return 0;
}
//设置非堵塞
void set_nonblocking(int fd)
{
    int flag;
    if((flag = fctl(fd, F_GETL)) < 0)
        SPIDER_LOG(SPIDER_LEVEL_ERROR, "fcntl getfl fail");
    flag |= 0_NONBLOCK;

    if((flag = fctl(fd, F_SETFL, flag)) < 0)
        SPIDER_LOG(SPIDER_LEVEL_ERROR, "fcntl getfl fail");
}

//得到返回的数据
#define HTML_MAXLEN 1024*1024
void * recv_response(void *arg)
{
    begin_thread();

    int i, n, trunc_head = 0, len = 0;
    chat * body_ptr = NULL;
    evso_arg *narg = (evso_arg *) arg;
    Response *resp = (Response *)malloc(sizeof(Response));
    resp->header = NULL;
    resp->body = (char *) malloc (HTML_MAXLEN);
    resp->body_len = 0;
    resp->url = narg->url;

    //regex_t 是一个结构提，存放编译后的正则
	//成员re_nsub是存放子正则表达式的个数
    regex_t re;
    //int regcomp(regex_t *complied, const char* pattern, int cflags)
    if ((regcomp, &re, HREF_PATTERN, 0) != 0)//正则需要匹配第一步是变异成re
        SPIDER_LOG(SPIDER_LEVEL_ERROR, "compile regex error");

    SPIDER_LOG(SPIDER_LEVEL_INFO, "Crawling url:%s/%s", narg->url->domain, narg->url->path);

    while(1)
    {
        //将读到的数据放入response,第一次读的时候可能会把整个响应头给读进来了
        n = read(narg->fd, resp->body+len, 1024);
        if(n < 0)
        {
            if(errno == EGAIN || errno == EWOULDBLOCK || errno = EINTR)
            {
                usleep(10000);
                continue;
            }
            SPIDER_LOG(SPIDER_LEVEL_WARN, "Read socket fail: %s", strerror(errno));
            break;
        }
        else if(n == 0)
        {
            //代表数据读完了
            resp->body_len = len;
            if(resp->body_len > 0)
            {
                //根据正则提取body里面的url
                extract_url(&re, resp->body, narg->url);
            }

            //处理响应体
            for(int i = 0; i < (int)MODULES_POST_HTML.size(); i++)
                modules_post_html[i]->handle(resp);//保存html模块
            break;
        }
        else
        {
            //接收的数据既有头，又有内存，所以先把响应头返回
            len +=n;//更新已经读取的长度
            resp->body[len] = '\0';

            if(!trunc_head)//用来标记这个头部有没有被提前去处理
            {
                if((body_ptr = strstr(resp->body, "\r\n\r\n")) != NULL)
                {
                    *(body_ptr + 2) = '\0';
                    resp->header = parse_header(resp->body);//将header组成header对象
                    if(!header_postcheck(resp->header))//解析状态码还有类型
                    {
                        SPIDER_LOG(SPIDER_LEVEL_WARN, "go to leave");
                        goto leave;
                    }

                    trunc_head = 1;//如果第一次读取已经取出头部，这里就设置为1,下次就不会在解析头了

                    //找到了\r\n\r\n的出现位置后，还需要吧后面的内容都存进来
                    body_ptr += 4;
                    for(i = 0; *body_ptr; i++)
                    {
                        resp->body[i] = *body_ptr;
                        body_ptr++;
                    }
                    resp->body[i] = '\0';
                    len = i;
                }
                continue;
            }
        }
        
        
    }


leave:
    close(narg->fd);
    free_url(narg->url);
    regfree(&re);

    //free 响应体
    free(resp->header->content_type);
    free(resp->header);
    free(resp->body);
    free(resp);

    end_thread();
    return NULL;
}


//使用模块对头部进行检查
static int header_postcheck(Header *header)
{
    unsigned int i;
    for(int i = 0; i < modules_post_header.size(); i++)
    {
        if(modules_post_header[i] -> handle(header) != MOdule_ok)
            return 0;
    }
    return 1;
}

//在接收函数中，用于解析HTTP的头部
static Header * parse_header(char *header)
{
    int c = 0;
    char *p = NULL;
    char **sps = NULL;
    char *start = header;
	
    Header *h = (Header *)calloc(1, sizeof(Header));
//找到第一次出现\r\n的地方,\r\n只占用一个字节
    if(( p =strstr(start, "\r\n")) != NULL)
    {
        *p = '\0';
		//按空格分割3次
        sps = strsplit(start, ' ', &c, 2);
        if(c == 3)
            h->status_code = atoi(sps[1]);//保存状态码
        else
        {
            h->status_code = 600;//设置自己的错误码
        }
        start = p+2;
    }

    if(( p = strstr(start, "\r\n")) != NULL)
    {
        *p = '\0';
        sps = strsplit(start, ':', &c, 1);
        if (c == 2)
        {
            if(strcasecmp(sps[0], "content-type") == 0)//查找内容的类型
                h->content_type = strdup(strim(sps[1]));
        }
        start = p+2;
    }
    return h;
}