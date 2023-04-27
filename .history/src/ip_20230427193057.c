#include "net.h"
#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包,IP数据包：IP头部+IP数据部分
 * @param src_mac 源mac地址
 */
int global_id = 0;

void ip_in(buf_t *buf, uint8_t *src_mac)
{
    // TO-DO
    ip_hdr_t *header = (ip_hdr_t*)(buf->data);
    if(buf->len > (4*header->hdr_len)){
        // check header
        // check version
        if(header->version != IP_VERSION_4)   
            return;
        // check length
        if(swap16(header->total_len16) > buf->len)    
            return;

        // calculate checksum
        uint16_t mychecksum = header->hdr_checksum16;
        // hdr checksum
        header->hdr_checksum16 = 0;
        // calculate checksum
        uint16_t new_checksum = checksum16((uint16_t*)header,20);
        // check checksum
        if(new_checksum != mychecksum)    
            return;
        header->hdr_checksum16 = mychecksum;

        // check IP
        int flag = 1;
        for(int i = 0; i < 4; i++){
            if(header->dst_ip[i] != net_if_ip[i]) 
                flag=0;
        }
        if(flag == 0) 
            return;

        // remove padding
        buf_remove_padding(buf,buf->len-swap16(header->total_len16));
        
        // upper protocol
        uint16_t proto=header->protocol;

        // copy data
        buf_t mybuf;
        buf_copy(&mybuf, buf, 0);

        // 移除IP报头
        memset(buf->data, 0, sizeof(ip_hdr_t));
        buf_remove_header(buf, sizeof(ip_hdr_t)); 
    
        // net_in
        ip_hdr_t *t_header = (ip_hdr_t*)mybuf.data;
        if(proto == NET_PROTOCOL_UDP || proto == NET_PROTOCOL_ICMP){
            net_in(buf, proto, t_header->src_ip);
        }else{
            icmp_unreachable(&mybuf, t_header->src_ip, ICMP_CODE_PROTOCOL_UNREACH); 
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
    buf_add_header(buf, 20);
    // add header
    ip_hdr_t *header = (ip_hdr_t*)buf->data;
    header->version = IP_VERSION_4;
    header->hdr_len = 5;
    header->tos = 0;
    header->total_len16 = swap16(buf->len);
    header->id16 = swap16(id);
    header->flags_fragment16 = 0;
    if(mf == 1){
        header->flags_fragment16 = header->flags_fragment16|IP_MORE_FRAGMENT;
    }
    header->flags_fragment16 = header->flags_fragment16|offset;
    header->flags_fragment16 = swap16(header->flags_fragment16);
    // ttl
    header->ttl = 64;
    // upper layer protocol
    header->protocol = protocol;
    // IP
    for (size_t i = 0; i < 4; i++)
    {
        header->src_ip[i] = net_if_ip[i];
        header->dst_ip[i] = ip[i];
    }

    header->hdr_checksum16 = 0;
    uint16_t mychecksum = checksum16((uint16_t*)header, 20);
    header->hdr_checksum16 = mychecksum;
    // 发送数据
    arp_out(buf, ip);
}

/**
 * @brief 处理一个要发送的ip数据包
 * 
 * @param buf 要处理的包，不含IP头部的上层数据
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
void ip_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol)
{   
    // TO-DO
    if(buf->len>1480){
        // needs sharding
        int length = buf->len;
        uint16_t offset = 0;
        int start = 0;

        while(length>1480){
            // IP sharding process
            buf_t ip_buf;
            buf_init(&ip_buf,1480);
            memcpy(ip_buf.data, buf->data + start, 1480);
            ip_fragment_out(&ip_buf, ip, protocol, global_id, offset,1);
            
            start+=1480;
            length-=1480;
            offset+=185;
        }
        buf_t ip_buf;
        buf_init(&ip_buf,length);
        memcpy(ip_buf.data,buf->data+start,length);
        ip_fragment_out(&ip_buf,ip,protocol,global_id,offset,0);
    }else{
        // 不需要分片
        ip_fragment_out(buf,ip,protocol,global_id,0,0);
    }
    global_id++;
}

/**
 * @brief 初始化ip协议
 * 
 */
void ip_init()
{
    net_add_protocol(NET_PROTOCOL_IP, ip_in);
}