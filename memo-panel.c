#if LVGL == 7
#include "lv_lib_png/lv_png.h"
#endif
#include "lvgl/lvgl.h"
#ifdef __linux__
#include "lvgl/lv_drivers/display/fbdev.h"
#include "lvgl/lv_drivers/indev/evdev.h"
#else /* __linux__ */
#if LVGL == 7
#include "lvgl/lv_drivers/display/monitor.h"
#include "lvgl/lv_drivers/indev/keyboard.h"
#include "lvgl/lv_drivers/indev/mouse.h"
#include "lvgl/lv_drivers/indev/mousewheel.h"
#endif
#include <SDL2/SDL.h>
#endif /* __linux__ */
#include <cJSON.h>
#include <confuse.h>
#include <curl/curl.h>
#include <dirent.h>
#include <libgen.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define NORMAL_COLOR "\x1B[0m"
#define GREEN "\x1B[32m"
#define BLUE "\x1B[34m"
#define RED "\x1B[31m"

#if LVGL > 7
#define lv_style_set_border_width(st, s, o) lv_style_set_border_width(st, o)
#define lv_style_set_text_font(st, s, o) lv_style_set_text_font(st, o)
#define lv_style_set_pad_top(st, s, o) lv_style_set_pad_top(st, o)
#define lv_style_set_pad_bottom(st, s, o) lv_style_set_pad_bottom(st, o)
#define lv_style_set_pad_left(st, s, o) lv_style_set_pad_left(st, o)
#define lv_style_set_pad_right(st, s, o) lv_style_set_pad_right(st, o)
#define lv_style_set_pad_inner(st, s, o) lv_style_set_pad_inner(st, o)
#endif

LV_FONT_DECLARE(digital_clock)

// Config options
static char *openweather_apikey = NULL;
static char *openweather_label = NULL;
static double openweather_coord[2] = { 0, 0 };

// display buffer size - not sure if this size is really needed
#define LV_BUF_SIZE (LV_HOR_RES_MAX) * (LV_VER_RES_MAX) // 1280x480

// A static variable to store the display buffers
static lv_disp_buf_t disp_buf;

// Static buffer(s).
static lv_color_t lvbuf1[LV_BUF_SIZE];

