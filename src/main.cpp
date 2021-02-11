#include <folly/Subprocess.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include <folly/io/async/EventBase.h>
#include <iostream>
// #include <ncurses.h>
// #include <algorithm>
// #include <istream>
// #include <regex>
// #include <sstream>
// #include <string>
// #include <unordered_map>
// #include <unordered_set>
// #include <vector>

#include "signal_handler.h"

using namespace folly;

void outputLines(int fd) {
  FILE *file_handle = fdopen(fd, "r");

  char *line_buf = nullptr;
  size_t line_buf_size = 0;
  ssize_t line_size = getline(&line_buf, &line_buf_size, file_handle);

  while (line_size >= 0) {
    printf("line: chars=%06zd, buf size=%06zu, contents: %s", line_size,
           line_buf_size, line_buf);
    line_size = getline(&line_buf, &line_buf_size, file_handle);
  }

  delete line_buf;
  fclose(file_handle);
}

int main(int argc, char *argv[]) {
  auto adb_proc = std::make_shared<Subprocess>(
      std::vector<std::string>{"adb", "-s", "emulator-5554", "logcat", "-b",
                               "all", "-T", "10"},
      Subprocess::Options().pipeStdout().usePath());

  auto thread = std::thread([adb_proc]() {
    EventBase eventBase;
    SignalHandler handler(&eventBase, adb_proc);
    handler.registerSignalHandler(SIGINT);
    eventBase.loopForever();
  });

  outputLines(adb_proc->stdoutFd());

  adb_proc->wait();
  thread.join();

  return 0;
}
