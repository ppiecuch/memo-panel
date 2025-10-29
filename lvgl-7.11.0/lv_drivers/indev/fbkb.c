/**
 * @file fbkb.c
 * @brief Framebuffer keyboard input driver for LVGL
 */

/*********************
 *      INCLUDES
 *********************/
#include "fbkb.h"
#if USE_FBKB

#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

/*********************
 *      DEFINES
 *********************/
#define FBKB_TTY_PATH "/dev/tty2"

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    int fd;
    struct termios orig_termios;
    bool raw_mode;
    bool nonblock;
    int saved_kdmode;
    uint32_t saved_fcntl_flags;
} fbkb_state_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static int fbkb_set_raw_mode(uint32_t flags);
static int fbkb_restore_mode(void);
static uint32_t fbkb_read_key(void);
static uint32_t fbkb_convert_to_lv_key(uint32_t key);

/**********************
 *  STATIC VARIABLES
 **********************/
static fbkb_state_t fbkb_state = {
    .fd = -1,
    .raw_mode = false,
    .nonblock = false
};

static uint32_t last_key = 0;
static lv_indev_state_t key_state = LV_INDEV_STATE_REL;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void hide_cursor() {
    printf("\033[?25l");
    fflush(stdout);
}

static void show_cursor() {
    printf("\033[?25h");
    fflush(stdout);
}

/**
 * Initialize the framebuffer keyboard
 */
void fbkb_init(void)
{
    fbkb_state.fd = open(FBKB_TTY_PATH, O_RDWR);
    if (fbkb_state.fd < 0) {
        perror("Failed to open TTY");
        return;
    }

    if (fbkb_set_raw_mode(FBKB_FL_KB_NONBLOCK) != FBKB_SUCCESS) {
        close(fbkb_state.fd);
        fbkb_state.fd = -1;
        perror("Failed to set raw mode on TTY");
    }

    hide_cursor();
}

/**
 * Deinitialize the framebuffer keyboard
 */
void fbkb_exit(void)
{
    if (fbkb_state.fd >= 0) {
        fbkb_restore_mode();
        close(fbkb_state.fd);
        fbkb_state.fd = -1;
    }

    show_cursor();
}

/**
 * Read keyboard input for LVGL
 * @param indev_drv pointer to the related input device driver
 * @param data store the read data here
 * @return false: no more data to be read
 */
bool fbkb_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    (void)indev_drv;

    if (fbkb_state.fd < 0) {
        return false;
    }

    uint32_t key = fbkb_read_key();

    if (key != 0) {
        last_key = fbkb_convert_to_lv_key(key);
        key_state = LV_INDEV_STATE_PR;
    } else if (key_state == LV_INDEV_STATE_PR) {
        key_state = LV_INDEV_STATE_REL;
    }

    data->key = last_key;
    data->state = key_state;

    return false;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Set the TTY keyboard input to raw mode
 */
static int fbkb_set_raw_mode(uint32_t flags)
{
    struct termios t;
    int rc;

    if (fbkb_state.raw_mode) {
        return FBKB_ERR_KB_WRONG_MODE;
    }

    if (ioctl(fbkb_state.fd, KDGKBMODE, &fbkb_state.saved_kdmode) != 0) {
        return FBKB_ERR_KB_MODE_GET_FAILED;
    }

    if (fbkb_state.saved_kdmode != K_XLATE) {
        if (ioctl(fbkb_state.fd, KDSKBMODE, K_XLATE) != 0) {
            return FBKB_ERR_KB_MODE_SET_FAILED;
        }
    }

    if (tcgetattr(fbkb_state.fd, &fbkb_state.orig_termios) != 0) {
        return FBKB_ERR_KB_MODE_GET_FAILED;
    }

    t = fbkb_state.orig_termios;
    t.c_iflag &= ~(BRKINT | INPCK | ISTRIP | IXON | ICRNL | INLCR);
    t.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;

    if (tcsetattr(fbkb_state.fd, TCSAFLUSH, &t) != 0) {
        return FBKB_ERR_KB_MODE_SET_FAILED;
    }

    fbkb_state.raw_mode = true;

    if (flags & FBKB_FL_KB_NONBLOCK) {
        rc = fcntl(fbkb_state.fd, F_GETFL, 0);

        if (rc < 0) {
            fbkb_restore_mode();
            return FBKB_ERR_KB_MODE_GET_FAILED;
        }

        fbkb_state.saved_fcntl_flags = rc;

        rc = fcntl(fbkb_state.fd, F_SETFL, fbkb_state.saved_fcntl_flags | O_NONBLOCK);

        if (rc < 0) {
            fbkb_restore_mode();
            return FBKB_ERR_KB_MODE_SET_FAILED;
        }

        fbkb_state.nonblock = true;
    }

    return FBKB_SUCCESS;
}

