#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/pwm.h"

#include "btstack.h"
#include "notify.h" // generated from .gatt file

#define NOTIFY_LED_PIN 15 
#define STATUS_LED_PIN 2
#define BTN_PIN 16 

const int MAX_DUTY_CYCLE = USHRT_MAX;

static btstack_packet_callback_registration_t hci_event_callback_registration;
static int le_notification_enabled = 0;
static btstack_timer_source_t btn_timer;

static volatile bool advertising = false;
static volatile bool btn_pressed = false;
static volatile uint32_t press_time_ms = 0;

static uint8_t blink_state = 0;

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
		set_led_duty_cycle(NOTIFY_LED_PIN, (int) floor(MAX_DUTY_CYCLE * ((float)i / 100)));
		sleep_ms(5);

	}

	sleep_ms(200);

	for (int i = 75; i >= 0; i--) {
		set_led_duty_cycle(NOTIFY_LED_PIN, (int) floor(MAX_DUTY_CYCLE * ((float)i / 100)));
		sleep_ms(5);
	}
}

void led_long_pulse() {
	for (int i = 0; i < 75; i++) {
		set_led_duty_cycle(NOTIFY_LED_PIN, (int) floor(MAX_DUTY_CYCLE * ((float)i / 100)));
		sleep_ms(5);

	}

	sleep_ms(500);

	for (int i = 75; i >= 0; i--) {
		set_led_duty_cycle(NOTIFY_LED_PIN, (int) floor(MAX_DUTY_CYCLE * ((float)i / 100)));
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
				adv_ready = true;
			}
			break;
		case HCI_EVENT_DISCONNECTION_COMPLETE:
			printf("Disconnected, restarting ads\n");
			setup_advertising();
			adv_ready = true;
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

static void handle_btn_interrupt(uint gpio, uint32_t events) {
	uint32_t now = to_ms_since_boot(get_absolute_time());

	if (events & GPIO_IRQ_EDGE_FALL) {
		static uint32_t last_event = 0;

		if ((now - last_event) > 50) {
			btn_pressed = true;
			press_time_ms = now;
		}

		last_event = now;
	}
}

static void handle_advertise() {
	advertising = true;
	setup_advertising();
	sleep_ms(50);
	// Stop Advertising
}

static void btn_poll_handler(btstack_timer_source_t *ts) {
	if (btn_pressed) {
		uint32_t held = to_ms_since_boot(get_absolute_time()) - press_time_ms;

		if (held > 1000) {
			btn_pressed = false;
			handle_advertise();
			blink_state = 1;
		}
	}

	// This logic I think runs regardless of the button press
	switch (blink_state) {
		case 1: gpio_put(STATUS_LED_PIN, 1); blink_state++; break;
		case 6: gpio_put(STATUS_LED_PIN, 0); blink_state++; break;
		case 7: gpio_put(STATUS_LED_PIN, 1); blink_state++; break;
		case 12: gpio_put(STATUS_LED_PIN, 0); blink_state = 0; break;
		default: if (blink_state > 0) blink_state++; break;
	}

	btstack_run_loop_set_timer(ts, 50);
	btstack_run_loop_add_timer(ts);
}

int main() {
	stdio_init_all();

	if (cyw43_arch_init()) {
		printf("CYW43 init failed\n");
		return -1;
	}

	l2cap_init();
	sm_init();

	gpio_init(STATUS_LED_PIN);
	gpio_set_function(NOTIFY_LED_PIN, GPIO_FUNC_PWM);
	gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
	// gpio_put(STATUS_LED_PIN, 0);

	gpio_init(BTN_PIN);
	gpio_set_dir(BTN_PIN, GPIO_IN);
	gpio_pull_up(BTN_PIN);

	// gpio_pull_down(BTN_PIN);

	gpio_set_irq_enabled_with_callback(
		BTN_PIN,
		GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
		true,
		&handle_btn_interrupt
	);

	att_server_init(profile_data, att_read_callback, att_write_callback);

	hci_event_callback_registration.callback = &packet_handler;
	hci_add_event_handler(&hci_event_callback_registration);

	att_server_register_packet_handler(packet_handler);

	hci_power_control(HCI_POWER_ON);
	
	btstack_run_loop_set_timer(&btn_timer, 50);
	btstack_run_loop_set_timer_handler(&btn_timer, btn_poll_handler);
	btstack_run_loop_add_timer(&btn_timer);

	btstack_run_loop_execute();

	return 0;
}
