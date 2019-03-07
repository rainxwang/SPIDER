#include "dso.h"
#include "socket.h"
#include "url.h"
#include "fctnl.h"


//解析图片的正则规则
static const char *IMAGE_PATTERN = "<img [^>]*src=\"\\s*\\([^>\"]*\\)\\s*\"";

static int handler(void *data)
{
	Response *r = (Response *)data;
	const size_t = nmatch = 2;
	regmatch_t matchptr[nmatch];
	int len;
	regex_t re;
	
	
}




void init(Module *mod)
{
	MODULES_POST_HTML(mod);
}



Module maxdepth = 
{
	STANDARD_MODULE_STUFF,
	init,
	handler
}
