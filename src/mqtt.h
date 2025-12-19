#ifndef MQTT_H
#define MQTT_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
// ==================== MQTT 3.1.1 协议常量 ====================
#define MQTT_PROTOCOL_NAME "MQTT"
#define MQTT_PROTOCOL_LEVEL 4     // MQTT 3.1.1

// 最大报文长度限制 (256MB - 1, 协议规定)
#define MQTT_MAX_PACKET_SIZE (256 * 1024 * 1024 - 1)

// ==================== 报文类型枚举 (Packet Type) ====================
typedef enum {
    MQTT_PACKET_RESERVED     = 0,   // 保留
    MQTT_PACKET_CONNECT      = 1,   // 客户端请求连接
    MQTT_PACKET_CONNACK      = 2,   // 连接确认
    MQTT_PACKET_PUBLISH      = 3,   // 发布消息
    MQTT_PACKET_PUBACK       = 4,   // QoS 1 发布确认
    MQTT_PACKET_PUBREC       = 5,   // QoS 2 发布接收
    MQTT_PACKET_PUBREL       = 6,   // QoS 2 发布释放
    MQTT_PACKET_PUBCOMP      = 7,   // QoS 2 发布完成
    MQTT_PACKET_SUBSCRIBE    = 8,   // 订阅请求
    MQTT_PACKET_SUBACK       = 9,   // 订阅确认
    MQTT_PACKET_UNSUBSCRIBE  = 10,  // 取消订阅
    MQTT_PACKET_UNSUBACK     = 11,  // 取消订阅确认
    MQTT_PACKET_PINGREQ      = 12,  // 心跳请求
    MQTT_PACKET_PINGRESP     = 13,  // 心跳响应
    MQTT_PACKET_DISCONNECT   = 14   // 断开连接
} mqtt_packet_type_t;

// ==================== QoS 等级枚举 ====================
typedef enum {
    MQTT_QOS_0 = 0,  // 至多一次
    MQTT_QOS_1 = 1,  // 至少一次
    MQTT_QOS_2 = 2   // 恰好一次
} mqtt_qos_t;

// ==================== 固定头 (Fixed Header) ====================
// 固定头标志位结构 (按协议位顺序，从高位到低位)
typedef struct {
    unsigned type   : 4;    // 报文类型 (bits 7-4)
    unsigned dup    : 1;    // 重发标志 (bit 3)
    unsigned qos    : 2;    // QoS 等级 (bits 2-1)
    unsigned retain : 1;    // 保留标志 (bit 0)
} mqtt_fixed_header_bits_t;

// 固定头联合体，支持位域和字节两种访问方式
typedef union {
    uint8_t byte;                     // 整个字节
    mqtt_fixed_header_bits_t bits;    // 位域访问
} mqtt_fixed_header_t;

// ==================== 连接标志 (Connect Flags) ====================
// 连接标志位域 (按协议字节顺序，从高位到低位)
typedef struct {
    unsigned reserved      : 1;  // Bit 7: 保留位 (必须为0)
    unsigned clean_session : 1;  // Bit 6: 清理会话标志 (1=清理, 0=保留)
    unsigned will_flag     : 1;  // Bit 5: 遗嘱标志 (1=有遗嘱, 0=无遗嘱)
    unsigned will_qos_high : 1;  // Bit 4: 遗嘱QoS高位
    unsigned will_qos_low  : 1;  // Bit 3: 遗嘱QoS低位
    unsigned will_retain   : 1;  // Bit 2: 遗嘱保留标志 (1=保留, 0=不保留)
    unsigned password_flag : 1;  // Bit 1: 密码标志 (1=有密码, 0=无密码)
    unsigned username_flag : 1;  // Bit 0: 用户名标志 (1=有用户名, 0=无用户名)
} mqtt_connect_flags_bits_t;

// 连接标志联合体
typedef union {
    uint8_t byte;                          // 整个字节
    mqtt_connect_flags_bits_t bits;        // 位域访问
} mqtt_connect_flags_t;

// ==================== CONNECT 报文结构 ====================
typedef struct {
    // 固定头 (已解析)
    mqtt_fixed_header_t fixed_header;
    
    // 可变头 (Variable Header)
    uint8_t protocol_level;                 // 协议级别 (固定为4)
    mqtt_connect_flags_t connect_flags;     // 连接标志
    uint16_t keep_alive;                    // 保活时间 (秒)
    
    // 有效载荷 (Payload)
    uint16_t client_id_len;                 // 客户端ID长度
    char* client_id;                        // 客户端ID (UTF-8字符串)
    
    uint16_t will_topic_len;                // 遗嘱主题长度 (仅当will_flag=1)
    char* will_topic;                       // 遗嘱主题
    
    uint16_t will_message_len;              // 遗嘱消息长度 (仅当will_flag=1)
    uint8_t* will_message;                  // 遗嘱消息 (二进制数据)
    
    uint16_t username_len;                  // 用户名长度 (仅当username_flag=1)
    char* username;                         // 用户名
    
    uint16_t password_len;                  // 密码长度 (仅当password_flag=1)
    uint8_t* password;                      // 密码 (二进制数据)
    
    // 辅助字段 (根据标志自动设置)
    bool has_will;                          // 是否有遗嘱
    bool has_username;                      // 是否有用户名
    bool has_password;                      // 是否有密码
} mqtt_connect_packet_t;

