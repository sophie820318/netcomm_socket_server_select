/**************************************************************************
  * @ file    : pthread_define.h
  * @ author  : syc
  * @ version : 1.0
  * @ date    : 2020.7.16
  * @ brief   :线程调用_功能函数封装
***************************************************************************/
#ifndef _PTHREAD_DEFINE_H_
#define _PTHREAD_DEFINE_H_


#include <pthread.h>
#include <stdio.h>


#ifdef _cplusplus
extern "C"{
#endif

	
int detach_thread_create(pthread_t *thread, void * start_routine, void *arg);
	


#ifdef __cplusplus
}
#endif


#endif // _PTHREAD_DEFINE_H_
