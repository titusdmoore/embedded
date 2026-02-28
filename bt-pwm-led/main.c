#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/pwm.h"

#include "btstack.h"
#include "notify.h" // generated from .gatt file

#define LED_PIN 15 
const int MAX_DUTY_CYCLE = USHRT_MAX;

static btstack_packet_callback_registration_t hci_event_callback_registration;
static int le_notification_enabled = 0;

// forward declare
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);


void set_led_duty_cycle(uint gpio_pin, uint16_t value) {
	uint slice = pwm_gpio_to_slice_num(gpio_pin);
	uint channel = pwm_gpio_to_channel(gpio_pin);

	pwm_set_wrap(slice, MAX_DUTY_CYCLE);
	pwm_set_chan_level(slice, channel, value);
	pwm_set_enabled(slice, true);
}

void led_short_pulse() {
	for (int i = 0; i < 75; i++) {
		set_led_duty_cycle(LED_PIN, (int) floor(MAX_DUTY_CYCLE * ((float)i / 100)));
		sleep_ms(5);

	}

	sleep_ms(200);

	for (int i = 75; i >= 0; i--) {
		set_led_duty_cycle(LED_PIN, (int) floor(MAX_DUTY_CYCLE * ((float)i / 100)));
		sleep_ms(5);
	}
}

void led_long_pulse() {
	for (int i = 0; i < 75; i++) {
		set_led_duty_cycle(LED_PIN, (int) floor(MAX_DUTY_CYCLE * ((float)i / 100)));
		sleep_ms(5);

	}

	sleep_ms(500);

	for (int i = 75; i >= 0; i--) {
		set_led_duty_cycle(LED_PIN, (int) floor(MAX_DUTY_CYCLE * ((float)i / 100)));
		sleep_ms(5);
	}
}

static void led_set_intensity(uint8_t value) {
	led_short_pulse();
	sleep_ms(250);
	led_long_pulse();
}

static const uint8_t adv_data[] = {
	0x02, 0x01, 0x06,        // flags: LE General Discoverable, BR/EDR not supported
	0x0B, 0x09, 'P','i','c','o','N','o','t','i','f','y' // complete local name
};

static void setup_advertising(void) {
	bd_addr_t null_addr = {};
	gap_advertisements_set_params(0x0030, 0x0060, 0, 0, null_addr, 0x07, 0x00);
	gap_advertisements_set_data(sizeof(adv_data), (uint8_t *)adv_data);
	gap_advertisements_enable(1);
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
	if (packet_type != HCI_EVENT_PACKET) return;

	switch (hci_event_packet_get_type(packet)) {
		case BTSTACK_EVENT_STATE:
			if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
				printf("BLE up and advertising\n");
				setup_advertising();
			}
			break;
		case HCI_EVENT_DISCONNECTION_COMPLETE:
			printf("Disconnected, restarting ads\n");
			setup_advertising();
			break;
		default:
			break;
	}
}

static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
	return 0; // nothing to read for now
}

static int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
	if (att_handle == ATT_CHARACTERISTIC_0x2A56_01_VALUE_HANDLE) {
		uint8_t value = buffer[0];
		printf("Received value: %d\n", value);
		led_set_intensity(value);
	}
	return 0;
}

int main() {
	stdio_init_all();

	if (cyw43_arch_init()) {
		printf("CYW43 init failed\n");
		return -1;
	}

	l2cap_init();
	sm_init();

	gpio_set_function(LED_PIN, GPIO_FUNC_PWM);

	att_server_init(profile_data, att_read_callback, att_write_callback);

	hci_event_callback_registration.callback = &packet_handler;
	hci_add_event_handler(&hci_event_callback_registration);

	att_server_register_packet_handler(packet_handler);

	hci_power_control(HCI_POWER_ON);

	btstack_run_loop_execute();

	return 0;
}
