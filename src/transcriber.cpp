#include "transcriber.h"
#include <algorithm>
#include <thread>
#include <whisper.h>

static whisper_context *g_ctx = nullptr;

bool transcriber_init(const std::string &model_path) {
    if (g_ctx)
        return true;
    whisper_context_params cparams = whisper_context_default_params();
    g_ctx = whisper_init_from_file_with_params(model_path.c_str(), cparams);
    return g_ctx != nullptr;
}
std::string transcribe(const std::vector<float> &samples) {
    if (!g_ctx || samples.size() < 8000)
        return "";
    whisper_full_params params =
        whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);
    params.single_segment = false;
    params.no_context = true;
    params.no_timestamps = true;
    params.suppress_blank = true;
    params.language = "tr";
    int threads = (int)std::thread::hardware_concurrency();
    params.n_threads = threads > 0 ? std::min(threads, 8) : 4;
    if (whisper_full(g_ctx, params, samples.data(), (int)samples.size()) != 0)
        return "";
    std::string out;
    int n = whisper_full_n_segments(g_ctx);
    for (int i = 0; i < n; i++)
        out += whisper_full_get_segment_text(g_ctx, i);
    size_t begin = out.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return "";
    size_t end = out.find_last_not_of(" \t\r\n");
    return out.substr(begin, end - begin + 1);
}
