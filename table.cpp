#include "table.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <utility>

namespace lrone {

bool LRItem::operator==(const LRItem &rhs) const {
  return (this->dotPosition == rhs.dotPosition) &&
         (this->endTerminal == rhs.endTerminal) && (this->ruleID == rhs.ruleID);
}

void LRItem::Display(const Grammar &grammar) const {
  auto rule = grammar.rules[this->ruleID];

  std::cout << grammar.nonTerminals[rule.first] << " → ";

  unsigned int i = 0;
  for (auto &symbol : rule.second) {
    if (i++ == this->dotPosition) {
      std::cout << "• ";
    }
    switch (symbol.type) {
    case Symbol::Type::Terminal:
      std::cout << ANSI_COLOR_MAGENTA << grammar.terminals[symbol.id] << ' '
                << ANSI_COLOR_RESET;
      break;
    case Symbol::Type::NonTerminal:
      std::cout << ANSI_COLOR_CYAN << grammar.nonTerminals[symbol.id] << ' '
                << ANSI_COLOR_RESET;
      break;
    }
  }
  if (i == this->dotPosition) {
    std::cout << "• ";
  }

  std::cout << ", " << grammar.terminals[this->endTerminal] << std::endl;
}

Symbol LRItem::GetNextSymbol(const Grammar &grammar) const {
  auto rhs = grammar.rules[this->ruleID].second;
  if (rhs.size() <= this->dotPosition) {
    // return the end terminal to represent no more symbols available
    return {.type = Symbol::Type::Terminal, .id = 0};
  }

  return rhs[this->dotPosition];
}

bool LRAction::operator==(const LRAction &rhs) const {
  return this->type == rhs.type && this->num == rhs.num;
}

void Closure(std::vector<LRItem> &itemSet, const Grammar &grammar) {
  PROFILE_FUNC;
  for (unsigned int item_it = 0; item_it < itemSet.size(); ++item_it) {
    const auto item = itemSet[item_it];

    const auto next = item.GetNextSymbol(grammar);
    if (next.id == 0) { // we have reached the end
      continue;
    }

    if (next.type == Symbol::Type::NonTerminal) {
      unsigned int i = 0;

      const auto &itemrhs = grammar.rules[item.ruleID].second;

      for (const auto &rule : grammar.rules) {
        if (rule.first == next.id) {
          auto first = grammar.First(
              itemrhs.begin() + item.dotPosition + 1, itemrhs.end());
          if (first.empty() ||
              std::find(first.begin(), first.end(), 0) != first.end()) {
            first.push_back(item.endTerminal);
          }

          for (auto end_term : first) {
            LRItem newitem = {
                .ruleID = i,
                .dotPosition = 0,
                .endTerminal = end_term,
            };
            if (std::find(itemSet.begin(), itemSet.end(), newitem) ==
                itemSet.end()) {
              itemSet.push_back(newitem);
            }
          }
        }
        i++;
      }
    }
  }
}

void LRTable::Display(const Grammar &grammar) {
  std::cout << "   ";
  for (const auto &terminal : grammar.terminals) {
    std::cout << " │ " << std::setw(3) << terminal.substr(0, 3);
  }

  for (auto nt = grammar.nonTerminals.begin() + 1;
       nt != grammar.nonTerminals.end(); ++nt) {
    std::cout << " │ " << std::setw(2) << nt->substr(0, 2);
  }
  std::cout << std::endl;

  unsigned int i = 0;
  for (unsigned int row = 0; row < this->actions.size(); ++row) {
    std::cout << std::setw(3) << std::right << i++ << std::left;
    for (const auto &column : this->actions[row]) {
      std::cout << " │ ";

      switch (column.type) {
      case LRAction::Type::Error:
        std::cout << ANSI_COLOR_RED << 'E' << std::setw(2) << ANSI_COLOR_RESET
                  << "  ";
        break;
      case LRAction::Type::Shift:
        std::cout << ANSI_COLOR_YELLOW << 'S' << std::setw(2) << column.num
                  << ANSI_COLOR_RESET;
        break;
      case LRAction::Type::Reduce:
        std::cout << ANSI_COLOR_CYAN << 'R' << std::setw(2) << column.num
                  << ANSI_COLOR_RESET;
        break;
      case LRAction::Type::Accept:
        std::cout << ANSI_COLOR_GREEN << 'A' << std::setw(2) << "  "
                  << ANSI_COLOR_RESET;
      }
    }

    for (unsigned int col = 1; col < grammar.nonTerminals.size(); ++col) {
      unsigned int val = this->goTo[row][col];
      if (val == 0) {
        std::cout << " │   ";
      } else {
        std::cout << " │ " << std::right << std::setw(2) << this->goTo[row][col]
                  << std::left;
      }
    }

    std::cout << std::endl;
  }
}

void LRTable::WriteCSV(const char *filename, const Grammar &grammar) {
  PROFILE_FUNC;
  auto file = std::ofstream(filename);
  file << "State,";
  for (const auto &terminal : grammar.terminals) {
    file << '"' << terminal << "\", ";
  }
  file << std::endl;
  unsigned int i = 0;
  for (const auto &row : this->actions) {
    file << i << ", ";
    for (const auto &col : row) {
      file << '"';
      switch (col.type) {
      case LRAction::Type::Error:
        file << 'E';
        break;
      case LRAction::Type::Shift:
        file << 'S' << col.num;
        break;
      case LRAction::Type::Reduce:
        file << 'R' << col.num;
        break;
      case LRAction::Type::Accept:
        file << 'A';
        break;
      default:
        file << 'U';
      }
      file << "\", ";
    }

    for (const auto &col : this->goTo[i++]) {
      file << '"';
      if (col != 0) {
        file << col;
      }
      file << "\", ";
    }
    file << std::endl;
  }
}

LRTable GenerateTable(const Grammar &grammar) {
  PROFILE_FUNC;
  LRTable table;
  std::vector<std::vector<LRItem>> itemSets;
  std::vector<std::pair<unsigned int, Symbol>> backtrack;

  if (grammar.rules.size() == 0) {
    std::cerr << "No rules found in grammar" << std::endl;
  }

  // state 0
  itemSets.push_back({{.ruleID = 0, .dotPosition = 0, .endTerminal = 0}});
  Closure(itemSets[0], grammar);
  if (!benchmark_mode) {
    std::cout << "I0:" << std::endl;
    for (const auto &item : itemSets[0]) {
      item.Display(grammar);
    }
  }

  // this value is never used and only kept for correct offset
  backtrack.push_back({0, {}});

  // calculate next states
  for (unsigned int setid = 0; setid < itemSets.size(); ++setid) {
    PROFILE_SCOPE("Item Set");
    // copy to use as itemSets array maybe resized invalidating references
    auto set = itemSets[setid];

    // add new row for actions
    table.actions.push_back(std::vector<LRAction>(
        grammar.terminals.size(), {LRAction::Type::Error, 0}));

    // add new row for goTo
    table.goTo.push_back(
        std::vector<unsigned int>(grammar.nonTerminals.size(), 0));

    // handle reduce
    for (const auto &item : set) {
      auto next = item.GetNextSymbol(grammar);
      if (next.type == Symbol::Type::Terminal && next.id == 0) {
        if (table.actions[setid][item.endTerminal].type ==
            LRAction::Type::Error) {
          if (item.ruleID == 0) {
            table.actions[setid][item.endTerminal] = {
                .type = LRAction::Type::Accept, .num = item.ruleID};
          } else {
            table.actions[setid][item.endTerminal] = {
                .type = LRAction::Type::Reduce, .num = item.ruleID};
          }
        } else {
          switch (table.actions[setid][item.endTerminal].type) {
          case LRAction::Type::Shift:
            std::cout << ANSI_COLOR_RED
                      << "Shift-Reduce conflict after reading (RTL):"
                      << std::endl
                      << ANSI_COLOR_MAGENTA
                      << grammar.terminals[item.endTerminal]
                      << ANSI_COLOR_RESET;
            break;
          case LRAction::Type::Reduce:
            std::cout << ANSI_COLOR_RED
                      << "Reduce-Reduce conflict after reading (RTL):"
                      << std::endl
                      << ANSI_COLOR_MAGENTA
                      << grammar.terminals[item.endTerminal]
                      << ANSI_COLOR_RESET;
            break;
          default:
            break;
          }

          // provide example path on conflict
          for (unsigned int i = setid; i != 0; i = backtrack[i].first) {
            if (backtrack[i].second.id) {
              std::cout << " ← " << i << " ← " << ANSI_COLOR_MAGENTA;
              backtrack[i].second.Display(grammar);
              std::cout << ANSI_COLOR_RESET;
            }
          }
          std::cout << std::endl;
        }
      }
    }

    // handle non-terminal GOTOs
    for (unsigned int nonTerminal = 1;
         nonTerminal < grammar.nonTerminals.size(); ++nonTerminal) {
      std::vector<LRItem> newSet;

      for (const auto &item : set) {
        auto next = item.GetNextSymbol(grammar);
        if (next.type == Symbol::Type::NonTerminal && next.id == nonTerminal) {
          auto newitem = item;
          newitem.dotPosition += 1;
          newSet.push_back(newitem);
        }
      }

      if (newSet.size() == 0)
        continue;

      Closure(newSet, grammar);
      auto targetSet = std::find(itemSets.begin(), itemSets.end(), newSet);

      if (targetSet == itemSets.end()) {
        table.goTo[setid][nonTerminal] = itemSets.size();

        if (!benchmark_mode) {
          std::cout << 'I' << itemSets.size() << ':' << std::endl;
          for (const auto &item : newSet) {
            item.Display(grammar);
          }
        }

        itemSets.push_back(newSet);
        backtrack.push_back({
            setid,
            {.type = Symbol::Type::NonTerminal, .id = nonTerminal},
        });
      } else {
        table.goTo[setid][nonTerminal] = targetSet - itemSets.begin();
      }
    }

    // handle terminal GOTOs
    for (unsigned int terminal = 1; terminal < grammar.terminals.size();
         ++terminal) {
      std::vector<LRItem> newSet;

      for (const auto &item : set) {
        auto next = item.GetNextSymbol(grammar);
        if (next.type == Symbol::Type::Terminal && next.id == terminal) {
          auto newitem = item;
          newitem.dotPosition += 1;
          newSet.push_back(newitem);
        }
      }

      if (newSet.size() == 0)
        continue;

      Closure(newSet, grammar);
      auto targetSet = std::find(itemSets.begin(), itemSets.end(), newSet);
      if (targetSet == itemSets.end()) { // Not found
        if (table.actions[setid][terminal].type == LRAction::Type::Error) {
          table.actions[setid][terminal] = {
              .type = LRAction::Type::Shift, .num = itemSets.size()};
        } else if (
            table.actions[setid][terminal].type == LRAction::Type::Reduce) {
          std::cout << ANSI_COLOR_RED
                    << "Shift-Reduce conflict after reading (RTL):" << std::endl
                    << ANSI_COLOR_MAGENTA << grammar.terminals[terminal]
                    << ANSI_COLOR_RESET;

          // provide example path on conflict
          for (unsigned int i = setid; i != 0; i = backtrack[i].first) {
            if (backtrack[i].second.id) {
              std::cout << " ← " << i << " ← " << ANSI_COLOR_MAGENTA;
              backtrack[i].second.Display(grammar);
              std::cout << ANSI_COLOR_RESET;
            }
          }
          std::cout << std::endl;
        }

        if (!benchmark_mode) {
          std::cout << 'I' << itemSets.size() << ':' << std::endl;
          for (const auto &item : newSet) {
            item.Display(grammar);
          }
        }

        itemSets.push_back(newSet);
        backtrack.push_back({
            setid,
            {.type = Symbol::Type::Terminal, .id = terminal},
        });
      } else {
        LRAction new_action = {
            .type = LRAction::Type::Shift,
            .num = (unsigned int)(targetSet - itemSets.begin()),
        };
        if (table.actions[setid][terminal].type == LRAction::Type::Error) {
          table.actions[setid][terminal] = new_action;
        } else if (
            table.actions[setid][terminal].type == LRAction::Type::Reduce) {
          std::cout << ANSI_COLOR_RED
                    << "Shift-Reduce conflict after reading (RTL):" << std::endl
                    << ANSI_COLOR_MAGENTA << grammar.terminals[terminal]
                    << ANSI_COLOR_RESET;

          // provide example path on conflict
          for (unsigned int i = setid; i != 0; i = backtrack[i].first) {
            if (backtrack[i].second.id) {
              std::cout << " ← " << i << " ← " << ANSI_COLOR_MAGENTA;
              backtrack[i].second.Display(grammar);
              std::cout << ANSI_COLOR_RESET;
            }
          }
          std::cout << std::endl;
        }
      }
    }
  }

  return table;
}

} // namespace lrone
