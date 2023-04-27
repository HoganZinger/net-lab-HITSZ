#include "ethernet.h"
#include "utils.h"
#include "driver.h"
#include "arp.h"
#include "ip.h"
#include <stdio.h>
#include <stdlib.h>
/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 */
void ethernet_in(buf_t *buf)
{
    // TO-DO
    if(buf->len>14){
        // remove header
        buf_t buf2;
        buf_copy(&buf2, buf, 0);
        memset(buf->data, 0, sizeof(ether_hdr_t));
        buf_remove_header(buf, sizeof(ether_hdr_t));
        ether_hdr_t *hdr = (ether_hdr_t *)buf2.data;
        uint16_t protocol = swap16(hdr->protocol16);
        net_in(buf, protocol, hdr->src);
    }
}
/**
 * @brief 处理一个要发送的数据包
 * 
 * @param buf 要处理的数据包
 * @param mac 目标MAC地址
 * @param protocol 上层协议
 */
void ethernet_out(buf_t *buf, const uint8_t *mac, net_protocol_t protocol)
{
    // TO-DO
    if(buf->len<46){
        // padding
        buf_add_padding(buf,46-buf->len);
    }
    // add header
    buf_add_header(buf, sizeof(ether_hdr_t));
    ether_hdr_t *hdr = (ether_hdr_t *)buf->data;
    // dst mac
    for(int i = 0; i < 6; i++){
        hdr->dst[i] = mac[i];
    }
    // src mac
    for(int i = 0; i < 6; i++){
        hdr->src[i] = net_if_mac[i];
    }
    //封装以太网类型,并进行大小端交换
    hdr->protocol16=swap16(protocol);
    //发送buf
    driver_send(buf);
}
/**
 * @brief 初始化以太网协议
 * 
 */
void ethernet_init()
{
    buf_init(&rxbuf, ETHERNET_MAX_TRANSPORT_UNIT + sizeof(ether_hdr_t));
}

/**
 * @brief 一次以太网轮询
 * 
 */
void ethernet_poll()
{
    if (driver_recv(&rxbuf) > 0)
        ethernet_in(&rxbuf);
}
