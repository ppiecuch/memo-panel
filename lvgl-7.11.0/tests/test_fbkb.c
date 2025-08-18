/**
 * Test program for fbkb driver integration with LVGL
 */

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#define LV_CONF_INCLUDE_SIMPLE

#include "../lvgl.h"
#include "../lv_drivers/indev/fbkb.h"

static bool running = true;

void signal_handler(int sig) {
    running = false;
}

int main(void) {
#if USE_FBKB
    printf("Testing FBKB driver integration\n");
    printf("Press any keys to test (Ctrl+C to exit)\n\n");
    
    signal(SIGINT, signal_handler);
    
    /* Initialize LVGL */
    lv_init();
    
    /* Initialize fbkb driver */
    fbkb_init();
    
    /* Create input device driver */
    lv_indev_drv_t kb_drv;
    lv_indev_drv_init(&kb_drv);
    kb_drv.type = LV_INDEV_TYPE_KEYPAD;
    kb_drv.read_cb = fbkb_read;
    lv_indev_t *kb_indev = lv_indev_drv_register(&kb_drv);
    
    printf("FBKB driver initialized successfully\n");
    printf("Testing keyboard input (press keys, arrows, etc):\n");
    
    /* Simple test loop */
    lv_indev_data_t data;
    int count = 0;
    
    while (running && count < 20) {
        fbkb_read(&kb_drv, &data);
        
        if (data.state == LV_INDEV_STATE_PR && data.key != 0) {
            printf("Key pressed: 0x%02X", data.key);
            
            switch(data.key) {
                case LV_KEY_UP:
                    printf(" (UP arrow)");
                    break;
                case LV_KEY_DOWN:
                    printf(" (DOWN arrow)");
                    break;
                case LV_KEY_LEFT:
                    printf(" (LEFT arrow)");
                    break;
                case LV_KEY_RIGHT:
                    printf(" (RIGHT arrow)");
                    break;
                case LV_KEY_ENTER:
                    printf(" (ENTER)");
                    break;
                case LV_KEY_ESC:
                    printf(" (ESC)");
                    break;
                case LV_KEY_BACKSPACE:
                    printf(" (BACKSPACE)");
                    break;
                case LV_KEY_DEL:
                    printf(" (DELETE)");
                    break;
                case LV_KEY_HOME:
                    printf(" (HOME)");
                    break;
                case LV_KEY_END:
                    printf(" (END)");
                    break;
                case LV_KEY_NEXT:
                    printf(" (TAB/NEXT)");
                    break;
                default:
                    if (data.key >= 32 && data.key < 127) {
                        printf(" ('%c')", (char)data.key);
                    }
                    break;
            }
            printf("\n");
            count++;
        }
        
        usleep(10000); /* 10ms delay */
    }
    
    printf("\nCleaning up...\n");
    fbkb_deinit();
    
    printf("Test completed successfully!\n");
#else
    printf("Test not available!\n");
#endif
    return 0;
}
