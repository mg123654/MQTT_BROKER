#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "pack.h"


//获取8个字节
uint8_t unpack_u8(const uint8_t **buf)
{
    uint8_t val=**buf;
    (*buf)++;
    return val;
}
//获取16个字节
uint16_t unpack_u16(const uint8_t **buf){
    uint16_t val;
    memcpy(&val,*buf,sizeof(uint16_t));
    (*buf)+=sizeof(uint16_t);
    return nthol(val);

}
//获取四个字节
uint32_t unpack_u32(const uint8_t **buf) {
    uint32_t val;
    memcpy(&val, *buf, sizeof(uint32_t));
    (*buf) += sizeof(uint32_t);
    return ntohs(val);
}
//获取缓冲区固定长度的字符串
uint8_t* unpack_bytes(const uint8_t **buf ,size_t len,uint8_t *str){
    memcpy(str,*buf,len);
    str[len]='/0';
    (*buf )+=len;
    return str;
}

//获取MQTT报文当中的可变长度字符串
uint16_t unpack_string16(uint8_t **buf ,uint8_t **dest){
    uint16_t len=unpack_u16(buf);   //返回的两个字节数值是字符串长度
    *dest=malloc(len+1);
    *dest=unpack_bytes(buf,len,*dest);
    return len;
}

void pack_u8(uint8_t **buf ,uint8_t val){
    **buf=val;
    (*buf)++;
}

void pack_u16(uint8_t **buf ,uint16_t val){
    uint16_t nval=htons(val);
    memcpy(*buf,&nval,sizeof(uint16_t));
    (*buf)+=sizeof(uint16_t);
}

void pack_u32(uint8_t **buf ,uint32_t val){
    uint32_t nval=htonl(val);
    memcpy(*buf,&nval,sizeof(uint32_t));
    (*buf)+=sizeof(uint32_t);
}

void pack_bytes(uint8_t **buf ,uint8_t *src){
    size_t len=strlen((const char*)src);
    memcpy(*buf,src,len);
    (*buf)+=len;
}











