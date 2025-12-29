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

//解包publish报文，返回报文剩余长度
static size_t unpack_mqtt_publish(const unsigned char *buf ,mqtt_header *hdr,mqtt_packet* pkt){
    mqtt_publish publish ={.header = *hdr};
    pkt->publish=publish;

    size_t len=mqtt_decode_length((const unsigned char**)&buf);
    
    pkt->publish.topic_len=unpack_string16((const uint16_t**)&buf,(uint8_t*)pkt->publish.topic);

    size_t message_len=len;

    //pkt_id只会在QoS大于0时候出现，用于进行消息确认
    if(pkt->publish.header.bits.qos>AT_MOST_ONCE){
        pkt->publish.pkt_id=unpack_u16((const uint8_t *)&buf);
        message_len -= sizeof(uint16_t);

    }
    message_len-=(sizeof(uint16_t)+pkt->publish.topic_len);
    pkt->publish.payloadlen=message_len;
    pkt->publish.payload=malloc(message_len+1);
    unpack_bytes((const uint8_t**)&buf,message_len,pkt->publish.payload);
    return len;

}

static size_t unpack_subscribe(const unsigned char* buf,mqtt_header *hdr,mqtt_packet* pkt){

    mqtt_subscribe subscribe = {.header=hdr};
    
    size_t len=mqtt_decode_length((const unsigned char **)&buf);
    
    size_t remaining_bytes=(len-sizeof(uint16_t));

    //报文标识符
    unpack_u16((const uint8_t **)&buf);
    remaining_bytes-=sizeof(uint16_t);

    //对于mqtt3,没有属性（Properties）字段

    //解析有效载荷
    int i=0;
    while(remaining_bytes>0){
        //为tuples分配内存
        subscribe.tuples=realloc(subscribe.tuples,(i+1)*(sizeof(*subscribe.tuples)));
        
        subscribe.tuples[i].topic_len=unpack_string16((const uint8_t**)&buf,&(subscribe.tuples[i].topic));
        remaining_bytes-=(subscribe.tuples[i].topic_len+sizeof(uint16_t));

        //获取qos等级
        subscribe.tuples[i].qos=unpack_u8((unsigned char **)&buf);
        remaining_bytes-=sizeof(uint8_t);
        i++;

    }
    //i是tuples的数量和报文长度
    subscribe.tuples_len=i;
    pkt->subscribe=subscribe;
    return len;

}


static size_t unpack_mqtt_unsubscribe(const unsigned char *buf,mqtt_header* hdr ,mqtt_packet *pkt){

    mqtt_unsubscribe unsubscribe = {.header=*hdr};

    size_t len=mqtt_decode_length((const unsigned char **)&buf);
    
    size_t remaining_bytes=(len-sizeof(uint16_t));

    //报文标识符
    unpack_u16((const uint8_t **)&buf);
    remaining_bytes-=sizeof(uint16_t);

    //对于mqtt3,没有属性（Properties）字段

    //解析有效载荷
    int i=0;
    while(remaining_bytes>0){
        //为tuples分配内存
        unsubscribe.tuples=realloc(unsubscribe.tuples,(i+1)*(sizeof(*unsubscribe.tuples)));
        
        unsubscribe.tuples[i].topic_len=unpack_string16((const uint8_t**)&buf,&(unsubscribe.tuples[i].topic));
        remaining_bytes-=(unsubscribe.tuples[i].topic_len+sizeof(uint16_t));

        i++;
    }
    unsubscribe.tuples_len=i;
    pkt->unsubscribe = unsubscribe;
    return len;

}

typedef size_t mqtt_unpack_handler (const unsigned char **,mqtt_header* ,mqtt_packet*);

static mqtt_unpack_handler *unpack_handlers[11]={
    NULL,
    unpack_mqtt_connect,
    NULL,
    unpack_mqtt_publish,
    unpack_mqtt_ack,
    unpack_mqtt_ack,
    unpack_mqtt_ack,
    unpack_mqtt_ack,
    unpack_mqtt_subscribe,
    NULL,
    unpack_mqtt_unsubscribe
};

