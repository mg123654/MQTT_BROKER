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





