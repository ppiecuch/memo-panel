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

bool is_background_running();
void set_background_running(bool state);
void set_argv0(const char *argv0);
const char *get_stats();

#define WORDSURL "https://raw.githubusercontent.com/ppiecuch/shared-assets/master/words.txt"
#define LOCALCACHE "/tmp/words-memo.txt"
#define APPVERSION "0.9.1"

#ifdef __cplusplus
}
#endif

#endif // _memo_panel_sup_h_
