#include "denoise.h"
#include <webrtc-audio-processing-1/modules/audio_processing/include/audio_processing.h>

void denoise(std::vector<float> &samples) {
    static webrtc::AudioProcessing *apm = [] {
        webrtc::AudioProcessing *instance =
            webrtc::AudioProcessingBuilder().Create();
        webrtc::AudioProcessing::Config cfg;
        cfg.noise_suppression.enabled = true;
        cfg.noise_suppression.level =
            webrtc::AudioProcessing::Config::NoiseSuppression::kModerate;
        cfg.gain_controller2.enabled = true;
        cfg.gain_controller2.adaptive_digital.enabled = true;
        cfg.gain_controller2.fixed_digital.gain_db = 3.0f;
        (*instance).ApplyConfig(cfg);
        return instance;
    }();
    webrtc::StreamConfig stream(16000, 1);
    const int frame = stream.sample_rate_hz() / 100;
    (*apm).Initialize();
    for (int offset = 0; offset + frame <= (int)samples.size();
         offset += frame) {
        float *ptr = samples.data() + offset;
        (*apm).ProcessStream(&ptr, stream, stream, &ptr);
    }
}
