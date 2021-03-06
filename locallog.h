#pragma once

/*!
********************************************************************************

@brief  初始化本地日志

@param  pExeName :	可执行文件名称

@return  0：表示成功，其他表示失败并返回errno

@remark  本地日志文件存放路径："/home/'执行当前程序的用户'/gmt_local_log/local_log"

@author  p

*******************************************************************************/
int initLocalLog(const char* pExeName);

/*!
********************************************************************************

@brief  写入本地日志

@param  pSvcName	:	服务名称
@param  pDesc		:	日志描述信息

@return  0：表示成功，-1 : 表示失败

@remark

@author  p

*******************************************************************************/
int writeLocalLog(const char* pSvcName, const char* pDesc);

/*!
********************************************************************************

@brief  日志缓冲区中的数据同步到硬盘

@return  0：表示成功，-1 : 表示失败

@remark  

@author  p

*******************************************************************************/
int syncLocalLog();

/*!
********************************************************************************

@brief  释放本地日志

@return  0：表示成功，-1 : 表示失败

@remark  调用此函数后，在本地日志程序是可以释放的状态下，会先把日志缓冲区的数据写入到硬盘，随后再执行释放

@author  p

*******************************************************************************/
int disposeLocalLog();
