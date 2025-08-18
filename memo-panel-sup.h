#ifndef _memo_panel_sup_h_
#define _memo_panel_sup_h_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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
bool is_tts_available();
void stop_tts();

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
#define APPVERSION "0.9.10"

#ifdef __cplusplus
}
#endif

#endif // _memo_panel_sup_h_
