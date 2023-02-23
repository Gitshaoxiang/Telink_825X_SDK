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

#define ADV_IDLE_ENTER_DEEP_TIME  60  // 60 s
#define CONN_IDLE_ENTER_DEEP_TIME 60  // 60 s

#define MY_DIRECT_ADV_TMIE 2000000

#define MY_APP_ADV_CHANNEL  BLT_ENABLE_ADV_ALL
#define MY_ADV_INTERVAL_MIN ADV_INTERVAL_30MS
#define MY_ADV_INTERVAL_MAX ADV_INTERVAL_35MS

#define MY_RF_POWER_INDEX RF_POWER_P3p01dBm

#define BLE_DEVICE_ADDRESS_TYPE BLE_DEVICE_ADDRESS_PUBLIC

_attribute_data_retention_ own_addr_type_t app_own_address_type =
    OWN_ADDRESS_PUBLIC;

#define RX_FIFO_SIZE 64
#define RX_FIFO_NUM  8

#define TX_FIFO_SIZE 40
#define TX_FIFO_NUM  16

#if 0
	MYFIFO_INIT(blt_rxfifo, RX_FIFO_SIZE, RX_FIFO_NUM);
#else
_attribute_data_retention_ u8 blt_rxfifo_b[RX_FIFO_SIZE * RX_FIFO_NUM] = {0};
_attribute_data_retention_ my_fifo_t blt_rxfifo                        = {
                           RX_FIFO_SIZE, RX_FIFO_NUM, 0, 0, blt_rxfifo_b,
};
#endif

#if 0
	MYFIFO_INIT(blt_txfifo, TX_FIFO_SIZE, TX_FIFO_NUM);
#else
_attribute_data_retention_ u8 blt_txfifo_b[TX_FIFO_SIZE * TX_FIFO_NUM] = {0};
_attribute_data_retention_ my_fifo_t blt_txfifo                        = {
                           TX_FIFO_SIZE, TX_FIFO_NUM, 0, 0, blt_txfifo_b,
};
#endif

//////////////////////////////////////////////////////////////////////////////
//	 Adv Packet, Response Packet
//////////////////////////////////////////////////////////////////////////////
const u8 tbl_advData[] = {
    0x05, 0x09, 'T',  'E',  'S',  'T',
    0x02, 0x01, 0x05,  // BLE limited discoverable mode and BR/EDR not supported
    0x03, 0x19, 0x80, 0x01,  // 384, Generic Remote Control, Generic category
    0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,  // incomplete list of service class
                                         // UUIDs (0x1812, 0x180F)
};

const u8 tbl_scanRsp[] = {
    0x0c, 0x09, 'T', 'e', 'L', 'i', 'n', 'k', '_', '8', '2', '5', 'x',
};

_attribute_data_retention_ int device_in_connection_state;

_attribute_data_retention_ u32 advertise_begin_tick;

_attribute_data_retention_ u32 interval_update_tick;

_attribute_data_retention_ u8 sendTerminate_before_enterDeep = 0;

_attribute_data_retention_ u32 latest_user_event_tick;

_attribute_data_retention_ static u32 keyScanTick = 0;

extern u32 scan_pin_need;

_attribute_ram_code_ void ble_remote_set_sleep_wakeup(u8 e, u8 *p, int n) {
    if (blc_ll_getCurrentState() == BLS_LINK_STATE_CONN &&
        ((u32)(bls_pm_getSystemWakeupTick() - clock_time())) >
            80 * CLOCK_16M_SYS_TIMER_CLK_1MS) {  // suspend time > 30ms.add gpio
                                                 // wakeup
        bls_pm_setWakeupSource(
            PM_WAKEUP_PAD);  // gpio pad wakeup suspend/deepsleep
    }
}

void app_switch_to_indirect_adv(u8 e, u8 *p, int n) {
    bls_ll_setAdvParam(MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
                       ADV_TYPE_CONNECTABLE_UNDIRECTED, app_own_address_type, 0,
                       NULL, MY_APP_ADV_CHANNEL, ADV_FP_NONE);

    bls_ll_setAdvEnable(1);  // must: set adv enable
}

void ble_remote_terminate(u8 e, u8 *p, int n)  //*p is terminate reason
{
    device_in_connection_state = 0;

    if (*p == HCI_ERR_CONN_TIMEOUT) {
    } else if (*p == HCI_ERR_REMOTE_USER_TERM_CONN) {  // 0x13

    } else if (*p == HCI_ERR_CONN_TERM_MIC_FAILURE) {
    } else {
    }

#if (BLE_APP_PM_ENABLE)
    // user has push terminate pkt to ble TX buffer before deepsleep
    if (sendTerminate_before_enterDeep == 1) {
        sendTerminate_before_enterDeep = 2;
    }
#endif

    advertise_begin_tick = clock_time();
}

_attribute_ram_code_ void user_set_rf_power(u8 e, u8 *p, int n) {
    rf_set_power_level_index(MY_RF_POWER_INDEX);
}