/**
 * Restore the TTY keyboard input to its previous state
 */
static int fbkb_restore_mode(void)
{
    if (fbkb_state.nonblock) {
        fcntl(fbkb_state.fd, F_SETFL, fbkb_state.saved_fcntl_flags);
        fbkb_state.nonblock = false;
    }

    if (!fbkb_state.raw_mode) {
        return FBKB_ERR_KB_WRONG_MODE;
    }

    ioctl(fbkb_state.fd, KDSKBMODE, fbkb_state.saved_kdmode);

    if (tcsetattr(fbkb_state.fd, TCSAFLUSH, &fbkb_state.orig_termios) != 0) {
        return FBKB_ERR_KB_MODE_SET_FAILED;
    }

    fbkb_state.raw_mode = false;
    return FBKB_SUCCESS;
}

/**
 * Read a single key from the keyboard
 */
static uint32_t fbkb_read_key(void)
{
    static enum {
        STATE_INITIAL,
        STATE_ESC,
        STATE_BRACKET
    } state = STATE_INITIAL;

    static char seq[8];
    static int seq_len = 0;

    char c;
    int rc = read(fbkb_state.fd, &c, 1);

    if (rc <= 0) {
        if (state != STATE_INITIAL && errno == EAGAIN) {
            return 0;
        }
        state = STATE_INITIAL;
        seq_len = 0;
        return 0;
    }

    switch (state) {
        case STATE_INITIAL:
            if (c == '\033') {
                state = STATE_ESC;
                seq[0] = c;
                seq_len = 1;
                return 0;
            }
            return (uint32_t)c;

        case STATE_ESC:
            if (c == '[') {
                state = STATE_BRACKET;
                seq[seq_len++] = c;
                return 0;
            }
            state = STATE_INITIAL;
            seq_len = 0;
            return 0;

        case STATE_BRACKET:
            seq[seq_len++] = c;

            if ((c >= 0x40 && c <= 0x7E) || seq_len >= 7) {
                seq[seq_len] = '\0';
                state = STATE_INITIAL;
                seq_len = 0;

                if (strcmp(seq, "\033[A") == 0) return 0x80000001;
                if (strcmp(seq, "\033[B") == 0) return 0x80000002;
                if (strcmp(seq, "\033[C") == 0) return 0x80000003;
                if (strcmp(seq, "\033[D") == 0) return 0x80000004;
                if (strcmp(seq, "\033[2~") == 0) return 0x80000005;
                if (strcmp(seq, "\033[3~") == 0) return 0x80000006;
                if (strcmp(seq, "\033[1~") == 0) return 0x80000007;
                if (strcmp(seq, "\033[4~") == 0) return 0x80000008;
            }
            return 0;
    }

    return 0;
}

/**
 * Convert keyboard key to LVGL key code
 */
static uint32_t fbkb_convert_to_lv_key(uint32_t key)
{
    if (key < 0x80000000) {
        switch(key) {
            case '\n':
            case '\r':
                return LV_KEY_ENTER;
            case '\033':
                return LV_KEY_ESC;
            case '\b':
            case 127:
                return LV_KEY_BACKSPACE;
            case '\t':
                return LV_KEY_NEXT;
            default:
                return key;
        }
    }

    switch(key) {
        case 0x80000001:
            return LV_KEY_UP;
        case 0x80000002:
            return LV_KEY_DOWN;
        case 0x80000003:
            return LV_KEY_RIGHT;
        case 0x80000004:
            return LV_KEY_LEFT;
        case 0x80000005:
            return LV_KEY_NEXT;
        case 0x80000006:
            return LV_KEY_DEL;
        case 0x80000007:
            return LV_KEY_HOME;
        case 0x80000008:
            return LV_KEY_END;
        default:
            return 0;
    }
}

#endif /*USE_FBKB*/
