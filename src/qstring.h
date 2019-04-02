#ifndef Q_STRING
#define Q_STRING



extern char * strcat2(int argc, const char *str1, const char* str2, ...);


//剪切字符串
extern char * strim(char *str);


//分割字符串
extern char ** strsplit(char *line, char delimeter, int *count, int limit);


//把字符串的yesno转为int型用于判断
extern int yesnotoi(char *str);
#endif
