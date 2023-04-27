#include "net.h"
#include "icmp.h"
#include "ip.h"

/**
 * @brief 发送icmp响应
 * 
 * @param req_buf 收到的icmp请求包
 * @param src_ip 源ip地址
 */
static void icmp_resp(buf_t *req_buf, uint8_t *src_ip)
{
    // TO-DO
    buf_init(&txbuf, req_buf->len);
    // copy data
    buf_copy(&txbuf, req_buf, 0);
    // add header
    icmp_hdr_t *header = (icmp_hdr_t*)txbuf.data;
    header->type = 0;
    header->code = 0;
    // calculate checksum
    header->checksum16 = 0;
    uint16_t mychecksum = checksum16((uint16_t*)txbuf.data, txbuf.len);
    header->checksum16 = mychecksum;
    // send data 
    ip_out(&txbuf, src_ip, NET_PROTOCOL_ICMP);
}

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 * @param src_ip 源ip地址
 */
void icmp_in(buf_t *buf, uint8_t *src_ip)
{
    // TO-DO
    check length
    if(buf->len < 8)
        return;
    icmp_hdr_t *header = (icmp_hdr_t*)buf->data;
    if(header->type==8&&header->code==0){
    icmp_resp(buf,src_ip);
        }
}

/**
 * @brief 发送icmp不可达,该协议是接收方发送的
 * 
 * @param recv_buf 收到的ip数据包,包括IP首部
 * @param src_ip 源ip地址
 * @param code icmp code，协议不可达或端口不可达
 */
void icmp_unreachable(buf_t *recv_buf, uint8_t *src_ip, icmp_code_t code)
{
    // TO-DO
    // 初始化txbuf
    buf_init(&txbuf,0);
    // 填写IP数据包首部+前8个字节
    ip_hdr_t *ip_header=(ip_hdr_t*)recv_buf->data;
    buf_add_header(&txbuf,ip_header->hdr_len*4+8);
    //拷贝数据
    memcpy(txbuf.data,recv_buf->data,(ip_header->hdr_len)*4+8);
    // 填写ICMP数据包首部
    buf_add_header(&txbuf,8);
    for (size_t i = 0; i < 8; i++)
    {
        txbuf.data[i]=0;
    }
    icmp_hdr_t *header=(icmp_hdr_t*)txbuf.data;
    header->type=ICMP_TYPE_UNREACH;
    header->code=code;
    header->checksum16=0;
    uint16_t mychecksum=checksum16((uint16_t*)txbuf.data,txbuf.len);
    header->checksum16=mychecksum;
    // 发送数据
    ip_out(&txbuf,src_ip,NET_PROTOCOL_ICMP);
}

/**
 * @brief 初始化icmp协议
 * 
 */
void icmp_init(){
    net_add_protocol(NET_PROTOCOL_ICMP, icmp_in);
}