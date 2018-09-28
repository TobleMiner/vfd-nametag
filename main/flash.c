#include "nvs_flash.h"

#include "flash.h"

struct flash flash = { 0 };

esp_err_t flash_init() {
	esp_err_t err = nvs_flash_init();
	if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		if((err = nvs_flash_erase())) {
			return err;
		}
		err = nvs_flash_init();
	}
	if(!err) {
		flash.nvs_initialized = true;
	}
	return err;
}
