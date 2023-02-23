/********************************************************************************************************
 * @file     app.c
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     Sep. 18, 2018
 *
 * @par      Copyright (c) Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *			 The information contained herein is confidential and proprietary
 *property of Telink Semiconductor (Shanghai) Co., Ltd. and is available under
 *the terms of Commercial License Agreement between Telink Semiconductor
 *(Shanghai) Co., Ltd. and the licensee in separate contract or the terms
 *described here-in. This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information
 *in this file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is
 *provided.
 *
 *******************************************************************************************************/

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "vendor/common/blt_led.h"
#include "vendor/common/blt_common.h"
#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"

#define ADV_IDLE_ENTER_DEEP_TIME 60  // 60 s
#define CONN_IDLE_ENTER_DEEP_TIME 60 // 60 s

#define MY_DIRECT_ADV_TMIE 2000000

#define MY_APP_ADV_CHANNEL BLT_ENABLE_ADV_ALL
#define MY_ADV_INTERVAL_MIN ADV_INTERVAL_30MS
#define MY_ADV_INTERVAL_MAX ADV_INTERVAL_35MS

#define MY_RF_POWER_INDEX RF_POWER_P3p01dBm

#define BLE_DEVICE_ADDRESS_TYPE BLE_DEVICE_ADDRESS_PUBLIC

_attribute_data_retention_ own_addr_type_t app_own_address_type =
    OWN_ADDRESS_PUBLIC;

#define RX_FIFO_SIZE 64
#define RX_FIFO_NUM 8

#define TX_FIFO_SIZE 40
#define TX_FIFO_NUM 16

#if 0
	MYFIFO_INIT(blt_rxfifo, RX_FIFO_SIZE, RX_FIFO_NUM);
#else
_attribute_data_retention_ u8 blt_rxfifo_b[RX_FIFO_SIZE * RX_FIFO_NUM] = {0};
_attribute_data_retention_ my_fifo_t blt_rxfifo = {
    RX_FIFO_SIZE,
    RX_FIFO_NUM,
    0,
    0,
    blt_rxfifo_b,
};
#endif

#if 0
	MYFIFO_INIT(blt_txfifo, TX_FIFO_SIZE, TX_FIFO_NUM);
#else
_attribute_data_retention_ u8 blt_txfifo_b[TX_FIFO_SIZE * TX_FIFO_NUM] = {0};
_attribute_data_retention_ my_fifo_t blt_txfifo = {
    TX_FIFO_SIZE,
    TX_FIFO_NUM,
    0,
    0,
    blt_txfifo_b,
};
#endif


_attribute_data_retention_ int device_in_connection_state;

_attribute_data_retention_ u32 advertise_begin_tick;

_attribute_data_retention_ u32 interval_update_tick;

_attribute_data_retention_ u8 sendTerminate_before_enterDeep = 0;

_attribute_data_retention_ u32 latest_user_event_tick;

_attribute_data_retention_ static u32 keyScanTick = 0;

extern u32 scan_pin_need;

_attribute_ram_code_ void ble_remote_set_sleep_wakeup(u8 e, u8 *p, int n)
{
    if (blc_ll_getCurrentState() == BLS_LINK_STATE_CONN &&
        ((u32)(bls_pm_getSystemWakeupTick() - clock_time())) >
            80 * CLOCK_16M_SYS_TIMER_CLK_1MS)
    { // suspend time > 30ms.add gpio
      // wakeup
        bls_pm_setWakeupSource(
            PM_WAKEUP_PAD); // gpio pad wakeup suspend/deepsleep
    }
}


_attribute_ram_code_ void user_set_rf_power(u8 e, u8 *p, int n)
{
    rf_set_power_level_index(MY_RF_POWER_INDEX);
}



