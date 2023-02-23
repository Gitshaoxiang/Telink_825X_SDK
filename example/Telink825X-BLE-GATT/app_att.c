/********************************************************************************************************
 * @file     app_att.c
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     Sep. 18, 2018
 *
 * @par      Copyright (c) Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *			 The information contained herein is confidential and proprietary property of Telink
 * 		     Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *			 of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *			 Co., Ltd. and the licensee in separate contract or the terms described here-in.
 *           This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information in this
 *			 file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/

#include "tl_common.h"
#include "stack/ble/ble.h"

typedef struct
{
	/** Minimum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
	u16 intervalMin;
	/** Maximum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
	u16 intervalMax;
	/** Number of LL latency connection events (0x0000 - 0x03e8) */
	u16 latency;
	/** Connection Timeout (0x000A - 0x0C80 * 10 ms) */
	u16 timeout;
} gap_periConnectParams_t;

static const u16 clientCharacterCfgUUID = GATT_UUID_CLIENT_CHAR_CFG;

static const u16 userdesc_UUID = GATT_UUID_CHAR_USER_DESC;

static const u16 serviceChangeUUID = GATT_UUID_SERVICE_CHANGE;

static const u16 primaryServiceUUID = GATT_UUID_PRIMARY_SERVICE;

static const u16 characterUUID = GATT_UUID_CHARACTER;

static const u16 my_devServiceUUID = SERVICE_UUID_DEVICE_INFORMATION;

static const u16 my_PnPUUID = CHARACTERISTIC_UUID_PNP_ID;

static const u16 my_devNameUUID = GATT_UUID_DEVICE_NAME;

static const u16 my_gapServiceUUID = SERVICE_UUID_GENERIC_ACCESS;

static const u16 my_appearanceUIID = GATT_UUID_APPEARANCE;

static const u16 my_periConnParamUUID = GATT_UUID_PERI_CONN_PARAM;

static const u16 my_appearance = GAP_APPEARE_UNKNOWN;

static const u16 my_gattServiceUUID = SERVICE_UUID_GENERIC_ATTRIBUTE;

static const gap_periConnectParams_t my_periConnParameters = {20, 40, 0, 1000};

//// GAP attribute values
static const u8 my_devNameCharVal[5] = {
	CHAR_PROP_READ | CHAR_PROP_NOTIFY,
	U16_LO(GenericAccess_DeviceName_DP_H), U16_HI(GenericAccess_DeviceName_DP_H),
	U16_LO(GATT_UUID_DEVICE_NAME), U16_HI(GATT_UUID_DEVICE_NAME)};

// static const u8 my_appearanceCharVal[5] = {
// 	CHAR_PROP_READ,
// 	U16_LO(GenericAccess_Appearance_DP_H), U16_HI(GenericAccess_Appearance_DP_H),
// 	U16_LO(GATT_UUID_APPEARANCE), U16_HI(GATT_UUID_APPEARANCE)};

// static const u8 my_periConnParamCharVal[5] = {
// 	CHAR_PROP_READ,
// 	U16_LO(CONN_PARAM_DP_H), U16_HI(CONN_PARAM_DP_H),
// 	U16_LO(GATT_UUID_PERI_CONN_PARAM), U16_HI(GATT_UUID_PERI_CONN_PARAM)};

#define CUSTOM_CHARACTER_UUID_1 { 0x01, 0x19, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 }
#define CUSTOM_CHARACTER_UUID_2 { 0x02, 0x19, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 }
#define CUSTOM_CHARACTER_UUID_3 { 0x03, 0x19, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 }

u8 UUID_1[16] = CUSTOM_CHARACTER_UUID_1;
u8 UUID_2[16] = CUSTOM_CHARACTER_UUID_2;
u8 UUID_3[16] = CUSTOM_CHARACTER_UUID_3;

// u8 UUID_1[16] = TELINK_SPP_UUID_SERVICE;
// u8 UUID_2[16] = TELINK_SPP_DATA_SERVER2CLIENT;
// u8 UUID_3[16] = TELINK_SPP_DATA_CLIENT2SERVER;

u8 serviceCharacterValue_1[4] = {0x01, 0x01, 0x01, 0x01};
u8 serviceChangeCCC_1[2] = {0x00, 0x00};

u8 serviceCharacterValue_2[4] = {0x02, 0x02, 0x02, 0x02};
u8 serviceChangeCCC_2[4] = {0xcc, 0xcc, 0xcc, 0x02};

u8 serviceCharacterValue_3[4] = {0x03, 0x03, 0x03, 0x03};
u8 serviceChangeCCC_3[4] = {0xcc, 0xcc, 0xcc, 0x03};

static const u8 my_devName[] = {'T', 'e', 'L', 'i', 'n', 'k', '_', 'D', 'e', 'v', 'i', 'c', 'e'};

//// GATT attribute values
// static const u8 my_serviceChangeCharVal[5] = {
// 	CHAR_PROP_INDICATE,
// 	U16_LO(GenericAttribute_ServiceChanged_DP_H), U16_HI(GenericAttribute_ServiceChanged_DP_H),
// 	U16_LO(GATT_UUID_SERVICE_CHANGE), U16_HI(GATT_UUID_SERVICE_CHANGE)};

