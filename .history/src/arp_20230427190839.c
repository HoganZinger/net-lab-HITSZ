#include <string.h>
#include <stdio.h>
#include "net.h"
#include "arp.h"
#include "ethernet.h"
/**
 * @brief 初始的arp包
 * 
 */
static const arp_pkt_t arp_init_pkt = {
    .hw_type16 = constswap16(ARP_HW_ETHER),
    .pro_type16 = constswap16(NET_PROTOCOL_IP),
    .hw_len = NET_MAC_LEN,
    .pro_len = NET_IP_LEN,
    .sender_ip = NET_IF_IP,
    .sender_mac = NET_IF_MAC,
    .target_mac = {0}};

/**
 * @brief arp地址转换表，<ip,mac>的容器
 * 
 */
map_t arp_table;

/**
 * @brief arp buffer，<ip,buf_t>的容器
 * 
 */
map_t arp_buf;

/**
 * @brief 打印一条arp表项
 * 
 * @param ip 表项的ip地址
 * @param mac 表项的mac地址
 * @param timestamp 表项的更新时间
 */
void arp_entry_print(void *ip, void *mac, time_t *timestamp)
{
    printf("%s | %s | %s\n", iptos(ip), mactos(mac), timetos(*timestamp));
}

/**
 * @brief 打印整个arp表
 * 
 */
void arp_print()
{
    printf("===ARP TABLE BEGIN===\n");
    map_foreach(&arp_table, arp_entry_print);
    printf("===ARP TABLE  END ===\n");
}

/**
 * @brief 发送一个arp请求
 * 
 * @param target_ip 想要知道的目标的ip地址
 */
void arp_req(uint8_t *target_ip)
{
    // TO-DO
    // init buf
    buf_init(&txbuf, 0);
    // add arp header
    buf_add_header(&txbuf, 4*sizeof(uint8_t));
    for(int i=0;i<4;i++){
        txbuf.data[i]=target_ip[i];
    }
    // dst MAC
    buf_add_header(&txbuf, 6*sizeof(uint8_t));
    for(int i=0;i<6;i++){
        txbuf.data[i]=arp_init_pkt.target_mac[i];
    }
    // src IP
    buf_add_header(&txbuf, 4*sizeof(uint8_t));
    for(int i = 0; i < 4; i++){
        txbuf.data[i] = arp_init_pkt.sender_ip[i];
    }
    // src MAC
    buf_add_header(&txbuf, 6*sizeof(uint8_t));
    for(int i = 0; i < 6; i++){
        txbuf.data[i] = arp_init_pkt.sender_mac[i];
    }
    // code type
    buf_add_header(&txbuf, sizeof(uint16_t));
    uint16_t *hdr = (uint16_t *)txbuf.data;
    *hdr=swap16(ARP_REQUEST);
    // IP length
    buf_add_header(&txbuf, sizeof(uint8_t));
    txbuf.data[0] = arp_init_pkt.pro_len;
    // MAC length
    buf_add_header(&txbuf, sizeof(uint8_t));
    txbuf.data[0] = arp_init_pkt.hw_len;
    //upper protocol
    buf_add_header(&txbuf, sizeof(uint16_t));
    hdr = (uint16_t *)txbuf.data;
    *hdr = arp_init_pkt.pro_type16;
    // hdw type
    buf_add_header(&txbuf, sizeof(uint16_t));
    hdr = (uint16_t *)txbuf.data;
    *hdr = arp_init_pkt.hw_type16;
    uint8_t temp_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    ethernet_out(&txbuf, temp_mac, NET_PROTOCOL_ARP);
}

/**
 * @brief 发送一个arp响应
 * 
 * @param target_ip 目标ip地址
 * @param target_mac 目标mac地址
 */
