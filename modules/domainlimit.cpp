#include "dso.h"
#include "url.h"
#include "qstring.h"
#include <vector>
using namespace std;

//控制爬虫爬取的范围，外站的舍弃，本站的保存



typedef struct Dlnode
{
	char *prefix;
	int len;
} Dlnode;

static vector<Dlnode*> include_nodes;
static vector<Dlnode*> exclude_nodes;

static int handler(void *data)
{
	unsigned int i;
	Sur *url = (Sur *)data;
	//总是可以直接通过
	if(url->level == 0 || url->tye != TYPE_HTML)
		return MODULE_OK;
	
	//如果指定的include里面没有 url满足要求，返回Error
	for(i = 0; i <include_nodes.size(); i++)
	{
		if(strncmp(url->url, include_nodes[i]->prefix, include_nodes[i]->len) == 0)
			break;
	}
	if(i >= include_nodes.size() && include_nodes.size() >0)
		return MODULE_ERR;
		
	//如果指定哪些网址排除在外
	for(int i = 0; i < exclude_nodes.size(); i++)
	{
		if(strncmp(url->url, exclude_nodes[i]->prefix, exclude_nodes[i]->len) == 0)
			return MODULE_ERR;
	}
	
	return MODULE_OK;
}

static void init(Module *mod)
{
	SPIDER_ADD_MODULE_PRE_SURL(mod);
	if(g_conf->include_prefixes != NULL)
	{
		int c = 0;
		char ** ss = strsplit(g_conf->include_prefixes, ',' &c, 0);
		while(c--)
		{
			Dlnode *n =(Dlnode *)malloc(sizeof(Dlnode));
			n->prefix = strim(ss[c]);
			n->len = strlen(n->prefix);
			include_nodes.push_back(n);
		}
	}
	
	if(g_conf->exclude_prefixs != NULL)
	{
		int c = 0;
		char **ss = strsplit(g_conf->exclude_prefixes, ',', &c, 0);
		while(c--)
		{
			Dlnode *n =(Dlnode *)malloc(sizeof(Dlnode));
			n->prefix = strim(ss[c]);
			n->len = strlen(n->prefix);
			exclude_nodes.push_back(n);
		}
	}
	
}

Module domainlimit = 
{
	STANDARD_MODULE_STUFF,
	init,
	handler
}
	

