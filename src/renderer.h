#include <folly/Subprocess.h>
#include <ncurses.h>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace folly;

class Renderer {
public:
  Renderer(bool is_debug = false);
  void init();
  void start(std::shared_ptr<Subprocess> proc, vector<string> filters);
  void stop();

private:
  const bool is_debug_;
  int debug_counter_ = 0;

  int row_;
  int col_;
  int win_log_list_row();
  int win_log_list_col();
  int win_tag_list_row();
  int win_tag_list_col();
  int win_debug_row();
  int win_debug_col();
  WINDOW *win_log_list_;
  WINDOW *win_tag_list_;
  WINDOW *win_debug_;

  string cache_line_ = "";

  unordered_map<string, int> tag_count_;
  string date_;
  string level_;
  string timestamp_;
  string pid_;
  string tid_;
  string tag_;
  vector<string> lines_;

  void maybeHandleWindowResize();
  void renderLine(string line, vector<string> filters);
  void renderBorderAndRefresh();
};
