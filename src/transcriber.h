#pragma once
#include <string>
#include <vector>

bool transcriber_init(const std::string &model_path);
std::string transcribe(const std::vector<float> &samples);
