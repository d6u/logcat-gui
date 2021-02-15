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
  void start(std::shared_ptr<Subprocess> proc);
  void stop();

private:
  const bool is_debug_;

  int row_;
  int col_;
  int win_tag_list_row_;
  int win_tag_list_col_;
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
  void renderLine(string line);
  void renderBorderAndRefresh();
};
