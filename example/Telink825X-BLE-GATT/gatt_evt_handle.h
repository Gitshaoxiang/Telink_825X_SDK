// #include "app.h"
#include <stack/ble/ble.h>
#include "tl_common.h"
#include "drivers.h"
#include "app_config.h"
#include "vendor/common/blt_common.h"

int characater1_write(rf_packet_att_write_t *p);
int characater2_write(rf_packet_att_write_t *p);
int characater3_write(rf_packet_att_write_t *p);

int characater1_read(rf_packet_att_read_t *p);
int characater2_read(rf_packet_att_read_t *p);
int characater3_read(rf_packet_att_read_t *p);

extern u8 serviceCharacterValue_1[4];
extern u8 serviceCharacterValue_2[4];
extern u8 serviceCharacterValue_3[4];
