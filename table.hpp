#pragma once
#include "lrone.hpp"

#include "grammar.hpp"

#include <vector>

namespace lrone {

struct LRItem {
  unsigned int ruleID;
  unsigned int dotPosition;
  unsigned int endTerminal;

  bool operator==(const LRItem &rhs) const;

  Symbol GetNextSymbol(const Grammar &grammar) const;
  void Display(const Grammar &grammar) const;
};

struct LRAction {
  enum class Type { Error = 0, Shift, Reduce, Accept } type;
  unsigned long num;

  bool operator==(const LRAction &rhs) const;
};

struct LRTable {
  std::vector<std::vector<LRAction>> actions;
  std::vector<std::vector<unsigned int>> goTo;

  void Display(const Grammar &grammar);
  void WriteCSV(const char *filename, const Grammar &grammar);
};

LRTable GenerateTable(const Grammar &grammar);

} // namespace lrone
