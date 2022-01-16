#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2s.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "adac.h"
#include "math.h"
#include "cJSON.h"
#include "accessors.h"

#define ADDR_CSC2316 0x44

static const char TAG[] = "eBird";

static void speaker(bool active) { }
static void headset(bool active) { }
static bool volume(unsigned left, unsigned right);
static void power(adac_power_e mode) {};
static bool init(char *config, int i2c_port_num, i2s_config_t *i2s_config);

const struct adac_s dac_eBird = {"eBird", init, adac_deinit, power, speaker, headset, volume};

/****************************************************************************************
 * init
 */

void writeByteWithNoData(int i2c_addr,uint8_t reg) {
    uint8_t dummy = 0x00;
    ESP_LOGD(TAG, "Write no data to i2c_addr=0x%02X reg=0x%02X", i2c_addr, reg);
    adac_write(i2c_addr, reg, &dummy, 0);
}

void initSoundSwitch() {
    uint8_t chipaddr = ADDR_CSC2316;

    writeByteWithNoData(chipaddr, 0x58);    //
    writeByteWithNoData(chipaddr, 0xC0);    //
    writeByteWithNoData(chipaddr, 0xE0);    //
    writeByteWithNoData(chipaddr, 0x10);    //
    writeByteWithNoData(chipaddr, 0x9F);    //
    writeByteWithNoData(chipaddr, 0xBF);    //
    writeByteWithNoData(chipaddr, 0x3F);    //
    writeByteWithNoData(chipaddr, 0x5D);    //
    writeByteWithNoData(chipaddr, 0x5D);    //
    writeByteWithNoData(chipaddr, 0x3F);    //
    writeByteWithNoData(chipaddr, 0x80);    //
    writeByteWithNoData(chipaddr, 0xA0);    //
    writeByteWithNoData(chipaddr, 0x3F);    //
}

static bool init(char *config, int i2c_port, i2s_config_t *i2s_config) {	 
	/* find if there is a tas5713 attached. Reg 0 should read non-zero but not 255 if so */
  	const i2c_config_t * i2c_config = config_i2c_get(&i2c_port);
	adac_init(config, i2c_port);

    if (adac_read_byte(0x5A, 0x10) != 0x23) {
        ESP_LOGW(TAG, "No eBird detected");
        adac_deinit();
        return 0;
    }

    ESP_LOGI(TAG, "eBird found!");

    initSoundSwitch();
	return true;
}

/****************************************************************************************
 * change volume
 */
float mapToDecibel(unsigned value) {
    float v = 7*(log( value ) / log( 2 ) - 16);
    if (v < -78.75)
        v = -78.75;
    return v;
}

#include "squeezelite.h"
extern struct outputstate output;
    
static bool volume(unsigned left, unsigned right) { 
    uint32_t mean32 = (left + right) / 2;
    float dbValue = mapToDecibel(mean32);
    uint8_t vol8 =  (-dbValue/1.25);

    ESP_LOGI(TAG, "Volume request: %d, %d => %.3fdB -> 0x%02X = %d", left, right, dbValue, vol8, vol8);

    writeByteWithNoData(ADDR_CSC2316, vol8);

    output.gainL = 65536;
    output.gainR = 65536;
   
	return true; 
}