_attribute_ram_code_ void blt_pm_proc(void)
{
#if (BLE_APP_PM_ENABLE)

#if (PM_DEEPSLEEP_RETENTION_ENABLE)
    bls_pm_setSuspendMask(SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN |
                          DEEPSLEEP_RETENTION_CONN);
#else
    bls_pm_setSuspendMask(SUSPEND_ADV | SUSPEND_CONN);
#endif

#if (!TEST_CONN_CURRENT_ENABLE)
    // do not care about keyScan power here, if you care about this, please
    // refer to "8258_ble_remote" demo
    if (scan_pin_need || key_not_released)
    {
        bls_pm_setSuspendMask(SUSPEND_DISABLE);
    }

#if 1 // deepsleep
    if (sendTerminate_before_enterDeep == 2)
    { // Terminate OK
        analog_write(USED_DEEP_ANA_REG,
                     analog_read(USED_DEEP_ANA_REG) | CONN_DEEP_FLG);
        cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0); // deepsleep
    }

    if (!blc_ll_isControllerEventPending())
    { // no controller event pending
        // adv 60s, deepsleep
        if (blc_ll_getCurrentState() == BLS_LINK_STATE_ADV &&
            !sendTerminate_before_enterDeep &&
            clock_time_exceed(advertise_begin_tick,
                              ADV_IDLE_ENTER_DEEP_TIME * 1000000))
        {
            cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0); // deepsleep
        }
        // conn 60s no event(key/voice/led), enter deepsleep
        else if (device_in_connection_state &&
                 clock_time_exceed(latest_user_event_tick,
                                   CONN_IDLE_ENTER_DEEP_TIME * 1000000))
        {
            bls_ll_terminateConnection(
                HCI_ERR_REMOTE_USER_TERM_CONN); // push terminate cmd into ble
                                                // TX buffer
            bls_ll_setAdvEnable(0);             // disable adv
            sendTerminate_before_enterDeep = 1;
        }
    }

#endif
#endif

#endif
}

extern void at_print(char *str);
extern void at_print_array(char *data, u32 len);


u32 cur_conn_device_hdl; //conn_handle

#define USER_SPP_SERUUID_RX  0x16,0x96,0x24,0x47,0xc6,0x23, 0x61,0xba,0xd9,0x4b,0x4d,0x1e,0x43,0x53,0x53,0x49
u8 TelinkSppDataServer2ClientUUID_RX[16]      = {USER_SPP_SERUUID_RX};
#define Characteristic_NotifyIndex 1

int app_l2cap_handler(u16 conn_handle, u8 *raw_pkt)
{

    rf_packet_l2cap_t *ptrL2cap = blm_l2cap_packet_pack(conn_handle, raw_pkt);
    if (!ptrL2cap)
        return 0;

    // l2cap data channel id, 4 for att, 5 for signal, 6 for smp
    if (ptrL2cap->chanId == L2CAP_CID_ATTR_PROTOCOL) // att data
    {
        rf_packet_att_t *pAtt = (rf_packet_att_t *)ptrL2cap;
        u16 attHandle = pAtt->handle0 | pAtt->handle1 << 8;
        if (pAtt->opcode == ATT_OP_EXCHANGE_MTU_REQ || pAtt->opcode == ATT_OP_EXCHANGE_MTU_RSP)
        {
        }
        else if (pAtt->opcode == ATT_OP_READ_BY_TYPE_RSP || pAtt->opcode == ATT_OP_ERROR_RSP /*|| pAtt->opcode == ATT_OP_FIND_INFO_RSP*/) // slave ack ATT_OP_READ_BY_TYPE_REQ data//ATT_OP_FIND_INFO_RSP  ATT_OP_FIND_BY_TYPE_VALUE_RSP  ATT_OP_READ_BY_TYPE_RSP ATT_OP_READ_RSP ATT_OP_READ_BLOB_RSP ATT_OP_READ_MULTI_RSP ATT_OP_READ_BY_GROUP_TYPE_RSP
        {
            u16 consumer_key = CLIENT_CHAR_CFG_NOTI;//ʹ��notify�ļ��� slave_spp_handle+3 Ϊ2902�ľ��ƫ��ֵ
            u8 dat[32];

            rf_pkt_att_readByTypeRsp_t* ptr = (rf_pkt_att_readByTypeRsp_t*)ptrL2cap;
            u16 Rxhandle = ptr->data[0] | ptr->data[1] << 8;
            att_req_write(dat, Rxhandle + Characteristic_NotifyIndex, (u8*)&consumer_key, 2);
            if (blm_push_fifo(cur_conn_device_hdl, dat)) {

            }
        }
        else if (pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI || pAtt->opcode == ATT_OP_WRITE_CMD) // slave handle notify
        {
			u8 len = pAtt->l2capLen - 3;
            at_send((char *)pAtt->dat, len);
			// at_print_array((char *)pAtt->dat, len);
        }
        else if (pAtt->opcode == ATT_OP_ERROR_RSP)
        {
        }
    }
}



