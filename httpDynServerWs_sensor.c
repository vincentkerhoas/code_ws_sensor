// CREDITS : F. Harrouet, www.enib.fr/~harrouet
//======================================================================
// REM : all static pages have to be readable only !!
//======================================================================
#include "main.h"
#include "httpServerLib.h"
#include "libWs.h"
#include "senseHat.h"

#include <time.h> 
#include <sqlite3.h> 

#define BUFFER_SIZE 0x1000


//======================================================================
void * dialogThread(void *arg)
{
	char header[0x100];
	char handshake[29];
	float pressure, temperature;
    int values[2];
	
	  
	pthread_detach(pthread_self());
	char buffer[BUFFER_SIZE];
	Request *req=(Request *)arg;
	//---- receive and analyse HTTP request line by line ----
	bool first=true;
	for(;;)
	  {
				if(recvLine(req->sock,buffer,BUFFER_SIZE)<=0) 
				{ break; }
				
				printf("%s",buffer);
			  
				if(first)
				{
						first=false;
						sscanf(buffer,"%s %s",req->requestMethod,req->requestUri);
				}
				else if(sscanf(buffer,"Content-Length : %s",req->contentLength))
				{
					printf("CONTENTLENGTH\n");
						// nothing more to be done
				}
				else if(!strcmp(buffer,"\n")||!strcmp(buffer,"\r\n"))
				{
						break; // end of header
				}
				else if(sscanf(buffer, "Upgrade : %31s", req->upgrade)==1)
				{
				  // nothing more to be done
				}
				else if(sscanf(buffer, "Sec-WebSocket-Key : %31s", req->wsKey)==1)
				{
				  // nothing more to be done
				}								
	  }
	//------------------------------------------------------------------------------------------
	//---- prepare and send HTTP reply ----
	sprintf(req->fileName,"./www%s",req->requestUri);
	char *params=strchr(req->fileName,'?');
	if(params) { *params='\0'; }
	struct stat st;
	int r=stat(req->fileName,&st);
	if((r!=-1)&&S_ISDIR(st.st_mode))
	  {	
			  strcat(req->fileName,"/index.html");
			  r=stat(req->fileName,&st);
	  }
	//------------------------------------------------------------------------------------------  
	//---- HANDLE UPGRADE TO WEBSOCKET REQUEST ----	  
	if(!strcmp(req->requestMethod, "GET")&& !strcmp(req->requestUri, "/")&& !strcmp(req->upgrade, "websocket"))
	{
		printf("upgrading to websocket\n");
		//---- prepare and send reply header ----
		wsHandshake(handshake, req->wsKey);
		
		 const int hLen=snprintf(header, sizeof(header),
                            "HTTP/1.1 101 Switching Protocols\r\n"
                            "Connection: Upgrade\r\n"
                            "Upgrade: websocket\r\n"
                            "Sec-WebSocket-Accept: %s\r\n"
                            "\r\n",
                            handshake);
		  sendAll(req->sock, header, hLen);
		  
		      //---- receive some parameters from client ----
			uint32_t params[3];
			WsResult r=wsRecv(req->sock, params, sizeof(params));
			if((r.opcode==WS_BIN)&&(r.length!=sizeof(params)))
			{
			  fprintf(stderr, "cannot receive parameters\n");
			  goto threadEnd;
			}
		

			const int repeatCount=(int)ntohl(params[2]);
			

			 //---- produce and send values to client ----
			for(int repeat=0; repeat<repeatCount; ++repeat)
			{
				senseHat_getPressureTemperature(&pressure, &temperature);

				values[0] = (int)(1000.0*pressure);
				values[1] = (int)(1000.0*temperature);
							
				sleep(2);

				for(int i=0; i<2; ++i) { values[i]=htonl(values[i]); }

			  wsSend(req->sock, values, sizeof(values), WS_BIN);	//>>>>>>>>>>>>>>>>>>>> WS SEND
			}						
			
			//---- signal end of websocket dialog ----
			wsSend(req->sock, NULL, 0, WS_CLOSE);

		  //---- websocket dialog stops here ----
    goto threadEnd;
	}	
		  
	//------------------------------------------------------------------------------------------	 
	// IF WE HAVE TO SERVE AN EXECUTABLE SCRIPT FILE ( CGI SCRIPT ) : 
	// rem : CGI scripts have to be placed in www/cgi/ folder
	if((access(req->fileName,R_OK|X_OK)==0) && !strncmp(req->requestUri, "/cgi",4))
	  {
	  
			  if(strcmp(req->requestMethod,"GET")&&strcmp(req->requestMethod,"POST"))
				{
						r=sprintf(buffer,"HTTP/1.0 400 Bad Request\n"
										 "Connection: close\n"
										 "Content-Type: text/html\n"
										 "\n"
										 "<html><body>\n"
										 "<b>400 - Bad Request</b><br>\n"
										 "method: %s<br>\n"
										 "uri: %s<br>\n"
										 "fileName: %s<br>\n"
										 "</body></html>\n",
										 req->requestMethod,
										 req->requestUri,
										 req->fileName);
						sendAll(req->sock,buffer,r);
				}
			  else
				{
							// HTTP RESPONSE HEADER
							r=sprintf(buffer,"HTTP/1.0 200 OK\n"
											 "Connection: close\n");
							sendAll(req->sock,buffer,r);
							pid_t child=fork();	// execvp needs a new process, otherwhise the executed process will destroy all current threads.
							switch(child)
							  {
									  case -1:
										{
											perror("fork");
											break;
										}
									  //############ CHILD CODE ######################################
									  case 0:		
										{
											setenv("REQUEST_METHOD",req->requestMethod,1); 	// needed for queryProperties script
											setenv("REQUEST_URI",req->requestUri,1);	 	// needed for queryProperties script	
											setenv("CONTENT_LENGTH",req->contentLength,1); 	// needed for queryProperties script
											createQueryString(req->requestUri,sizeof(req->requestUri),req->queryString);
											setenv("QUERY_STRING",req->queryString,1);
																														
											printf("DEBUG REQ=%s, FILENAME=%s QUERYSTRING=%s \n",req -> requestUri, req -> fileName, req -> queryString);	
											
											if(dup2(req->sock,0)==-1)
																					{ perror("dup2"); exit(1); }
											if(dup2(req->sock,1)==-1)
																					{ perror("dup2"); exit(1); }
											if(dup2(req->sock,2)==-1)
																					{ perror("dup2"); exit(1); }
											close(req->sock);
											char *args[2];
											args[0]=req->fileName;
											args[1]=(char *)0;
											execvp(args[0],args);
											perror("execvp");
											exit(1);
										}
									  //############ PARENT CODE #####################################
									  default:		
										{
											if(waitpid(child,(int *)0,0)==-1)
																					{ perror("waitpid"); }
										}
									  }
							}
			  }
			//------------------------------------------------------------------------------------------  
			else   // STATIC HTTP FILE SERVER
			  {
			  int input=open(req->fileName,O_RDONLY);
			  if(input==-1)
				{
							r=sprintf(buffer,"HTTP/1.0 404 Not Found\n"
											 "Connection: close\n"
											 "Content-Type: text/html\n"
											 "\n"
											 "<html><body>\n"
											 "<b>404 - Not Found</b><br>\n"
											 "method: %s<br>\n"
											 "uri: %s<br>\n"
											 "fileName: %s<br>\n"
											 "</body></html>\n",
											 req->requestMethod,
											 req->requestUri,
											 req->fileName);
							sendAll(req->sock,buffer,r);
				}
			  else
				{
							const char *contentType="unknown/unknown";
							const char *ext=strrchr(req->fileName,'.');
							if(ext)
							  {
									  if(!strcmp(ext,".html"))     contentType="text/html";
									  else if(!strcmp(ext,".png")) contentType="image/png";
									  else if(!strcmp(ext,".ico")) contentType="image/vnd.microsoft.icon";
							  }
							r=sprintf(buffer,"HTTP/1.0 200 OK\n"
											 "Connection: close\n"
											 "Content-Type: %s\n"
											 "Content-Length: %ld\n\n",
											 contentType,
											 st.st_size);
							sendAll(req->sock,buffer,r);
							for(;;)
							  {
									  r=read(input,buffer,BUFFER_SIZE);
									  if(r<=0) break;
									  sendAll(req->sock,buffer,r);
							  }
							close(input);
				}
	  }
	  
threadEnd:
	destroyRequest(req);
	return (void *)0;
	}

