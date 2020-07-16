/**************************************************************************
* @ file    : tcp_server.c
* @ author  : syc
* @ version : 1.0
* @ date    : 2020.7.12
* @ brief   : tcp_server_select
***************************************************************************/
#include "pthread_define.h"
#include "msg_define.h"
#include "tcp_server.h"


//全局的变量，用m开头
int server_sockfd;
char *m_TempBuffer;
int m_RevDataSize = 0;
int serverexitflag=1;

//
int mega_send_data(int nSock, char *buf, int len)
{
    // if nSock is closed, return bytes is -1
    int bytes = -1;
    
    int length = len;
    
    do
    {
    ReSend:
        bytes = send(nSock, buf, length, 0);
        if ( bytes > 0 )
        {
            length -= bytes;
            buf += bytes;
        }
        else if (bytes <0)
        {
            if (errno == EINTR || errno == EWOULDBLOCK ||errno ==EAGAIN)
                goto ReSend;
        }
        else if(bytes==0 || bytes ==-1){
            return -1;
        }
    }
    while ( length > 0 && bytes > 0 );
    if ( length == 0 )
        bytes = len;
    return bytes;
}
//
int xsocket_tcp_send(int sock, char *buf, int len)
{
    int		ret;
    
    fd_set wfds;
    struct timeval tv;
    
    while(1)
    { 
        FD_ZERO(&wfds);
        FD_SET(sock, &wfds);
        tv.tv_sec = 3;
        tv.tv_usec = 0;
        ret = select(sock + 1, NULL, &wfds, NULL, &tv);
        if (ret == 0)  /* timeout */
        {
            continue;
        }
        if(ret < 0 && ret == EINTR)
        {
            usleep(1);
            continue;
        }
        break;
    }
    
    if( ret < 0 )
    {
        return -1; //timeout obut the socket is not ready to write; or socket is error
    }
    
    ret = mega_send_data(sock, buf, len);

    return ret;
}


int resp_net_Send(int socket, MsgHeader	*pMsgHead, void *pSendBuf)
{
    int ret = 0;
    char *lpbuf = 0;

    //先发送头
    ret = xsocket_tcp_send(socket, (char *)pMsgHead, sizeof(MsgHeader));

    if(ret != sizeof(MsgHeader))
    {
        printf("Video_Net_Send Client messageHeader(%s) Command(%d) Failed\n", pMsgHead->messageHeader, pMsgHead->controlCode);
        return -2;
    }
    //再发送数据
    ret = xsocket_tcp_send(socket, pSendBuf+sizeof(MsgHeader), pMsgHead->contentLength);

    if(ret != pMsgHead->contentLength)
    {
        printf("Video_Net_Send Client messageHeader(%s) Command(%d) Failed\n", pMsgHead->messageHeader, pMsgHead->controlCode);
        return -2;
    }

    return ret;
}


int tcp_receive(int hSock, char *pBuffer, unsigned int nSize)
{
    int ret = 0;
    fd_set fset;
    struct timeval to;
    unsigned int  dwRecved = 0;
    
    memset(&to,0,sizeof(to));
    
    if (hSock < 0 || hSock > 65535 || nSize <= 0)
    {
        return -2;
    }
    
    while (dwRecved < nSize)
    {
        FD_ZERO(&fset);
        FD_SET(hSock, &fset);
        to.tv_sec = 10;
        to.tv_usec = 0;
        
        ret = select(hSock+1, &fset, NULL, NULL, &to);  //&to
        if ( ret == 0 || (ret == -1 && errno == EINTR))
        {
            return -1;
        }
        if (ret == -1)
        {
            return -2;
        }
        
        if(!FD_ISSET(hSock, &fset))
        {
            return -1;
        }
        
        ret = recv(hSock, pBuffer + dwRecved, nSize - dwRecved, 0);
        if( (ret < 0) && (errno == EAGAIN || errno == EINTR ||errno == EWOULDBLOCK))
        {
            continue;
        }
        if (ret <= 0)
        {
            printf("TcpReceive recv return %d errno = %d\n",ret,errno);
            return -2;
        }
        dwRecved += ret;
    }
    return dwRecved;
}

