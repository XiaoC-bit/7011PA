#ifndef _DATA_DEFINE_H
#define _DATA_DEFINE_H
#include <qstring.h>

struct Config{
	int debugPort;//前端调试端口
	int webSocketPort;//websocket端口
	int frontPort;//前端URL端口
	QString frontExeName;//前端启动进程名称
};


#endif
