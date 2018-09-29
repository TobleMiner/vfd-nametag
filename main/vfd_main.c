#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "hcs_12SS59t.h"
#include "ui.h"
#include "datastore.h"
#include "menu.h"
#include "menu_render.h"
#include "userio.h"
#include "encoder.h"
#include "button.h"
#include "wifi.h"
#include "random.h"
#include "flash.h"

#include "device_vars.h"

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22

#define PIN_NUM_RST  18

static esp_err_t generate_wifi_password_(char** retval) {
	*retval = wifi_alloc_password(12);
	if(!*retval) {
		return ESP_ERR_NO_MEM;
	}
	return ESP_OK;

}

static esp_err_t generate_wifi_password(void** value, const char* key, int datatype, void* priv) {
	if(datatype != DATATYPE_STRING) {
		return ESP_ERR_INVALID_ARG;
	}

	return generate_wifi_password_(value);
}

static esp_err_t set_wifi_state_(struct datastore* ds, bool state) {
	esp_err_t err = ESP_OK;

	printf("Setting wifi state to %u\n", (unsigned)state);

	if(state) {
		char* passwd;

		if((err = datastore_load(ds, &passwd, "wifi.password", DATATYPE_STRING))) {
			goto fail;
		}

		err = wifi_ap_start(NAMETAG_SSID, passwd);

		free(passwd);
	} else {
		wifi_ap_stop();
	}

fail:
	return err;
}

static esp_err_t reset_wifi_password(struct menu* menu, struct menu_entry* entry, void* priv) {
	esp_err_t err;
	char* passwd;
	struct datastore* ds = menu_get_datastore(menu, entry);

	printf("Resetting WiFi password\n");

	if((err = generate_wifi_password_(&passwd))) {
		return err;
	}

	if((err = datastore_store(menu->ds_persistent, passwd, "wifi.password", DATATYPE_STRING))) {
		return err;
	}

	if(wifi_enabled()) {
		if((err = set_wifi_state_(menu->ds_persistent, false))) {
			return err;
		}
		if((err = set_wifi_state_(menu->ds_persistent, true))) {
			return err;
		}
	}

	return ESP_OK;
}

static esp_err_t set_wifi_state(struct menu* menu, struct menu_entry* entry, void* priv) {
	esp_err_t err;
	ssize_t len;
	uint8_t state;
	struct datastore* ds = menu_get_datastore(menu, entry);

	printf("Setting WiFi state\n");

	if((len = datastore_load_inplace(ds, &state, sizeof(state), entry->entry_data.key, entry->entry_data.datatype)) < 0) {
		err = -len;
		return err;
	}

	return set_wifi_state_(menu->ds_persistent, state);
}

struct menu_entry menu_entries_0[] = {
	{
		.name = "STATE",
		.entry_data = {
			.datatype = DATATYPE_INT8,
			.semantic_type = MENU_ENTRY_TYPE_ON_OFF,
			.key = "wifi.enable",
		},
		.select_cb = set_wifi_state,
	},
	{
		.name = "SHOW PASSWD",
		.entry_data = {
			.datatype = DATATYPE_STRING,
			.key = "wifi.password",
			.flags = {
				.readonly = 1,
				.persistent = 1,
			}
		}
	},
	{
		.name = "RESET PASSWD",
		.entry_data = {
			.key = "wifi.password",
			.flags = {
				.persistent = 1,
				.suppress_editor = 1,
			}
		},
		.select_cb = reset_wifi_password,
	},
	{ },
};

struct menu_entry menu_entries_1[] = {
	{
		.name = "ON_OFF",
		.entry_data = {
			.datatype = DATATYPE_INT8,
			.semantic_type = MENU_ENTRY_TYPE_ON_OFF,
			.key = "test.on_off",
			.flags = {
				.readonly = 1,
			}
		},
	},
	{
		.name = "STRING",
		.entry_data = {
			.datatype = DATATYPE_STRING,
			.key = "test.string",
			.flags = {
				.readonly = 1,
			}
		}
	},
	{
		.name = "INT8",
		.entry_data = {
			.datatype = DATATYPE_INT8,
			.key = "test.int8",
			.flags = {
				.readonly = 1,
			}
		}
	},
	{
		.name = "INT16",
		.entry_data = {
			.datatype = DATATYPE_INT16,
			.key = "test.int16",
			.flags = {
				.readonly = 1,
			}
		}
	},
	{
		.name = "INT32",
		.entry_data = {
			.datatype = DATATYPE_INT32,
			.key = "test.int32",
			.flags = {
				.readonly = 1,
			}
		}
	},
	{
		.name = "INT64",
		.entry_data = {
			.datatype = DATATYPE_INT64,
			.key = "test.int64",
			.flags = {
				.readonly = 1,
			}
		}
	},
	{ },
};

struct menu_entry menu_entries_2[] = {
	{
		.name = "menu20"
	},
	{
		.name = "menu21"
	},
	{
		.name = "menu22"
	},
	{ },
};

struct menu_entry menu_entries_main[] = {
	{
		.name = "WIFI",
		.entries = menu_entries_0,
	},
	{
		.name = "TYPE TESTING",
		.entries = menu_entries_1,
	},
	{
		.name = "menu2",
		.entries = menu_entries_2,
	},
	{ },
};

struct menu_entry main_menu = {
	.name = NULL,
	.entries = menu_entries_main,
};

const int8_t ZERO = 0;
const int8_t TESTVAL8 = 0x42;
const int16_t TESTVAL16 = 0x4242;
const int32_t TESTVAL32 = 0x424242;
const int64_t TESTVAL64 = 0x42424242;

