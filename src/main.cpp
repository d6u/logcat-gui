#include <folly/Subprocess.h>
#include <iostream>
#include <signal.h>
// #include <ncurses.h>
// #include <algorithm>
// #include <istream>
// #include <regex>
// #include <sstream>
// #include <string>
// #include <unordered_map>
// #include <unordered_set>
// #include <vector>

// using namespace std;
using namespace folly;

void outputLines(int fd) {
  char *line_buf = nullptr;
  size_t line_buf_size = 0;
  ssize_t line_size;

  FILE *file = fdopen(fd, "r");

  line_size = getline(&line_buf, &line_buf_size, file);

  while (line_size >= 0) {
    printf("line: chars=%06zd, buf size=%06zu, contents: %s", line_size,
           line_buf_size, line_buf);
    line_size = getline(&line_buf, &line_buf_size, file);
  }

  delete line_buf;
  fclose(file);
}

std::shared_ptr<Subprocess> proc;

void signal_callback_handler(int signum) {
  std::cout << "Receive signal: " << signum << std::endl;
  if (signum == SIGINT && proc != nullptr) {
    proc->terminate();
  }
}

int main(int argc, char *argv[]) {
  signal(SIGINT, signal_callback_handler);

  proc = std::make_shared<Subprocess>(
      std::vector<std::string>{"adb", "-s", "emulator-5554", "logcat"},
      Subprocess::Options().pipeStdout().usePath());

  outputLines(proc->stdoutFd());

  proc->wait();

  return 0;
}