// Display info and controls
static const char *DAY[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
static const char *MONTH[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

static lv_style_t style_large, style_clock, style_memo;
static const lv_task_t *time_task, *net_task, *memo_task, *weather_task;
static const lv_font_t *font_large, *font_normal;

static lv_obj_t *clock_label[8];
static lv_obj_t *date_label, *weather_label;

static lv_obj_t *led1;
static lv_obj_t *controls_panel, *memo_panel;

static char weatherString[64] = { 0 };

// Utilities functions

#define _ssprintf(...) \
	({ int _ss_size = snprintf(0, 0, ##__VA_ARGS__);    \
    char *_ss_ret = (char*)alloca(_ss_size+1);          \
    snprintf(_ss_ret, _ss_size+1, ##__VA_ARGS__);       \
    _ss_ret; })

static void time_timer_cb(lv_task_t *timer) {
	char timeString[16] = { 0 };
	char dateString[128] = { 0 };

	time_t t = time(NULL);
	struct tm *local = localtime(&t);

	snprintf(timeString, 16, "%02d:%02d:%02d", local->tm_hour, local->tm_min, local->tm_sec);
	for (int c = 0; c < 8; c++) {
		const char str[2] = { timeString[c], 0 };
		lv_label_set_text(clock_label[c], str);
	}

	if (strlen(weatherString) > 0)
		snprintf(dateString, 128, "%s | %s %02d %04d | ", DAY[local->tm_wday], MONTH[local->tm_mon], local->tm_mday, local->tm_year + 1900);
	else
		snprintf(dateString, 128, "%s | %s %02d %04d", DAY[local->tm_wday], MONTH[local->tm_mon], local->tm_mday, local->tm_year + 1900);
	lv_label_set_text(date_label, dateString);

	lv_obj_set_x(weather_label, lv_obj_get_width(date_label));
	lv_obj_set_width(weather_label, lv_obj_get_width(controls_panel) - lv_obj_get_width(date_label));
	lv_label_set_text(weather_label, weatherString);
}

static int get_current_network_speed_cb() {
	static unsigned long int kb_sent = 0, kb_sent_prev = 0;

	FILE *fp = fopen("/proc/net/dev", "r");
	if (fp) {
		char buf[200], ifname[20];
		unsigned long int r_bytes, t_bytes, r_packets, t_packets;

		// skip first two lines
		for (int i = 0; i < 2; i++) {
			fgets(buf, 200, fp);
		}

		while (fgets(buf, 200, fp)) {
			sscanf(buf, "%[^:]: %lu %lu %*lu %*lu %*lu %*lu %*lu %*lu %lu %lu",
					ifname, &r_bytes, &r_packets, &t_bytes, &t_packets);
			if (strstr(ifname, "wlan0") != NULL) {
				kb_sent = r_bytes / 1024;
			}
		}

		unsigned long int net_speed = (kb_sent - kb_sent_prev) * 2;
		kb_sent_prev = kb_sent;

		fclose(fp);
		return net_speed;
	} else
		return -1;
}

static void net_timer_cb(lv_task_t *timer) {
	int net_speed = get_current_network_speed_cb();
	if (net_speed > 0) {
		lv_led_on(led1);
	} else
		lv_led_off(led1);
}

static void memo_timer_cb(lv_task_t *timer) {
}

static size_t round_up(size_t v) {
	if (v == 0)
		return 0;
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	/* 64 bit only */
#if SIZE_MAX > 4294967296
	v |= v >> 32;
#endif
	return ++v;
}

// Color palette
static lv_color_t cell_colors[] = {
	LV_COLOR_RED, LV_COLOR_BLUE, LV_COLOR_GREEN, LV_COLOR_YELLOW,
	LV_COLOR_ORANGE, LV_COLOR_PURPLE, LV_COLOR_TEAL, LV_COLOR_MAROON
};

lv_obj_t *create_checkerboard_canvas(lv_obj_t *parent, int cols, int rows, int cell_size) {
	int width = cols * cell_size;
	int height = rows * cell_size;

	// Create a canvas object
	lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(cell_size * cols, cell_size * rows)];
	lv_obj_t *canvas = lv_canvas_create(parent, NULL);
	lv_canvas_set_buffer(canvas, cbuf, width, height, LV_IMG_CF_TRUE_COLOR);
	lv_canvas_fill_bg(canvas, LV_COLOR_WHITE, LV_OPA_COVER);

	lv_draw_label_dsc_t label_dsc;
	lv_draw_label_dsc_init(&label_dsc);
	label_dsc.color = LV_COLOR_WHITE;

	char buf[8];
	int cell_num = 1;
	for (int row = 0; row < rows; ++row) {
		for (int col = 0; col < cols; ++col) {
			// Pick color for this cell
			lv_color_t color = cell_colors[rand() % (sizeof(cell_colors) / sizeof(cell_colors[0]))];
			lv_area_t cell_area = {
				.x1 = col * cell_size,
				.y1 = row * cell_size,
				.x2 = (col + 1) * cell_size - 1,
				.y2 = (row + 1) * cell_size - 1
			};
			lv_canvas_draw_rect(canvas, cell_area.x1, cell_area.y1, cell_size, cell_size,
					&(lv_draw_rect_dsc_t){
							.bg_color = color,
							.bg_opa = LV_OPA_COVER,
							.border_color = LV_COLOR_BLACK,
							.border_width = 1 });

			// Draw cell number in the center
			snprintf(buf, sizeof(buf), "%d", cell_num++);
			lv_point_t txt_size;
			_lv_txt_get_size(&txt_size, buf, label_dsc.font, 0, 0, LV_COORD_MAX, LV_TXT_FLAG_NONE);
			int txt_x = cell_area.x1 + (cell_size - txt_size.x) / 2;
			int txt_y = cell_area.y1 + (cell_size - txt_size.y) / 2;
			lv_canvas_draw_text(canvas, txt_x, txt_y, cell_size,
					&label_dsc,
					buf, LV_LABEL_ALIGN_LEFT);
		}
	}

	return canvas;
}

struct _mem_chunk {
	char *buf;
	size_t size;
	CURLcode res;
	bool busy;
};

static size_t _curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
	const size_t contents_size = size * nmemb;
	struct _mem_chunk *chunk = (struct _mem_chunk *)userp;

	/* realloc can be slow, therefore increase buffer to nearest 2^n */
	chunk->buf = realloc(chunk->buf, round_up(chunk->size + contents_size));
	if (!chunk->buf)
		return 0;
	/* append data and increment size */
	memcpy(chunk->buf + chunk->size, contents, contents_size);
	chunk->size += contents_size;
	chunk->buf[chunk->size] = 0; // zero-termination
	return contents_size;
}

static void *fetch_weather_api(void *thread_data) {
	// https://openweathermap.org/one-call-transfer
	const char *URL_BASE = "https://api.openweathermap.org/data/3.0/onecall?lat=%g&lon=%g&units=metric&appid=%s";

	struct _mem_chunk *chunk = (struct _mem_chunk *)thread_data;
	chunk->busy = true;
	chunk->size = 0;
	chunk->res = CURLE_OK;
	CURL *curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, _ssprintf(URL_BASE, openweather_coord[0], openweather_coord[1], openweather_apikey));
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _curl_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0/picture-frame");
		chunk->res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if (chunk->buf) {
			printf("%s[INFO]%s Downloaded %lu bytes\n", GREEN, NORMAL_COLOR, strlen(chunk->buf));
			cJSON *json = cJSON_Parse(chunk->buf);
			if (json) {
				cJSON *cod = cJSON_GetObjectItemCaseSensitive(json, "cod");
				if (cod) {
					switch (cod->valueint) {
						case 401: {
							cJSON *message = cJSON_GetObjectItemCaseSensitive(json, "message");
							if (cJSON_IsString(message)) {
								printf("%s[ERROR]%s Request error: %s\n", RED, NORMAL_COLOR, message->valuestring);
							}
						} break;
						default: {
							printf("%s[ERROR]%s Unknown JSON data: %s\n", RED, NORMAL_COLOR, chunk->buf);
						}
					}
				} else {
					strcpy(weatherString, "");
					cJSON *current = cJSON_GetObjectItemCaseSensitive(json, "current");
					if (current) {
						cJSON *temp = cJSON_GetObjectItemCaseSensitive(current, "temp");
						if (temp && cJSON_IsNumber(temp))
							strcat(weatherString, _ssprintf("Temp. %d\x7f"
															"C",
														  temp->valueint));
						cJSON *feels = cJSON_GetObjectItemCaseSensitive(current, "feels_like");
						if (feels && cJSON_IsNumber(feels))
							strcat(weatherString, _ssprintf(" / Feels %d\x7f"
															"C",
														  feels->valueint));
						cJSON *clouds = cJSON_GetObjectItemCaseSensitive(current, "clouds");
						if (clouds && cJSON_IsNumber(clouds))
							strcat(weatherString, _ssprintf(" / Clouds %d%%", clouds->valueint));
					} else
						printf("%s[ERROR]%s Unknown JSON data: %s\n", RED, NORMAL_COLOR, chunk->buf);
				}
			} else
				printf("%s[ERROR]%s Failed to parse JSON data: %s\n", RED, NORMAL_COLOR, chunk->buf);
		}
	}
	chunk->busy = false;
	return NULL;
}

