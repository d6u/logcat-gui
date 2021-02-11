#include "renderer.h"

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

const regex kHeaderRegex(
    "\\[ (\\d\\d-\\d\\d) (\\d\\d:\\d\\d:\\d\\d\\.\\d\\d\\d) +(\\d+): *(\\d+) (\\w)/(.+?) +\\][\\S\\s]*");
const string kDividerPrefix("--------- beginning of");

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

void Renderer::renderLogLine(string line) {
  if (line.rfind(kDividerPrefix, 0) == 0) {
    return;
  }

  smatch result;

  if (regex_match(line, result, kHeaderRegex)) {
    if (!date_.empty()) {
      const string log = ConcatLines(lines_);

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

      //     if (blocklist_tags.count(tag) == 0) {
      SetTagColor(win_log_list_, level_);
      wprintw(win_log_list_, "%s:%s", tag_.c_str(), level_.c_str());

      SetLogColor(win_log_list_, level_);
      wprintw(win_log_list_, " %s\n", log.c_str());

      wattrset(win_log_list_, COLOR_PAIR(1));
      wrefresh(win_log_list_);
      //     }

      // Print tag list

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
        // if (blocklist_tags_.count(pair.first) == 0) {
        mvwprintw(win_tag_list_, i + 1, 1, "%s : %d\n", pair.first.c_str(),
                  pair.second);
        // } else {
        //   mvwprintw(win_tag_list_, i + 1, 1, "- %s : %d\n",
        //   pair.first.c_str(),
        //             pair.second);
        // }
      }

      wattrset(win_tag_list_, COLOR_PAIR(1));
      wborder(win_tag_list_, '|', '|', '-', '-', '+', '+', '+', '+');
      wrefresh(win_tag_list_);
    }

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
  } else {
    lines_.push_back(line);
  }
}

void Renderer::stop() {}
