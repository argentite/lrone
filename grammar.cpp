#include "grammar.hpp"

#include <fstream>
#include <iomanip>
#include <map>

namespace lrone {

bool Symbol::operator==(const Symbol &rhs) const {
  return (this->id == rhs.id) && (this->type == rhs.type);
}

void Symbol::Display(const Grammar &grammar) const {
  switch (this->type) {
  case Symbol::Type::Terminal:
    std::cout << ANSI_COLOR_MAGENTA << grammar.terminals[this->id]
              << ANSI_COLOR_RESET;
    break;
  case Symbol::Type::NonTerminal:
    std::cout << ANSI_COLOR_CYAN << grammar.nonTerminals[this->id]
              << ANSI_COLOR_RESET;
    break;
  }
}

Grammar::Grammar() { AddTerminal("EOF"); }

Grammar::Grammar(std::istream &grammarFile) {
  PROFILE_FUNC;
  AddTerminal("$");
  std::map<std::string, unsigned long> terminalsMap;
  terminalsMap.insert({"$", 0});

  { // extract terminals separated by space
    std::string terminalsLine;
    std::getline(grammarFile, terminalsLine);

    auto start = terminalsLine.begin();
    auto end = start;
    while (end != terminalsLine.end()) {
      end = std::find(start, terminalsLine.end(), ' ');
      auto name = std::string(start, end);

      if (terminalsMap.contains(name)) {
        std::cerr << ANSI_COLOR_YELLOW
                  << "Warning: Attempted to insert new terminal symbol '"
                  << name << "' when existing terminal with same name exists"
                  << ANSI_COLOR_RESET << std::endl;
      }

      terminalsMap[name] = terminalsMap.size();
      this->AddTerminal(name);

      start = end + 1;
    }
  }

  AddNonTerminal("S'");
  std::map<std::string, unsigned long> nonTerminalsMap;
  nonTerminalsMap.insert({"S'", 0});
  { // extract non-terminals separated by space
    std::string nonTerminalsLine;
    std::getline(grammarFile, nonTerminalsLine);

    auto start = nonTerminalsLine.begin();
    auto end = start;
    while (end != nonTerminalsLine.end()) {
      end = std::find(start, nonTerminalsLine.end(), ' ');
      auto name = std::string(start, end);

      if (terminalsMap.contains(name)) {
        std::cerr << ANSI_COLOR_YELLOW
                  << "Warning: Attempted to insert new non-terminal symbol '"
                  << name << "' when existing terminal with same name exists"
                  << ANSI_COLOR_RESET << std::endl;
      }

      if (nonTerminalsMap.contains(name)) {
        std::cerr << ANSI_COLOR_YELLOW
                  << "Warning: Attempted to insert new non-terminal symbol '"
                  << name
                  << "' when existing non-terminal with same name exists"
                  << ANSI_COLOR_RESET << std::endl;
      }
      nonTerminalsMap[name] = nonTerminalsMap.size();
      this->AddNonTerminal(std::string(start, end));

      start = end + 1;
    }
  }

  AddRule(0, std::vector<Symbol>{{.type = Symbol::Type::NonTerminal, .id = 1}});

  while (!grammarFile.eof()) {
    std::string rule;
    std::getline(grammarFile, rule);
    if (rule.length() == 0)
      continue;

    // find LHS
    unsigned long lhs;
    auto start = rule.begin();
    auto end = std::find(start, rule.end(), ' ');
    auto nt = std::string(start, end);
    if (nonTerminalsMap.contains(nt)) {
      lhs = nonTerminalsMap[nt];
    } else {
      std::cerr << ANSI_COLOR_RED << "Unknown non-terminal '" << nt
                << "', ignoring rule '" << rule << ANSI_COLOR_RESET
                << std::endl;
      continue;
    }

    // find RHS
    std::vector<Symbol> rhs;
    start = end + 1;
    while (end != rule.end()) {
      end = std::find(start, rule.end(), ' ');
      auto item = std::string(start, end);
      if (terminalsMap.contains(item)) {
        rhs.push_back(
            {.type = Symbol::Type::Terminal, .id = terminalsMap[item]});
      } else if (nonTerminalsMap.contains(item)) {
        rhs.push_back(
            {.type = Symbol::Type::NonTerminal, .id = nonTerminalsMap[item]});
      } else {
        std::cerr << ANSI_COLOR_RED << "Unknown symbol '" << item
                  << "', ignoring" << ANSI_COLOR_RESET << std::endl;
      }

      start = end + 1;
    }

    this->AddRule(lhs, rhs);
  }
}

Grammar Grammar::FromFile(const std::string &filename) {
  std::ifstream grammarFile(filename);
  if (!grammarFile.is_open()) {
    std::cerr << ANSI_COLOR_RED << "Failed to open Grammar file: " << filename
              << ANSI_COLOR_RESET << std::endl;
    std::exit(EXIT_FAILURE);
  }
  std::cout << ANSI_COLOR_GREEN << "Loading grammar from file: " << filename
            << ANSI_COLOR_RESET << std::endl;

  return Grammar(grammarFile);
}

void Grammar::AddTerminal(const std::string &name) {
  terminals.push_back(name);
}

void Grammar::AddNonTerminal(const std::string &name) {
  nonTerminals.push_back(name);
  first.push_back({});
}

static unsigned int first_nt_recursion_depth = 0;

std::vector<unsigned int> Grammar::FirstNonTerminal(unsigned int nt) const {
  PROFILE_FUNC;
  if (!this->first[nt].empty())
    return this->first[nt];

  first_nt_recursion_depth += 1;

  if (first_nt_recursion_depth > 100) {
    std::cerr << "Infinite recursion detected while determining FIRST()"
              << std::endl;
    std::exit(EXIT_FAILURE);
  }

  std::vector<unsigned int> result;

  for (const auto &rule : this->rules) {
    if (rule.first != nt)
      continue;

    if (rule.second.size() == 0) { // null production
      result.push_back(0);         // null terminal
      continue;
    }

    for (const auto &symbol : rule.second) {
      switch (symbol.type) {
      case Symbol::Type::Terminal:
        // nothing following a terminal can be in first
        result.push_back(symbol.id);
        goto donerule;
      case Symbol::Type::NonTerminal:
        if (symbol.id == nt) { // same production recursion
          goto donerule;       // ignore
        }
        auto start = this->FirstNonTerminal(symbol.id);
        auto null_index = std::find(start.begin(), start.end(), 0);
        if (null_index == start.end()) {
          // null not in start so nothing following can be in start
          result.insert(result.end(), start.begin(), start.end());
          goto donerule;
        } else {
          // null in start so consider next symbols
          start.erase(null_index);
          result.insert(result.end(), start.begin(), start.end());
        }
        break;
      }
    }
  donerule:
    continue;
  }

  // remove duplicates
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());