static const u8 u1_serviceChangeCharVal[19] = {
	CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_NOTIFY,
	U16_LO(GenericAttribute_ServiceChanged_CD_H), U16_HI(GenericAttribute_ServiceChanged_CD_H),
	CUSTOM_CHARACTER_UUID_1};

static const u8 u2_serviceChangeCharVal[19] = {
	CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_NOTIFY,
	U16_LO(GenericAttribute_ServiceChanged_DP_H), U16_HI(GenericAttribute_ServiceChanged_DP_H),
	CUSTOM_CHARACTER_UUID_2};

static const u8 u3_serviceChangeCharVal[19] = {
	CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_NOTIFY,
	U16_LO(GenericAttribute_ServiceChanged_DP_H), U16_HI(GenericAttribute_ServiceChanged_DP_H),
	CUSTOM_CHARACTER_UUID_3};

extern int characater1_write(rf_packet_att_write_t *p);
extern int characater2_write(rf_packet_att_write_t *p);
extern int characater3_write(rf_packet_att_write_t *p);

extern int characater1_read(rf_packet_att_read_t *p);
extern int characater2_read(rf_packet_att_read_t *p);
extern int characater3_read(rf_packet_att_read_t *p);

static const attribute_t my_Attributes[] = {

	{ATT_END_H - 1, 0, 0, 0, 0, 0}, // total num of attribute
	// 0001 - 0007  gap
	{3, ATT_PERMISSIONS_READ, 2, 2, (u8 *)(&primaryServiceUUID), (u8 *)(&my_gapServiceUUID), 0},
	{0, ATT_PERMISSIONS_READ, 2, sizeof(my_devNameCharVal), (u8 *)(&characterUUID), (u8 *)(my_devNameCharVal), 0},
	{0, ATT_PERMISSIONS_READ, 2, sizeof(my_devName), (u8 *)(&my_devNameUUID), (u8 *)(my_devName), 0},
	// {0, ATT_PERMISSIONS_READ, 2, sizeof(my_appearanceCharVal), (u8 *)(&characterUUID), (u8 *)(my_appearanceCharVal), 0},
	// {0, ATT_PERMISSIONS_READ, 2, sizeof(my_appearance), (u8 *)(&my_appearanceUIID), (u8 *)(&my_appearance), 0},
	// {0, ATT_PERMISSIONS_READ, 2, sizeof(my_periConnParamCharVal), (u8 *)(&characterUUID), (u8 *)(my_periConnParamCharVal), 0},
	// {0, ATT_PERMISSIONS_READ, 2, sizeof(my_periConnParameters), (u8 *)(&my_periConnParamUUID), (u8 *)(&my_periConnParameters), 0},
	// 0008 - 000b gatt
	{4, ATT_PERMISSIONS_READ, 2, 2, (u8 *)(&primaryServiceUUID), (u8 *)(&my_gattServiceUUID), 0},

	// character declartion 1
	{0, ATT_PERMISSIONS_READ, 2, sizeof(u1_serviceChangeCharVal), (u8 *)(&characterUUID), (u8 *)(u1_serviceChangeCharVal), 0},
	{0, ATT_PERMISSIONS_RDWR, 16, sizeof(serviceCharacterValue_1), (u8 *)(&UUID_1), (u8 *)(&serviceCharacterValue_1), (att_readwrite_callback_t)&characater1_write, (att_readwrite_callback_t)&characater1_read},
	{0, ATT_PERMISSIONS_RDWR, 2, sizeof(serviceChangeCCC_1), (u8 *)(&clientCharacterCfgUUID), (u8 *)(serviceChangeCCC_1), 0},

	// character declartion 2
	// {0, ATT_PERMISSIONS_READ, 2, sizeof(u2_serviceChangeCharVal), (u8 *)(&characterUUID), (u8 *)(u2_serviceChangeCharVal), 0},
	// {0, ATT_PERMISSIONS_RDWR, 16, sizeof(serviceCharacterValue_2), (u8 *)(&UUID_2), (u8 *)(serviceCharacterValue_2), (att_readwrite_callback_t)&characater2_write, (att_readwrite_callback_t)&characater2_read},
	// {0, ATT_PERMISSIONS_RDWR, 2, sizeof(serviceChangeCCC_2), (u8 *)(&clientCharacterCfgUUID), (u8 *)(serviceChangeCCC_2), 0},

	// // character declartion 3
	// {0, ATT_PERMISSIONS_READ, 2, sizeof(u3_serviceChangeCharVal), (u8 *)(&characterUUID), (u8 *)(u3_serviceChangeCharVal), 0},
	// {0, ATT_PERMISSIONS_RDWR, 16, sizeof(serviceCharacterValue_3), (u8 *)(&UUID_3), (u8 *)(serviceCharacterValue_3), (att_readwrite_callback_t)&characater3_write, (att_readwrite_callback_t)&characater3_read},
	// {0, ATT_PERMISSIONS_RDWR, 2, sizeof(serviceChangeCCC_3), (u8 *)(&clientCharacterCfgUUID), (u8 *)(serviceChangeCCC_3), 0},

};

void my_att_init(void)
{
	bls_att_setAttributeTable((u8 *)my_Attributes);
}
