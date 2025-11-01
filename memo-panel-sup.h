#ifndef _memo_panel_sup_h_
#define _memo_panel_sup_h_

#include <stdbool.h>
#include <stdio.h>

// Ascii commands

#define ASCII_DELIMITER '\t'

static inline void puts_no_eol(const char *s) {
	while (*s)
		putchar(*s++);
	fflush(stdout);
}

// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html

#define console_clear() puts_no_eol("\033[1;1H\033[2J")
#define console_alt_enter() puts_no_eol("\033[?1049h")
#define console_alt_exit() puts_no_eol("\033[?1049l")

#define console_save() puts_no_eol("\033[?47h")
#define console_restore() puts_no_eol("\033[?47l")

#define cursor_reset() puts_no_eol("\033[0;0H")
#define cursor_hide() puts_no_eol("\033[?25l")
#define cursor_show() puts_no_eol("\033[?25h")
#define cursor_home() puts_no_eol("\033[H")
#define cursor_save() puts_no_eol("\0337")
#define cursor_restore() puts_no_eol("\0338")

#ifdef __cplusplus
extern "C" {
#endif

bool tty_is_devpts(const char *tty);
bool is_linux_console();
int is_console (int fd);
void vt_activate(int con_num);

void init_memo_panel();
void refresh_memo_panel();
void dump_memo_panel();
void print_memo_panel();
void finish_memo_panel();

void enable_verbose();
void disable_verbose();

// tts functions
void speak_memo_content();
void speak_text(const char *text);
void set_tts_language(const char *language);
void set_tts_speed(float speed);
void set_tts_cache_dir(const char *cache_dir);
void set_tts_enabled(bool enabled);
void set_tts_auto_speak_interval(int seconds);
bool is_tts_available();
void stop_tts();

// printer functions
void set_printer_enabled(bool enabled);

// integration functions
bool is_background_running();
void set_background_running(bool state);
void set_argv0(const char *argv0);
const char *get_stats();
const char *get_memo_line1();
const char *get_memo_line2();

typedef struct thread_handle thread_handle_t;

thread_handle_t *thread_handle_create(void (*func)(void *), void *arg); // Create a new thread, running the given function with the given argument
void thread_handle_destroy(thread_handle_t *handle); // Wait for the thread to finish and destroy the handle

extern long cron_next_schedule;

#define WORDSURL "https://raw.githubusercontent.com/ppiecuch/shared-assets/master/words.txt"
#define LOCALCACHE "/tmp/words-memo.txt"
#define APPVERSION "0.9.12"

#ifdef __cplusplus
}
#endif

#endif // _memo_panel_sup_h_
