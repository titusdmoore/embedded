#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

const int DEBUG_MODE = 0;

// Setup Constants for PINS
const int LED_PIN = 15;
const int MAX_DUTY_CYCLE = USHRT_MAX;

volatile int stateValue = 0;
volatile bool stateChanged = false;

void setLedDutyCycle(uint gpio_pin, uint16_t value) {
	uint slice = pwm_gpio_to_slice_num(gpio_pin);
	uint channel = pwm_gpio_to_channel(gpio_pin);

	pwm_set_wrap(slice, MAX_DUTY_CYCLE);
	pwm_set_chan_level(slice, channel, value);
	pwm_set_enabled(slice, true);
}

int main() {
	stdio_init_all();

	// gpio_init(LED_PIN);
	gpio_set_function(LED_PIN, GPIO_FUNC_PWM);

	int brightness = 0;
	int direction = 1;

	while (true) {
		// if (brightness >= 75) direction = -1;
		// if (brightness <= 0) direction = 1;
		//
		// printf("Setting Value: %d %d\n", brightness, (int) floor(MAX_DUTY_CYCLE * ((float)brightness / 100)));
		// setLedDutyCycle(LED_PIN, (int) floor(MAX_DUTY_CYCLE * ((float)brightness / 100)));
		// sleep_ms(100);
		//
		// brightness += direction;

		for (int i = 0; i < 75; i++) {
			setLedDutyCycle(LED_PIN, (int) floor(MAX_DUTY_CYCLE * ((float)i / 100)));
			sleep_ms(5);
			
		}

		sleep_ms(500);

		for (int i = 75; i > 0; i--) {
			setLedDutyCycle(LED_PIN, (int) floor(MAX_DUTY_CYCLE * ((float)i / 100)));
			sleep_ms(5);
		}

		sleep_ms(250);

		/* do {
			printf("This is the level: %f %d %d\n", ((float)brightness / 100), brightness, (int) floor(MAX_DUTY_CYCLE * ((float)brightness / 100)));
			if (brightness >= 75) {
				direction = -1;
			}


			brightness += direction;
		} while(brightness > 0); */

		// setLedDutyCycle(LED_PIN, 1000);
	}
}
