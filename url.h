#ifndef QURL_H
#define QURL_H


typedef struct Surl
{
	char *url;//原始的url
	int level;//深度
	int type;//类型，是html还是image
} Surl;

typedef struct Url
{
	char *domain;//域名
	char *path;//路径
	int port;//解析后的端口
	char *ip;//解析后的ip
	int level;//深度
} Url;//解析后的url

typedef struct evso_arg
{
	int fd;//将url通信的的套接字和url进行关联
	Url *url;
} evso_arg;


//对外的接口
extern void * urlparser(void *arg);//原始url解析
extern int extract_url(regext_t *re, char *str, Url *domain);//提取页面中的新的url

extern void push_surlqueue(Surl *url);//添加原始队列,内部接口实现删除
extern Url * pop_ourlqueue();//

//内部存在ourlqueue和surlqueue队列，外部通过下面接口访问
extern int is_ourlqueue_empty();
extern int is_surlqueue_empty();
extern int get_ourlqueue_size();
extern int get_surlqueue_size();

extern char * attach_domain(char *url, const char *domain);//检测域名是否完整
extern int iscrawled(char * url);//判断有没有被抓取过
extern char * url2fn(const Url * url);//url句柄关联
extern char * url_normalied(char *url);//url切割

extern void free_url(Url *ourl);//释放url

#endif