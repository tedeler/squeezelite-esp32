#ifndef AUDIOSINK_H
#define AUDIOSINK_H

#include <vector>

class AudioSink
{
public:
    AudioSink() {}
    virtual ~AudioSink() {}
    virtual void feedPCMFrames(std::vector<uint8_t> &data) = 0;
    virtual void volumeChanged(uint16_t volume) {}
	virtual void Play() {}
	virtual void Pause() {}
	virtual void Stop() {}
    bool softwareVolumeControl = true;
    bool usign = false;
};

#endif
