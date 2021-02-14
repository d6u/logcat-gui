#include "renderer.h"

#include <boost/algorithm/string/trim.hpp>
#include <folly/json.h>
#include <iostream>
#include <istream>
#include <ncurses.h>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace folly;

// "[ 02-14 21:50:45.526  1918:26780 D/AudioManager ]\n"
//
// "Sleep timeout exceeded. Timeout:1057059.168695, Now:1057059.168700.
// Sleeping... \n"
const regex kHeaderRegex(
    "\\[ (\\d\\d-\\d\\d) (\\d\\d:\\d\\d:\\d\\d\\.\\d\\d\\d) +(\\d+): *(\\d+) (\\w)/(.+?) +\\]\\s*");

// "--------- beginning of main\n"
const regex kDividerPrefix("--------- beginning of[\\S\\s]*");

// void rtrim(std::string &s) {
//   s.erase(std::find_if(s.rbegin(), s.rend(),
//                        [](unsigned char ch) { return !std::isspace(ch); })
//               .base(),
//           s.end());
// }

const unordered_map<string, int> kLevelRowMap = {
    {"V", 0}, {"D", 1}, {"I", 2}, {"W", 3}, {"E", 4}, {"F", 5},
};

bool Compare(pair<string, int> &a, pair<string, int> &b) {
  return a.second > b.second;
}

string ConcatLines(vector<string> lines) {
  string log;
  for (size_t i = 0; i < lines.size(); i++) {
    if (i < lines.size() - 2) {
      log += lines[i];
      log += "\n";
    } else if (i == lines.size() - 2) {
      log += lines[i];
    } else {
      if (!lines[i].empty()) {
        log += "\n";
        log += lines[i];
      }
    }
  }
  return log;
}

void SetTagColor(WINDOW *win, string level) {
  if (level == "V") {
    wattrset(win, COLOR_PAIR(1));
  } else if (level == "D") {
    wattrset(win, COLOR_PAIR(3));
  } else if (level == "I") {
    wattrset(win, COLOR_PAIR(5));
  } else if (level == "W") {
    wattrset(win, COLOR_PAIR(7));
  } else if (level == "E") {
    wattrset(win, COLOR_PAIR(9));
  } else if (level == "F") {
    wattrset(win, COLOR_PAIR(11));
  }
}

void SetLogColor(WINDOW *win, string level) {
  if (level == "V") {
    wattrset(win, COLOR_PAIR(2));
  } else if (level == "D") {
    wattrset(win, COLOR_PAIR(4));
  } else if (level == "I") {
    wattrset(win, COLOR_PAIR(6));
  } else if (level == "W") {
    wattrset(win, COLOR_PAIR(8));
  } else if (level == "E") {
    wattrset(win, COLOR_PAIR(10));
  } else if (level == "F") {
    wattrset(win, COLOR_PAIR(12));
  }
}

Renderer::Renderer() {
  initscr();

  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_WHITE, COLOR_BLACK);
  init_pair(3, COLOR_WHITE, COLOR_BLUE);
  init_pair(4, COLOR_BLUE, COLOR_BLACK);
  init_pair(5, COLOR_WHITE, COLOR_GREEN);
  init_pair(6, COLOR_GREEN, COLOR_BLACK);
  init_pair(7, COLOR_WHITE, COLOR_YELLOW);
  init_pair(8, COLOR_YELLOW, COLOR_BLACK);
  init_pair(9, COLOR_WHITE, COLOR_RED);
  init_pair(10, COLOR_RED, COLOR_BLACK);
  init_pair(11, COLOR_WHITE, COLOR_MAGENTA);
  init_pair(12, COLOR_MAGENTA, COLOR_BLACK);

  cbreak();
  curs_set(0);
  noecho();

  getmaxyx(stdscr, row_, col_);

  win_tag_list_row_ = row_;
  win_tag_list_col_ = min(50, col_ / 2);
  win_log_list_ = newwin(row_, col_ - win_tag_list_col_, 0, 0);
  win_tag_list_ =
      newwin(win_tag_list_row_, win_tag_list_col_, 0, col_ - win_tag_list_col_);

  scrollok(win_log_list_, 1);
}

void Renderer::renderLogLine(char *line_buf) {
  string line(line_buf);
  boost::algorithm::trim(line);

  // Skip divider lines
  if (regex_match(line, kDividerPrefix)) {
    return;
  }

  // Cache non-header lines
  smatch result;
  if (!regex_match(line, result, kHeaderRegex)) {
    lines_.push_back(line);
    return;
  }

  if (!date_.empty()) {

    // Handle terminal window resizing
    // ----------

    int new_row;
    int new_col;
    getmaxyx(stdscr, new_row, new_col);

    if (new_row != row_ || new_col != col_) {
      row_ = new_row;
      col_ = new_col;
      win_tag_list_row_ = row_;
      win_tag_list_col_ = min(50, col_ / 2);
      wresize(win_log_list_, row_, col_ - win_tag_list_col_);
      wresize(win_tag_list_, win_tag_list_row_, win_tag_list_col_);
      mvwin(win_tag_list_, 0, col_ - win_tag_list_col_);
      wclear(win_tag_list_);
    }

    // Print log list
    // ----------

    const string log = ConcatLines(lines_);

    SetTagColor(win_log_list_, level_);
    wprintw(win_log_list_, "%s:%s", tag_.c_str(), level_.c_str());

    SetLogColor(win_log_list_, level_);
    wprintw(win_log_list_, " %s\n", log.c_str());

    wattrset(win_log_list_, COLOR_PAIR(1));
    wrefresh(win_log_list_);

    // Print tag list
    // ----------

    vector<pair<string, int>> sorted;
    for (const auto &it : tag_count_) {
      sorted.push_back(it);
    }

    sort(sorted.begin(), sorted.end(), Compare);

    for (int i = 0; i < sorted.size(); i++) {
      if (i > win_tag_list_row_ - 2) {
        break;
      }
      const auto pair = sorted[i];
      mvwprintw(win_tag_list_, i + 1, 1, "%s : %d\n", pair.first.c_str(),
                pair.second);
    }

    wattrset(win_tag_list_, COLOR_PAIR(1));
    wborder(win_tag_list_, '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(win_tag_list_);
  }

  // Extract metadata for new lines
  // ----------

  date_ = result[1].str();
  timestamp_ = result[2].str();
  pid_ = result[3].str();
  tid_ = result[4].str();
  level_ = result[5].str();
  tag_ = result[6].str();
  lines_.clear();

  if (tag_count_.count(tag_) > 0) {
    tag_count_[tag_]++;
  } else {
    tag_count_[tag_] = 1;
  }
}

void Renderer::stop() {}
