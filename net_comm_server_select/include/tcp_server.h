/**************************************************************************
  * @ file    : tcp_server.h
  * @ author  : syc
  * @ version : 1.0
  * @ date    : 2020.7.12
  * @ brief   : tcp_server_socket
***************************************************************************/
#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include<sys/ioctl.h>
#include <unistd.h> 
#include <stdio.h>                                
#include <stdlib.h>                                
#include <string.h>                                
#include <sys/socket.h>                                                   
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>


#ifdef _cplusplus
extern "C"{
#endif
	

	#define TEMP_FILE_SIZE     (64*1024)
	#define CMD_PART_LENTH          500

	#define SOCK_SNDRCV_LEN         (1024*32)
	#define TCP_NODELAY             0x0001
	//定义全局变量
	/*套接字描述符*/

	
                 //#define int    TEMP_DATA_SIZE
	/*服务接收缓冲区*/
	//char buf[1024];
    

	/**
	* @brief 初始化socketserver
	* @port 监听的端口号
	* @return 成功返回0,返回其他失败
	*/
	int init_server(int port);

	/**
	* @brief 开始监听
	* @return 成功返回0,返回其他失败
	*/
	int start_listen();
	
	/**
	* @brief 开始监听
	* @return 成功返回0,返回其他失败
	*/
	int stop_server();


#ifdef __cplusplus
}
#endif


#endif // _TCP_SERVER_H_
