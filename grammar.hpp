#pragma once
#include "lrone.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace lrone {

class Grammar;

struct Symbol {
  enum class Type { Terminal, NonTerminal } type;
  unsigned long id;

  void Display(const Grammar &grammar) const;
  bool operator==(const Symbol &rhs) const;
};

class Grammar {
public:
  typedef std::pair<unsigned long, std::vector<Symbol>> Rule;
  Grammar();
  Grammar(std::istream &grammarFile);
  static Grammar FromFile(const std::string &filename);

  void AddTerminal(const std::string &name);
  void AddNonTerminal(const std::string &name);
  void AddRule(const unsigned int lhs, const std::vector<Symbol> &rhs);

  std::vector<unsigned int> FirstNonTerminal(unsigned int nonterminal) const;
  std::vector<unsigned int> First(
      const std::vector<Symbol>::const_iterator start,
      const std::vector<Symbol>::const_iterator end) const;

  void Calculate();
  void Display() const;

  std::vector<std::string> terminals;
  std::vector<std::string> nonTerminals;
  std::vector<Rule> rules;
  std::vector<std::vector<unsigned int>> first;
};

} // namespace lrone