  first_nt_recursion_depth -= 1;
  return result;
}

std::vector<unsigned int> Grammar::First(
    const std::vector<Symbol>::const_iterator start,
    const std::vector<Symbol>::const_iterator end) const {
  PROFILE_FUNC;
  std::vector<unsigned int> result;
  for (auto it = start; it != end; ++it) {
    // We have some symbols in this round, so clear previous nulls
    auto prev_null_index = std::find(result.begin(), result.end(), 0);
    if (prev_null_index != result.end())
      result.erase(prev_null_index);

    const auto &symbol = *it;
    switch (symbol.type) {
    case Symbol::Type::Terminal:
      // nothing following a terminal can be in first
      result.push_back(symbol.id);
      return result;
    case Symbol::Type::NonTerminal:
      auto start = this->FirstNonTerminal(symbol.id);
      auto null_index = std::find(start.begin(), start.end(), 0);
      if (null_index == start.end()) {
        // null not in start so nothing following can be in start
        result.insert(result.end(), start.begin(), start.end());
        goto done;
      } else {
        // null in start so consider next symbols
        result.insert(result.end(), start.begin(), start.end());
      }
      break;
    }
  }

done:
  // remove duplicates
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());

  return result;
}

void Grammar::AddRule(const unsigned int lhs, const std::vector<Symbol> &rhs) {
  rules.push_back({lhs, rhs});
}

void Grammar::Calculate() {
  PROFILE_FUNC;
  for (unsigned int i = 0; i < this->first.size(); i++) {
    this->first[i] = FirstNonTerminal(i);
  }
}

void Grammar::Display() const {
  // terminals
  std::cout << "Terminals" << std::endl << "═════════" << std::endl;

  int i = 0;
  for (auto &terminal : this->terminals) {
    std::cout << std::right << std::setw(3) << i++ << "│" << std::left
              << terminal << std::endl;
  }
  std::cout << std::endl;

  // Non-terminals
  std::cout << "Non-Terminals (First)" << std::endl
            << "═════════════════════" << std::endl;

  i = 0;
  for (auto &nonterminal : this->nonTerminals) {
    std::cout << std::right << std::setw(3) << i << "│" << std::left
              << nonterminal << "\t\t";

    for (auto terminal : this->FirstNonTerminal(i)) {
      auto t = this->terminals[terminal];
      if (terminal == 0)
        t = "ε";
      std::cout << t << " ";
    }

    std::cout << std::endl;
    i++;
  }
  std::cout << std::endl;

  // rules
  unsigned int lhsmaxlen = 0;
  for (auto &rule : this->rules) {
    auto len = UTF8Length(this->nonTerminals[rule.first]);
    if (len > lhsmaxlen) {
      lhsmaxlen = len;
    }
  }

  std::cout << "Rules" << std::endl << "═════" << std::endl;

  i = 0;
  for (auto &rule : this->rules) {
    std::cout << std::right << std::setw(3) << i++ << "│ "
              << std::setw(lhsmaxlen) << this->nonTerminals[rule.first] << " → "
              << std::left;
    for (auto &symbol : rule.second) {
      symbol.Display(*this);
      std::cout << ' ';
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}
}; // namespace lrone
