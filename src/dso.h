#ifndef DSO_H
#define DSO_H

#include <vector>

#define MODULE_OK 0
#define MODULE_ERR 1

#define MAJOR_NUM 20190307
#define MINOR_NUM 0

//用于统计信息的时间戳
#define STANDARD_MODULE_STUFF MAJOR_NUM, \
								MINOR_NUM, \
								__FILE__

//无论什么模块都考虑因该有以下的信息
typedef struct Module
{
	int version;
	int minor_version;
	const char* name;//模块名称
	void (*init)(Module*);//每个模块都有init初始化函数，这样动态载入的时候可以调用这个模块的函数进行初始化
	int (*handle)(void *);//这个就相当于每个模块都有这么一个入口，
} Module;

//处理url的模块
extern vector<Module*> modules_pre_surl;
//载入这些url相关模块需要的初始化操作
#define SPIDER_ADD_MODULE_PRE_SURL(module) do { \
	modules_pre_surl.push_back(module); \
} while(0)
//socket通讯后，返回的htmlbody需要进行保存
extern vector<Module*> modules_post_html;
#define MODULES_POST_HTML(module) do { \
	modules_post_html.push_back(module); \
} while(0)

//接收到的的html会被分离成header和body，检查header的状态码和类型
extern vector<Module*> modules_post_header;
#define MODULES_POST_HEADER(module) do { \
	modules_post_header.push_back(module); \
} while(0)

//主流程需要调用的载入操作
extern Module* dso_load(const char* path, const char* name);


#endif