//主要的数据逻辑在这里
int process_client_message(int socket, MsgHeader *lpcommhead)
{
    char 				*pCommData = NULL;
    int 				ret;
    MsgHeader 	toclientMsgHeader;
    char				replyBuf[512];
    memset(replyBuf, 0, sizeof(replyBuf));
    printf("lpcommhead->contentLength:%d\n",lpcommhead->contentLength);
    if(lpcommhead->contentLength > 0)
    {
        pCommData = (char *)malloc(lpcommhead->contentLength);
        ret = tcp_receive(socket, pCommData, lpcommhead->contentLength);
        if (-2 == ret)
        {
            return -1;
        }
        if (-1 == ret)
        {
            return 0;
        }
    }
    
	//toclientMsgHeader.messageHeader = "VS_C"

    toclientMsgHeader.messageHeader[0] = 'V';
    toclientMsgHeader.messageHeader[1] = 'S';
    toclientMsgHeader.messageHeader[2] = '_';
    toclientMsgHeader.messageHeader[3] = 'C';
    printf("lpcommhead->controlCode:%d\n",lpcommhead->controlCode);
    
    switch(lpcommhead->controlCode)
    {
        case CONTROLCODE_LOGINREQUEST:
        {
            LoginRequestReply *reply = (LoginRequestReply *)(replyBuf + sizeof(MsgHeader));
            
            reply->result = 0;
            strcpy(reply->uuID, "1");
            strcpy(reply->softVersion, "1.0");
            toclientMsgHeader.controlCode = CONTROLCODE_LOGINREPLY;
            toclientMsgHeader.contentLength = sizeof(LoginRequestReply);
            resp_net_Send(socket, &toclientMsgHeader, replyBuf);
        }
            break;
		//这才到验证用户名和密码呢
        case CONTROLCODE_VERIFIYREQUEST:
        {
            VerifyRequestCommContent *pReq = (VerifyRequestCommContent *)pCommData;
            VerifyRequestReply *reply = (VerifyRequestReply *)(replyBuf + sizeof(MsgHeader));
            printf("校验用户名和密码：%s:%s\n",pReq->userName, pReq->password);
            //比较用户名和密码
            //...
            reply->result = VERIFYREPLYRETURNVALUE_OK;
            toclientMsgHeader.controlCode = CONTROLCODE_VERIFIYREPLY;
            toclientMsgHeader.contentLength = sizeof(VerifyRequestReply);
            resp_net_Send(socket, &toclientMsgHeader, replyBuf);
        }
            break;
		
        /*case CONTROLCODE_VIDEOTRANSLATION_REQUEST:
        {
            videoTranslationRequest *pReq = (videoTranslationRequest *)pCommData;
            videoTranslationRequestReply *reply = (videoTranslationRequestReply *)(replyBuf + sizeof(MsgHeader));
            reply->result = 0;
            reply->videoID = 1;
            toclientMsgHeader.controlCode = CONTROLCODE_VIDEOTRANSLATION_REPLY;
            toclientMsgHeader.contentLength = sizeof(videoTranslationRequestReply);
            resp_net_Send(socket, &toclientMsgHeader, replyBuf);
        }
            break;
		
        case CONTROLCODE_VIDEOTRANSLATION_STOP:
        {
            if(m_video_status)
            {
                m_video_status = 0;
            }
        }
            break;
       
        case CONTROLCODE_LISTENSTART_COMMAND:
        {
            printf("START LISTEN:::\n");
            audioRequestCommand *pReq = (audioRequestCommand *)pCommData;
            audioRequestCommandReply *reply = (audioRequestCommandReply *)(replyBuf + sizeof(MsgHeader));
            reply->result = 0;
            reply->audioID = 1;
            toclientMsgHeader.controlCode = CONTROLCODE_LISTENSTART_REPLY;
            toclientMsgHeader.contentLength = sizeof(audioRequestCommandReply);
            resp_net_Send(socket, &toclientMsgHeader, replyBuf);
            m_audio_status = 1;

        }
            break;
        case CONTROLCODE_LISTENSTOP_COMMAND:
        {
            printf("STOP LISTEN:::\n");
            if(m_audio_status)
            {
                m_audio_status = 0;
            }
        }
         break;

        case CONTROLCODE_DECODER_CONTROLL:
        {

        }
            break;
        default:
            break;
        */
    }
    
    if(pCommData != NULL)
    {
        free(pCommData);
        pCommData = NULL;
    }
    return 0;
}



