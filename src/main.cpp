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
using namespace folly;

int main(int argc, char *argv[]) {
  Subprocess proc(
    vector<string>{"ls", "-al"}, 
    Subprocess::Options().pipeStdout().usePath());

  // We couldn't use communicate here because it blocks until the
  // stdout/stderr is closed.
  char *line_buf = nullptr;
  size_t line_buf_size = 0;
  ssize_t line_size;

  FILE *fd = fdopen(proc.stdoutFd(), "r");
  
  line_size = getline(&line_buf, &line_buf_size, fd);
  
  while (line_size >= 0) {
    printf("line: chars=%06zd, buf size=%06zu, contents: %s",
        line_size, line_buf_size, line_buf);
    line_size = getline(&line_buf, &line_buf_size, fd);
  }

  delete line_buf;
  fclose(fd);

  // const auto [stdout, stderr] = proc.communicate();

  proc.wait();

  return 0;
}