#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"

const int DEBUG_MODE = 0;

// Setup Constants for PINS
const int PINS[] = { 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
const int PIN_COUNT = sizeof(PINS) / sizeof(PINS[0]);
const int BTN_PIN = 0;

volatile int stateValue = 0;
volatile bool stateChanged = false;

void buttonCallback(uint gpio, uint32_t events) {
	if (events & GPIO_IRQ_EDGE_RISE) {
		if (DEBUG_MODE == 1) {
			printf("Button Pressed\n");
		}

		stateValue++;
		stateChanged = true;
	}
}

int main() {
	stdio_init_all();

	gpio_init(BTN_PIN);
	gpio_set_dir(BTN_PIN, GPIO_IN);
	gpio_pull_down(BTN_PIN);

	for (int i = 0; i < PIN_COUNT; i++) {
		gpio_init(PINS[i]);
		gpio_set_dir(PINS[i], GPIO_OUT);
	}

	gpio_set_irq_enabled_with_callback(
		BTN_PIN,
		GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
		true,
		&buttonCallback
	);

	while (true) {
		tight_loop_contents();

		if (stateChanged) {
			for (int i = 0; i < PIN_COUNT; i++) {
				if (DEBUG_MODE == 1) {
					printf("Running for pin: %d: %d\n", i, PINS[i]);
				}

				gpio_put(PINS[i], 0);
				int bitValue = (stateValue & (0b000000001 << i)) >> i;
				gpio_put(PINS[i], bitValue);
				stateChanged = false;
			}
		}
	}
}
