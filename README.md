# SPIDER

1. 这是一个爬虫系统，适用于新手这种没项目，可以用来练手，又可以用巩固操作系统，网络编程等知识
2. 主要采用的是C语言风格，从main函数一步步往下走，没有封装，继承等概念
3. 这个项目是玩玩全全仿照传智播客的一个爬虫项目，同时在理解他们这些源码的同时加上了注释

主要用到的知识：
- linux的线程使用方法
- 守护进程的创建
- libevent库来解析DNS
- epoll对事件的监听
- 如何调用动态库
- socket编程
- 正则表达式解析页面
- 文件系统
- 各种字符串的处理方法
...

## 主处理流程
爬虫相当于是一个客户端请求浏览器的资源，再进行保存，保存的页面又可以解析出新的页面，直到达到截断条件。
1. 获取命令行参数
	类似于./app [option]给出相应的option操作
2. 读取配置文件
	系统是用来爬虫的，所以一开始的种子，调用的模块名，等信息需要读取出来
3. 载入配置文件中的模块，进行相应的设置
4. 种子加入队列，扔到一个线程中进行DNS解析
5. 创建epoll任务，
	取出一个url，创建socket通信。将这个socket加入epoll里面进行关注
6. epoll_wait返回活跃的时间，每一个活跃的事件开启一个线程进行页面处理
	在这个线程里面，会提取页面的url。url如果是新的，那么就会在结束的时候调用epoll任务，使得这个新的url可以被epoll关注
7. 新的url能加入了，那么主要的就是循环第6步

核心的数据结构体和函数接口提前想好：

