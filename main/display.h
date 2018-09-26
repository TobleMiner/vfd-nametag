#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#define DISPLAY_CAP_TEXT	0b1
#define DISPLAY_CAP_GRAPHICS	0b10

struct display;

struct display_ops {
	esp_err_t (*blank)(struct display* disp, bool blank);
	esp_err_t (*set_brightness)(struct display* disp, unsigned brightness);
};

struct display_ops_text {
	esp_err_t (*get_width)(struct display* disp, size_t* width);
	esp_err_t (*display)(struct display* disp, char* str);
	esp_err_t (*display_bin)(struct display* disp, char* str, size_t len);
	esp_err_t (*define_char)(struct display* disp, char id, void* data);
};

struct display {
	uint8_t capabilities;

	struct display_ops ops;
	struct display_ops_text text_ops;
};

#define display_blank(disp, blank) ((disp)->ops.blank((disp), (blank)))
#define display_set_brightness(disp, brightness) ((disp)->ops.set_brightness((disp), (brightness)))

#define display_text_get_width(disp, width) ((disp)->text_ops.get_width((disp), (width)))
#define display_text_display(disp, str) ((disp)->text_ops.display((disp), (str)))
#define display_text_display_bin(disp, str, len) ((disp)->text_ops.display_bin((disp), (blank), (len)))
#define display_text_define_char(disp, id, data) ((disp)->text_ops.define_char((disp), (id), (data)))

#endif
