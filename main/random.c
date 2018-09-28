#include <stdint.h>

#if RANDOM_HEADERS_AVAIL
#include <bootloader_random.h>
#endif

#include "esp_err.h"

#include "random.h"
#include "wifi.h"

extern struct wifi wifi;

static struct random random = { 0 };

void random_enable() {
#if RANDOM_HEADERS_AVAIL
	bootloader_random_enable();
	random.initialized = true;
#endif
}

void random_disable() {
#if RANDOM_HEADERS_AVAIL
	bootloader_random_disable();
	random.initialized = false;
#endif
}

bool random_initialized() {
	return random.initialized || wifi.enabled;
}

uint32_t random_uint32() {
	return esp_random();
};