int controller_event_callback(u32 h, u8 *p, int n)
{
    if (h & HCI_FLAG_EVENT_BT_STD) // ble controller hci event
    {
        u8 evtCode = h & 0xff;

        if (evtCode == HCI_EVT_LE_META)
        {
            u8 subEvt_code = p[0];
            if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT) // ADV packet
            {
                // after controller is set to scan state, it will report all the
                // adv packet it received by this event

                event_adv_report_t *pa = (event_adv_report_t *)p;
                // s8 rssi = (s8)pa->data[pa->len];  // rssi has already plus 110.
                s8 rssi = pa->data[pa->len];
                u8 *mac = pa->mac;
                u8 subcode = pa->subcode;
                u8 nreport = pa->nreport;
                u8 event_type = pa->event_type;
                u8 adr_type = pa->adr_type;
                char data[128] = {0};
                sprintf(data, "[MAC: %02X %02X %02X %02X %02X %02X][RSSI: %d][ADR TYPE: %d]\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], rssi, adr_type);
                // at_print(data);
                // at_print("[DATA: ");
                // at_print_array((char *)p, (pa->len + 11));
                // at_print("]\n-------------------------\n");

                u8 target_mac_addr[] = {0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
                if (memcmp(target_mac_addr, mac, 6) == 0)
                {
                    u8 status = blc_ll_createConnection(SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS, INITIATE_FP_ADV_SPECIFY,
                                                        pa->adr_type, pa->mac, BLE_ADDR_PUBLIC,
                                                        CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 0, CONN_TIMEOUT_4S,
                                                        0, 0xFFFF);
                    if (status != BLE_SUCCESS)
                    {
                        at_print("createConnection fall error\r\n");
                    }
                    else
                    {
                        blc_ll_setScanEnable(BLC_SCAN_DISABLE, DUP_FILTER_DISABLE);
                        at_print("createConnection success\r\n");
                    }
                }
            }
            else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_ESTABLISH)
            {
                event_connection_complete_t *pCon = (event_connection_complete_t *)p;
                cur_conn_device_hdl = pCon->handle;  
                at_print("Connected!!\r\n");
                u8 dat[100];
                u8 read_by_type_req_uuid[16];
                memcpy(read_by_type_req_uuid, TelinkSppDataServer2ClientUUID_RX, 16);
                // memcpy(read_by_type_req_uuid, TelinkSppDataServer2ClientUUID_TX, 16);
                //att_req_find_info(dat, 1, 0xffff);
                att_req_read_by_type (dat, 1, 0xffff, read_by_type_req_uuid, 16);
                // att_req_write(dat, Rxhandle + Characteristic_NotifyIndex, (u8*)&consumer_key, 2);  

                if (blm_push_fifo(cur_conn_device_hdl, dat)) {
                    at_print("+read_by_type_req_uuid TelinkSppDataServer2ClientUUID_RX\r\n>");
                }

            }
        }
    }
}

