#include <algorithm>
#include <boost/process.hpp>
#include <iostream>
#include <ncurses.h>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace boost::process;

const regex kHeaderRegex(
    "\\[ (\\d\\d-\\d\\d) (\\d\\d:\\d\\d:\\d\\d\\.\\d\\d\\d) +(\\d+): *(\\d+) (\\w)/(.+?) +\\]");
const string kDividerPrefix("--------- beginning of");

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

int main(int argc, char *argv[]) {
  unordered_set<string> blocklist_tags;
  for (int i = 1; i < argc; i++) {
    blocklist_tags.emplace(argv[i]);
  }

  unordered_map<string, int> level_row_map = {
      {"V", 0}, {"D", 1}, {"I", 2}, {"W", 3}, {"E", 4}, {"F", 5},
  };

  unordered_map<string, int> tag_count;

  string date;
  string timestamp;
  string pid;
  string tid;
  string level;
  string tag;
  vector<string> lines;

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

  int row;
  int col;
  getmaxyx(stdscr, row, col);

  int win_tag_list_row = row;
  int win_tag_list_col = min(50, col / 2);
  WINDOW *win_log_list = newwin(row, col - win_tag_list_col, 0, 0);
  WINDOW *win_tag_list =
      newwin(win_tag_list_row, win_tag_list_col, 0, col - win_tag_list_col);

  scrollok(win_log_list, 1);

  ipstream logcat_stream;
  child c("adb logcat -b all -v long -T 100", std_out > logcat_stream);

  for (string line; getline(logcat_stream, line);) {
    if (line.rfind(kDividerPrefix, 0) == 0) {
      continue;
    }

    smatch result;

    if (regex_match(line, result, kHeaderRegex)) {
      if (!date.empty()) {
        const string log = ConcatLines(lines);

        int new_row;
        int new_col;
        getmaxyx(stdscr, new_row, new_col);

        if (new_row != row || new_col != col) {
          row = new_row;
          col = new_col;
          win_tag_list_row = row;
          win_tag_list_col = min(50, col / 2);
          wresize(win_log_list, row, col - win_tag_list_col);
          wresize(win_tag_list, win_tag_list_row, win_tag_list_col);
          mvwin(win_tag_list, 0, col - win_tag_list_col);
          wclear(win_tag_list);
        }

        // Print log list

        if (blocklist_tags.count(tag) == 0) {
          SetTagColor(win_log_list, level);
          wprintw(win_log_list, "%s:%s", tag.c_str(), level.c_str());

          SetLogColor(win_log_list, level);
          wprintw(win_log_list, " %s\n", log.c_str());

          wattrset(win_log_list, COLOR_PAIR(1));
          wrefresh(win_log_list);
        }

        // Print tag list

        vector<pair<string, int>> sorted;
        for (const auto &it : tag_count) {
          sorted.push_back(it);
        }

        sort(sorted.begin(), sorted.end(), Compare);

        for (int i = 0; i < sorted.size(); i++) {
          if (i > win_tag_list_row - 2) {
            break;
          }
          const auto pair = sorted[i];
          if (blocklist_tags.count(pair.first) == 0) {
            mvwprintw(win_tag_list, i + 1, 1, "%s : %d\n", pair.first.c_str(),
                      pair.second);
          } else {
            mvwprintw(win_tag_list, i + 1, 1, "- %s : %d\n", pair.first.c_str(),
                      pair.second);
          }
        }

        wattrset(win_tag_list, COLOR_PAIR(1));
        wborder(win_tag_list, '|', '|', '-', '-', '+', '+', '+', '+');
        wrefresh(win_tag_list);
      }

      date = result[1].str();
      timestamp = result[2].str();
      pid = result[3].str();
      tid = result[4].str();
      level = result[5].str();
      tag = result[6].str();
      lines.clear();

      if (tag_count.count(tag) > 0) {
        tag_count[tag]++;
      } else {
        tag_count[tag] = 1;
      }
    } else {
      lines.push_back(line);
    }
  }

  endwin();

  return 0;
}