struct datastore_kvpair_default datastore_nvs_defaults[] = {
	{
		.kvpair = {
			.key = "wifi.password",
			.datatype = DATATYPE_STRING,
		},
		.default_cb = generate_wifi_password,
	},
};

struct datastore_kvpair_default datastore_mem_defaults[] = {
	{
		.kvpair = {
			.key = "wifi.enable",
			.value = &ZERO,
			.datatype = DATATYPE_INT8,
		},
	},
	{
		.kvpair = {
			.key = "default.test",
			.value = "Default content",
			.datatype = DATATYPE_STRING,
		},
	},
	{
		.kvpair = {
			.key = "test.on_off",
			.value = &ZERO,
			.datatype = DATATYPE_INT8,
		},
	},
	{
		.kvpair = {
			.key = "test.string",
			.value = "TESTSTRING",
			.datatype = DATATYPE_STRING,
		},
	},
	{ 
		.kvpair =
		{
			.key = "test.int8",
			.value = &TESTVAL8,
			.datatype = DATATYPE_INT8,
		},
	},
	{ 
		.kvpair =
		{
			.key = "test.int16",
			.value = &TESTVAL16,
			.datatype = DATATYPE_INT16,
		},
	},
	{ 
		.kvpair =
		{
			.key = "test.int32",
			.value = &TESTVAL32,
			.datatype = DATATYPE_INT32,
		},
	},
	{ 
		.kvpair =
		{
			.key = "test.int64",
			.value = &TESTVAL64,
			.datatype = DATATYPE_INT64,
		},
	},
};

struct event_queue_data {
	struct userio* userio;
	struct ui* ui;
};

void userio_event_loop(void* arg) {
	struct event_queue_data* eq_data = arg;
	struct userio* userio = eq_data->userio;
	struct ui* ui = eq_data->ui;
	userio_action action;
	esp_err_t err;

	while(1) {
		if(userio_wait_event(userio, &action)) {
			err = ui_action_performed(ui, action);
			if(err) {
				printf("Action failed: %x\n", err);
			}
		}
	}
}

void app_main()
{
	esp_err_t err;

	random_enable();

	printf("Starting VFD name badge app\n");

	err = flash_init();
	ESP_ERROR_CHECK(err);

	err = wifi_init();
	ESP_ERROR_CHECK(err);

	printf("Initializing SPI...\n");
	spi_bus_config_t buscfg = {
		.miso_io_num = PIN_NUM_MISO,
		.mosi_io_num = PIN_NUM_MOSI,
		.sclk_io_num = PIN_NUM_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 32
	};

	err = spi_bus_initialize(HSPI_HOST, &buscfg, 0);
	printf("SPI init finish, %d\n", err);
	ESP_ERROR_CHECK(err);
	printf("SPI initialized\n");

	struct display* disp;

	printf("Initializing VFD...\n");
	err = hcs_alloc(&disp, HSPI_HOST, PIN_NUM_CS, PIN_NUM_RST);
	printf("VFD init finish, %d\n", err);
	ESP_ERROR_CHECK(err);
	printf("VFD initialized\n");

	err = display_text_display(disp, "Hello_World");
	ESP_ERROR_CHECK(err);

	struct ui* ui;
	err = ui_alloc(&ui, disp);
	ESP_ERROR_CHECK(err);

	printf("Allocating volatile datastore...\n");
	struct datastore* ds_volatile;
	err = datastore_alloc(&ds_volatile, 0, datastore_mem_defaults, ARRAY_LEN(datastore_mem_defaults));
	ESP_ERROR_CHECK(err);
	printf("Volatile datastore allocated\n");

	printf("Allocating persistent datastore...\n");
	struct datastore* ds_persistent;
	err = datastore_alloc(&ds_persistent, DATASTORE_FLAG_PERSISTENT, datastore_nvs_defaults, ARRAY_LEN(datastore_nvs_defaults));
	ESP_ERROR_CHECK(err);
	printf("Persistent datastore allocated\n");

	struct menu* menu;
	err = menu_alloc(&menu, ui, &main_menu, ds_volatile, ds_persistent, NULL, NULL);
	ESP_ERROR_CHECK(err);

	ui_add_element(&menu->ui_element, ui);
	ui_set_active_element(ui, &menu->ui_element);

	struct userio* userio;
	err = userio_alloc(&userio);
	ESP_ERROR_CHECK(err);

	struct encoder* enc;
	err = encoder_alloc(&enc, userio, 12, 13);
	ESP_ERROR_CHECK(err);

	struct button_gpio* button_ok;
	err = button_gpio_alloc(&button_ok, userio, 14, USERIO_ACTION_SELECT);
	ESP_ERROR_CHECK(err);

	struct button_gpio* button_back;
	err = button_gpio_alloc(&button_back, userio, 27, USERIO_ACTION_BACK);
	ESP_ERROR_CHECK(err);

	struct event_queue_data eq_data;
	eq_data.userio = userio;
	eq_data.ui = ui;

	xTaskCreate(userio_event_loop, "userio_event_loop", 8192, &eq_data, 10, NULL);

	uint8_t brightness = 1;
	uint8_t direction = 0;

	while(1) {
		vTaskDelay(60 / portTICK_PERIOD_MS);
		if(direction) {
				if(brightness-- <= 1) {
				direction = 0;
				
			}
		} else {
			if(brightness++ >= 14) {
				direction = 1;
			}
		}
		ui_do_render(ui);
//		display_text_display(disp, menu_current_name(&menu->state));
		display_set_brightness(disp, brightness);

//		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}
