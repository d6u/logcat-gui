#include <ncurses.h>
#include <string>
#include <vector>

using namespace std;

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
