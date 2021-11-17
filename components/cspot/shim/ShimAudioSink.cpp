#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <Session.h>
#include <SpircController.h>
#include <MercuryManager.h>
#include <ZeroconfAuthenticator.h>
#include <ApResolve.h>
#include "ConfigJSON.h"
#include "Logger.h"
#include "ShimAudioSink.h"
#include "cspot_sink.h"
#include "platform_config.h"

#define CSPOT_STACK_SIZE (1*4096)

static const char *TAG = "cspot";

struct cspot_s {
	char name[32];
	cspot_cmd_cb_t cHandler;
	cspot_data_cb_t dHandler;
	TaskHandle_t TaskHandle;
};

std::shared_ptr<ConfigJSON> configMan;

static void cspotTask(void *pvParameters) {

	char configName[] = "cspot_config";
	std::string jsonConfig;	
	struct cspot_s *cspot = (struct cspot_s*) pvParameters;
    auto zeroconfAuthenticator = std::make_shared<ZeroconfAuthenticator>();
	
    // Config file
    auto file = std::make_shared<NVSFile>();
	configMan = std::make_shared<ConfigJSON>(configName, file);
   
	// We might have no config at all
	if (!file->readFile(configName, jsonConfig) || !jsonConfig.length()) {
		ESP_LOGW(TAG, "Cannot load config, using default");
		
		configMan->deviceName = cspot->name;
		configMan->format = AudioFormat::OGG_VORBIS_160;
		configMan->volume = 32767;

		configMan->save();	
	}
	
	// safely load config now
	configMan->load();
	if (!configMan->deviceName.length()) configMan->deviceName = cspot->name;
	ESP_LOGI(TAG, "Started CSpot with %s (bitrate %d)", configMan->deviceName.c_str(), configMan->format == AudioFormat::OGG_VORBIS_320 ? 320 : (configMan->format == AudioFormat::OGG_VORBIS_160 ? 160 : 96));

    // Blob file
    std::shared_ptr<LoginBlob> blob;
    std::string jsonData;
    
	// Using NVS is complicated (stack and SPIRAM) and we don't want spiffs, authenticate at every start...
    auto authenticator = std::make_shared<ZeroconfAuthenticator>();
    blob = authenticator->listenForRequests();
	
    auto session = std::make_unique<Session>();
    session->connectWithRandomAp();
    auto token = session->authenticate(blob);

    // Auth successful
    if (token.size() > 0)
    {
        // @TODO Actually store this token somewhere
        auto mercuryManager = std::make_shared<MercuryManager>(std::move(session));
        mercuryManager->startTask();

        auto audioSink = std::make_shared<ShimAudioSink>(cspot->cHandler, cspot->dHandler);
        auto spircController = std::make_shared<SpircController>(mercuryManager, blob->username, audioSink);
		
        mercuryManager->reconnectedCallback = [spircController]() {
            return spircController->subscribe();
        };
        mercuryManager->handleQueue();
    }
	
	// something went wrong
	ESP_LOGE(TAG, "Can't get token, CSpot won't start");
	vTaskDelete(NULL);
}

extern "C" void *cspot_create(const char *name, cspot_cmd_cb_t cmd_cb, cspot_data_cb_t data_cb) {
	static DRAM_ATTR StaticTask_t xTaskBuffer __attribute__ ((aligned (4)));
	static EXT_RAM_ATTR StackType_t xStack[CSPOT_STACK_SIZE] __attribute__ ((aligned (4)));
	static EXT_RAM_ATTR struct cspot_s cspot;
	
	setDefaultLogger();
	
	cspot.cHandler = cmd_cb;
	cspot.dHandler = data_cb;
	strncpy(cspot.name, name, sizeof(cspot.name) - 1);
    cspot.TaskHandle = xTaskCreateStatic(&cspotTask, "cspot", CSPOT_STACK_SIZE, &cspot, ESP_TASK_PRIO_MIN + 1, xStack, &xTaskBuffer);
	
	return (void*) &cspot;
}

/****************************************************************************************
 * AudioSink class to push data to squeezelite backend (decode_external)
 */
void ShimAudioSink::volumeChanged(uint16_t volume) {
	cHandler(CSPOT_SINK_VOLUME, volume);
}

void ShimAudioSink::Play() {
	cHandler(CSPOT_SINK_PLAY, 44100);
}

void ShimAudioSink::Stop() {
	cHandler(CSPOT_SINK_STOP);
}

void ShimAudioSink::Pause() {
	cHandler(CSPOT_SINK_PAUSE);
}

void ShimAudioSink::feedPCMFrames(std::vector<uint8_t> &data) {	
	dHandler(&data[0], data.size());
}

/****************************************************************************************
 * NVSFile class to store config
 */
 bool NVSFile::readFile(std::string filename, std::string &fileContent) {
	char *content = (char*) config_alloc_get(NVS_TYPE_STR, filename.c_str());
	
	if (!content) return false;
	
	fileContent = content;
	free(content);
	return true;
}

bool NVSFile::writeFile(std::string filename, std::string fileContent) {
	return (ESP_OK == config_set_value(NVS_TYPE_STR, filename.c_str(), fileContent.c_str()));
}