void task_connect(u8 e, u8 *p, int n) {
    //	bls_l2cap_requestConnParamUpdate (8, 8, 19, 200);  // 200mS
    bls_l2cap_requestConnParamUpdate(8, 8, 99, 400);  // 1 S
    //	bls_l2cap_requestConnParamUpdate (8, 8, 149, 600);  // 1.5 S
    //	bls_l2cap_requestConnParamUpdate (8, 8, 199, 800);  // 2 S
    //	bls_l2cap_requestConnParamUpdate (8, 8, 249, 800);  // 2.5 S
    //	bls_l2cap_requestConnParamUpdate (8, 8, 299, 800);  // 3 S

    latest_user_event_tick = clock_time();

    device_in_connection_state = 1;  //

    interval_update_tick = clock_time() | 1;  // none zero
}

void task_conn_update_req(u8 e, u8 *p, int n) {
}

void task_conn_update_done(u8 e, u8 *p, int n) {
}

_attribute_ram_code_ void blt_pm_proc(void) {
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
    if (scan_pin_need || key_not_released) {
        bls_pm_setSuspendMask(SUSPEND_DISABLE);
    }

#if 1                                           // deepsleep
    if (sendTerminate_before_enterDeep == 2) {  // Terminate OK
        analog_write(USED_DEEP_ANA_REG,
                     analog_read(USED_DEEP_ANA_REG) | CONN_DEEP_FLG);
        cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  // deepsleep
    }

    if (!blc_ll_isControllerEventPending()) {  // no controller event pending
        // adv 60s, deepsleep
        if (blc_ll_getCurrentState() == BLS_LINK_STATE_ADV &&
            !sendTerminate_before_enterDeep &&
            clock_time_exceed(advertise_begin_tick,
                              ADV_IDLE_ENTER_DEEP_TIME * 1000000)) {
            cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  // deepsleep
        }
        // conn 60s no event(key/voice/led), enter deepsleep
        else if (device_in_connection_state &&
                 clock_time_exceed(latest_user_event_tick,
                                   CONN_IDLE_ENTER_DEEP_TIME * 1000000)) {
            bls_ll_terminateConnection(
                HCI_ERR_REMOTE_USER_TERM_CONN);  // push terminate cmd into ble
                                                 // TX buffer
            bls_ll_setAdvEnable(0);              // disable adv
            sendTerminate_before_enterDeep = 1;
        }
    }

#endif
#endif

#endif
}

extern void at_print(char *str);
extern void at_print_array(char *data, u32 len);

int controller_event_callback(u32 h, u8 *p, int n) {
    if (h & HCI_FLAG_EVENT_BT_STD)  // ble controller hci event
    {
        u8 evtCode = h & 0xff;

        if (evtCode == HCI_EVT_LE_META) {
            u8 subEvt_code = p[0];
            if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)  // ADV packet
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
                at_print(data);
                at_print("[DATA: ");
                at_print_array((char *)p, (pa->len + 11));
                at_print("]\n-------------------------\n");

                
                // u8 target_mac_addr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
                // if( memcmp(target_mac_addr , mac, 6) == 0){
                //     u8 status = blc_ll_createConnection( SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS, INITIATE_FP_ADV_SPECIFY,  \
                //                             pa->adr_type, pa->mac, BLE_ADDR_PUBLIC, \
                //                             CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 0, CONN_TIMEOUT_4S, \
                //                             0, 0xFFFF);
                //     if(status!=BLE_SUCCESS)
                //     {
                //         printf("createConnection fall error =%02x\r\n",status);
                        
                //     }else{
                //         printf("createConnection success \r\n");
                //     }
                // }

#if (DBG_ADV_REPORT_ON_RAM)
                if (pa->len > 31) {
                    pa->len = 31;
                }
                memcpy((u8 *)AA_advRpt[AA_advRpt_index++], p, pa->len + 11);
                if (AA_advRpt_index >= RAM_ADV_MAX) {
                    AA_advRpt_index = 0;
                }
#endif
            }
        }
    }
}

