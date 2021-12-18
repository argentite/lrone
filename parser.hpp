#pragma once

#include "lrone.hpp"

#include "table.hpp"

#include <memory>

namespace lrone {

std::vector<unsigned int>
StringToTerminals(const std::string &terminalsLine, const Grammar &grammar);

class LRParser {
public:
  LRParser(LRTable &table, Grammar &grammar);
  void Parse(const std::vector<unsigned int> &input);

  LRTable *table;
  Grammar *grammar;
};

} // namespace lrone
