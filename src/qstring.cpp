#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include "qstring.h"

char * strcat2(int argc, const char *str1, const char *str2, ...)
{
    int tmp = 0; 
    char *dest = NULL;
    char *cur = NULL;
    va_list va_ptr;//用valist初定义一个指针
    size_t len = strlen(str1) + strlen(str2);

    argc -= 2;
    tmp = argc;

    va_start(va_ptr, str2);//用start定义，str2就是可变参数最左边的那个变量

    while(argc-- && (cur = va_arg(va_ptr, char *)))//获取可变参数的类型,第二个参数是类型
    {
        len = strlen(cur);
    }

    va_end(va_ptr);

    dest = (char *)malloc(len+1);
    dest[0] = '\0';
    strcat(dest, str1);//追加到dest结尾的字符串
    strcat(dest, str2);

    //动态拷贝后面的
    argc = tmp;
    va_start(va_ptr, str2);
    while(argc-- && (cur = va_arg(va_ptr, char *)))
    {
        strcat(dest, cur);
    }
    va_end(va_ptr);
    return dest;
}


//去除空格
char *strim(char *str)
{
    char *end, *sp, *ep;
    size_t len;
    sp = str;
    end = ep = str+strlen(str) - 1;
    
    while(sp <= end && isspace(*sp)) sp++;
    while(ep >= sp && isspace(*ep)) ep--;

    len = (ep < sp) ? 0 : (ep -sp +1);
    sp[len] = '\0';
    return sp;
}


//切割字符串，根据delimeter进行分割
char ** strsplit(char *line, char delimeter, int *count, int limit)
{
    char *ptr = NULL;
    char *str = line;
    char **vector = NULL;

    *count = 0;

    while((ptr = strchr(str, delimeter)))
    {
        *ptr = '\0';
        vector = (char **)realloc(vector, ((*count+1))*sizeof(char*));
        vector[*count] = strim(str);
        str = ptr + 1;
        (*count) ++;
        if(--limit == 0) break;
    }

    if(*str != '\0')
    {
        vector = (char **)realloc(vector, ((*count)+1)*sizeof(char *));
        vector[*count] = strim(str);
        (*count)++;
    }
    return vector;
}


int yesnotoi(char *str)
{
    if(strcasecmp(str, "yes") == 0) return 1;
    if(strcasecmp(str, "no") == 0) return 0;
    return -1;
}
