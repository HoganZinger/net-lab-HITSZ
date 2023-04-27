#include "udp.h"
#include "ip.h"
#include "icmp.h"

/**
 * @brief udp处理程序表
 * 
 */
map_t udp_table;

/**
 * @brief udp伪校验和计算
 * 
 * @param buf 要计算的包
 * @param src_ip 源ip地址
 * @param dst_ip 目的ip地址
 * @return uint16_t 伪校验和
 */
static uint16_t udp_checksum(buf_t *buf, uint8_t *src_ip, uint8_t *dst_ip)
{
    // TO-DO

    uint16_t length = buf->len;
    // add peso header
    buf_add_header(buf, 12);

    // copy IP header
    buf_t t_buf;
    buf_init(&t_buf, 0);
    buf_add_header(&t_buf, 12);
    memcpy(t_buf.data, buf->data, 12);

    // add peso header
    udp_peso_hdr_t *header = (udp_peso_hdr_t*)buf->data;
    for(int i = 0; i < 4; i++){
        header->src_ip[i] = src_ip[i];
        header->dst_ip[i] = dst_ip[i];
    }
    header->placeholder = 0;
    // default protocol be UDP
    header->protocol = NET_PROTOCOL_UDP;
    // length
    header->total_len16 = swap16(length);
    // odd number, padding 1
    if(length%2 == 1){
        buf_add_padding(buf, 1);
    }

    // checksum
    uint16_t res = checksum16((uint16_t*)buf->data,buf->len);

    // remove padding
    buf_remove_padding(buf, 1);
    // copy IP head
    memcpy(buf->data,t_buf.data, 12);
    // remove peso header
    buf_remove_header(buf, 12);

    // return checksum
    return res;
}

/**
 * @brief 处理一个收到的udp数据包
 * 
 * @param buf 要处理的包,只包含UDP头部和数据
 * @param src_ip 源ip地址
 */
void udp_in(buf_t *buf, uint8_t *src_ip)
{
    // TO-DO
    if(buf->len > 8){
        udp_hdr_t *header = (udp_hdr_t*)buf->data;
        if(buf->len >= swap16(header->total_len16)){
            // check checksum
            uint16_t in_checksum = header->checksum16;
            header->checksum16 = 0;
            
            // calculate checksum
            uint16_t new_checksum = udp_checksum(buf, src_ip, net_if_ip);
            if(new_checksum != in_checksum)   
                return;
            header->checksum16 = in_checksum;

            // check whether port in udp_table 
            uint16_t dst_port = swap16(header->dst_port16);
            udp_handler_t *handler = map_get(&udp_table, &dst_port);

            // src port
            uint16_t src_port = swap16(header->src_port16);
            if (handler)
            {   
                // data header = 0
                memset(buf->data, 0, sizeof(udp_hdr_t));
                // move head pointer
                buf_remove_header(buf, sizeof(udp_hdr_t));
                // use handler to process
                (*handler)(buf->data, buf->len, src_ip, src_port);
            }else{
                // unable to access port
                buf_add_header(buf, 20);
                // add ipv4 header
                ip_hdr_t *header = (ip_hdr_t*)buf->data;
                header->version = IP_VERSION_4;
                header->hdr_len = 5;
                header->tos = 0;
                header->total_len16 = swap16(buf->len);
                header->id16 = swap16(0);
                header->flags_fragment16 = 0;
                header->ttl = 64;
                header->protocol = NET_PROTOCOL_UDP;
                for (size_t i = 0; i < 4; i++)
                {
                    header->src_ip[i] = src_ip[i];
                    header->dst_ip[i] = net_if_ip[i];
                }
                // calculate checksum
                header->hdr_checksum16 = 0;
                uint16_t mychecksum = checksum16((uint16_t*)header, 20);
                header->hdr_checksum16 = mychecksum;

                // port unreachable
                icmp_unreachable(buf, src_ip, ICMP_CODE_PORT_UNREACH);
            }
        }
    }       
}

/**
 * @brief 处理一个要发送的数据包
 * 
 * @param buf 要处理的包
 * @param src_port 源端口号
 * @param dst_ip 目的ip地址
 * @param dst_port 目的端口号
 */
void udp_out(buf_t *buf, uint16_t src_port, uint8_t *dst_ip, uint16_t dst_port)
{
    // TO-DO
    // add header
    buf_add_header(buf,8);
    udp_hdr_t *header=(udp_hdr_t*)buf->data;
    // src port
    header->src_port16=swap16(src_port);
    // dst port
    header->dst_port16=swap16(dst_port);
    // length
    uint16_t length=buf->len;
    header->total_len16=swap16(length);
    // 校验和
    header->checksum16=0;
    uint16_t mychecksum=udp_checksum(buf,net_if_ip,dst_ip);
    header->checksum16=mychecksum;
    // 发送ip_out数据报
    ip_out(buf,dst_ip,NET_PROTOCOL_UDP);
}

/**
 * @brief 初始化udp协议
 * 
 */
void udp_init()
{
    map_init(&udp_table, sizeof(uint16_t), sizeof(udp_handler_t), 0, 0, NULL);
    net_add_protocol(NET_PROTOCOL_UDP, udp_in);
}

/**
 * @brief 打开一个udp端口并注册处理程序
 * 
 * @param port 端口号
 * @param handler 处理程序
 * @return int 成功为0，失败为-1
 */
int udp_open(uint16_t port, udp_handler_t handler)
{
    return map_set(&udp_table, &port, &handler);
}

/**
 * @brief 关闭一个udp端口
 * 
 * @param port 端口号
 */
void udp_close(uint16_t port)
{
    map_delete(&udp_table, &port);
}

/**
 * @brief 发送一个udp包
 * 
 * @param data 要发送的数据
 * @param len 数据长度
 * @param src_port 源端口号
 * @param dst_ip 目的ip地址
 * @param dst_port 目的端口号
 */
void udp_send(uint8_t *data, uint16_t len, uint16_t src_port, uint8_t *dst_ip, uint16_t dst_port)
{
    buf_init(&txbuf, len);
    memcpy(txbuf.data, data, len);
    udp_out(&txbuf, src_port, dst_ip, dst_port);
}