//传参应该是socketfd
void* thread_process_message(void *lpVoid)
{
    int 				nKeepAlive = 0;
    int					msg_socket;
    fd_set      		readfds;
    struct timeval		tv;
    int					nMaxfd;
    MsgHeader		commhead;
    memset(&commhead, 0, sizeof(commhead));
    int nSocket = -1;
    nSocket = *((int*)lpVoid);//把指针指向的地址取出来，后面会有
    if(lpVoid)//这里将传参释放，不知道会有什么问题
    {
        free(lpVoid);
    }
    while(1)
    {
        nMaxfd = 0;
        nMaxfd = nSocket;
        msg_socket = nSocket;
        //printf("---------------------nMaxfd:%d  nKeepAlive:%d\n",nMaxfd, nKeepAlive);
        
        tv.tv_sec  = 1; // 秒数
        tv.tv_usec = 0; // 微秒
        //清空读字符描述集
        FD_ZERO(&readfds);
		//将socket放入读字符描述集
        FD_SET(msg_socket, &readfds);
        
        //readfds:
        //	1、If listen has been called and a connection is pending, accept will succeed.
        //	2、Data is available for reading (includes OOB data if SO_OOBINLINE is enabled).
        //	3、Connection has been closed/reset/terminated.
        errno = 0;	//清除错误
        int nreturn = select(nMaxfd + 1,&readfds,NULL, NULL,&tv);//执行成功则返回文件描述词状态已改变的个数
        //printf("nreturn %d \n", nreturn);
        if(nreturn == 0)
        {
            usleep(1000);
            //超时，给各个的KeepAlive+1
            nKeepAlive++;
            if(nKeepAlive > 60)
            {
                printf("line:%d fun:%s -------------\n", __LINE__, __FUNCTION__);
                close(msg_socket);
                break;
            }
            continue;
        }
        if(nreturn > 0)
        {
            //printf("line:%d fun:%s -------------nreturn(%d) nKeepAlive:%d msg_socket:%d\n", __LINE__, __FUNCTION__, nreturn, nKeepAlive, msg_socket);
            
            int fdSet = FD_ISSET(msg_socket, &readfds);
            usleep(5);
            //printf("fdSet %d \n", fdSet);
            if(fdSet)
            {
                int bytes = 0;
                int nerr = errno;
                ioctl(msg_socket, FIONREAD, &bytes);
                if(bytes == 0)
                {
                    nerr = errno;
                    nKeepAlive++;		//主动关闭会导致连续读0
                    if(nKeepAlive > 30)
                    {
                        printf("line:%d fun:%s -------------\n", __LINE__, __FUNCTION__);
                        close(msg_socket);
                        continue;
                    }
                }
                nreturn = recv(msg_socket,&commhead,sizeof(commhead),0);
                //if(commhead.controlMask != CONTROLLCODE_KEEPALIVECOMMAND)
                //{
                //	printf("nreturn %d \n", nreturn);
                //}
                if(nreturn < 0)
                {
                    //printf("line:%d fun:%s -------------\n", __LINE__, __FUNCTION__);
                    close(msg_socket);
                    msg_socket = -1;
                    continue;
                }
                if(nreturn == sizeof(commhead))
                {

                    nKeepAlive = 0;
					//正常通信后 就是keepalive了，第一个是请求
                    if(commhead.controlCode == CONTROLCODE_KEEPALIVECOMMAND)
                    {
                        printf("KEEP ALIVE PACAKGE commhead.nCommand [%d]\n", commhead.controlCode);
                        nKeepAlive = 0;
                        continue ;
                    }

                    if(commhead.contentLength >= 0)
                    {
                        printf("messageHeader[%s] controlCode[%d] contentLength[%d]-------\n", commhead.messageHeader, commhead.controlCode, commhead.contentLength);
                        process_client_message(msg_socket, &commhead);
                    }
                    
                }
            }
            else
            {
                //没有发生网络消息，则在一般情况下，未来的1秒内会收到至少1次nKeepAlive，把计数器清0
                nKeepAlive++;
                //printf("line:%d fun:%s -------------lpInfo->nKeepAlive:%d lpInfo->nSocket:%d\n", __LINE__, __FUNCTION__,lpInfo3->nKeepAlive,lpInfo3->nSocket);
                if(nKeepAlive > 60)
                {
                    printf("line:%d fun:%s -------------\n", __LINE__, __FUNCTION__);
                    close(msg_socket);
                    continue;
                }
            }
        }
        else
        {
            int nerr = errno;
            printf("select errno %d %s\n", nerr,strerror(nerr));
            close(msg_socket);
            //m_video_status = 0;
            //m_audio_status = 0;
            break;
        }
    }
    return 0 ;
}

