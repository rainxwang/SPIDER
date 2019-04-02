#include "threads.h"
#include "spider.h"
#include "configparser.h"


//主函数1个；DNS解析一个；最多的maxjobnum，个线程

//当前线程个数
int g_cur_thread_num = 0;
//用于个给线程上锁
pthread_mutex_t gctn_lock = PTHREAD_MUTEX_INITIALIZER;

int create_thread(void*(* start_func)(void *), void *args, pthread_t * pid, pthread_attr_t * pattr)
{
    pthread_attr_t attr;//线程的属性
    pthread_t pt;//传出参数保存创建线程的pid

    if(pattr == NULL)//如果传入的时候没有指定用哪个线程属性，这里设置默认的
    {
        pattr = &attr;
        pthread_attr_init(pattr);
        pthread_attr_setstacksize(pattr, 1024*1024);//设置线程的堆栈大小
        pthread_attr_setdetachstate(pattr, PTHREAD_CREATE_DETACHED);//设置线程分离,自己运行结束了就自动回收资源，不需要其他的线程等待 
    }

    if(pid == NULL)
        pid = &pt;

    int rv = pthread_create(pid, pattr, start_func, args);
    pthread_attr_destroy(pattr);
    
    return rv;
}
void begin_thread()
{
    SPIDER_LOG(SPIDER_LEVEL_DEBUG, "Begin Thread %lu", pthread_self());
}
void end_thread()
{
    //上面设置的线程已经是分离的了，所以这里的end只是用来提示一下结束信息，
    //如果事件数max_job_num - g_cur_thread_num那么就可以重新添加线程；
    //如果线程数已经达到了上线，那么就等其他线程结束来处理
    //epoll_task的，发送请求，注册epoll事件，重复epoll_wait的结果返回线程执行任务。
    pthread_mutex_lock(&gctn_lock);
    //创建可能执行的任务数
    int left = g_conf->max_job_num - (-- g_cur_thread_num);
    if(left == 1)
    {
        attach_epoll_task();
    }
    else if(left > 1)
    {
        /* 可以开启两个线程*/
        attach_epoll_task();
        attach_epoll_task();
    }
    else
    {
        //线程数不够用，不用开始epoll任务
    }
    
    //打印当前线程ID， 剩余的任务数
    SPIDER_LOG(SPIDER_LEVEL_DEBUG, "End Thread %lu, cur_thread_num = %d", pthread_self(), g_cur_thread_num);
    pthread_mutex_unlock(&gctn_lock);
}

