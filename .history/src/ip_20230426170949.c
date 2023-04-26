#include "net.h"
#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"

int global_id=0;

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

        // remove padding
        buf_remove_padding(buf, buf->len-swap16(header->total_len16));

        // upper-layer protocol
        uint16_t up_protocal = header->protocol;

        // copy data
        buf_t my_buf;
        buf_copy(&my_buf, buf , 0);

        // remove IP head
        memset(buf->data, 0, sizeof(ip_hdr_t));
        buf_remove_header(buf, sizeof(ip_hdr_t));

        // pass IP to upper layer
        ip_hdr_t *up_header = (ip_hdr_t*)my_buf.data;
        if(up_protocal == NET_PROTOCOL_UDP || up_protocal == NET_PROTOCOL_ICMP){
            net_in(buf, up_protocal, up_header->src_ip);
        }
        else{
            icmp_unreachable(&my_buf, up_header->src_ip, ICMP_CODE_PROTOCOL_UNREACH);
        }

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
    // add header
    buf_add_header(buf, 20);
    ip_hdr_t *header = (ip_hdr_t*)buf->data;
    // add version, length, TOS
    header->version = IP_VERSION_4;
    header->hdr_len = 5;
    header->tos = 0;
    // add total_len
    header->total_len16 = swap16(buf->len);
    // add flag
    header->flags_fragment16 = 0;
    if(mf == 1){
        header->flags_fragment16 = header->flags_fragment16|IP_MORE_FRAGMENT;
    }
    // add offset
    header->flags_fragment16 = header->flags_fragment16|offset;
    header->flags_fragment16 = swap(header->flags_fragment16);
    // add ttl
    header->ttl = 64;
    // add upper protocol
    header->protocol = protocol;
    // IP
    for(size_t i = 0; i < 4; i++){
        header->src_ip[i] = net_if_ip[i];
        header->dst_ip[i] = ip[i];
    }
    
    // calculate checksum
    header->hdr_checksum16 = 0;
    uint16_t my_checksum = checksum16((uint16_t*)header, 20);
    header->hdr_checksum16 = my_checksum;

    // send
    arp_out(buf, ip);
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
    // IP sharding
    if(buf->len > 1480){
        uint16_t offset = 0;
        int len = buf->len;
        int start = 0;
        while(len > 1480){
            buf_t ip_buf;
            buf_init(&ip_buf, 1480);
            memcpy(ip_buf.data, buf->data + start, 1480);
            ip_fragment_out(&ip_buf, ip, protocol, global_id, offset, 1);
        
            start += 1480;
            len -= 1480;
            offset += 185;
        }
        buf_t ip_buf;
        buf_init(&ip_buf, len);
        memcpy(ip_buf.data, buf->data + start, len)
    }
}

/**
 * @brief 初始化ip协议
 * 
 */
void ip_init()
{
    net_add_protocol(NET_PROTOCOL_IP, ip_in);
}