// ==================== CONNACK 报文结构 ====================
typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
    
    // 可变头
    uint8_t session_present;                // 会话存在标志 (bit 0)
    uint8_t return_code;                    // 返回码
} mqtt_connack_packet_t;

// CONNACK 返回码枚举
typedef enum {
    CONNACK_ACCEPTED                 = 0x00,  // 连接已接受
    CONNACK_REFUSED_PROTOCOL_VERSION = 0x01,  // 不支持的协议版本
    CONNACK_REFUSED_IDENTIFIER       = 0x02,  // 标识符拒绝
    CONNACK_REFUSED_SERVER           = 0x03,  // 服务器不可用
    CONNACK_REFUSED_BAD_CREDENTIALS  = 0x04,  // 错误的用户名或密码
    CONNACK_REFUSED_NOT_AUTHORIZED   = 0x05   // 未授权
} mqtt_connack_return_code_t;

// ==================== PUBLISH 报文结构 ====================
typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
    
    // 可变头
    uint16_t topic_name_len;                // 主题名长度
    char* topic_name;                       // 主题名 (UTF-8字符串)
    uint16_t packet_id;                     // 报文标识符 (仅QoS>0时需要)
    
    // 有效载荷
    uint32_t payload_len;                   // 消息载荷长度
    uint8_t* payload;                       // 消息载荷 (二进制数据)
} mqtt_publish_packet_t;

// ==================== PUBACK 报文结构 ====================
typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
    
    // 可变头
    uint16_t packet_id;                     // 报文标识符
} mqtt_puback_packet_t;

// ==================== PUBREC 报文结构 ====================
typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
    
    // 可变头
    uint16_t packet_id;                     // 报文标识符
} mqtt_pubrec_packet_t;

// ==================== PUBREL 报文结构 ====================
typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
    
    // 可变头
    uint16_t packet_id;                     // 报文标识符
} mqtt_pubrel_packet_t;

// ==================== PUBCOMP 报文结构 ====================
typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
    
    // 可变头
    uint16_t packet_id;                     // 报文标识符
} mqtt_pubcomp_packet_t;

// ==================== SUBSCRIBE 报文结构 ====================
// 单个订阅请求
typedef struct {
    uint16_t topic_filter_len;              // 主题过滤器长度
    char* topic_filter;                     // 主题过滤器 (UTF-8字符串)
    uint8_t qos;                            // 请求的QoS等级
} mqtt_subscription_t;

typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
    
    // 可变头
    uint16_t packet_id;                     // 报文标识符
    
    // 有效载荷
    uint16_t subscription_count;            // 订阅数量
    mqtt_subscription_t* subscriptions;     // 订阅数组
} mqtt_subscribe_packet_t;

// ==================== SUBACK 报文结构 ====================
typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
    
    // 可变头
    uint16_t packet_id;                     // 报文标识符
    
    // 有效载荷
    uint16_t return_code_count;             // 返回码数量
    uint8_t* return_codes;                  // 返回码数组 (每个订阅对应一个)
} mqtt_suback_packet_t;

// SUBACK 返回码
typedef enum {
    SUBACK_SUCCESS_QOS_0    = 0x00,  // 成功 - QoS 0
    SUBACK_SUCCESS_QOS_1    = 0x01,  // 成功 - QoS 1
    SUBACK_SUCCESS_QOS_2    = 0x02,  // 成功 - QoS 2
    SUBACK_FAILURE          = 0x80   // 失败
} mqtt_suback_return_code_t;

// ==================== UNSUBSCRIBE 报文结构 ====================
typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
    
    // 可变头
    uint16_t packet_id;                     // 报文标识符
    
    // 有效载荷
    uint16_t topic_filter_count;            // 主题过滤器数量
    uint16_t* topic_filter_lengths;         // 每个过滤器的长度数组
    char** topic_filters;                   // 主题过滤器数组
} mqtt_unsubscribe_packet_t;

// ==================== UNSUBACK 报文结构 ====================
typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
    
    // 可变头
    uint16_t packet_id;                     // 报文标识符
} mqtt_unsuback_packet_t;

// ==================== PINGREQ 报文结构 ====================
typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
} mqtt_pingreq_packet_t;

// ==================== PINGRESP 报文结构 ====================
typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
} mqtt_pingresp_packet_t;

// ==================== DISCONNECT 报文结构 ====================
typedef struct {
    // 固定头
    mqtt_fixed_header_t fixed_header;
} mqtt_disconnect_packet_t;

// ==================== 通用报文联合体 ====================
// 用于存储任意类型的报文
typedef union {
    mqtt_fixed_header_t fixed_header;         // 所有报文都有固定头
    
    mqtt_connect_packet_t connect;
    mqtt_connack_packet_t connack;
    mqtt_publish_packet_t publish;
    mqtt_puback_packet_t puback;
    mqtt_pubrec_packet_t pubrec;
    mqtt_pubrel_packet_t pubrel;
    mqtt_pubcomp_packet_t pubcomp;
    mqtt_subscribe_packet_t subscribe;
    mqtt_suback_packet_t suback;
    mqtt_unsubscribe_packet_t unsubscribe;
    mqtt_unsuback_packet_t unsuback;
    mqtt_pingreq_packet_t pingreq;
    mqtt_pingresp_packet_t pingresp;
    mqtt_disconnect_packet_t disconnect;
} mqtt_any_packet_t;

#endif