void user_init_normal(void)
{
    // random number generator must be initiated here( in the beginning of
    // user_init_nromal) when deepSleep retention wakeUp, no need initialize
    // again
    random_generator_init(); // this is must

    ////////////////// BLE stack initialization
    ///////////////////////////////////////
    u8 mac_public[6];
    u8 mac_random_static[6];
    blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);

#if (BLE_DEVICE_ADDRESS_TYPE == BLE_DEVICE_ADDRESS_PUBLIC)
    app_own_address_type = OWN_ADDRESS_PUBLIC;
#elif (BLE_DEVICE_ADDRESS_TYPE == BLE_DEVICE_ADDRESS_RANDOM_STATIC)
    app_own_address_type = OWN_ADDRESS_RANDOM;
    blc_ll_setRandomAddr(mac_random_static);
#endif

    ////// Controller Initialization  //////////
    blc_ll_initBasicMCU();
    blc_ll_initStandby_module(mac_public);    // mandatory
    blc_ll_initScanning_module(mac_public);   // scan module: 		 mandatory for BLE master,
    blc_ll_initInitiating_module();           // initiate module: 	 mandatory for BLE master,
    blc_ll_initConnection_module();           // connection module  mandatory for BLE slave/master
    blc_ll_initMasterRoleSingleConn_module(); // master module: 	 mandatory for BLE master,

    ////// Host Initialization  //////////
    blc_gap_central_init();                                            // gap initialization
    blc_l2cap_register_handler(app_l2cap_handler);                     // l2cap initialization
    blc_hci_registerControllerEventHandler(controller_event_callback); // controller hci event to host all processed in this func

    // bluetooth event
    blc_hci_setEventMask_cmd(HCI_EVT_MASK_DISCONNECTION_COMPLETE | HCI_EVT_MASK_ENCRYPTION_CHANGE);

    // bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_CONNECTION_COMPLETE | HCI_LE_EVT_MASK_ADVERTISING_REPORT | HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE | HCI_LE_EVT_MASK_DATA_LENGTH_CHANGE | HCI_LE_EVT_MASK_CONNECTION_ESTABLISH); // connection establish: telink private event

    // blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS,
    //                         SCAN_INTERVAL_100MS, OWN_ADDRESS_PUBLIC,
    //                         SCAN_FP_ALLOW_ADV_ANY);
    blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_30MS,
                            SCAN_INTERVAL_30MS, OWN_ADDRESS_PUBLIC,
                            SCAN_FP_ALLOW_ADV_ANY);

    // ll_whiteList_reset();
    // u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
    // ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);

    // blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS,
    //                         SCAN_INTERVAL_100MS, OWN_ADDRESS_PUBLIC,
    //                         SCAN_FP_ALLOW_ADV_WL);

    blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);

    at_print("SCAN ENABLE");

}

_attribute_ram_code_ void user_init_deepRetn(void)
{
#if (PM_DEEPSLEEP_RETENTION_ENABLE)

    blc_ll_initBasicMCU(); // mandatory
    rf_set_power_level_index(MY_RF_POWER_INDEX);

    blc_ll_recoverDeepRetention();

    DBG_CHN0_HIGH; // debug

    irq_enable();

#if (!TEST_CONN_CURRENT_ENABLE)
    /////////// keyboard gpio wakeup init ////////
    u32 pin[] = KB_DRIVE_PINS;
    for (int i = 0; i < (sizeof(pin) / sizeof(*pin)); i++)
    {
        cpu_set_gpio_wakeup(pin[i], Level_High,
                            1); // drive pin pad high wakeup deepsleep
    }
#endif
#endif
}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////

void main_loop(void)
{
    ////////////////////////////////////// BLE entry
    ////////////////////////////////////
    blt_sdk_main_loop();
    ////////////////////////////////////// UI entry
    ///////////////////////////////////
    ////////////////////////////////////// PM Process
    ////////////////////////////////////
    blt_pm_proc();
}
