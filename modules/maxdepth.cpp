#include "dso.h
#include "url.h"

//判断有没有超过最大限制
static int handler(void *data)
{
	Sur *url = (Surl *)data;
	if(url->level > g_conf->max_depth)
		return MODULE_ERR;
	return MODULE_OK;
}


static void init(Module *mod)
{
	SPIDER_ADD_MODULE_PRE_SURL(mod);
}

Module maxdepth = 
{
	STANDARD_MODULE_STUFF,
	init,
	handler
}
