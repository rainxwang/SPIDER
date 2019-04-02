TARGET=spider
SOURCES=$(wildcard *.cpp) #wildcard执行文件匹配
OBJS=$(patsubst %.cpp, %.o, $(SOURCES)) #内容的替换

CXX:=g++




