#include "url.h"
#include "dso.h"

static queue<Surl *> surl_queue;
static queue<Url *> ourl_queue;

//新来的url的ip地址可以用url和ip关联起来，这样方便查找
static map<string, string> host_to_ip;
static Url *surl2ourl(Surl *url);
static void dns_callback(int result, char type, int count, int ttl, void *address, void *arg);
static int is_bin_url(char *url);//判断url对应的结果是不是二进制
static int surl_precheck(Surl *surl);//检查surl的结果是不是正确
static void get_timespec(timespec * ts, int millisecond);//得到时间用于等待

pthread_mutex_t oq_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sq_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t oq_lock = PTHREAD_COND_INITIALIZER;
pthread_cond_t sq_lock = PTHREAD_COND_INITIALIZER;


//对外的接口
//将surl解析成ourl加入队列里面.死循环，只要启动，就一直解析
void * urlparser(void *none)//原始url解析
{
	 Surl *url = NULL;
	 Url *ourl = NULL;
	 map<string, string>::const_iterator itr;

	 while (1)
	 {
		 pthread_mutex_lock(&sq_lock);//解析前先把surlqueue锁住
		 while (surl_queue.empty())
		 {
			//如果surl里面什么都没有，下面的操作也是浪费
			//信号满足时解除阻塞，并重新lock
			pthread_cond_wait(&sq_cond, &sq_lock);
		 }
		 url = surl_queue.front();//取出头部的surl
		 surl_queue.pop();
		 pthread_mutex_unlock(&sq_lock);//使用完释放
		 ourl = surl2ourl(url);//把surl转为ourl
		
		 //libevent解析
		 itr = host_ip_map.find(ourl->domain);//先查看一下之前的hostip能不能找找到
		 if (itr == host_ip_map.end())
		 {
			 event_base *base = event_init();//libevent初始化
			 evdns_init();
			 //参数1：想解析的域名，参数2：falgs可以0或禁用搜索查询
			 //参数3：解析完成后的回调,参数4：传给回调函数的参数
			 evdns_resolve_ipv4(ourl->domain, 0, dns_callback, ourl);//对执行的域名执行callback函数
			 event_dispatch();
			 event_base_free(base);
		 }
		 else
		 {
			 //如果之前的map里面有解析过得，那么直接拷贝
			 ourl->ip = strdup(iter->second.c_str());
			 push_ourlqueue(ourl);//加入队列
		 }
	 }
	 return NULL;
}
 
int extract_url(regex_t *re, char *str, Url *domain)//提取页面中的新的url
{
	 const size_t nmatch = 2;
	 regmatch_t matchptr[nmatch];
	 int len;
	 char *p = str;
	 while (regexec(re, p, nmatch, matchptr, 0) != REG_NOMATCH)
	 {
		 len = (matchptr[1].rm_eo - matchptr[1].rm_eo);
        p = p + matchptr[1].rm_so;
        char *tmp = (char*)calloc(len+1, 1);
        strncpy(tmp, p, len);
       tmp[len] = '\0';
        p = p + len + (matchptr[0].rm_eo - matchptr[1].rm_eo);

         //排除二进制
         if(is_bin_url(tmp))
         {
             free(tmp);
             continue;
          }
            
         char *url = attach_domain(tmp, our->domain);
         if(url != NULL)
         {
             SPIDER_LOG(SPIDER_LEVEL_DEBUG, "I find a url %s", url);
             Surl * surl = (Surl*)malloc(sizeof(Surl));
             surl->level = ourl->level + 1;
             surl->type = TYPE_HTML;
			//下一级的url需要先规范化
             if((surl->url = url_normalized(ul)) == NULL)
             {
                 SPIDER_LOG(SPIDER_LEVEL_WARN, "Normalize url fail");
                 free(surl);
                 continue;
            }
			//url需要判断有没有先被抓取
             if(iscrawled(url->url))
             {
                SPIDER_LOG(SPIDER_LEVEL_DEBUG, "I seen this url:%s", surl->url);
                free(surl->url);
                free(surl);
                continue;
            }
            else
            {
                 push_surlqueue（surl):
            }

        }

	 }
	 return (p - str);
 }

 void push_surlqueue(Surl *url)//添加原始队列,内部接口实现删除
 {
     if(url != NULL && surl_precheck(url))
	 {
		 SPIDER_LOG(SPIDER_LEVEL_DEBUG, "I want this url %s", url->url);
		 pthread_mutex_lock(&sq_lock);
		 surl_queue.push(url);
		 //发送信号给另外一个阻塞在等待状态的线程，使得线程脱离等待
		 if(surl_queue.size() == 1)
		 	pthread_cond_signal(&sq_cond);//如果添加进来的有了那么给刚刚解析阻塞的那个发送一个信号
		pthread_mutex_unlock(&sq_lock);
	 }

 }


Url * pop_ourlqueue()//
{
	 Url *url = NULL;
	 pthread_mutex_lock(&oq_lock);
	 if(!ourl_queue.empty())
	 {
		 url = ourl_queue.front();
		 ourl_queue.pop();
		 pthread_mutex_unlock(&oq_lock);
		 return url;
	 }
	 else
	 {
		 int trynum = 3;
		 struct timespec timeout;
		 while(trynum -- && ourl_queue.empty())
		 {
			 get_timespec(&timeout, 500);//0.5s的延迟
			 pthread_cond_timedwait(&oq_cond, &oq_lock, &timeout);
		 }

		 if(!ourl_queue.empty())
		 {
			 url = ourl_queue.front();
			 ourl_queue.pop();
		 }
		 return url;
	 }
}

