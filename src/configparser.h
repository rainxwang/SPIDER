#ifndef CONFIGEPARSERR_H
#define CONFIGEPARSERR_H
#include <vector>


using namespace std;
#define MAX_CONF_LEN 1024
#define CONF_FILE "spider.conf"


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

	int		stat_interval;
};

extern Config * initconfig();//配置文件的全局变量初始化
extern void loadconfig(Config* conf);//读取配置文件


#endif