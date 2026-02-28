#include <stdio.h>
#include <ctype.h>
#include "pico/stdlib.h"

const int DEBUG_MODE = 1;


// Setup Constants for PINS
const int EXTERNAL_LED_01 = 0;

// Setup constants for project logic
const int DOT_DURATION = 100;
const int DASH_DURATION = DOT_DURATION * 2;


typedef struct {
	char letter;
	const char *pattern;
} MorseChar;

const MorseChar MorseCode[] = {
	{'A', ".-"},
	{'B', "-..."},
	{'C', "-.-."},
	{'D', "-.."},
	{'E', "."},
	{'F', "..-."},
	{'G', "--."},
	{'H', "...."},
	{'I', ".."},
	{'J', ".---"},
	{'K', "-.-"},
	{'L', ".-.."},
	{'M', "--"},
	{'N', "-."},
	{'O', "---"},
	{'P', ".--."},
	{'Q', "--.-"},
	{'R', ".-."},
	{'S', "..."},
	{'T', "-"},
	{'U', "..-"},
	{'V', "...-"},
	{'W', ".--"},
	{'X', "-..-"},
	{'Y', "-.--"},
	{'Z', "--.."},
	{'1', ".----"},
	{'2', "..---"},
	{'3', "...--"},
	{'4', "....-"},
	{'5', "....."},
	{'6', "-...."},
	{'7', "--..."},
	{'8', "---.."},
	{'9', "----."},
	{'0', "-----"},
	{' ', "|"},
};

int displayChar(const char *pattern) {
	int i = 0;

	while (pattern[i] != '\0') {
		switch (pattern[i]) {
			case '.':
				gpio_put(EXTERNAL_LED_01, 1);
				sleep_ms(DOT_DURATION);
				gpio_put(EXTERNAL_LED_01, 0);
				break;
			case '-':
				gpio_put(EXTERNAL_LED_01, 1);
				sleep_ms(DASH_DURATION);
				gpio_put(EXTERNAL_LED_01, 0);
				break;
			case '|':
				sleep_ms(DOT_DURATION * 1.5);
				break;
		}

		i++;
	}

	return 0;
}

const char* charToMorse(char character) {
	int tableSize = sizeof(MorseCode) / sizeof(MorseCode[0]);

	for (int i = 0; i < tableSize; i++) {
		if (MorseCode[i].letter == character) {
			return MorseCode[i].pattern;
		}
	}

	return "";
}

int main() {
	stdio_init_all();

	gpio_init(EXTERNAL_LED_01);
	gpio_set_dir(EXTERNAL_LED_01, GPIO_OUT);

	const char TEST_MESSAGE[] = "Hello World";

	printf("Hello, World! 2\n");

	while (true) {
		int messageIdx = 0;
		while (TEST_MESSAGE[messageIdx] != '\0') {
			if (DEBUG_MODE == 1) {
				printf("Printing Char: %c\n", TEST_MESSAGE[messageIdx]);
			}

			displayChar(charToMorse(toupper(TEST_MESSAGE[messageIdx])));
			sleep_ms(DASH_DURATION);
			messageIdx++;
		}

		sleep_ms(DASH_DURATION * 2);
	}
}
