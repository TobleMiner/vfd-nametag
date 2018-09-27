#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "hcs_12SS59t.h"
#include "ui.h"
#include "menu.h"
#include "menu_render.h"
#include "userio.h"
#include "encoder.h"
#include "button.h"

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22

#define PIN_NUM_RST  18

struct menu_entry menu_entries_0[] = {
	{
		.name = "STATE",
		.entry_data = {
			.datatype = DATATYPE_INT8,
			.semantic_type = MENU_ENTRY_TYPE_ON_OFF,
			.key = "wifi.enable",
		}
	},
	{
		.name = "SHOW PASSWD",
		.entry_data = {
			.datatype = DATATYPE_STRING,
			.key = "wifi.password",
			.flags = {
				.readonly = 1,
			}
		}
	},
	{
		.name = "RESET PASSWD"
	},
	{ },
};

struct menu_entry menu_entries_1[] = {
	{
		.name = "menu10"
	},
	{
		.name = "menu11"
	},
	{
		.name = "menu12"
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
		.name = "menu1",
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
	}
};

struct event_queue_data {
	struct userio* userio;
	struct menu* menu;
};

void userio_event_loop(void* arg) {
	struct event_queue_data* eq_data = arg;
	struct userio* userio = eq_data->userio;
	struct menu_state* state = &eq_data->menu->state;
	userio_action action;

	while(1) {
		if(userio_wait_event(userio, &action)) {
			switch(action) {
				case USERIO_ACTION_NEXT:
					menu_next(state);
					break;
				case USERIO_ACTION_PREV:
					menu_prev(state);
					break;
				case USERIO_ACTION_SELECT:
					menu_descend(state);
					break;
				case USERIO_ACTION_BACK:
					menu_ascend(state);
					break;
			}
		}
	}
}

void app_main()
{
	esp_err_t err;

	printf("Starting VFD name badge app\n");

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

	printf("Allocating datastore...\n");
	struct datastore* ds;
	err = datastore_alloc(&ds, 0, datastore_mem_defaults, ARRAY_LEN(datastore_mem_defaults));
	ESP_ERROR_CHECK(err);
	printf("Datastore allocated\n");

	printf("Adding values to datastore...\n");
	err = datastore_store(ds, "Hello World!", "test", DATATYPE_STRING);
	ESP_ERROR_CHECK(err);

	int32_t intval = 0x42424242;
	err = datastore_store(ds, &intval, "test.int", DATATYPE_INT32);
	ESP_ERROR_CHECK(err);
	printf("Values added\n");

	printf("Reading back values from datastore...\n");
	char* strval = NULL;
	int32_t* intvalp = NULL;

	err = datastore_load(ds, (void**)&strval, "test", DATATYPE_STRING);
	ESP_ERROR_CHECK(err);

	printf("Read back key test as '%s'\n", strval ? strval : "NULL");

	err = datastore_load(ds, (void**)&intvalp, "test.int", DATATYPE_INT32);
	ESP_ERROR_CHECK(err);

	printf("Read back key test.int as '%d'\n", intvalp ? *intvalp : 0);

	err = datastore_load(ds, (void**)&strval, "default.test", DATATYPE_STRING);
	ESP_ERROR_CHECK(err);

	printf("Read back key default.test as '%s'\n", strval ? strval : "NULL");

	struct menu* menu;
	err = menu_alloc(&menu, ui, &main_menu, ds, ds, NULL, NULL);
	ESP_ERROR_CHECK(err);

	ui_add_element(&menu->ui_element, ui);
	ui_set_active_element(ui, &menu->ui_element);

	struct menu_render* menu_render;
	err = menu_render_alloc(&menu_render);
	ESP_ERROR_CHECK(err);

	ui_element_attach_render(&menu_render->ui_render, &menu->ui_element);

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
	eq_data.menu = menu;

	xTaskCreate(userio_event_loop, "userio_event_loop", 2048, &eq_data, 10, NULL);

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
