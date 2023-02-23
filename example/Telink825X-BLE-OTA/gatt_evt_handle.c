#include "gatt_evt_handle.h"
#include "application/print/u_printf.h"

int characater1_write(rf_packet_att_write_t *p) {
    at_print("CHAR 1\r\n");
    u8 len = p->l2capLen - 3;
    if (len > 0) {
        //安可信规定AT指令和返回以回车换行符开始，回车换行符结束

        char payload[100];
        u8 payload_len = sprintf(payload, "\r\n+DATA:%d,", len);
        at_send(payload, payload_len);    //输出蓝牙传输的值
        at_send((char *)&p->value, len);  //输出蓝牙传输的值

        at_print("\r\n");  //输出AT指令结尾\r\n
    }
    return 0;
}

int characater2_write(rf_packet_att_write_t *p) {
    at_print("CHAR 2\r\n");
    u8 len = p->l2capLen - 3;
    if (len > 0) {
        //安可信规定AT指令和返回以回车换行符开始，回车换行符结束

        char payload[100];
        u8 payload_len = sprintf(payload, "\r\n+DATA:%d,", len);
        at_send(payload, payload_len);    //输出蓝牙传输的值
        at_send((char *)&p->value, len);  //输出蓝牙传输的值

        at_print("\r\n");  //输出AT指令结尾\r\n
    }
    return 0;
}

int characater3_write(rf_packet_att_write_t *p) {
    at_print("CHAR 3\r\n");
    u8 len = p->l2capLen - 3;
    if (len > 0) {
        //安可信规定AT指令和返回以回车换行符开始，回车换行符结束

        char payload[100];
        u8 payload_len = sprintf(payload, "\r\n+DATA:%d,", len);
        at_send(payload, payload_len);    //输出蓝牙传输的值
        at_send((char *)&p->value, len);  //输出蓝牙传输的值

        at_print("\r\n");  //输出AT指令结尾\r\n
    }
    return 0;
}

int characater1_read(rf_packet_att_read_t *p) {
    serviceCharacterValue_1[3]++;
    return 0;
}

int characater2_read(rf_packet_att_read_t *p) {
    serviceCharacterValue_2[3]++;
    return 0;
}

int characater3_read(rf_packet_att_read_t *p) {
    serviceCharacterValue_3[3]++;
    return 0;
}
