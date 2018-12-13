#include "httpServerLib.h"

//======================================================================
int recvLine(int sock, char *line, int lineSize)
{
	--lineSize;
	int len=0;
	while(len<lineSize)
	  {
		  int r=recv(sock,line+len,sizeof(char),0);
		  if(r<0) return -1;
		  if(!r||(line[len++]=='\n')) break;
	  }
	line[len]='\0';
	return len;
}

//======================================================================
void sendAll(int sock, const void *data, int dataSize)
{
	const char *ptr=(const char *)data;
	int remaining=dataSize;
	while(remaining)
	  {
		  int r=send(sock,ptr,remaining,0);
		  if(r<=0) break;
		  ptr+=r;
		  remaining-=r;
	  }
}

//======================================================================
// read bytes (bufferCapacity expected) or 0 (EOF)
int recvAll(int fd, void *buffer,int bufferCapacity)
{
  char *ptr=(char *)buffer;
  int remaining=bufferCapacity;
  while(remaining)
  {
    int r;
    RESTART_SYSCALL(r, (int)read(fd, ptr, (size_t)remaining));
    if(r<=0)
    {
      break;
    }
    ptr+=r;
    remaining-=r;
  }
  return bufferCapacity-remaining;
}

//======================================================================
Request * createRequest(int sock)
{
	Request *req=(Request *)malloc(sizeof(Request));
		req->sock=sock;
		req->requestMethod[0]='\0';
		req->requestUri[0]='\0';
		req->contentLength[0]='\0';
		req->fileName[0]='\0';
		req->upgrade[0]='\0';	//###############################
		req->wsKey[0]='\0';		//###############################
	return req;
}

//======================================================================
void destroyRequest(Request *req)
{
	close(req->sock);
	free(req);
}


//======================================================================
void createQueryString(char* str, int len, char* dest)
{
	int i=0;
	int n=0;
	
	while(*str != '\0')
	{
		if (*str=='?') { break;}
		str++;		
	}	
	str++;	
	for(n=0 ; n<(len-i) ; n++)
	{			
		dest[n]=*str;
		str++;
	}
}

//======================================================================
