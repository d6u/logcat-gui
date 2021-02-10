#include <folly/Subprocess.h>
#include <iostream>
// #include <algorithm>
// #include <istream>
// #include <ncurses.h>
// #include <regex>
// #include <sstream>
// #include <string>
// #include <unordered_map>
// #include <unordered_set>
// #include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  folly::Subprocess("adb logcat -b all -T 10 -d").wait();
  cout << "Hello World!" << endl;
  return 0;
}