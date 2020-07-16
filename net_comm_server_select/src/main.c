/**************************************************************************
  * @ file    : main.c
  * @ author  : syc
  * @ version : 1.0
  * @ date    : 2020.7.13
  * @ brief   :验证server工作逻辑
***************************************************************************/
#include "tcp_server.h"

int main()
{
//server test

	 if(init_server(8000))
	{
		 printf("init_tcp_server failed");
		 return -1;
	}
	 	
 	start_listen();

	//stop_server();

}
