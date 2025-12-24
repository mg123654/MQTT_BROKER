//实现mqtt.h当中定义的函数，在本文件中首先定义了一些私有工具函数
#include <stdlib.h>
#include <string.h>
#include "mqtt.h"
#include "pack.h"

/*报文类型可以分为：
    1、连接相关：
        conenct connack(服务端的响应） disconnect（客户端断开连接） 
    2、发布订阅相关
    3、QoS相关确认报文**
    4、心跳/网络保持报文**

*/ 
static size_t unpack_mqtt_connect(const unsigned char* ,mqtt_header* ,mqtt_packet *);
static size_t unpack_mqtt_publish(const unsigned char*,mqtt_header*,mqtt_packet*);
static size_t unpack_mqtt_subscribe(const unsigned char *,  mqtt_header *, mqtt_packet *);
static size_t unpack_mqtt_unsubscribe(const unsigned char *, mqtt_header *,  mqtt_packet *);
static size_t unpack_mqtt_ack(const unsigned char *,  mqtt_header *, mqtt_packet *);
static unsigned char *pack_mqtt_header(const  mqtt_header *);
static unsigned char *pack_mqtt_ack(const mqtt_packet *);
static unsigned char *pack_mqtt_connack(const  mqtt_packet *);
static unsigned char *pack_mqtt_suback(const mqtt_packet *);
static unsigned char *pack_mqtt_publish(const mqtt_packet *);

//
static const int MAX_LEN_BYTES = 4;

//
int mqtt_encode_length(unsigned char *buf , size_t len ){
    int bytes = 0;
    do{
        if (bytes+1 > MAX_LEN_BYTES)
            return bytes;
        short d =len %0x80;
        len/=128;
        //如果还有更多的数据要编码，则将最高位设置为1
        if (len >0)
            d |=0x80;
        buf[bytes++] = (unsigned char)d;
    
        } while (len >0);
        return bytes;
  
}


unsigned long long mqtt_decode_length(const unsigned char **buf){
    int multiplier =1;
    long long value =0;
    unsigned char encodedByte;
    int bytes =0;
    do {
        encodedByte = unpack_u8(buf);
        value += (encodedByte &0x7F) * multiplier;
        multiplier *=128;
        bytes++;

    } while ((encodedByte &0x80) !=0);
    return value;
}


//传入的buf指针指向起始位置后面第一个字节（猜测是固定抱头的第二个字节）
static size_t unpack_mqtt_connect(const unsigned char *buf, mqtt_header *hdr,mqtt_packet* pkt){
    mqtt_connect connect={.header=*hdr};
    //结构体值拷贝操作，直接覆盖旧的结构体内存
    pkt->connect=connect;
    const unsigned char *init = buf;

    size_t len = mqtt_decode_length(&buf);
    
    //init指针始终保持原位，buf用来移动。
    buf = init + 8;

    pkt->connect.byte=unpack_u8((const uint8_t**)&buf);
    pkt->connect.payload.keepalive=unpack_u16((const uint16_t**)&buf);

    //获取客户端的可变长ID，长度由前面两个字节表示
    unpack_string16((uint8_t**)&buf,pkt->connect.payload.client_id);
    
    //根据标志位获取遗嘱主题和遗嘱消息
    if (pkt->connect.bits.will){
        unpack_string16((uint8_t**)&buf,pkt->connect.payload.will_topic);
        unpack_string16((uint8_t**)&buf,pkt->connect.payload.will_message);
    }

    //根据标志位获取用户名和密码
    if (pkt->connect.bits.username){
        unpack_string16((uint8_t**)&buf,pkt->connect.payload.username);
    }

    if (pkt->connect.bits.password){
        unpack_string16((uint8_t**)&buf,pkt->connect.payload.password);
    }
    return len;
}


static size_t unpack_mqtt_publish(const unsigned char *buf ,mqtt_header *hdr,mqtt_packet* pkt){
    mqtt_publish publish ={.header = *hdr};
    pkt->publish=publish;

    size_t len=mqtt_decode_length((const unsigned char**)&buf);

    pkt->publish.topic_len=unpack_string16((const uint16_t**)&buf,(uint8_t*)pkt->publish.topic);

    size_t message_len=len;

    //pkt_id只会在QoS大于0时候出现，用于进行消息确认
    if(pkt->publish.header.bits.qos>AT_MOST_ONCE){
        pkt->publish.pkt.id=
    }
}













