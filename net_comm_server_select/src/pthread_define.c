#include "pthread_define.h"
int detach_thread_create(pthread_t *thread, void * start_routine, void *arg)
{
    pthread_attr_t attr;
    pthread_t thread_t;
    int ret = 0;

    if(thread==NULL){
        thread = &thread_t;
    }
    //初始化线程的属性
    if(pthread_attr_init(&attr))
    {
        printf("pthread_attr_init fail!\n");
        return -1;
    }

    //设置线程detachstate属性。该表示新线程是否与进程中其他线程脱离同步
    if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
    {//新线程不能用pthread_join()来同步，且在退出时自行释放所占用的资源。
        printf("pthread_attr_setdetachstate fail!\n");
        goto error;
    }

    ret = pthread_create(thread, &attr, (void *(*)(void *))start_routine, arg);
    if(ret < 0){
        printf("pthread_create fail!\n");
        goto error;
    }

    //将状态改为unjoinable状态，确保资源的释放。
   ret =  pthread_detach(thread_t);

error:
    pthread_attr_destroy(&attr);

    return ret;
}