void user_init_normal(void) {
    // random number generator must be initiated here( in the beginning of
    // user_init_nromal) when deepSleep retention wakeUp, no need initialize
    // again
    random_generator_init();  // this is must

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
    blc_ll_initBasicMCU();                  // mandatory
    // blc_ll_initStandby_module(mac_public);  // mandatory
	blc_ll_initScanning_module(mac_public); 			//scan module: 		 mandatory for BLE master,
    // blc_ll_initAdvertising_module(
    //     mac_public);                 // adv module: 		 mandatory for BLE slave,
    blc_ll_initConnection_module();  // connection module  mandatory for BLE
                                     // slave/master
    blc_ll_initSlaveRole_module();   // slave module: 	 mandatory for BLE
                                     // slave,
    blc_ll_initPowerManagement_module();  // pm module:      	 optional

    ////// Host Initialization  //////////
    blc_gap_peripheral_init();  // gap initialization
    extern void my_att_init();
    my_att_init();  // gatt initialization
    blc_l2cap_register_handler(
        blc_l2cap_packet_receive);  // l2cap initialization

    // Smp Initialization may involve flash write/erase(when one sector stores
    // too much information,
    //    is about to exceed the sector threshold, this sector must be erased,
    //    and all useful information should re_stored) , so it must be done
    //    after battery check
#if (BLE_REMOTE_SECURITY_ENABLE)
    blc_smp_peripheral_init();
#else
    blc_smp_setSecurityLevel(No_Security);
#endif

    // set rf power index, user must set it after every suspend wakeup, cause
    // relative setting will be reset in suspend
    user_set_rf_power(0, 0, 0);
    bls_app_registerEventCallback(BLT_EV_FLAG_SUSPEND_EXIT, &user_set_rf_power);

    // //scan setting
    // blc_ll_initScanning_module(mac_public);
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
    blc_hci_registerControllerEventHandler(controller_event_callback);

#if 1  // report all adv
    blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS,
                            SCAN_INTERVAL_100MS, OWN_ADDRESS_PUBLIC,
                            SCAN_FP_ALLOW_ADV_ANY);
#else  // report adv only in whitelist
    ll_whiteList_reset();
    u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
    ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
    blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS,
                            SCAN_INTERVAL_100MS, OWN_ADDRESS_PUBLIC,
                            SCAN_FP_ALLOW_ADV_WL);
#endif
    blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);

    at_print("SCAN ENABLE");

    blc_ll_addScanningInAdvState();       // add scan in adv state
    blc_ll_addScanningInConnSlaveRole();  // add scan in conn slave role

    // ble event call back
    bls_app_registerEventCallback(BLT_EV_FLAG_CONNECT, &task_connect);
    bls_app_registerEventCallback(BLT_EV_FLAG_TERMINATE, &ble_remote_terminate);

    bls_app_registerEventCallback(BLT_EV_FLAG_CONN_PARA_REQ,
                                  &task_conn_update_req);
    bls_app_registerEventCallback(BLT_EV_FLAG_CONN_PARA_UPDATE,
                                  &task_conn_update_done);

    ///////////////////// Power Management initialization///////////////////
#if (BLE_APP_PM_ENABLE)
    blc_ll_initPowerManagement_module();

#if (PM_DEEPSLEEP_RETENTION_ENABLE)
    bls_pm_setSuspendMask(SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN |
                          DEEPSLEEP_RETENTION_CONN);
    blc_pm_setDeepsleepRetentionThreshold(95, 95);
    blc_pm_setDeepsleepRetentionEarlyWakeupTiming(
        TEST_CONN_CURRENT_ENABLE ? 220 : 240);
    // blc_pm_setDeepsleepRetentionType(DEEPSLEEP_MODE_RET_SRAM_LOW32K);
    // //default use 16k deep retention
#else
    bls_pm_setSuspendMask(SUSPEND_ADV | SUSPEND_CONN);
#endif

    bls_app_registerEventCallback(BLT_EV_FLAG_SUSPEND_ENTER,
                                  &ble_remote_set_sleep_wakeup);
#else
    bls_pm_setSuspendMask(SUSPEND_DISABLE);
#endif

    advertise_begin_tick = clock_time();

    gpio_set_func(GPIO_PC4, AS_GPIO);
    gpio_set_output_en(GPIO_PC4, 1);
    gpio_set_input_en(GPIO_PC4, 0);

    gpio_set_func(GPIO_PA7, AS_GPIO);
    gpio_setup_up_down_resistor(GPIO_PA7, PM_PIN_PULLUP_10K);
    gpio_set_output_en(GPIO_PA7, 0);
    gpio_set_input_en(GPIO_PA7, 1);
}

_attribute_ram_code_ void user_init_deepRetn(void) {
#if (PM_DEEPSLEEP_RETENTION_ENABLE)

    blc_ll_initBasicMCU();  // mandatory
    rf_set_power_level_index(MY_RF_POWER_INDEX);

    blc_ll_recoverDeepRetention();

    DBG_CHN0_HIGH;  // debug

    irq_enable();

#if (!TEST_CONN_CURRENT_ENABLE)
    /////////// keyboard gpio wakeup init ////////
    u32 pin[] = KB_DRIVE_PINS;
    for (int i = 0; i < (sizeof(pin) / sizeof(*pin)); i++) {
        cpu_set_gpio_wakeup(pin[i], Level_High,
                            1);  // drive pin pad high wakeup deepsleep
    }
#endif
#endif
}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////

void main_loop(void) {
    ////////////////////////////////////// BLE entry
    ////////////////////////////////////
    blt_sdk_main_loop();
    ////////////////////////////////////// UI entry
    ///////////////////////////////////
    ////////////////////////////////////// PM Process
    ////////////////////////////////////
    blt_pm_proc();
}
