#include "led_task.h"

int LED_PINS[]= {
	GPIO_PC1, 
	GPIO_PB5,
	GPIO_PC4,
	GPIO_PD2,
	GPIO_PD3
};


void led_init(){
	for(int i=0; i<(sizeof(LED_PINS)/sizeof(int));i++){
		gpio_set_func(LED_PINS[i], AS_GPIO);
		gpio_set_output_en(LED_PINS[i], 1);
		gpio_set_input_en(LED_PINS[i], 0); 
	}
	gpio_write(LED_PINS[1], 1);
}


void led_pwm_task()
{
	// led_init();
	// u8 count = 0;
	// while(1)
	// {
    //     // at_print("Hello FreeRTOS!!\r\n");
	// 	// printf("count: %d\n", count++);
	// 	for(int i=0; i<(sizeof(LED_PINS)/sizeof(int));i++){
	// 		gpio_write(LED_PINS[i], 1);
	// 		WaitMs(500);
	// 		gpio_write(LED_PINS[i], 0);
	// 		WaitMs(500);
	// 	}
	// }
}