//内部存在ourlqueue和surlqueue队列，外部通过下面接口访问
int is_ourlqueue_empty()
{
	pthread_mutex_lock(&oq_lock);
	int val = ourl_queue.empty();
	pthread_mutex_unlock(&oq_lock);
	return val;
}
int is_surlqueue_empty()
{
	 pthread_mutex_lock(&sq_lock);
	 int val = surl_queue.empty();
	 pthread_mutex_unlock(&sq_lock);
	 return val;
}
int get_ourlqueue_size()
{
	 return ourl_queue.size();
}
 int get_surlqueue_size()
{
	 return surl_queue.size();
}

//判断是不是一个二进制文件的url
static chat BIN_SUFFIXES = ".jpg.jpeg.gif.png.ico.bmp.swf";
static int is_bin_url(char *url)
{
	char *p = NULL;
	if((p == strrchr(url, '.')) != NULL)
	{
		if(strstr(BIN_SUFFIXS, p) == NULL)
			return 0;
		else
			return 1;
	}
	return 0;
}
//extract  url里面用他补充域名
char * attach_domain(char *url, const char *domain)//检测域名是否完整
{
	if(url == NULL)
		return NULL;
	
	if(strncmp(url, "http", 4) == 0)
		return url;
	else if(*url == '/')
	{
		int i;
		int ulen = strlen(url);
		int dlen = strlen(domain);
		char *tmp  = (char *)malloc(url+dlen+1);
		for(i = 0; i < dlen; i++)
			tmp[i] = domain[i];
		for(i = 0; i < ulen; i++)
			tmp[i+dlen] = url[i];
		tmp[ulen + dlen] = '\0';
		free(url);
		return tmp;
	}
	else
	{
		free(url);
		return NULL;
	}
}

//url写到文件名
char * url2fn(const Url * url)
{
	int i = 0;
	int l1 = strlen(url->domain);
	int l2 = strlen(url->path);
	char *fn = (char *)malloc(l1+l1+2);
	if(i = 0; i < l1; i++)
		fn[i] = url->domain[i];
	fn[l1++] = '_';
	for(int i = 0; i < l2; i++)
		fn[l1+i] = (url->path[i] == '/' ? '_' : url->path[i]);
	
	fn[l1+l2] = '\0';

	return fn;
}

char * url_normalized(char *url)//url切割
{
	 if(url == NULL) return NULL;

	 /*retrim url*/
	 int len = strlen(url);
	 while(len && isspace(url[len-1]))
	 	len --;
	url[len] = '\0';

	if(len == 0)
	{
		free(url);
		return NULL;
	}

	//移除https://  正好0-7
	if(en > 7 && strncmp(url, "http", 4) == 0)
	{
		int vlen = 7;
		if(url[4] == 's')//如果下标4为s那么长度就是8
			vlen++;
		len -= vlen;//最终真正的长度应该减去开头的长度

		char *tmp = (char *) malloc(len + 1);
		strncpy(tmp, url+vlen, len);//从参数2开始，拷贝len个，并让最后一位是\0
		tmp[len] = '\0';
		free(url);
		url = tmp;
	}

	//如果url的后面有/
	if(url[len - 1] == '/')
		url[--len] = '\0';

	if(len > MAX_LINK_LEN)
	{
		free(url);
		return NULL;
	}

	return url;
}

extern void free_url(Url *ourl)//释放url
{
	 free(ourl->domain);
	 free(ourl->ip);
	 free(ourl):
}


//主要将原始的url进行切割，分理处域名和路径，端口等，存放入解析后的url结构
Url *surl2ourl(Surl *url)
{
//calloc分配后初始化为0，malloc分配后是垃圾数据
	Url *ourl = (Url *) calloc(1, sizeof(Url));
	//查找url里面首次出现'/'的位置
	char *p = strchr(surl->url, '/');
	if(p == NULL)
	{
		ourl->domain = surl->url;//直接就是域名，不是资源目录
		ourl->path = surl->url + strlen(surl->url);//路径是空的
	}
	else
	{
		*p = '\0';//以第一次出现的那个/进行分割
		ourl->domain = surl->url;//提取域名
		ourl->path = p+1;//路径就是后面的那些东西
	}

	//查找端口出现的位置
	p = strchr(ourl->domain, ':');
	if(p != NULL)
	{
		*p = '\0';
		ourl->port = atoi(p+1);
		if(ourl->port == 0)
			ourl->port = 80;
	}
	else
	{
		ourl->port = 80;
	}
	//level
	ourl->level = surl->level;
	return ourl;
}

//想通过DNS解析，那么libevent需要提供一个回调
static void dns_callback(int result, char type, int count, int ttl, void *address, void *arg)
{
	Url * ourl = (Url *)arg;
	struct in_addr * addrs = (in_addr *)address;

	if(result != DNS_ERR_NONE || count == 0)
	{
		SPIDER_LOG(SPIDER_LEVEL_WARN, "Dns resolve fail %s", ourl->domain);
	}
	else
	{
		char *ip = inet_ntoa(addrs[0]);//网络字节序转化为主机字节序
		SPIDER_LOG(SPIDER_LEVEL_DEBUG, "Dns resolve ok: %s - > %s", ourl->domain, ip);
		host_ip_map[ourl->domain] = strdup(ip);//把域名-ip放到map里面
		ourl->ip = ip;
		push_ourlqueue(ourl);//把解析后的url加入队列
	}
	event_loopexit(NULL);
}
	
int iscrawed(char * url)
{
	return 0;
}