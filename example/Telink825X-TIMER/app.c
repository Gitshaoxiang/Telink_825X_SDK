/********************************************************************************************************
 * @file     app.c 
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     May. 12, 2018
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
#include "drivers.h"


extern void app_timer_test_init(void);

extern void app_i2c_test_init(void);
extern void app_i2c_test_start(void);

extern void app_spi_test_init(void);
extern void app_spi_test_start(void);

extern void app_uart_test_init(void);
extern void app_uart_test_start(void);

extern void app_adc_test_init(void);
extern void app_adc_test_start(void);

extern void app_pwm_test(void);

extern void app_gpio_irq_test_init(void);

extern void app_led_init(void);

extern void test_low_power(void);

extern void app_emi_init(void);
extern void app_rf_emi_test_start(void);

// RGB1 18 GPIO_PC4
// RGB2 23 GPIO_PC1
// RGB3 15 GPIO_PB4
// C_LED 34 GPIO_PD2
// H_LED 00 GPIO_PD3

int PINS[]= {
	GPIO_PC1, 
	GPIO_PB5,
	GPIO_PC4,
	GPIO_PD2,
	GPIO_PD3
};


void user_init()
{

	for(int i=0; i<(sizeof(PINS)/sizeof(int));i++){
		gpio_set_func(PINS[i], AS_GPIO);
		gpio_set_output_en(PINS[i], 1);
		gpio_set_input_en(PINS[i], 0); 
	}

#if (DRIVER_TEST_MODE == TEST_HW_TIMER)

	app_timer_test_init();

#elif (DRIVER_TEST_MODE == TEST_GPIO_IRQ)

	app_gpio_irq_test_init();
	app_led_init();

#elif (DRIVER_TEST_MODE == TEST_UART)

	app_uart_test_init();

#elif (DRIVER_TEST_MODE == TEST_IIC)

	app_i2c_test_init();

#elif (DRIVER_TEST_MODE == TEST_SPI)

	app_spi_test_init();

#elif (DRIVER_TEST_MODE == TEST_ADC)

	app_adc_test_init();

#elif (DRIVER_TEST_MODE == TEST_PWM)

	app_pwm_test();

#elif (DRIVER_TEST_MODE == TEST_LOW_POWER)

	test_low_power();

#elif (DRIVER_TEST_MODE == TEST_RF_EMI)

	app_emi_init();

#else


#endif

}


/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
u32 tick_wakeup;
void main_loop (void)
{
#if (DRIVER_TEST_MODE == TEST_UART)

	app_uart_test_start();

#elif (DRIVER_TEST_MODE == TEST_IIC)

	app_i2c_test_start();

#elif (DRIVER_TEST_MODE == TEST_SPI)

	app_spi_test_start();

#elif (DRIVER_TEST_MODE == TEST_ADC)
	app_adc_test_start();

#elif (DRIVER_TEST_MODE == TEST_RF_EMI)

	app_rf_emi_test_start();

#else

#endif

}