//将解包函数通过指针函数调用的形式进行使用。
int unpack_mqtt_packet(const unsigned char *buf ,mqtt_header*hdr ,mqtt_packet* pkt){
    int rc =0;
    unsigned char type = *buf;
    mqtt_header header={.byte=type};

    if(header.bits.type==DISCONNECT||header.bits.type==PINGREQ||header.bits.type==PINGRESP){

        pkt->header=header;

    }
    else{
        rc=unpack_handlers[header.bits.type](++buf,&header,pkt);
    }
    return rc;

}


//其它的一些报文结构生成函数：publish ack connack suback header 

mqtt_header* mqtt_packet_header(unsigned char byte){
    static mqtt_header header;
    header.byte=byte;
    return &header;

}

mqtt_ack* mqtt_pack_ack(unsigned char byte,unsigned short pkt_id){
    static mqtt_ack ack;
    ack.header.byte=byte;
    ack.pkt_id=pkt_id;
    return &ack;
}

mqtt_connack *mqtt_packet_connack(unsigned char byte,unsigned char cflags, unsigned char rc){
    static mqtt_connack connack;
    connack.byte=cflags;
    connack.header.byte=byte;
    connack.rc=rc;
    return &connack;

}

mqtt_suback *mqtt_packet_suback(unsigned char byte,unsigned short pkt_id,unsigned char *rcs,unsigned short rcslen){
    static mqtt_suback suback ;
    suback.header.byte=byte;
    suback.pkt_id=pkt_id;
    suback.rcslen=rcslen;
    suback.rcs=rcs;
    return &suback;
}

mqtt_publish *mqtt_packet_publish(unsigned char byte,unsigned short pkt_id,size_t topiclen,unsigned char *topic,size_t payloadlen,unsigned char* payload){
    mqtt_publish *publish = malloc(sizeof(*publish));
    publish->header.byte=byte;
    publish->pkt_id = pkt_id;
    publish->topic_len=topiclen;
    publish->topic=topic;
    publish->payloadlen=payloadlen;
    publish->payload=payload;
    return publish;
}


void mqtt_packet_release(mqtt_packet *pkt, unsigned type) {
    switch (type) {
        case CONNECT:
            free(pkt->connect.payload.client_id);
            if (pkt->connect.bits.username == 1)
                free(pkt->connect.payload.username);
            if (pkt->connect.bits.password == 1)
                free(pkt->connect.payload.password);
            if (pkt->connect.bits.will == 1) {
                free(pkt->connect.payload.will_message);
                free(pkt->connect.payload.will_topic);
            }
            break;
        case SUBSCRIBE:
        case UNSUBSCRIBE:
            for (unsigned i = 0; i < pkt->subscribe.tuples_len; i++)
                free(pkt->subscribe.tuples[i].topic);
            free(pkt->subscribe.tuples);
            break;
        case SUBACK:
            free(pkt->suback.rcs);
            break;
        case PUBLISH:
            free(pkt->publish.topic);
            free(pkt->publish.payload);
            break;
        default:
            break;
    }
}


typedef unsigned char *mqtt_pack_handler(const mqtt_packet*);
//定义指针函数
static mqtt_pack_handler *hadlers[13]={
    NULL,
    NULL,
    pack_mqtt_connack,
    pack_mqtt_publish,
    pack_mqtt_ack,
    pack_mqtt_ack,
    pack_mqtt_ack,
    pack_mqtt_ack,
    NULL,
    pack_mqtt_suback,
    NULL,pack_mqtt_ack,
    NULL
};


//将报文结构体生成报文数据包。
static unsigned char* pack_mqt_header(const mqtt_header*hdr){

    unsigned char *packed=malloc(MQTT_HEADER_LEN);
    unsigned char *ptr=packed;

    pack_u8(&ptr,hdr->byte);
    mqtt_encode_length(ptr,0);
    return packed;
}
#include <stdio.h>
int main(){
    return 0;
}
