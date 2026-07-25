#pragma once
#include "miniaudio.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <vector>

class Recorder {
  public:
    Recorder();
    ~Recorder();
    bool start();
    void pause();
    bool resume();
    void stop();
    void clear();
    float level() const;
    std::vector<float> snapshot();
    void on_full(std::function<void()> callback);

  private:
    static void data_callback(ma_device *device, void *output,
                              const void *input, ma_uint32 frames);
    void feed(const float *samples, ma_uint32 frames);
    ma_context m_context;
    ma_device m_device;
    std::vector<float> m_samples;
    std::mutex m_mutex;
    std::function<void()> m_full;
    std::atomic<float> m_level;
    std::atomic<bool> m_running;
    std::atomic<bool> m_done;
    bool m_ready;
};
