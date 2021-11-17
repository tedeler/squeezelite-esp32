/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef __CSPOT_SINK_H__
#define __CSPOT_SINK_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum { 	CSPOT_SINK_SETUP, CSPOT_SINK_PLAY, CSPOT_SINK_PAUSE, CSPOT_SINK_STOP, CSPOT_SINK_VOLUME, CSPOT_SINK_METADATA, CSPOT_SINK_PROGRESS } cspot_event_t;
				
typedef bool (*cspot_cmd_cb_t)(cspot_event_t event, ...);				
typedef bool (*cspot_cmd_vcb_t)(cspot_event_t event, va_list args);
typedef void (*cspot_data_cb_t)(const uint8_t *data, uint32_t len);

/**
 * @brief     init sink mode (need to be provided)
 */
void cspot_sink_init(cspot_cmd_vcb_t cmd_cb, cspot_data_cb_t data_cb);

/**
 * @brief     deinit sink mode (need to be provided)
 */
void cspot_sink_deinit(void);

/**
 * @brief     force disconnection
 */
void cspot_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif /* __CSPOT_APP_SINK_H__*/
