#include "lrone.hpp"

#include "grammar.hpp"
#include "parser.hpp"
#include "table.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

bool benchmark_mode = false;
unsigned int parsing_col_size = 20;

int main(int argc, char *argv[]) {
  char *grammarFile = NULL;
  char *inputString = NULL;
  char *csvFile = NULL;

  { // argument parsing
    int op;
    while ((op = getopt(argc, argv, "bg:hl:o:p:s:")) != -1) {
      switch (op) {
      case 'b':
        benchmark_mode = true;
        break;
      case 'g':
        grammarFile = optarg;
        break;
      case 'h':
        std::cout << "Usage: " << argv[0] << " [OPTION]" << std::endl;
        std::cout << " -b\t\tBenchmark mode, show timings and disable output"
                  << std::endl;
        std::cout << " -g file\tLoad grammar from file" << std::endl;
        std::cout << " -h\t\tDisplay this information" << std::endl;
        std::cout << " -l\t\tSet column length for parsing result table"
                  << std::endl;
        std::cout << " -o file\tSave parsing table as CSV" << std::endl;
        std::cout << " -p file\tSave profiling data as JSON" << std::endl;
        std::cout << " -s string\tInput String" << std::endl;
        std::exit(0);
        break;
      case 'l':
        parsing_col_size = atoi(optarg);
        break;
      case 'o':
        csvFile = optarg;
        break;
      case 'p':
        benchmark_mode = true;
        lrone::Profiler::Initialize(optarg);
        break;
      case 's':
        inputString = optarg;
        break;
      }
    }
  }

  if (!grammarFile) {
    std::cerr << "Error: No grammar file specified! Try -h for help."
              << std::endl;
    std::exit(EXIT_FAILURE);
  }

  // Load grammar and computer FIRST()
  auto timeStart = std::chrono::system_clock::now();

  auto g = lrone::Grammar::FromFile(grammarFile);
  g.Calculate();

  auto timeEnd = std::chrono::system_clock::now();
  if (benchmark_mode) {
    std::cout << "Grammar loading time: "
              << std::chrono::duration_cast<std::chrono::nanoseconds>(
                     timeEnd - timeStart)
                         .count() /
                     1000.0
              << " us" << std::endl;
  }
  if (!benchmark_mode) {
    g.Display();
  }

  // Build the parsing table
  timeStart = std::chrono::high_resolution_clock::now();

  auto table = lrone::GenerateTable(g);

  timeEnd = std::chrono::system_clock::now();
  if (benchmark_mode) {
    std::cout << "Parsing table building time: "
              << std::chrono::duration_cast<std::chrono::nanoseconds>(
                     timeEnd - timeStart)
                         .count() /
                     1000.0
              << " us" << std::endl;
  }

  if (!benchmark_mode) {
    table.Display(g);
  }

  if (csvFile) {
    table.WriteCSV(csvFile, g);
  }

  // Parse
  if (inputString) {
    timeStart = std::chrono::system_clock::now();

    auto parser = lrone::LRParser(table, g);
    auto input = std::string(inputString);
    parser.Parse(lrone::StringToTerminals(input, g));

    timeEnd = std::chrono::system_clock::now();
    if (benchmark_mode) {
      std::cout << "Parsing time: "
                << std::chrono::duration_cast<std::chrono::nanoseconds>(
                       timeEnd - timeStart)
                           .count() /
                       1000.0
                << " us" << std::endl;
    }
    // std::cout << std::endl;
  }

  lrone::Profiler::Finalize();
  return 0;
}
