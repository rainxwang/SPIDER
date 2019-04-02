#include "dso.h"
#include "socket.h"



static int handler(void *data)
{
	Header *h = (Header *)data;
	//if not 2xx
	if(h->status_num < 200 || h->status_num >= 300)
		return MODULE_ERR;
	if(h->content_type != NULL)
	{
		if(strstr(h->content_type, "text/html") != NULL)
			return MODULE_OK;//判断str2是不是str1的子串，如果是返回位置
		for(int i = 0; i < g_conf->accept_types.size(); i++)
		{
			if(strstr(h->content_type, g_conf->accept_types[i] != NULL)
				return MODULE_OK;
		}
		return MODULE_ERR;
	}
	return MODUEL_OK;
}


static void init(Module *mod)
{
	MODULES_POST_HEADER(mod);
}

Module headerfilter = 
{
	STANDARD_MODULE_STUFF
	init,
	handler
};