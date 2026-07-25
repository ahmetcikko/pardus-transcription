#include "recorder.h"
#include <cmath>
#include <cstddef>

static constexpr int SAMPLE_RATE = 16000;
static constexpr std::size_t MAX_SAMPLES = (std::size_t)SAMPLE_RATE * 1800;

Recorder::Recorder()
    : m_level(0.0f), m_running(false), m_done(false), m_ready(false) {}
Recorder::~Recorder() { stop(); }
void Recorder::on_full(std::function<void()> callback) {
    m_full = std::move(callback);
}
bool Recorder::start() {
    if (ma_context_init(nullptr, 0, nullptr, &m_context) != MA_SUCCESS)
        return false;
    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.format = ma_format_f32;
    config.capture.channels = 1;
    config.sampleRate = SAMPLE_RATE;
    config.dataCallback = data_callback;
    config.pUserData = this;
    if (ma_device_init(&m_context, &config, &m_device) != MA_SUCCESS) {
        ma_context_uninit(&m_context);
        return false;
    }
    m_running.store(true);
    if (ma_device_start(&m_device) != MA_SUCCESS) {
        ma_device_uninit(&m_device);
        ma_context_uninit(&m_context);
        m_running.store(false);
        return false;
    }
    m_ready = true;
    return true;
}
void Recorder::pause() {
    if (!m_ready)
        return;
    m_running.store(false);
    m_level.store(0.0f);
    ma_device_stop(&m_device);
}
bool Recorder::resume() {
    if (!m_ready)
        return false;
    if (ma_device_start(&m_device) != MA_SUCCESS)
        return false;
    m_running.store(true);
    return true;
}
void Recorder::stop() {
    if (!m_ready)
        return;
    m_ready = false;
    m_running.store(false);
    ma_device_uninit(&m_device);
    ma_context_uninit(&m_context);
}
void Recorder::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_samples.clear();
    m_done.store(false);
}
float Recorder::level() const { return m_level.load(); }
std::vector<float> Recorder::snapshot() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_samples;
}
void Recorder::data_callback(ma_device *device, void *, const void *input,
                             ma_uint32 frames) {
    Recorder *self = static_cast<Recorder *>((*device).pUserData);
    (*self).feed(static_cast<const float *>(input), frames);
}
void Recorder::feed(const float *samples, ma_uint32 frames) {
    if (!m_running.load())
        return;
    double sum = 0.0;
    for (ma_uint32 i = 0; i < frames; i++)
        sum += (double)samples[i] * samples[i];
    m_level.store(frames ? (float)std::sqrt(sum / frames) : 0.0f);
    bool full = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_samples.insert(m_samples.end(), samples, samples + frames);
        full = m_samples.size() >= MAX_SAMPLES;
    }
    if (full && !m_done.exchange(true)) {
        m_running.store(false);
        if (m_full)
            m_full();
    }
}
