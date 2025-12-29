#ifndef MQTT_H
#define MQTT_H
#include <stdlib.h>
#include <stdio.h>
// ==================== MQTT 3.1.1 协议常量 ====================
#define MQTT_PROTOCOL_NAME "MQTT"
#define MQTT_PROTOCOL_LEVEL 4     // MQTT 3.1.1
#define CONNACK_BYTE  0x20
#define PUBLISH_BYTE  0x30
#define PUBACK_BYTE   0x40
#define PUBREC_BYTE   0x50
#define PUBREL_BYTE   0x60
#define PUBCOMP_BYTE  0x70
#define SUBACK_BYTE   0x90
#define UNSUBACK_BYTE 0xB0
#define PINGRESP_BYTE 0xD0

#define MQTT_HEADER_LEN 2

/* Message types */
enum packet_type {
    CONNECT     = 1,
    CONNACK     = 2,
    PUBLISH     = 3,
    PUBACK      = 4,
    PUBREC      = 5,
    PUBREL      = 6,
    PUBCOMP     = 7,
    SUBSCRIBE   = 8,
    SUBACK      = 9,
    UNSUBSCRIBE = 10,
    UNSUBACK    = 11,
    PINGREQ     = 12,
    PINGRESP    = 13,
    DISCONNECT  = 14
};

enum qos_level { AT_MOST_ONCE, AT_LEAST_ONCE, EXACTLY_ONCE };

typedef union  {
    unsigned char byte;
    struct {
        unsigned retain : 1;
        unsigned qos : 2;
        unsigned dup : 1;
        unsigned type : 4;
    } bits;
}mqtt_header;

typedef struct  {
    mqtt_header header;
    union {
        unsigned char byte;
        struct {
            int reserved : 1;
            unsigned clean_session : 1;
            unsigned will : 1;
            unsigned will_qos : 2;
            unsigned will_retain : 1;
            unsigned password : 1;
            unsigned username : 1;
        } bits;
    };
    struct {
        unsigned short keepalive;
        unsigned char *client_id;
        unsigned char *username;
        unsigned char *password;
        unsigned char *will_topic;
        unsigned char *will_message;
    } payload;
}mqtt_connect;

typedef struct  {
    mqtt_header header;
    union {
        unsigned char byte;
        struct {
            unsigned session_present : 1;
            unsigned reserved : 7;
        } bits;
    };
    unsigned char rc;
}mqtt_connack;

typedef struct {
    mqtt_header header;
    unsigned short pkt_id;
    unsigned short tuples_len;
    struct {
        unsigned short topic_len;
        unsigned char * topic;
        unsigned qos;
    }*tuples;
}mqtt_subscribe;

typedef struct {
    mqtt_header header;
    unsigned short pkt_id;
    unsigned short tuples_len;
    struct {
        unsigned short topic_len;
        unsigned char *topic;

    }*tuples;
}mqtt_unsubscribe;

typedef struct{
    mqtt_header header;
    unsigned short pkt_id;
    unsigned short rcslen;
    unsigned char *rcs;

}mqtt_suback;

typedef struct {
    mqtt_header header;
    unsigned short pkt_id;
    unsigned short topic_len;
    unsigned char * topic;
    unsigned short payloadlen;
    unsigned char *payload;
}mqtt_publish;

typedef struct 
{   
    mqtt_header header;
    unsigned short pkt_id ;

    
}mqtt_ack;


//语意分离

typedef mqtt_ack mqtt_puback;
typedef  mqtt_ack mqtt_pubrec;
typedef mqtt_ack mqtt_pubrel;
typedef mqtt_ack mqtt_pubcomp;
typedef  mqtt_ack mqtt_unsuback;
typedef mqtt_header mqtt_pingreq;
typedef  mqtt_header mqtt_pingresp;
typedef  mqtt_header mqtt_disconnect;


typedef union {
     mqtt_ack ack;
     mqtt_header header;
     mqtt_connect connect;
     mqtt_connack connack;
    mqtt_suback suback;
     mqtt_publish publish;
    mqtt_subscribe subscribe;
    mqtt_unsubscribe unsubscribe;
}mqtt_packet;

//对数值进行编解码，对报文包装和解包装
int mqtt_encode_length(unsigned char *, size_t);
unsigned long long mqtt_decode_length(const unsigned char **);
int unpack_mqtt_packet(const unsigned char *, mqtt_packet *);
unsigned char *pack_mqtt_packet(const  mqtt_packet *, unsigned);


//工具函数
mqtt_header *mqtt_packet_header(unsigned char);
mqtt_ack *mqtt_packet_ack(unsigned char ,unsigned char ,unsigned char);
mqtt_connack *mqtt_pack_connack(unsigned char, unsigned char,unsigned char);
mqtt_suback *mqtt_packet_suback(unsigned char ,unsigned short ,unsigned char*,unsigned short );
mqtt_publish *mqtt_packet_publish (unsigned char,unsigned short ,size_t, unsigned char* , size_t, unsigned char*);
void mqtt_packet_release(mqtt_packet*,unsigned int);

//size_t 在不同平台被定义为不同的字节长度，表示为该平台的最大字节类型。
//typedef unsigned long size_t;  



















#endif

