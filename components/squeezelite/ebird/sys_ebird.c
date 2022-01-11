#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2s.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "adac.h"
#include "math.h"
#include "cJSON.h"
#include "accessors.h"

#define ADDR_AW9523_A 0x5A
#define ADDR_AW9523_B 0x59
#define ADDR_CSC2316 0x44

static const char TAG[] = "eBird";

static void speaker(bool active) { }
static void headset(bool active) { }
static bool volume(unsigned left, unsigned right);
static void power(adac_power_e mode) {};
static bool init(char *config, int i2c_port_num, i2s_config_t *i2s_config);

const struct adac_s dac_eBird = {"eBird", init, adac_deinit, power, speaker, headset, volume};


#define ACK_CHECK_EN 0x1            /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0           /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                 /*!< I2C ack value */
#define NACK_VAL 0x1                /*!< I2C nack value */

int i2c_system_port;

/****************************************************************************************
 * init
 */

uint8_t readByte(uint8_t chip_addr, uint8_t data_addr) {
    uint8_t data[1];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, chip_addr << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data_addr, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, chip_addr << 1 | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_system_port, cmd, 50 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Reading from 0x%02X - 0x%02X: 0x%02X", chip_addr, data_addr, *data);
        return *data;
    } else {
        ESP_LOGE(TAG, "Error reading from 0x%02X - 0x%02X", chip_addr, data_addr);
        return 0x00;
    }
}

void writeByte1(uint8_t chip_addr, uint8_t data_addr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, chip_addr << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data_addr, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_system_port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Wrote to 0x%02X - 0x%02X", chip_addr, data_addr);
    } else if (ret == ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "Bus is busy");
    } else {
        ESP_LOGW(TAG, "Write Failed to 0x%02X - 0x%02X", chip_addr, data_addr);
    }
}

void writeByte(uint8_t chip_addr, uint8_t data_addr, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, chip_addr << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data_addr, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_system_port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Wrote 0x%02X to 0x%02X - 0x%02X", data, chip_addr, data_addr);
    } else if (ret == ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "Bus is busy");
    } else {
        ESP_LOGW(TAG, "Write Failed");
    }
}


void initLEDDriver() {
    uint8_t chipaddr = ADDR_AW9523_B;

    writeByte(chipaddr, 0x06, 0xFF);   //Disable all Interrupts P0
    writeByte(chipaddr, 0x07, 0xFF);   //Disable all Interrupts P1
    writeByte(chipaddr, 0x11, 0x10);    //P0 is Push-Pull
    writeByte(chipaddr, 0x02 ,0x9F);    //P0 Output 1001 1111
    writeByte(chipaddr, 0x03, 0x5D);    //P1 Output 0101 1101
    writeByte(chipaddr, 0x04, 0xFF);    //P0 Data Direction IIII IIII
    writeByte(chipaddr, 0x05, 0x0C);    //P1 Data Direction OOOO IIOO
    writeByte(chipaddr, 0x12, 0xFE);    //P0 Gpio Led       GGGG GGGL
    writeByte(chipaddr, 0x24, 0x00);    //LED (P0-0) CurrentControl 0
    writeByte(chipaddr, 0x13, 0xF3);    //P1 Gpio Led       GGGG LLGG
    writeByte(chipaddr, 0x22, 0x00);    //LED (P1-2) CurrentControl 0
    writeByte(chipaddr, 0x23, 0x00);    //LED (P1-3) CurrentControl 0
    writeByte(chipaddr, 0x03, 0x5D);    //P1 Output 0101 1110
    writeByte(chipaddr, 0x11, 0x13);    //P0 Push-Pull Dimming Range 0-Imax/4
}

void initButtonDriver() {
    uint8_t chipaddr = ADDR_AW9523_A;
    writeByte(chipaddr, 0x04, 0xFF);    //P0 All Input
    writeByte(chipaddr, 0x05, 0xFF);    //P1 All Input
    writeByte(chipaddr, 0x11, 0x10);    //P0 is Push-Pull
    writeByte(chipaddr, 0x06, 0x50);    //P0 Interrupt Enable E.E. EEEE
    writeByte(chipaddr, 0x07, 0xF7);    //P1 Interrupt Enable .... E...
}

void initSoundSwitch() {
    uint8_t chipaddr = ADDR_CSC2316;

    writeByte1(chipaddr, 0x58);    //
    writeByte1(chipaddr, 0xC0);    //
    writeByte1(chipaddr, 0xE0);    //
    writeByte1(chipaddr, 0x10);    //
    writeByte1(chipaddr, 0x9F);    //
    writeByte1(chipaddr, 0xBF);    //
    writeByte1(chipaddr, 0x3F);    //
    writeByte1(chipaddr, 0x5D);    //
    writeByte1(chipaddr, 0x5D);    //
    writeByte1(chipaddr, 0x3F);    //
    writeByte1(chipaddr, 0x80);    //
    writeByte1(chipaddr, 0xA0);    //
    writeByte1(chipaddr, 0x00);    //
//    writeByte1(chipaddr, 0x16);    //
}


void setLEDValue(uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t chipaddr = 0x59;

    red = ((uint32_t)red) * 0x80 / 0xFF;
    green = ((uint32_t)green) * 0x37 / 0xFF;
    blue = ((uint32_t)blue) * 0x32 / 0xFF;

    writeByte(chipaddr, 0x22, green);    //LED (P1-2) CurrentControl 0
    writeByte(chipaddr, 0x23, blue);    //LED (P1-2) CurrentControl 0
    writeByte(chipaddr, 0x24, red);    //LED (P1-2) CurrentControl 0
}

static bool init(char *config, int i2c_port, i2s_config_t *i2s_config) {	 
	/* find if there is a tas5713 attached. Reg 0 should read non-zero but not 255 if so */
	adac_init(config, i2c_port);

  	const i2c_config_t * i2c_config = config_i2c_get(&i2c_system_port);
	ESP_LOGI(TAG,"Configuring I2C sda:%d scl:%d port:%u speed:%u", i2c_config->sda_io_num, i2c_config->scl_io_num, i2c_system_port, i2c_config->master.clk_speed);

    if (readByte(ADDR_AW9523_A, 0x10) != 0x23) {
        ESP_LOGW(TAG, "No eBird detected");
        adac_deinit();
        return 0;
    }

    ESP_LOGI(TAG, "eBird found");

/*
    initLEDDriver();
    initButtonDriver();
    initSoundSwitch();
    setLEDValue(255, 128, 0);
*/
    initSoundSwitch();
	return true;
}

/****************************************************************************************
 * change volume
 */
float mapToDecibel(unsigned value) {
    float v = 10*(log( value ) / log( 2 )) - 160;
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

    writeByte1(0x44, vol8);

    output.gainL = 65536;
    output.gainR = 65536;
   
	return true; 
}