int init_socket_params(int senSocket)
{
    int result;
    int opt = 1;
    struct timeval to;
	// TCP_NODELAY  0x0001，头文件宏中定义
    result = setsockopt(senSocket, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    opt = SOCK_SNDRCV_LEN;//1024*32,头文件宏中定义
    result = setsockopt(senSocket, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt));
    
    to.tv_sec  = 5;
    to.tv_usec = 0;
	//设置超时时间为5S
    result = setsockopt(senSocket,SOL_SOCKET,SO_SNDTIMEO,(char *)&to,sizeof(to));
    fcntl(senSocket, F_SETFL, O_NONBLOCK);
    return 1;
}


int process_msg_command(int p_client_sockfd)
{
    init_socket_params(p_client_sockfd);
    //把非阻塞变为阻塞
    int flags = fcntl(p_client_sockfd, F_GETFL, 0);
    fcntl(p_client_sockfd, F_SETFL,flags & ~O_NONBLOCK);
    int cmdSocket = -1;
    char *pcmdSocket = (char *)malloc(4);
    cmdSocket = p_client_sockfd;
    memcpy(pcmdSocket, &cmdSocket, 4);
	//在pthread_udefine封装库中定义
    detach_thread_create(NULL,thread_process_message,pcmdSocket);
	//这为什么要给赋值为-1呢？
    p_client_sockfd = -1;
    return 0;
}



//处理客户的数据
int process_socket_command(int p_client_sockfd)
{
    int ret = 0;
    MsgHeader *lpHead = (MsgHeader *)m_TempBuffer;

    printf("ProcessSocketCommand Socket=%d\n",p_client_sockfd);
    //VS_C(信令) 和VS_D（流媒体数据），信令有可能是登录数据
    if(lpHead->messageHeader[3] == 'C')
    {
        //先登录0代表登录请求
        if(lpHead->controlCode == CONTROLCODE_LOGINREQUEST)
        {
            char				replyBuf[512];
            memset(replyBuf, 0, sizeof(replyBuf));
			//在replyBuf首地址偏移MsgHeader个字节，作为回复的数据体reply，并将reply中的数据赋值
            LoginRequestReply *reply = (LoginRequestReply *)(replyBuf + sizeof(MsgHeader));
            printf("\n");
            reply->result = 0;
			
            strcpy(reply->uuID, "1");
            strcpy(reply->softVersion, "1.0");
            lpHead->controlCode = CONTROLCODE_LOGINREPLY;
            lpHead->contentLength = sizeof(LoginRequestReply);
            //接收到登录请求以后，回复可以登录，并附加新的信息
            resp_net_Send(p_client_sockfd, lpHead, (void *)replyBuf);
        }
        
        printf("处理命令 Control code: %d\n",lpHead->controlCode);
        process_msg_command(p_client_sockfd);
    }
    else if(lpHead->messageHeader[3] == 'D')
    {
    
        
    }
    else
    {
        
    }
    
    return 0;
}