![](https://raw.githubusercontent.com/wangxinyu1997/SPIDER/master/spider%E6%B5%81%E7%A8%8B%E5%9B%BE.png)


## 配置文件解析confparser.h
	typedef struct Config
	{
		int 	max_job_num;//最大的任务数
		char* 	seeds;//原始种子
		char*	logfile;//日志文件名称
		int 	log_level;//日志的等级
		int 	max_depth;//每个url的最大深度
		char*	module_path;//模块的路径
		vector<char*>	modules;//哪些模块可以被加载
		vector<char*>	accept_types;//保存的时候可以支持的类型
	}
	
	extern Config* initconfig();//配置文件的全局变量初始化
	extern void loadconfig(Config* conf);//读取配置文件

## 动态模块的载入 dso.h
(1）结构清晰、易于理解。由于借鉴了硬件总线的结构，而且各个插件之间是相互独立的，所以结构非常清晰也更容易理解。
(2）易修改、可维护性强。由于插件与宿主程序之间通过接口联系，就像硬件插卡一样，可以被随时删除，插入和修改，所以结构很灵活，容易修改，方便软件的升级和维护。
(3）可移植性强、重用力度大。因为插件本身就是由一系列小的功能结构组成，而且通过接口向外部提供自己的服务，所以复用力度更大，移植也更加方便。
(4）结构容易调整。系统功能的增加或减少，只需相应的增删插件，而不影响整个体系结构，因此能方便的实现结构调整。：
(5）插件之间的耦合度较低。由于插件通过与宿主程序通信来实现插件与插件，插件与宿主程序间的通信，所以插件之间的耦合度更低。
(6）可以在软件开发的过程中修改应用程序。由于采用了插件的结构，可以在软件的开发过程中随时修改插件，也可以在应用程序发行之后，通过补丁包的形式增删插件，通过这种形式达到修改应用程序的目的。
(7）灵活多变的软件开发方式。可以根据资源的实际情况来调整开发的方式，资源充足可以开发所有的插件，资源不充足可以选择开发部分插件，也可以请第三方的厂商开发，用户也可以根据自己的需要进行开发。 

对于一个cpp，怎么载入呢？

使用时在dlopen（）函数以指定模式打开指定的动态链接库文件，并返回一个句柄给dlsym（）的调用进程。使用dlclose（）来卸载打开的库 
### dlopen
功能：打开一个动态链接库
包含头文件： dlfcn.h
函数定义： void * dlopen( const char * pathname, int mode );
函数描述： 在dlopen的（）函数以指定模式打开指定的动态连接库文件，并返回一个句柄给调用进程。使用dlclose（）来卸载打开的库。
　　mode：模式有下面这些
　　RTLD_LAZY 暂缓决定，等有需要时再解出符号
　　RTLD_NOW 立即决定，返回前解除所有未决定的符号。
　　RTLD_LOCAL
　　RTLD_GLOBAL 允许导出符号
　　RTLD_GROUP
　　RTLD_WORLD
返回值: 打开错误返回NULL ，成功，返回库引用 ，编译时候要加入 -ldl (指定dl库) 

### dlsym
功能：根据动态链接库操作句柄与符号，返回符号对应的地址。
包含头文件：dlfcn.h
函数定义：void\*dlsym(void\* handle,const char* symbol)
函数描述：dlsym根据动态链接库操作句柄(handle)与符号(symbol)，返回符号对应的地址。使用这个函数不但可以获取函数地址，也可以获取变量地址。handle是由dlopen打开动态链接库后返回的指针，symbol就是要求获取的函数或全局变量的名称。 

### dlclose
dlclose用于关闭指定句柄的动态链接库，只有当此动态链接库的使用计数为0时,才会真正被系统卸载。

假设在my.so中定义了一个void mytest()函数，那在使用my.so时先声明一个函数指针： 
void(\*pMytest)(); 
接下来先将那个my.so载入： 
pHandle=dlopen("my.so",RTLD_LAZY);//详见dlopen函数 然后使用dlsym函数将函数指针 pMytest 指向 mytest() 函数：
pMytest=(void(\*)())
dlsym(pHandle,"mytest");//可见放在双引号中的mytest不用加括号,即使有参数也不用 （可调用dlerror();返回错误信息，正确返回为空）！

### saveimage,savehtml,maxdepth等模块的设计
这样在模块设计的时候，在模块中给出相应的结构体，就能通过这个结构体去访问内部的函数，这就要求这个结构体中有指向内部函数的指针。
比如：savehtml模块
通过动态库的调用，**可以访问saveimage这个结构体**，通过结构体中的内容就可以访问具体的函数：

	static int handler()
	{
	}
	static void init()
	{
	}
	Module saveimage = 
	{
		//版本信息变量
		init,
		handler
	};

### dso.h的设计
dso.h是用来管理不同模块进行加载的，//无论什么模块都考虑因该有以下的信息

	typedef struct Module
	{
		int version;
		int minor_version;
		const char* name;
		void (*init)(Module*);//每个模块都有init初始化函数，这样动态载入的时候可以调用这个模块的函数进行初始化
		int (*handle)(void *);//这个就相当于每个模块都有这么一个入口，
	} Module;
	
	//主流程需要调用的载入操作
	extern Modules* do_load(const char* path, const char* name);

管理不同的模块，也需要将这些模块进行分类一下，方便后续的调用，比如处理html就把结构体都放在处理html的vector中，处理http头的就放在http头的vector中。
那么在配置文件中写了一个模块名，应该怎么加载到对应的vector中呢，这步操作，在do_load里面实现，对于每个模块，里面定义init函数（里面就是下面对于的这些宏），do_load的时候就init一下。

	//处理url的模块
	extern vector<Module*> modules_pre_surl;
	//载入这些url相关模块需要的初始化操作
	#define SPIDER_ADD_MODULE_PRE_SURL(module) do { \
		modules_pre_surl.push_back(module); \
	} while(0)
	
	extern vector<Module*> modules_post_html;
	#define MODULES_POST_HTML(module) do { \
		modules_post_html.push_back(module); \
	} while(0)
	
	
	extern vector<Module*> modules_post_header;
	#define MODULES_POST_HEADER(module) do { \
		modules_post_header.push_back(module); \
	} while(0)

## url管理模块url.h
url管理是核心，保存原始的url域名，和DNS解析成处理后的url。

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
	1、在attach_epoll_task中，需要创建一个event事件，然后在主流程中wait它，
	wait到这个事件响应，会创建一个线程，线程执行recv_response，这个recv_response需要处理页面，需要知道url的信息。
	2、所以可以将fd和url绑定在一起，当epoll_wait响应之后可以得到这个结构，并将这个结构当做参数传递给recv_response
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
	

## 通信用的socket.h
爬虫是向服务器请求资源，所以应该有个socket用于通信，里面把一些功能增强一下，用于代表浏览器向服务器请求资源

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

## 线程的设计threads.h

	//对原先pthread——create进行封装，创建一个线程 执行strat_routine函数指针
	extern int create_thread(void*(*start_routine)(void *), void *args, pthread_t * thread, pthread_attr_t * pAttr);
	extern void begin_thread();//打印线程 开始的相关信息
	extern void end_thread();//结束线程，在结束一个线程的同时可以尝试访问能不能在epoll_task中咋加入一个套接字进行监听
