// #include "net.h"
// #include "icmp.h"
// #include "ip.h"

// /**
//  * @brief 发送icmp响应
//  * 
//  * @param req_buf 收到的icmp请求包
//  * @param src_ip 源ip地址
//  */
// static void icmp_resp(buf_t *req_buf, uint8_t *src_ip)
// {
//     // TO-DO
//     buf_init(&txbuf, req_buf->len);
//     // copy data
//     memcpy(txbuf.data, req_buf->data, req_buf->len);
//     // add header
//     icmp_hdr_t *req_header = (icmp_hdr_t *)req_buf->data;
//     icmp_hdr_t *header = (icmp_hdr_t*)txbuf.data;
//     header->type = ICMP_TYPE_ECHO_REPLY;
//     header->code = 0;
//     header->checksum16 = checksum16((uint16_t*)txbuf.data,txbuf.len);
//     header->id16 = req_header->id16;
//     header->seq16 = req_header->seq16;
//     // send data
//     ip_out(&txbuf, src_ip, NET_PROTOCOL_ICMP);
// }

// /**
//  * @brief 处理一个收到的数据包
//  * 
//  * @param buf 要处理的数据包
//  * @param src_ip 源ip地址
//  */
// void icmp_in(buf_t *buf, uint8_t *src_ip)
// {
//     // TO-
//     if(buf->len < 8)
//         return;
    
//     icmp_hdr_t *header = (icmp_hdr_t*)buf->data;
//     if(header->type == ICMP_TYPE_ECHO_REQUEST && 
//         header->code == ICMP_TYPE_ECHO_REPLY){
//         icmp_resp(buf, src_ip);
//     }
    
// }

// /**
//  * @brief 发送icmp不可达
//  * 
//  * @param recv_buf 收到的ip数据包
//  * @param src_ip 源ip地址
//  * @param code icmp code，协议不可达或端口不可达
//  */
// void icmp_unreachable(buf_t *recv_buf, uint8_t *src_ip, icmp_code_t code)
// {
//     // TO-DO
//     buf_init(&txbuf, 0);
//     // add IP header
//     ip_hdr_t *ip_header = (ip_hdr_t*)recv_buf->data;
//     buf_add_header(&txbuf, ip_header->hdr_len * 4 + 8);
//     // copy data
//     memcpy(txbuf.data, recv_buf->data, (ip_header->hdr_len) * 4 + 8);
//     // add icmp header
//     buf_add_header(&txbuf, 8);
//     for(size_t i = 0; i < 8; i++){
//         txbuf.data[i] = 0;
//     }
//     icmp_hdr_t *header = (icmp_hdr_t*)txbuf.data;
//     header-> type = ICMP_TYPE_UNREACH;
//     header->code = code;
//     header->checksum16 = checksum16((uint16_t*)txbuf.data,txbuf.len);
//     header->id16 = 0;
//     header->seq16 = 0;
//     // send
//     ip_out(&txbuf, src_ip, NET_PROTOCOL_ICMP);

// }

// /**
//  * @brief 初始化icmp协议
//  * 
//  */
// void icmp_init(){
//     net_add_protocol(NET_PROTOCOL_ICMP, icmp_in);
// }
