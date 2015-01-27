#include "stdio.h"
#include "fcntl.h"
#include "string.h"
#include "errno.h"
#include "sys/file.h"
#include "stdlib.h"
#include "unistd.h"
#include "time.h"
#include "pthread.h"

#include "locallog.h"

#define LOG_ROOT_PATH getenv("HOME")

#define CATCH_LINE 100

#define CACHE_SIZE 32768			//取页的整数倍大小，这个大小能存放100条日志，以最大85个汉字为一条计算

#define EXE_TITLE " exe: "

#define SVC_TITLE " svc: "

#define DESC_TITLE "desc: "

char g_fullLogPath[128] = { 0 };		//完整的日志文件路径
char g_exeName[128] = { 0 };			//执行文件名称
int g_catchCount = 0;					//当前缓存计数
int g_fd;										//日志的文件描述符
char *g_cache;								//日志的缓存区
int g_curCacheSize = 0;					//当前缓存区大小
int g_localLogIsInit = 0;					//是否初始化

//互斥锁
pthread_mutex_t g_mutex;

//初始化本地日志
void initLocalLog(const char* pExeName)
{
	//保存服务名称
	strcpy(g_exeName, pExeName);

	//根目录
	strcpy(g_fullLogPath, LOG_ROOT_PATH);

	//分类目录
	strcat(g_fullLogPath, "/gmt_local_log/");

	//检查目录
	if (access(g_fullLogPath, F_OK) == -1)
	{
		//如果根目录不存在则新建
		if (mkdir(g_fullLogPath, 0775) == -1)
		{
			//如果根目录创建失败
			perror(g_fullLogPath);
			return;
		}
	}

	//日志文件
	strcat(g_fullLogPath, g_exeName);

	strcat(g_fullLogPath, ".log");

	//如果发现缺少文件夹则新建
	//以读写模式打开文件
	g_fd = open(g_fullLogPath, O_RDWR | O_APPEND);

	//如果找不到文件
	if (errno == ENOENT)
	{
		//以读写形式创建日志文件
		g_fd = open(g_fullLogPath, O_RDWR | O_CREAT, 0664);

	}
	else if (g_fd == -1)
	{
		perror("open");
		return;
	}

	//初始化互斥锁
	pthread_mutex_init(&g_mutex, NULL);

	//初始化缓存
	g_cache = (char *)calloc(1, CACHE_SIZE);

	g_catchCount = 0;

	//初始化完成
	g_localLogIsInit = 1;
}

//写入本地日志
void writeLocalLog(const char* pSvcName, const char* pDesc)
{
	if (!g_localLogIsInit) return;

	int desclen = strlen(pDesc);

	//线程互斥锁保证临界区只有一个线程执行
	pthread_mutex_lock(&g_mutex);

	time_t t = time(NULL);

	char curTime[32] = { 0 };

	//格式化输出日期
	strftime(curTime, sizeof(curTime), "%Y-%m-%d %H:%M:%S\n", localtime(&t));

	g_cache[g_curCacheSize++] = '\n';

	//向缓存区写入日期
	memcpy(g_cache + g_curCacheSize, curTime, strlen(curTime));
	g_curCacheSize += strlen(curTime);

	//向缓存区写入svcname
	memcpy(g_cache + g_curCacheSize, SVC_TITLE, strlen(SVC_TITLE));
	g_curCacheSize += strlen(SVC_TITLE);
	memcpy(g_cache + g_curCacheSize, pSvcName, strlen(pSvcName));
	g_curCacheSize += strlen(pSvcName);

	g_cache[g_curCacheSize++] = '\n';

	//向缓存区写入日志描述信息
	memcpy(g_cache + g_curCacheSize, DESC_TITLE, strlen(DESC_TITLE));
	g_curCacheSize += strlen(DESC_TITLE);
	memcpy(g_cache + g_curCacheSize, pDesc, desclen);
	g_curCacheSize += desclen;

	//换行
	g_cache[g_curCacheSize++] = '\n';

	g_catchCount++;

	if (g_catchCount >= CATCH_LINE) syncLocalLog();

	//解锁线程
	pthread_mutex_unlock(&g_mutex);
}

//日志缓冲区中的数据同步到硬盘
void syncLocalLog()
{
	if (!g_localLogIsInit) return;

	//启动文件互斥锁
	flock(g_fd, LOCK_EX);

	//缓冲区数据同步到硬盘
	write(g_fd, g_cache, g_curCacheSize);

	//解除文件互斥锁
	flock(g_fd, LOCK_UN);

	memset(g_cache, 0, CACHE_SIZE);

	g_curCacheSize = 0;

	g_catchCount = 0;
}

//释放本地日志
void disposeLocalLog()
{
	if (!g_localLogIsInit) return;

	syncLocalLog();

	g_localLogIsInit = 0;

	free(g_cache);

	pthread_mutex_destroy(&g_mutex);

	close(g_fd);

	memset(g_fullLogPath, 0, 128);
	memset(g_exeName, 0, 128);
	g_catchCount = 0;
	g_fd = 0;
	g_cache = NULL;
	g_curCacheSize = 0;
}