//客户端处理，每一个客户端由一个新的socket 处理通讯
void client_sock_process(int* p_client_sockfd)
{
	int client_sockfd = *p_client_sockfd;
	//读描述符集
    	fd_set 		read_set;
	int 		soketerror = 0;
	int  		wait_count = 0;
	int 		revsize  = 0;
  	struct timeval 	tmval; 
	int    buffersize = sizeof(MsgHeader);//CMD_PART_LENTH;
	//设置超时时间5s
	tmval.tv_sec = 5;
	tmval.tv_usec = 0;
	m_RevDataSize = 0;
	//初始化与客户端通信的socket的
	if(init_socket_params(client_sockfd) < 0 || m_TempBuffer == 0)
    	{
        		if(client_sockfd)
        		{
            			close(client_sockfd);
            			client_sockfd = 0;
        		}
        		return ;
    	}

	FD_ZERO(&read_set);
	FD_SET(client_sockfd,&read_set);
	printf("\n");
    m_RevDataSize = 0;
	//累计10次出错，或者
	while((wait_count < 10) && (!soketerror))
    	{
       	 	int ret = 0;
        		//m_nRevDataSize = 0;
        		ret = select(client_sockfd+1,&read_set, NULL, NULL,&tmval);
        		printf("ret = %d\n",ret);
        		switch(ret)
        		{
            			case -1:  
                			soketerror = 1;
                			break;
            			case 0:
                			wait_count++;
                			break;
            			default:
				if( FD_ISSET( client_sockfd, &read_set ))
                			{
                				//连接上以后，服务器端先接收协议头数据
                    				revsize = recv(client_sockfd, m_TempBuffer+m_RevDataSize, (int)sizeof(MsgHeader), 0 );
                    				printf("revsize[%d] sizeof(MsgHeader)[%d]\n", revsize,(int)sizeof(MsgHeader));
                    				if(revsize <= 0)
                    				{
                        					soketerror = 1;
                        					break;
                    				}

                    				printf("m_RevDataSize [%d]\n",m_RevDataSize);
                    				if(m_RevDataSize >= sizeof(MsgHeader))
                    				{
                        					MsgHeader *lpHead = (MsgHeader *)m_TempBuffer;
                        					printf("messageHeader[%s] controlCode[%d] contentLength[%d]-------\n", lpHead->messageHeader, lpHead->controlCode, lpHead->contentLength);
                        					//if(lpHead->nFlag == 9000)
                        					{
                            						break;
                        					}
                    				}
                    				m_RevDataSize += revsize;
                    				buffersize = CMD_PART_LENTH - m_RevDataSize;
                			}
                			else
                			{
                    				wait_count++;
                			}
				break;
        	}
			if(m_RevDataSize == CMD_PART_LENTH)
            	break;
        	if(m_RevDataSize == sizeof(MsgHeader))
        	{
            	MsgHeader *lpHead = (MsgHeader *)m_TempBuffer;
            	printf("messageHeader[%s] controlCode[%d] contentLength[%d]-------\n", lpHead->messageHeader, lpHead->controlCode, lpHead->contentLength);
            	//if(lpHead->nFlag == 9000)
            	{
                	break;
            	}
        	}

	}

	
	if((soketerror || wait_count >= 5) && (!(m_RevDataSize == CMD_PART_LENTH || m_RevDataSize == sizeof(MsgHeader))))
    {
        if(client_sockfd)
        {
            close(client_sockfd);
            client_sockfd = 0;
        }
        //return -1;
    }
    //接收到头数据以后，再接收其他的数据，比如接收登录信息
    if(m_RevDataSize == sizeof(MsgHeader))
    {
        process_socket_command(client_sockfd);
        //return 0;
        return;
    }
	//连接上后，长时间不发送数据，那么断开非法连接
    if(client_sockfd)
    {
        close(client_sockfd);
        client_sockfd = 0;
    }
    //return -1;
    
				
}
		
          


