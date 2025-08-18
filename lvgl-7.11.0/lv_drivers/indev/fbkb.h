/**
 * @file fbkb.h
 * @brief Framebuffer keyboard input driver for LVGL
 */

#ifndef FBKB_H
#define FBKB_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifndef LV_DRV_NO_CONF
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lv_drv_conf.h"
#else
#include "../../lv_drv_conf.h"
#endif
#endif

#if USE_FBKB

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include <stdint.h>
#include <stdbool.h>


/*********************
 *      DEFINES
 *********************/

/*Error codes*/
#define FBKB_SUCCESS                       0
#define FBKB_ERR_KB_WRONG_MODE            -1
#define FBKB_ERR_KB_MODE_GET_FAILED       -2
#define FBKB_ERR_KB_MODE_SET_FAILED       -3



/**********************
 *      TYPEDEFS
 **********************/

/**
 * Type used to represent keystrokes
 */
typedef uint64_t fbkb_key_t;

/**********************
 *      FLAGS
 **********************/

/**
 * Non-blocking input mode flag
 */
#define FBKB_FL_KB_NONBLOCK (1 << 0)

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the framebuffer keyboard
 */
void fbkb_init(void);

/**
 * Deinitialize the framebuffer keyboard
 */
void fbkb_deinit(void);

/**
 * Read keyboard input for LVGL
 * @param indev_drv pointer to the related input device driver
 * @param data store the read data here
 * @return false: no more data to be read
 */
bool fbkb_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);


/**********************
 *      MACROS
 **********************/

#endif /*USE_FBKB*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*FBKB_H*/
