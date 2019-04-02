#ifndef SPIDER_H
#define SPIDER_H
#include <stdarg.h>
#include <vector>
#include "socket.h"
#include "threads.h"
#include "dso.h"
#include "configparser.h"
#include "url.h"

#define MAX_MSG_LEN 1024
#define SPIDER_LEVEL_DEBUG 0
#define SPIDER_LEVEL_INFO 1
#define SPIDER_LEVEL_WARN 2
#define SPIDER_LEVEL_ERROR 3
#define SPIDER_LEVEL_CRIT 4

static const char *LOG_STR[] =
{
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"CRIT"
}

extern Config *g_conf;

//可变参数 
//输出日期，时间，日志级别，源码文件，行号，信息 
//'\'后面不要加注释 
#define SPIDER_LOG(level, format, ...) do{ \
	if (level >= g_conf->log_level) {\
		time_t now = time(NULL); \
		char msg[MAX_MSG_LEN]; \
		char buf[32]; \
		sprintf(msg, format, ##__VA_ARGS__);             \
		strftime(buf, sizeof(buf), "%Y%m%d %H:%M:%S", localtime(&now)); \
		fprintf(stdout, "[%s] [%s] [file:%s] [line:%d] %s\n", buf, LOG_STR[level],__FILE__,__LINE__, msg); \
		fflush (stdout); \
	}\
	if (level == SPIDER_LEVEL_ERROR) {\
	exit(-1); \
	} \
 } while(0)

 
 extern int attach_epoll_task();


#endif