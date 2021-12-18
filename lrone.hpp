#pragma once

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <string>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

extern bool benchmark_mode;
extern bool interactive_mode;
extern unsigned int parsing_col_size;

unsigned int GetTermW();
size_t UTF8Length(std::string s);
void interaction_pause();

namespace lrone {
class Profiler {
public:
  static void Initialize(const char *filename);
  static void Finalize();
  explicit inline Profiler(const char *label) : name(label) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> offsetTime =
        currentTime - startTime;
    file << "{\"pid\": 1, \"ts\": " << offsetTime.count() << ", \"name\": \""
         << name << "\", \"ph\": \"B\"},\n";
  }
  inline ~Profiler() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> offsetTime =
        currentTime - startTime;
    file << "{\"pid\": 1, \"ts\": " << offsetTime.count() << ", \"name\": \""
         << name << "\", \"ph\": \"E\"},\n";
  }

private:
  const char *name;
  static bool enabled;
  static std::ofstream file;
  static std::chrono::high_resolution_clock::time_point startTime;
};
} // namespace lrone

#define PROFILE_SCOPE(label) Profiler _timer(label)
#define PROFILE_FUNC PROFILE_SCOPE(__PRETTY_FUNCTION__)
