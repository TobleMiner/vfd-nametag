#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "hcs_12SS59t.h"
#include "menu.h"
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
			.type = MENU_ENTRY_ON_OFF,
			.key = "wifi.enable",
			
		}
	},
	{
		.name = "PASSWORD"
	},
	{
		.name = "PW RESET"
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


struct event_queue_data {
	struct userio* userio;
	struct menu_state* state;
};

void userio_event_loop(void* arg) {
	struct event_queue_data* eq_data = arg;
	struct userio* userio = eq_data->userio;
	struct menu_state* state = eq_data->state;
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

	struct menu_state state;
	menu_init(&main_menu, &state);

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
	eq_data.state = &state;

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
		display_text_display(disp, menu_current_name(&state));
		display_set_brightness(disp, brightness);

//		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}