//块注释
/*
	printf("%d\n",client_sockfd);
	int ret;
	char data[100];
	char data1[100];
	while(1)
	{
		//5,read the data from client
		ret = recv(client_sockfd,data,100,0);
		if(-1 == ret)
		{
			printf("recv error");
		}
		printf("data from client:%s\n",data);
		//6,send the data to client
		printf("server send data:");
		scanf("%s",data1);
		ret = send(client_sockfd,data1,100,0);
		if(-1 == ret)
		{
	    	perror("send"),exit(-1);
		}                                                  
	}
	pthread_exit(0);
*/



int start_listen()
{
	int ret;

	struct sockaddr_in client_addr;
	socklen_t client_addrlen;
	int client_sockfd;

	/*3,listen the socket*/
	ret = listen(server_sockfd,6);
	if(-1 == ret)
	{
		printf("listen error");
		ret = -1;
		return ret;
	}
	//extern exitflag;
	m_TempBuffer = (char *)malloc(TEMP_FILE_SIZE);
                serverexitflag = 1;
	while(serverexitflag)
	{
		/*4,accept the data from the client*/
		client_addrlen = sizeof(struct sockaddr);
		client_sockfd = accept(server_sockfd,(struct sockaddr *)&client_addr,&client_addrlen);
		if(client_sockfd > 0)
		{    
		    printf("IP is %s\n",inet_ntoa(client_addr.sin_addr));
		    printf("Port is %d\n",ntohs(client_addr.sin_port));
		    pthread_t pt;
		    printf("%d\n",client_sockfd);
		    ret = pthread_create(&pt,NULL,(void *)client_sock_process,&client_sockfd);
		    if(0 != ret)
		    {
		        printf("ip:%s connect failed",inet_ntoa(client_addr.sin_addr));
		    }
  			//pthread_join(pt,(void*)buf);
			pthread_detach(pt);
		}
		usleep(500000);
	}

	return 0;         

}


//初始化
int init_server(int port)
{
	//port = 5000;
	//定义的全局变量，可以直接用
	//int server_sockfd;

	int ret;
	//server地址
	struct sockaddr_in server_addr;
	socklen_t server_addrlen;

	int on = 1;
	//char buf[20];
	//创建TCPsocket，返回socket描述符
	server_sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(-1 == server_sockfd)
	{
	 		printf("socket");
	 ret = -1;
	 return ret;
	}
	//启用端口复用选项功能，一个端口释放后等待两分钟才能再被使用，SO_REUSEADDR是让端口释放立即就可以被再次使用
	ret = setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
	//非阻塞模式
	fcntl(server_sockfd, F_SETFL, O_NONBLOCK);
	/*2,bind the socket with an ip*/
	//将地址空间清零
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addrlen = sizeof(struct sockaddr);
	ret = bind(server_sockfd,(const struct sockaddr*)&server_addr,server_addrlen);
	if(-1 == ret)
	{
		printf("bind error");
		ret = -1;
		return ret;
	}
}

int stop_server()
{
	serverexitflag = 0;
	close(server_sockfd);
	return 0;
}





                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
