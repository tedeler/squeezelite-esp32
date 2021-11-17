#ifndef SHIMAUDIOSINK_H
#define SHIMAUDIOSINK_H

#include <vector>
#include <iostream>
#include "AudioSink.h"
#include "FileHelper.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "cspot_sink.h"

typedef bool (*cspot_cmd_cb_t)(cspot_event_t cmd, ...);

class ShimAudioSink : public AudioSink {
public:
	ShimAudioSink(cspot_cmd_cb_t cmd, cspot_data_cb_t data) : cHandler(cmd), dHandler(data) { softwareVolumeControl = false; }
    void feedPCMFrames(std::vector<uint8_t> &data);
	virtual void volumeChanged(uint16_t volume);
	void Play();
	void Pause();
	void Stop();
protected:
private:
	cspot_cmd_cb_t cHandler;
	cspot_data_cb_t dHandler;
};

class NVSFile : public FileHelper {
public:
    bool readFile(std::string filename, std::string &fileContent);
    bool writeFile(std::string filename, std::string fileContent);
};

#endif