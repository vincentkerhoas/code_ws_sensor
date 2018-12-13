// CREDITS : Fabric HARROUET - www.enib.fr/~harrouet
//======================================================================
#include "libWs.h"
#include "httpServerLib.h"
//======================================================================
// see https://developer.mozilla.org
//     /en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers
//======================================================================
void wsSha1_(uint32_t hash[5],
        uint32_t b[16])
{
  uint32_t a[5]={hash[4], hash[3], hash[2], hash[1], hash[0]};
  for(int i=0; i<16; ++i)
  {
    a[i%5]+=((a[(3+i)%5]&(a[(2+i)%5]^a[(1+i)%5]))^a[(1+i)%5])+
            b[i]+0x5a827999+wsRol_(a[(4+i)%5], 5);
    a[(3+i)%5]=wsRol_(a[(3+i)%5], 30);
  }
  for(int i=0; i<4; ++i)
  {
    b[i]=wsBlk_(b, i);
    a[(1+i)%5]+=((a[(4+i)%5]&(a[(3+i)%5]^a[(2+i)%5]))^a[(2+i)%5])+
                b[i]+0x5a827999+wsRol_(a[(5+i)%5], 5);
    a[(4+i)%5]=wsRol_(a[(4+i)%5], 30);
  }
  for(int i=0; i<20; ++i)
  {
    b[(i+4)%16]=wsBlk_(b, (i+4)%16);
    a[i%5]+=(a[(3+i)%5]^a[(2+i)%5]^a[(1+i)%5])+
            b[(i+4)%16]+0x6ed9eba1+wsRol_(a[(4+i)%5], 5);
    a[(3+i)%5]=wsRol_(a[(3+i)%5], 30);
  }
  for(int i=0; i<20; ++i)
  {
    b[(i+8)%16]=wsBlk_(b, (i+8)%16);
    a[i%5]+=(((a[(3+i)%5]|a[(2+i)%5])&a[(1+i)%5])|(a[(3+i)%5]&a[(2+i)%5]))+
            b[(i+8)%16]+0x8f1bbcdc+wsRol_(a[(4+i)%5], 5);
    a[(3+i)%5]=wsRol_(a[(3+i)%5], 30);
  }
  for(int i=0; i<20; ++i)
  {
    b[(i+12)%16]=wsBlk_(b, (i+12)%16);
    a[i%5]+=(a[(3+i)%5]^a[(2+i)%5]^a[(1+i)%5])+
            b[(i+12)%16]+0xca62c1d6+wsRol_(a[(4+i)%5], 5);
    a[(3+i)%5]=wsRol_(a[(3+i)%5], 30);
  }
  for(int i=0; i<5; ++i)
  {
    hash[i]+=a[4-i];
  }
}
//======================================================================
void wsHandshake(char handshake[29],char key[25])
{
  // inspired from https://github.com/alexhultman/libwshandshake
  int kLen=(int)strlen(key);
  while(kLen<24)
  {
    key[kLen++]='=';
  }
  if(kLen>24)
  {
    kLen=24;
  }
  key[kLen]='\0';
  uint32_t b_output[5]=
    {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
  uint32_t b_input[16]=
    {0, 0, 0, 0, 0, 0, 0x32353845, 0x41464135, 0x2d453931, 0x342d3437,
     0x44412d39, 0x3543412d, 0x43354142, 0x30444338, 0x35423131, 0x80000000};
  for(int i=0; i<6; i++)
  {
    b_input[i]=(uint32_t)((key[4*i+3]&255)<<0  | (key[4*i+2]&255)<<8 |
                          (key[4*i+1]&255)<<16 | (key[4*i+0]&255)<<24);
  }
  wsSha1_(b_output, b_input);
  uint32_t last_b[16]={0, 0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 480};
  wsSha1_(b_output, last_b);
  for(int i=0; i<5; i++)
  {
    uint32_t tmp=b_output[i];
    char *bytes=(char *)&b_output[i];
    bytes[3]=(char)((tmp>>0)&255);
    bytes[2]=(char)((tmp>>8)&255);
    bytes[1]=(char)((tmp>>16)&255);
    bytes[0]=(char)((tmp>>24)&255);
  }
  unsigned char *src=(unsigned char *)b_output;
  const char *b64="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                  "abcdefghijklmnopqrstuvwxyz"
                  "0123456789+/";
  char *dst=handshake;
  for(int i=0; i<18; i+=3)
  {
    *dst++=b64[(src[i]>>2)&63];
    *dst++=b64[((src[i]&3)<<4) | ((src[i+1]&240)>>4)];
    *dst++=b64[((src[i+1]&15)<<2) | ((src[i+2]&192)>>6)];
    *dst++=b64[src[i+2]&63];
  }
  *dst++=b64[(src[18]>>2)&63];
  *dst++=b64[((src[18]&3)<<4) | ((src[19]&240)>>4)];
  *dst++=b64[((src[19]&15)<<2)];
  *dst++='=';
  *dst++='\0';
}
//======================================================================
void wsSend(int fd, const void *content, int contentSize, WsOpcode opcode)
{
  uint8_t header[10];
  int headerLength=2;
  header[0]=(uint8_t)(128|(int)opcode);
  if(contentSize<126)
  {
    const uint8_t sz=(uint8_t)contentSize;
    header[1]=sz;
  }
  else if(contentSize<65536)
  {
    const uint16_t sz=(uint16_t)contentSize;
    header[1]=126;
    header[2]=(uint8_t)((sz>>8)&255);
    header[3]=(uint8_t)((sz>>0)&255);
    headerLength+=(int)sizeof(sz);
  }
  else
  {
    const uint64_t sz=(uint64_t)contentSize;
    header[1]=127;
    header[2]=(uint8_t)((sz>>56)&255);
    header[3]=(uint8_t)((sz>>48)&255);
    header[4]=(uint8_t)((sz>>40)&255);
    header[5]=(uint8_t)((sz>>32)&255);
    header[6]=(uint8_t)((sz>>24)&255);
    header[7]=(uint8_t)((sz>>16)&255);
    header[8]=(uint8_t)((sz>>8)&255);
    header[9]=(uint8_t)((sz>>0)&255);
    headerLength+=(int)sizeof(sz);
  }
  sendAll(fd, header, headerLength);
  sendAll(fd, content, contentSize); 
}
//======================================================================
WsResult wsRecv(int fd, void *buffer, int bufferCapacity)
{
  WsResult result={WS_NONE, 0};
  uint8_t header[2];
  if(recvAll(fd, header, 2)!=2)
  {
    return result;
  }
  // const bool fin=header[0]&128; // ignore fragmentation for now
  const WsOpcode opcode=(WsOpcode)(header[0]&15);
  uint32_t mask=header[1]&128;
  int length=(int)(header[1]&127);
  if(length==126)
  {
    uint8_t sz[2];
    if(recvAll(fd, &sz, sizeof(sz))!=sizeof(sz))
    {
      return result;
    }
    length=(int)((((uint16_t)sz[0])<<8)|(((uint16_t)sz[1])<<0));
  }
  else if(length==127)
  {
    uint8_t sz[8];
    if(recvAll(fd, &sz, sizeof(sz))!=sizeof(sz))
    {
      return result;
    }
    length=(int)((((uint64_t)sz[0])<<56)|(((uint64_t)sz[1])<<48)|
                 (((uint64_t)sz[2])<<40)|(((uint64_t)sz[3])<<32)|
                 (((uint64_t)sz[4])<<24)|(((uint64_t)sz[5])<<16)|
                 (((uint64_t)sz[6])<<8 )|(((uint64_t)sz[7])<<0 ));
  }
  if(mask&&(recvAll(fd, &mask, sizeof(mask))!=sizeof(mask)))
  {
    return result;
  }
  if(!length)
  {
    return result;
  }
  const int trailingZero=opcode==WS_TXT ? 1 : 0;
  if(length+trailingZero>bufferCapacity)
  {
    fprintf(stderr, "insufficient capacity for websocket message\n");
    abort();
  }
  if(recvAll(fd, buffer, length)!=length)
  {
    return result;
  }
  if(mask)
  {
    uint32_t *words=(uint32_t *)buffer;
    const int wordCount=length/(int)sizeof(*words);
    for(int i=0; i<wordCount; ++i)
    {
      words[i]^=mask;
    }
    const uint8_t *byteMask=(const uint8_t *)&mask;
    uint8_t *bytes=(uint8_t *)(words+wordCount);
    const int byteCount=length-wordCount*(int)sizeof(*words);
    for(int i=0; i<byteCount; ++i)
    {
      bytes[i]^=byteMask[i];
    }
  }
  if(trailingZero)
  {
    ((char *)buffer)[length]='\0';
  }
  result.opcode=opcode;
  result.length=length;
  return result;
}
//======================================================================
