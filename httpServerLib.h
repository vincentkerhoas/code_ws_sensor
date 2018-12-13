#include "main.h"


typedef struct
{
	int sock;
	char queryString[0x100];
	char requestMethod[0x100];
	char requestUri[0x90];
	char contentLength[0x100];
	char fileName[0x100];
	char upgrade[32];
  	char wsKey[32];
} Request;

//======================================================================

int recvLine(int sock, char *line, int lineSize);
void sendAll(int sock, const void *data, int dataSize);
int recvAll(int fd, void *buffer,int bufferCapacity);
Request * createRequest(int sock);
void destroyRequest(Request *req);
void createQueryString(char* str, int len, char* dest);

//======================================================================
