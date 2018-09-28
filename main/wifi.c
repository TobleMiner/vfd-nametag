#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "esp_system.h"

#include "wifi.h"

void wifi_generate_password(char* buf, size_t len) {
	size_t i, num_passwd_chars = strlen(WIFI_PASSWD_CHARS);

	for(i = 0; i < len; i++) {
		buf[i] = WIFI_PASSWD_CHARS[esp_random() % num_passwd_chars];
	}
}

char* wifi_alloc_password(size_t len) {
	char* passwd = calloc(1, len + 1);
	if(!passwd) {
		return NULL;
	}

	wifi_generate_password(passwd, len);

	return passwd;
}
