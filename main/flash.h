#ifndef _FLASH_H_
#define _FLASH_H_

#include <stdbool.h>

struct flash {
	bool nvs_initialized;
};

esp_err_t flash_init();

#endif
