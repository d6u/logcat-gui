#include <ncurses.h>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class Renderer {
public:
  Renderer();
  void renderLogLine(char *line_buf);
  void stop();

private:
  int row_;
  int col_;
  int win_tag_list_row_;
  int win_tag_list_col_;
  WINDOW *win_log_list_;
  WINDOW *win_tag_list_;

  unordered_map<string, int> tag_count_;
  string date_;
  string level_;
  string timestamp_;
  string pid_;
  string tid_;
  string tag_;
  vector<string> lines_;
};
