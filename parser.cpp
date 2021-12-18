#include "parser.hpp"

#include <iomanip>
#include <iostream>

namespace lrone {

std::vector<unsigned int>
StringToTerminals(const std::string &terminalsLine, const Grammar &grammar) {
  PROFILE_FUNC;
  std::vector<unsigned int> inputTerminals;
  // extract terminals separated by space
  auto start = terminalsLine.begin();
  auto end = start;
  while (end != terminalsLine.end()) {
    end = std::find(start, terminalsLine.end(), ' ');
    auto name = std::string(start, end);

    auto terminal =
        std::find(grammar.terminals.begin(), grammar.terminals.end(), name);
    if (terminal == grammar.terminals.end()) {
      std::cerr << ANSI_COLOR_RED
                << "Unknown terminal in input: " << ANSI_COLOR_RESET << name
                << std::endl;
      std::exit(EXIT_FAILURE);
    } else {
      inputTerminals.push_back(terminal - grammar.terminals.begin());
    }

    start = end + 1;
  }

  inputTerminals.push_back(0); // $
  return inputTerminals;
}

LRParser::LRParser(LRTable &table, Grammar &grammar) {
  this->table = &table;
  this->grammar = &grammar;
}

struct LRParserState {
  std::vector<unsigned int> stateStack;
  std::vector<Symbol> symbolStack;
  std::vector<unsigned int>::const_iterator inputPosition;

  void Display(const Grammar &grammar, const std::vector<unsigned int> &input) {
    int col = 0;
    for (auto n : this->stateStack) {
      std::cout << n << " ";
    }
    col += parsing_col_size;
    std::cout << "\x1b[" << col << "G";

    for (auto n : this->symbolStack) {
      n.Display(grammar);
      std::cout << " ";
    }
    col += parsing_col_size;
    std::cout << "\x1b[" << col << "G";

    // std::cout << "Input: ";
    for (auto it = this->inputPosition; it != input.end(); ++it) {
      std::cout << grammar.terminals[*it] << " ";
    }
    col += parsing_col_size;
    std::cout << "\x1b[" << col << "G";
  }
};

void LRParser::Parse(const std::vector<unsigned int> &input) {
  PROFILE_FUNC;
  LRParserState state;
  state.stateStack.push_back(0);
  state.inputPosition = input.begin();

  if (!benchmark_mode) {
    std::cout << std::setw(parsing_col_size) << "Stack"
              << std::setw(parsing_col_size) << "Current symbols"
              << std::setw(parsing_col_size) << "Remaining input"
              << std::setw(parsing_col_size) << "Next Action" << std::endl;
  }

  while (true) {
    if (!benchmark_mode) {
      state.Display(*this->grammar, input);
    }
    auto lrstate = *(state.stateStack.end() - 1);
    auto action = this->table->actions[lrstate][*state.inputPosition];
    switch (action.type) {
    case LRAction::Type::Shift: {
      if (!benchmark_mode) {
        std::cout << ANSI_COLOR_YELLOW << "Shifting to " << action.num
                  << ANSI_COLOR_RESET << std::endl;
      }

      // go to new state
      state.stateStack.push_back(action.num);

      // push new terminal
      state.symbolStack.push_back(Symbol{
          .type = Symbol::Type::Terminal,
          .id = *state.inputPosition,
      });

      // go to next input terminal
      ++state.inputPosition;
    } break;

    case LRAction::Type::Reduce: {
      if (!benchmark_mode) {
        std::cout << ANSI_COLOR_CYAN << "Reducing by " << action.num
                  << ANSI_COLOR_RESET << std::endl;
      }

      auto rule = this->grammar->rules[action.num];

      // remove each RHS symbol
      for (int i = rule.second.size(); i != 0; --i) {
        state.symbolStack.pop_back();
        state.stateStack.pop_back();
      }

      // put non-terminal from LHS
      state.symbolStack.push_back(Symbol{
          .type = Symbol::Type::NonTerminal,
          .id = rule.first,
      });

      // get the new current state
      lrstate = *(state.stateStack.end() - 1);

      // go to new state according to non-terminal
      state.stateStack.push_back(table->goTo[lrstate][rule.first]);
    } break;

    case LRAction::Type::Accept: {
      if (!benchmark_mode) {
        std::cout << ANSI_COLOR_GREEN << "Input accepted!" << ANSI_COLOR_RESET
                  << std::endl;
      }
      return;
    }

    case LRAction::Type::Error: {
      std::cout << ANSI_COLOR_RED << "Error: Found terminal "
                << ANSI_COLOR_MAGENTA
                << grammar->terminals[*state.inputPosition] << ANSI_COLOR_RED
                << " expected one of ";
      for (unsigned int t = 0; t < this->table->actions[lrstate].size(); ++t) {
        if (this->table->actions[lrstate][t].type != LRAction::Type::Error) {
          std::cout << ANSI_COLOR_MAGENTA << grammar->terminals[t]
                    << ANSI_COLOR_RED << ' ';
        }
      }
      std::cout << ANSI_COLOR_RESET << std::endl;
      return;
    }
    }
  }
}

} // namespace lrone
