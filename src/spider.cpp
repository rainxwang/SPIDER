#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/resource.h>//资源操作
#include <sys/time.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include "spider.h"
#include "qstring.h"
#include "threads.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>



int g_epfd;//创建epoll树时候返回的句柄
Config *g_conf;//配置文件的全局变量

extern int g_cur_thread_num;//这里需要用到这个外部变量，这个变量在其他的文件中包含了；
static int set_nofile(rlim_t limit);//Resouce limit指在一个进程的执行过程中，它所能得到的资源的限制
static void daemonize();
static void stat(int sig);
static int set_ticker(int second);

static void version()
{
	printf("Version: spider 1.0\n");
	exit(1);
}

static void usage()
{
	printf("Usage: ./spider [option]\n"
			"\nOption:\n"
			" -h\t: this help\n"
			" -v\t: print spider version"
			" -d\t: run program as daemon process\n\n");
	exit(1);
}

int main(int argc, char **argv)
{
	struct epoll_event events[10];//创建需要监听的事件组
	int daemonized = 0;
	char ch;
	//1.获取命令行参数int getopt(int argc,char * const argv[ ],const char * optstring);
	while((ch = getopt(argc, (char * const *)argv, "vdf")) != -1)
	{
		switch(ch)
		{
			case 'v':
				version();
				break;
			case 'd':
				daemonized = 1;
				break;
			case 'h':
			case '?':
			default:
				usage();
		}
	}
			
	// 2. 读取配置文件 系统是用来爬虫的，所以一开始的种子，调用的模块名，等信息需要读取出来
	g_conf = initconfig();
	loadconfig(g_conf);
	set_nofile(1024);//设置fd最大能打开的上线是1024
	// 3. 载入配置文件中的模块，进行相应的设置
	vector<char *>::iterator it = g_conf->modules.begin();
	for(; it != g_conf->modules.end(); it++)
	{
		dso_load(g_conf->module_path, *it);
	}
	
	// 4. 种子加入队列，扔到一个线程中进行DNS解析
	if(g_conf->seeds == NULL)
		SPIDER_LOG(SPIDER_LEVEL_ERROR, "we have no seeds");
	else
	{
		int c = 0;
		char **splits = strsplit(g_conf->seeds, ',',&c, 0);
		//分割配置文件中的种子，加入原始队列中
		while(c--)
		{
			Surl *surl = (Surl *)malloc(sizeof(Surl));//url的原始状态
			surl->url = url_normalized(strdup(splits[c]));
			surl->level = 0;
			surl->type = TYPE_HTML;
			if(surl->url != NULL)
				push_surlqueue(surl);
		}
	}
	//设置守护进程
	if(daemonized)
		daemonize();
	//设定目录
	chdir("HOME");
	//DNS解析
	int err = -1;
	//这个线程就一直解析，如果队列空，就暂时不解析.但是线程依然在哪里
	if((err = create_thread(urlparser, NULL, NULL, NULL)) < 0)
		SPIDER_LOG(SPIDER_LEVEL_ERROR, "create urlparser thread fail: %s", strerror(err));
	
	int try_num = 1;
	while(try_num < 8 && is_ourlqueue_empty())
        usleep((10000<<try_num++));//没有解析好就睡眠

    if(try_num >= 8)
		SPIDER_LOG(SPIDER_LEVEL_ERROR, "DNS parser error");
		
	if(g_conf->stat_interval > 0)
	{
		signal(SIGALRM, stat);//注册一个信号捕捉函数，发生信号时，执行stat
		set_ticker(g_conf->stat_interval);
	}
	// 5. 创建epoll任务， 取出一个url，创建socket通信。将这个socket加入epoll里面进行关注
	int ourl_num = 0;
	g_epfd = epoll_create(g_conf->max_job_num);//后面的size会被忽略
	while(ourl_num++ < g_conf->max_job_num)//如果抓取的数量小于最大的job_num
	{//如果epolltask无法执行监听某个fd那么就会被忽略
		if(attach_epoll_task() < 0)
			break;
	}
	//6. while是主循环，返回活跃的时间，每个事件开启一个线程处理
	//线程结束的时候返回一个新的任务
	int n, i;
	while(1)
	{
		n = epoll_wait(g_epfd, events, 10, 2000);//参数的大小，超时时间
		printf("epoll: %d\n", n);
		if(n == -1)
			printf("epoll errno:%s\n", strerror(errno));
		fflush(stdout);
		
		if(n <= 0)
		{
			//ourl队列队列里面已经是空的了，那么主循环就可以退出
			if(g_cur_thread_num <= 0 && is_ourlqueue_empty() && is_surlqueue_empty())
			{
				sleep(1);
					if(g_cur_thread_num <= 0 && is_ourlqueue_empty() && is_surlqueue_empty())
						break;
      }
		}
		
		for(int i = 0; i < n; i++)
		{//返回的不是EPOLLIN读事件，就挂起
			evso_arg * arg = (evso_arg *)(events[i].data.ptr);
			if((events[i].events & EPOLLERR || events[i].events & EPOLLHUP || (!(events[i].events & EPOLLERR )))
			{
				SPIDER_LOG(SPIDER_LEVEL_WARN, "epoll fail, close socket %d", arg->fd);
				close(arg->fd);
				continue;
			}
		//删除,将这个event从epoll树上删除,epolltask的每个线程都变成createthread
		epoll_ctl(g_epfd, EPOLL_CTL_DEL, arg->fd, &events[i]);
		printf("hello epoll:event = %d\n", events[i].events);
		//立即清除缓冲区，写到物理设备上
		fflush(stdout);//清空缓冲区
		//生成一个线程去接受，线程结束的时候调用了endthread会开启新的任务，执行recv_response的回调
		create_thread(recv_response, arg, NULL, NULL);
		}
	}
	
	SPIDER_LOG(SPIDER_LEVEL_DEBUG, "Task down");
	close(g_epfd);
	return 0;
}
	
	
	
static int set_nofile(rlim_t limit)
{
	struct rlimit rl;
	if(getrlimit(RLIMIT_NOFILE, &r1) < 0)
	{
		SPIDER_LOG(SPIDER_LEVEL_WARN, "getrlimit fail");
		return -1;
	}
	if(limit > rl.lim_max)
	{
		SPIDER_LOG(SPIDER_LEVEL_WARN, "limit should not be greater than %lu", rl.rlimit_max);
	}
	rl.rlim_cur = limit;
	if(setrlimit(RLIMIT_NOFILE, &rl) < 0)
	{
		SPIDER_LOG(SPIDER_LEVEL_WARN, "setrlimit fail");
		return -1;
	}
	return 0;
}


static void daemonize()
{
//设置守护进程
//创建子进程，父进程退出，子进程当做会长
	int fd;
	if( fork() != 0) exit(0);
	setsid();
	//守护进程的核心逻辑
	SPIDER_LOG(SPIDER_LEVEL_INFO, "Daemonized...pid = %d", (int)getpid());
	//重定向stdin，stdout,stderr到dev/null
	if((fd = open("/dev/null", O_RDWR, 0)) != -1)
	{
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if(fd > STDERR_FILENO)
			close(fd);
	}
	
}

int attach_epoll_task()
{
	struct epoll_event ev;
	int sock_rv;
	int sockfd;
	Url *ourl = pop_ourlqueue();//从url里面取出url
	if(ourl == NULL)
	{
		SPIDER_LOG(SPIDER_LEVEL_WARN, "pop ourlqueue fail!");
		return -1;
	}
	//得到url后用socket去连接
	if((sock_rv = build_connect(&sockfd, our->ip, ourl->port)) < 0)
	{
		SPIDER_LOG(SPIDER_LEVEL_WARN, "Build connect fail %s", ourl->ip);
		return -1;
	}
	set_nonblocking(sockfd);//设置sockfd非堵塞，函数在socket里面；
	
	//向远程服务器请求资源,sock_rv代表着返请求的正确与否
	if((sock_rv = send_request(sockfd,ourl)) < 0)
	{
		SPIDER_LOG(SPIDER_LEVEL_WARN, "send socket request fail:%s", ourl->ip);
		reurn -1;
	}

//epoll_event{_unit32_t events, epoll_data_t data}
//epoll_data里面可以包含四中类型的数据，void*ptr, 或者int fd, 或者 u32，或者u64
//这里用于通信的sockfd，与对应的ourl进行关联，那么每当epoll响应的时候，就能吧url也取出
	evso_arg *arg = (evso_arg *)calloc(1, sizeof(evso_arg));
	arg->fd = sockfd;
	arg->url = ourl;
	
	ev.data.ptr = arg;
	ev.events = EPOLLIN | EPOLLET;//读事件，并且边沿处触发
	if ((epoll_ctl(g_epfd, EPOLL_CTR_ADD, sockfd, &ev) == 0)
		SPIDER_LOG(SPIDER_LEVEL_DEBUG, "Build connect success ");
	else
	{
		SPIDER_LOG(SPIDER_LEVEL_WARN, "Build connect fail");
		return -1;
	}

	//g_cur_thread_num++维护当前线程的数量，	
	//当前正在关注的事件数++，每个关注的事件都是一个fd，都需要用一个线程去响应
	g_cur_thread_num ++;
	return 0;
}

/*
struct itimerval
{ 
struct timeval it_interval;周期性的时间设置 
struct timeval it_value;次的闹钟时间 
};
struct timeval { 
time_t tv_sec; 秒 
suseconds_t tv_usec 微秒 
};
*/
static int set_ticker(int second)
{
	struct itimerval itimer;
	itimer.it_interval.tv_sec = (long) second;
	itimer.it_interval.tv_usec = 0;
	itimer.it_value.tv_sec = (long) second;
	itimer.it_value.tv_usec = 0;
	return setitimer(ITIMER_REAL, &itimer, NULL);
}


static void stat(int sig)
{
	SPIDER_LOG(SPIDER_LEVEL_DEBUG,
				"cur_thread_num = %d \tsur_num=%d\tourl_num = %d",
				g_cur_thread_num,//当前进程数
				get_surl_queue_size(),
				get_ourl_queue_size());
}
