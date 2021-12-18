#include "lrone.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sys/ioctl.h>

unsigned int GetTermW() {
  struct winsize w;
  ioctl(0, TIOCGWINSZ, &w);
  return w.ws_col;
}

size_t UTF8Length(std::string s) {
  return std::count_if(s.begin(), s.end(), [](char c) {
    return (static_cast<unsigned char>(c) & 0xC0) != 0x80;
  });
}

void interaction_pause() {
  std::cout << std::endl << "Press any key to continue" << std::endl;
}

std::ofstream lrone::Profiler::file;
bool lrone::Profiler::enabled;
std::chrono::high_resolution_clock::time_point lrone::Profiler::startTime;

void lrone::Profiler::Initialize(const char *filename) {
  enabled = true;
  file.open(filename);
  file << "{\n\"traceEvents\": [\n";
  startTime = std::chrono::high_resolution_clock::now();
}

void lrone::Profiler::Finalize() {
  if (enabled) {
    file << "{}\n]\n}\n";

    file.close();
  }
}
