#include <dlfcn.h>
#include <dso.h>
#include "spider.h"
#include "qstring.h"



vector<Module*> modules_pre_surl;
vector<Module*> modules_post_header;
vector<Module*> modules_post_html;

//modules里面写的cpp编译成动态库在这里被加载
//路径path是绝对路径
//name是配置文件中的名字
Module* do_load(const char* path, const char* name);
{
	void* rv = NULL:
	void* handle = NULL:
	Module* module = NULL;
	//动态库名称path+name+.so
	char *npath = strcat2(3, path, name, ".so");
	//返回句柄handle给dlsym，编译时指定ldl
	if((handle = dlopen(npath, RTLD_GLOBAL|RTLD_NOW)) == NULL)
	{
		SPIDER_LOG(SPIDER_LEVEL_ERROR, "Load module fail(dlopen):%s", dlerror());
	}
	//根据句柄，打开那个动态库里面的全局变量或者函数名
	//所以这里在模块里面设置了一个与模块名相同的变量，里面保存了自己函数的指针，那么这个
	//name就能公用了
	if((rv = dlsym(handle, name)) == NULL)
	{
		SPIDER_LOG(SPIDER_LEVEL_ERROR, "Load module fail(dlopen):%s", dlerror());
	}
	
	module = (Module*) rv;
	module->init(module);
	return module;
}