void arp_resp(uint8_t *target_ip, uint8_t *target_mac)
{
    // TO-DO
    // init buf
    buf_init(&txbuf, 0);
    // add arp header
    buf_add_header(&txbuf, 4*sizeof(uint8_t));
    for(int i = 0; i < 4; i++){
        txbuf.data[i] = target_ip[i];
    }
    // dst MAC
    buf_add_header(&txbuf, 6*sizeof(uint8_t));
    for(int i = 0; i < 6; i++){
        txbuf.data[i] = target_mac[i];
    }
    // src IP
    buf_add_header(&txbuf, 4*sizeof(uint8_t));
    for(int i = 0; i < 4; i++){
        txbuf.data[i] = net_if_ip[i];
    }
    // src MAC
    buf_add_header(&txbuf, 6*sizeof(uint8_t));
    for(int i=0;i<6;i++){
        txbuf.data[i]=net_if_mac[i];
    }
    //填写操作类型
    buf_add_header(&txbuf, sizeof(uint16_t));
    uint16_t *hdr = (uint16_t *)txbuf.data;
    *hdr=swap16(ARP_REPLY);
    //填写IP地址长度
    buf_add_header(&txbuf, sizeof(uint8_t));
    txbuf.data[0]=arp_init_pkt.pro_len;
    //填写MAC的长度
    buf_add_header(&txbuf, sizeof(uint8_t));
    txbuf.data[0]=arp_init_pkt.hw_len;
    //填写上层协议类型
    buf_add_header(&txbuf, sizeof(uint16_t));
    hdr=(uint16_t *)txbuf.data;
    *hdr=arp_init_pkt.pro_type16;
    //填写硬件类型
    buf_add_header(&txbuf, sizeof(uint16_t));
    hdr=(uint16_t *)txbuf.data;
    *hdr=arp_init_pkt.hw_type16;
    ethernet_out(&txbuf,target_mac,NET_PROTOCOL_ARP);
}

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 * @param src_mac 源mac地址
 */
void arp_in(buf_t *buf, uint8_t *src_mac)
{   
    //buf是46字节的负载
    // TO-DO
    if(buf->len>8){
        //检查硬件类型
        int flag=1;
        uint16_t *hdr = (uint16_t *)buf->data;
        uint16_t data=(*hdr);
        if(data!=arp_init_pkt.hw_type16)    flag=0;
        //检查上层协议类型
        hdr=(uint16_t *)(buf->data+2);
        data=(*hdr);
        if(data!=arp_init_pkt.pro_type16)   flag=0;
        //检查MAC硬件地址长度
        if(buf->data[4]!=arp_init_pkt.hw_len)  flag=0;
        //检查IP协议地址长度
        if(buf->data[5]!=arp_init_pkt.pro_len)     flag=0;
        //检查操作类型
        hdr=(uint16_t*)(buf->data+6);
        data=(*hdr);
        if(data!=swap16(ARP_REQUEST)&&data!=swap16(ARP_REPLY))   flag=0;
        //检查通过
        if(flag){
            //寻找源主机的IP
            uint8_t *src_ip=(uint8_t*)(buf->data+14);
            map_set(&arp_table,src_ip,src_mac);
            buf_t *temp_buf=map_get(&arp_buf,src_ip);
            if(temp_buf){
                //存在一个发送到源主机的buf,此时已经建立了src_ip->src_mac的映射
                //直接发送数据包
                ethernet_out(temp_buf,src_mac,NET_PROTOCOL_IP);
                map_delete(&arp_buf,src_ip);
            }else{
                //不存在发送到当前主机的buf
                //判断操作类型
                hdr=(uint16_t*)(buf->data+6);
                data=(*hdr);
                //获取请求报文的目的ip
                uint8_t *target_ip=(uint8_t*)(buf->data+24);
                int visited_flag=1;
                for(int i=0;i<4;i++){
                    if(target_ip[i]!=net_if_ip[i])  visited_flag=0;
                }
                //请求本机MAC的请求报文
                if(visited_flag&&data==swap16(ARP_REQUEST)){
                    arp_resp(src_ip,src_mac);
                }
            }
        }
    }
}

/**
 * @brief 处理一个要发送的数据包
 * 
 * @param buf 要处理的数据包
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
void arp_out(buf_t *buf, uint8_t *ip)
{
    // TO-DO
    uint8_t *p=map_get(&arp_table,ip);
    if(p){
        //非空
        ethernet_out(buf,p,NET_PROTOCOL_IP);
    }else{
        if(map_size(&arp_buf)==0){
            //没有包等待才发送
            //ip是目的主机的ip,当接收到arp应答请求后,会自动建立ip->mac,之后将buf中多余的数据发送出去
            map_set(&arp_buf,ip,buf);
            arp_req(ip);
        }
    }
}

/**
 * @brief 初始化arp协议
 * 
 */
void arp_init()
{
    map_init(&arp_table, NET_IP_LEN, NET_MAC_LEN, 0, ARP_TIMEOUT_SEC, NULL);
    map_init(&arp_buf, NET_IP_LEN, sizeof(buf_t), 0, ARP_MIN_INTERVAL, buf_copy);
    net_add_protocol(NET_PROTOCOL_ARP, arp_in);
    arp_req(net_if_ip);
}