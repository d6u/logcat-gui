#include "renderer.h"

#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <fcntl.h>
#include <folly/Subprocess.h>
#include <folly/json.h>
#include <iostream>
#include <istream>
#include <memory>
#include <ncurses.h>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define TAG_WIN_WIDTH 60
#define DEBUG_WIN_HEIGHT 50
#define READ_BUF_SIZE 200

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

const string kNewline = "\n";

enum class TagFilterType { kInclude, kUnspecified };

// Cannot use const on field, because sort() requires struct to be move
// assignable
struct Tag {
  TagFilterType type;
  string name;
  int count;
};

bool Compare(const Tag &a, const Tag &b) {
  if (a.type == TagFilterType::kInclude && b.type != TagFilterType::kInclude) {
    return true;
  } else if (a.type != TagFilterType::kInclude &&
             b.type == TagFilterType::kInclude) {
    return false;
  } else {
    return a.count > b.count;
  }
}

string ConcatLines(vector<string> lines) {
  string log;
  for (size_t i = 0; i < lines.size(); i++) {
    if (i < lines.size() - 1) {
      log += lines[i] + "\n";
    } else {
      log += lines[i];
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

Renderer::Renderer(bool is_debug) : is_debug_(is_debug) {}

void Renderer::init() {
  // Start curses mode
  initscr();
  clear();

  // Start color
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

  cbreak();   // Disable terminal line buffering, and send signal to the program
  noecho();   // Turn off echoing, i.e. user typed chars won't be displayed
  timeout(0); // Make calling getch non-block
  keypad(stdscr, true); // Recognize F1 key, etc.
  curs_set(0);          // Make cursor invisible

  getmaxyx(stdscr, row_, col_);

  win_log_list_ = newwin(win_log_list_row(), win_log_list_col(), 0, 0);
  win_tag_list_ =
      newwin(win_tag_list_row(), win_tag_list_col(), 0, win_log_list_col());
  if (is_debug_) {
    win_debug_ =
        newwin(win_debug_row(), win_debug_col(), win_log_list_row(), 0);
    scrollok(win_debug_, true);
  }
  scrollok(win_log_list_, true);
}

void Renderer::start(std::shared_ptr<Subprocess> proc, vector<string> filters) {
  int fd = proc->stdoutFd();

  // Config fd to non-blocking
  int flags = fcntl(fd, F_GETFL, 0);
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    return;
  }

  while (true) {
    switch (getch()) {
    case KEY_F(1): // Use F1 as exit
      return;
    case KEY_RESIZE:
      maybeHandleWindowResize();
      break;
    }

    char line_buf[READ_BUF_SIZE];
    ssize_t line_size = read(fd, line_buf, READ_BUF_SIZE);

    switch (line_size) {
    case 0: // EOF
      return;
    case -1:
      if (errno != EAGAIN) { // No data yet
        return;
      }
      renderBorderAndRefresh();
      break;
    default:
      if (is_debug_) {
        wprintw(win_debug_, "\n%d\tcache_line_ = %s", debug_counter_++,
                cache_line_.c_str());
        wprintw(win_debug_, "\n%d\tline_buf = %s", debug_counter_++, line_buf);
      }

      // Must use line_size to construct string, otherwise it will contain
      // buffer from last read.
      string multi_lines = cache_line_ + string(line_buf, line_size);
      size_t pos;

      while ((pos = multi_lines.find(kNewline)) != string::npos) {
        // Don't need to trim newline because we split by newline
        string line = multi_lines.substr(0, pos);
        renderLine(line, filters);
        renderBorderAndRefresh();
        multi_lines.erase(0, pos + kNewline.size());
      }

      cache_line_ = multi_lines;
      break;
    }
  }
}

void Renderer::stop() { endwin(); }

int Renderer::win_log_list_row() {
  return row_ - (is_debug_ ? win_debug_row() : 0);
}

int Renderer::win_log_list_col() { return col_ - win_tag_list_col(); }

int Renderer::win_tag_list_row() { return row_; }

int Renderer::win_tag_list_col() { return min(TAG_WIN_WIDTH, col_ / 2); }

int Renderer::win_debug_row() { return min(DEBUG_WIN_HEIGHT, row_ / 2); }

int Renderer::win_debug_col() { return win_log_list_col(); }

void Renderer::maybeHandleWindowResize() {
  int new_row;
  int new_col;
  getmaxyx(stdscr, new_row, new_col);

  if (new_row == row_ && new_col == col_) {
    return;
  }

  row_ = new_row;
  col_ = new_col;

  wresize(win_log_list_, win_log_list_row(), win_log_list_col());
  wresize(win_tag_list_, win_tag_list_row(), win_tag_list_col());
  mvwin(win_tag_list_, 0, win_log_list_col());
  if (is_debug_) {
    wresize(win_debug_, win_debug_row(), win_debug_col());
    mvwin(win_debug_, win_log_list_row(), 0);
  }
}

void Renderer::renderLine(string line, vector<string> filters) {
  // Skip empty lines
  if (line.empty()) {
    return;
  }

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

    // Print log list
    // ----------

    if (filters.size() == 0 ||
        find(filters.begin(), filters.end(), tag_) != filters.end()) {
      const string log = ConcatLines(lines_);

      SetTagColor(win_log_list_, level_);
      wprintw(win_log_list_, "\n%s:%s", tag_.c_str(), level_.c_str());

      SetLogColor(win_log_list_, level_);
      wprintw(win_log_list_, " %s", log.c_str());

      wattrset(win_log_list_, COLOR_PAIR(1));
      wrefresh(win_log_list_);
    }

    // Print tag list
    // ----------

    vector<Tag> tags;
    for (const auto &it : tag_count_) {
      bool is_top =
          find(filters.begin(), filters.end(), it.first) == filters.end();

      tags.push_back(
          Tag{is_top ? TagFilterType::kUnspecified : TagFilterType::kInclude,
              it.first, it.second});
    }

    sort(tags.begin(), tags.end(), Compare);

    for (int i = 0; i < tags.size(); i++) {
      if (i > win_tag_list_row() - 2) { // Exclude top and bottom border width
        break;
      }
      auto tag = tags[i];
      if (tag.type == TagFilterType::kInclude) {
        mvwprintw(win_tag_list_, i + 1, 1, "+ %s : %d\n", tag.name.c_str(),
                  tag.count);
      } else {
        mvwprintw(win_tag_list_, i + 1, 1, "%s : %d\n", tag.name.c_str(),
                  tag.count);
      }
    }
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

void Renderer::renderBorderAndRefresh() {
  wattrset(win_tag_list_, COLOR_PAIR(1));
  wborder(win_tag_list_, '|', '|', '-', '-', '+', '+', '+', '+');
  wrefresh(win_tag_list_);
  wrefresh(win_log_list_);
  if (is_debug_) {
    wborder(win_debug_, ' ', ' ', '-', ' ', '-', '-', ' ', ' ');
    wrefresh(win_debug_);
  }
}
