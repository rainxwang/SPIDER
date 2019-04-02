#include "spider.h"
#include "qstring.h"
#include "configparser.h"
#include "string.h"


#define INF 0x7FFFFFFF //定义最大的INT MAx

Config * initconfig()
{
    Config *conf = (Config *)malloc(sizeof(Config));

    conf->max_job_num = 10;//
    conf->seeds = NULL;
    conf->logfile = NLL;
    conf->log_level = 0;
    conf->max_depth = INF;
    conf->module_path = NULL;
    conf->stat_interval = 0;
    return conf;
}

void loadconfig(Config* conf)//读取配置文件
{
    FILE *fp = NULL;
    char buf[MAX_CONF_LEN + 1];//配置文件最长不能超过1024
    int argc = 0;
    char **argv = NULL;
    int linenum = 0;
    char *line = NULL;
    const char *err = NULL;

    if((fp = fopen(CONF_FILE, "r")) == NULL)//一个宏，是文件名
    {
        SPIDER_LOG(SPIDER_LEVEL_ERROR, "Can't load %s .conf", CONF_FILE);
    }

    while(fgets(buf, MAX_CONF_LEN+1, fp) != NULL)//按行读，会包含换行负
    {
        linenum++;
        line = strim(buf);//去除空格
        if(line[0] == '#' || line[0] == '\0') continue;
        argv = strsplit(line, '=', &argc, 1);
        if(argc == 2)
        {
            if(strcasecmp(argv[0], "logfile") == 0)
            {
                conf->logfile = strdup(argv[1]);
            }
            else if(strcasecmp(argv[0], "seeds") == 0)
            {
                conf->seeds= strdup(argv[1]);
            }
            else if(strcasecmp(argv[0], "module_path") == 0)
            {
                conf->module_path= strdup(argv[1]);
            }
            else if(strcasecmp(argv[0], "log_evel") == 0)
            {
                conf->log_level= atoi(argv[1]);
            }
            else if(strcasecmp(argv[0], "max_depth") == 0)
            {
                conf->max_depth= atoi(argv[1]);
            }
            else if(strcasecmp(argv[0], "load_module") == 0)
            {
                conf->modules.push_back(strdup(argv[1]));
            }
            else if(strcasecmp(argv[0], "accept_types") == 0)
            {
                conf->accept_types.push_back(strdup(argv[1]));
            }
            else if(strcasecmp(argv[0], "stat_interval"))
            {
                conf->stat_interval = atoi(argv[1]);
            }
            else
            {
                err = "Unknow type"; goto conferr;
            }
        }
        else
        {
            err = "direactive must be key = value"; goto conferr;
        }
        
    }
    return ;

conferr:
    SPIDER_LOG(SPIDER_LEVEL_ERROR,"Bad directive in %s[line %d] %s", CONF_FILE, linenum, err);

}
