#include "main.h"

// see https://developer.mozilla.org
//     /en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers
//======================================================================
typedef enum
{
  WS_NONE=-1, WS_CONT_=0, WS_TXT=1, WS_BIN=2,
  WS_CLOSE=8, WS_PING=9, WS_PONG=10
} WsOpcode;

typedef struct
{
  WsOpcode opcode;
  int length;
} WsResult;
//======================================================================
inline static
uint32_t
wsRol_(uint32_t value,
       int bits)
{
  return (value<<bits)|(value>>(32-bits));
}

inline static
uint32_t
wsBlk_(uint32_t b[16],
       int s)
{
  return wsRol_(b[(s+13)&15]^b[(s+8)&15]^b[(s+2)&15]^b[s], 1);
}
//======================================================================
void wsSha1_(uint32_t hash[5],   uint32_t b[16]);
void wsHandshake(char handshake[29], char key[25]);
void wsSend(int fd, const void *content, int contentSize, WsOpcode opcode);
WsResult wsRecv(int fd, void *buffer, int bufferCapacity);