//======================================================================
//						MAIN
//======================================================================

int main(int argc, char **argv)
{

	  senseHat_init();

	//---- avoid exiting on broken client connection (ignore SIGPIPE) ----
	struct sigaction sa;
	memset(&sa,0,sizeof(struct sigaction));
	sa.sa_handler=SIG_IGN;
	if(sigaction(SIGPIPE,&sa,(struct sigaction *)0)==-1)
											{ perror("sigaction"); exit(1); }

	//---- check command line arguments ----
	if(argc!=2)
											{ fprintf(stderr,"usage: %s http_port\n",argv[0]); exit(1); }

	//---- extract local port number ----
	int httpPortNumber;
	if(sscanf(argv[1],"%d",&httpPortNumber)!=1)
											{ fprintf(stderr,"invalid port %s\n",argv[1]); exit(1); }

	//---- multithreaded TCP server launching dialogThread() ----

	//---- create HTTP listen socket ----
	int httpSocket=socket(PF_INET,SOCK_STREAM,0);
	if(httpSocket==-1)
											{ perror("socket"); exit(1); }
	// ... avoiding timewait problems (optional)
	int on=1;
	if(setsockopt(httpSocket,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int))==-1)
											{ perror("setsockopt"); exit(1); }
	// ... bound to any local address on the specified port
	struct sockaddr_in myAddr;
	myAddr.sin_family=AF_INET;
	myAddr.sin_port=htons(httpPortNumber);
	myAddr.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(httpSocket,(struct sockaddr *)&myAddr,sizeof(myAddr))==-1)
											{ perror("bind"); exit(1); }
	// ... listening connections
	if(listen(httpSocket,10)==-1)
											{ perror("listen"); exit(1); }

	for(;;)
	  {
		  //---- accept new HTTP connection ----
		  struct sockaddr_in fromAddr;
		  socklen_t len=sizeof(fromAddr);
		  int dialogSocket=accept(httpSocket,(struct sockaddr *)&fromAddr,&len);
		  if(dialogSocket==-1)
												{ perror("accept"); exit(1); }
		  printf("new HTTP connection from %s:%d\n",
							inet_ntoa(fromAddr.sin_addr),ntohs(fromAddr.sin_port));

		  //---- start a new dialog thread ----
		  pthread_t th;
		  if(pthread_create(&th,(pthread_attr_t *)0, dialogThread,createRequest(dialogSocket)))
												{ fprintf(stderr,"cannot create thread\n"); exit(1); }
	  }

	close(httpSocket);
	return 0;
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^ EOF ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