struct _mem_chunk weather_info = { NULL, 0, 0, false };

static void weather_timer_cb(lv_task_t *timer) {
	static int _lock_count = 0;
	if (weather_info.busy) {
		printf("%s[INFO]%s Download already in progress\n", GREEN, NORMAL_COLOR);
		_lock_count++;
		if (_lock_count > 3) {
			printf("%s[INFO]%s Download locked. Restarting.\n", GREEN, NORMAL_COLOR);
			exit(1);
		}
		return;
	}
	static pthread_t thread;
	if (pthread_create(&thread, NULL, fetch_weather_api, &weather_info))
		printf("%s[ERROR]%s Couldn't create a thread.\n", RED, NORMAL_COLOR);
	pthread_detach(thread);

	_lock_count = 0;
}

//  Main entry

static void panel_init(char *prog_name, lv_obj_t *root) {
	font_large = &lv_font_montserrat_24;
	font_normal = &lv_font_montserrat_16;

#if LV_USE_THEME_MATERIAL
	LV_THEME_DEFAULT_INIT(lv_theme_get_color_primary(), lv_theme_get_color_primary(),
			LV_THEME_DEFAULT_FLAG,
			lv_theme_get_font_small(), lv_theme_get_font_normal(), lv_theme_get_font_subtitle(), lv_theme_get_font_title());
#endif

	lv_style_init(&style_large);
	lv_style_set_text_font(&style_large, LV_STATE_DEFAULT, font_large);

	lv_style_init(&style_clock);
	lv_style_set_text_font(&style_clock, LV_STATE_DEFAULT, &digital_clock);

	if (!root)
		root = lv_scr_act();

	// Open configuration file

	cfg_opt_t opts[] = {
		CFG_SIMPLE_STR("openweather_apikey", &openweather_apikey),
		CFG_SIMPLE_STR("openweather_label", &openweather_label),
		CFG_FLOAT_LIST("openweather_coord", "{0, 0}", CFGF_NONE),
		CFG_END()
	};
	cfg_t *cfg = cfg_init(opts, 0);
	if (cfg_parse(cfg, _ssprintf("%s.ini", basename(prog_name))) == CFG_PARSE_ERROR)
		printf("%s[ERROR]%s Couldn't open configuration file.\n", RED, NORMAL_COLOR);
	openweather_coord[0] = cfg_getnfloat(cfg, "openweather_coord", 0);
	openweather_coord[1] = cfg_getnfloat(cfg, "openweather_coord", 1);
	cfg_free(cfg);

	// Memo panel

	memo_panel = lv_cont_create(root, NULL);
	lv_obj_set_pos(memo_panel, 0, 0);
	lv_obj_set_size(memo_panel, lv_obj_get_width(root), lv_obj_get_height(root) - 150);
	lv_obj_set_auto_realign(memo_panel, true); /*Auto realign when the size changes*/
	lv_cont_set_layout(memo_panel, LV_LAYOUT_ROW_TOP);

	lv_style_init(&style_memo);

	lv_style_set_border_width(&style_memo, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_memo, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_memo, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_memo, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_memo, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_inner(&style_memo, LV_STATE_DEFAULT, 0);

	lv_obj_add_style(memo_panel, LV_CONT_PART_MAIN, &style_memo);

	printf("%s[INFO]%s Memo panel is: %d x %d\n",
			GREEN, NORMAL_COLOR,
			lv_obj_get_width(memo_panel), lv_obj_get_height(memo_panel));

	weather_timer_cb(NULL);

	// Time/date controls

	controls_panel = lv_cont_create(root, NULL);
	lv_obj_set_pos(controls_panel, 0, lv_obj_get_height(memo_panel));
	lv_obj_set_size(controls_panel, lv_obj_get_width(root), 150);

	const int gl_h = 118, gl_w = 71;
	int x_off = (lv_obj_get_width(root) - 8 * gl_w) / 2;
	for (int c = 0; c < 8; c++, x_off += gl_w) {
		clock_label[c] = lv_label_create(controls_panel, NULL);
		lv_obj_set_pos(clock_label[c], x_off, 3);
		lv_obj_set_size(clock_label[c], gl_w, gl_h);
		lv_obj_add_style(clock_label[c], LV_LABEL_PART_MAIN, &style_clock);
		lv_label_set_text(clock_label[c], "");
		lv_label_set_long_mode(clock_label[c], LV_LABEL_LONG_EXPAND);
	}

	date_label = lv_label_create(controls_panel, NULL);
	lv_obj_set_y(date_label, gl_h + 4);
	lv_obj_set_size(date_label, lv_obj_get_width(controls_panel), 25);
	lv_label_set_text(date_label, "");
	lv_obj_add_style(date_label, LV_LABEL_PART_MAIN, &style_large);
	lv_label_set_long_mode(date_label, LV_LABEL_LONG_EXPAND);

	weather_label = lv_label_create(controls_panel, NULL);
	lv_obj_set_y(weather_label, gl_h + 4);
	lv_label_set_text(weather_label, "");
	lv_obj_add_style(weather_label, LV_LABEL_PART_MAIN, &style_large);
	lv_label_set_long_mode(weather_label, LV_LABEL_LONG_SROLL);

	led1 = lv_led_create(controls_panel, NULL);
	lv_obj_set_pos(led1, 785, 1);
	lv_obj_set_size(led1, 14, 14);
	lv_led_off(led1);

	// Start processing ..

	time_task = lv_task_create(time_timer_cb, 1000, LV_TASK_PRIO_MID, NULL);
	net_task = lv_task_create(net_timer_cb, 3000, LV_TASK_PRIO_LOW, NULL);
	memo_task = lv_task_create(memo_timer_cb, 15000, LV_TASK_PRIO_LOW, NULL);
	weather_task = lv_task_create(weather_timer_cb, 10 * 60000, LV_TASK_PRIO_LOW, NULL);

	lv_obj_t *checker = create_checkerboard_canvas(root, lv_obj_get_width(root) / 80, lv_obj_get_height(root) / 80, 80);
	lv_obj_set_pos(checker, 0, 0);
	lv_obj_set_size(checker, lv_obj_get_width(root), lv_obj_get_height(root));
}

#ifdef __linux__
static void hal_init() {
	fbdev_init(); // Linux frame buffer device init
	evdev_init(); // Touch pointer device init

	// Initialize `disp_buf` with the display buffer(s)
	lv_disp_buf_init(&disp_buf, lvbuf1, NULL, LV_BUF_SIZE);

	// Initialize and register a display driver
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.flush_cb = fbdev_flush; // flushes the internal graphical buffer to the frame buffer
	disp_drv.buffer = &disp_buf; // set teh display buffere reference in the driver
	disp_drv.rotated = LV_DISP_ROT_NONE;
	disp_drv.sw_rotate = 0;
	lv_disp_drv_register(&disp_drv);

	// Initialize and register a pointer device driver
	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = evdev_read; // defined in lv_drivers/indev/evdev.h
	lv_indev_drv_register(&indev_drv);
}

static void hal_exit() {
	fbdev_exit();
}

#else /* __linux__ */

// A task to measure the elapsed time for LVGL
static int tick_thread(void *data) {
	(void)data;
	while (1) {
		SDL_Delay(5);
		lv_tick_inc(5); /* Tell LVGL that 5 milliseconds were elapsed */
	}
	return 0;
}

static void hal_init() {
	/* Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
	monitor_init();
	/* Tick init.
	 * You have to call 'lv_tick_inc()' in periodically to inform LittelvGL about
	 * how much time were elapsed Create an SDL thread to do this */
	SDL_CreateThread(tick_thread, "tick", NULL);

	/* Create a display buffe r*/
	lv_disp_buf_init(&disp_buf, lvbuf1, NULL, LV_BUF_SIZE);

	/*Create a display*/
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.flush_cb = monitor_flush; // flushes the internal graphical buffer to the frame buffer
	disp_drv.buffer = &disp_buf; // set teh display buffere reference in the driver
	lv_disp_drv_register(&disp_drv);
	disp_drv.antialiasing = 1;

	lv_group_t *g = lv_group_create();

	/* Add the mouse as input device
	 * Use the 'mouse' driver which reads the PC's mouse*/
	mouse_init();
	static lv_indev_drv_t indev_drv_1;
	lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
	indev_drv_1.type = LV_INDEV_TYPE_POINTER;

	/*This function will be called periodically (by the library) to get the mouse position and state*/
	indev_drv_1.read_cb = mouse_read;
	lv_indev_drv_register(&indev_drv_1);

	keyboard_init();
	static lv_indev_drv_t indev_drv_2;
	lv_indev_drv_init(&indev_drv_2); /* Basic initialization */
	indev_drv_2.type = LV_INDEV_TYPE_KEYPAD;
	indev_drv_2.read_cb = keyboard_read;
	lv_indev_t *kb_indev = lv_indev_drv_register(&indev_drv_2);
	lv_indev_set_group(kb_indev, g);
	mousewheel_init();
	static lv_indev_drv_t indev_drv_3;
	lv_indev_drv_init(&indev_drv_3); /* Basic initialization */
	indev_drv_3.type = LV_INDEV_TYPE_ENCODER;
	indev_drv_3.read_cb = mousewheel_read;

	lv_indev_t *enc_indev = lv_indev_drv_register(&indev_drv_3);
	lv_indev_set_group(enc_indev, g);
}

static void hal_exit() {
}

#endif /* __linux__ */

int main(int argc, char *argv[]) {
	srand(time(NULL));

	lv_init(); // LVGL init
	lv_png_init(); // png file support

	hal_init();

	// Panel initialization
	panel_init(argv[0], NULL);

	// Handle LVGL tasks (tickless mode)
	while (1) {
		lv_tick_inc(5);
		lv_task_handler();
		usleep(5000);
	}

	hal_exit();
	return 0;
}
