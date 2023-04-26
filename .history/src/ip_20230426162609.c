#include "net.h"
#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 * @param src_mac 源mac地址
 */
void ip_in(buf_t *buf, uint8_t *src_mac)
{
    // TO-DO
    ip_hdr_t *header=(ip_hdr_t*)(buf->data);
    if(buf->len > (4*header->hdr_len)){
        // check header
        // check version
        if(header->version!=IP_VERSION_4)
            return;
        // check length
        if(swap16(header->total_len16) > buf->len)
            return;

        // caculate checksum
        // save total_len
        uint16_t my_checksum = header->hdr_checksum16;
        // hdr_checksum=0
        header->hdr_checksum16 = 0;
        // caculate header_checksum
        uint16_t new_checksum = checksum16((uint16_t*)header, 20);
        // check checksum
        if(new_checksum != my_checksum)
            return;

        header->hdr_checksum16 = my_checksum;

        // check IP address
        int flag = 1;
        for(int i = 0; i < 4; i++){
            if(header->dst_ip[i] != net_if_ip[i])
                flag = 0;
        }
        if(flag == 0)   
            return;

    }
}

/**
 * @brief 处理一个要发送的ip分片
 * 
 * @param buf 要发送的分片
 * @param ip 目标ip地址
 * @param protocol 上层协议
 * @param id 数据包id
 * @param offset 分片offset，必须被8整除
 * @param mf 分片mf标志，是否有下一个分片
 */
void ip_fragment_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol, int id, uint16_t offset, int mf)
{
    // TO-DO
}

/**
 * @brief 处理一个要发送的ip数据包
 * 
 * @param buf 要处理的包
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
void ip_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol)
{
    // TO-DO
}

/**
 * @brief 初始化ip协议
 * 
 */
void ip_init()
{
    net_add_protocol(NET_PROTOCOL_IP, ip_in);
}