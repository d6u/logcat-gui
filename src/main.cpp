#include "renderer.h"
#include "signal_handler.h"

#include <algorithm>
#include <folly/Subprocess.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include <folly/io/async/EventBase.h>
#include <iostream>
#include <istream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace folly;

void outputLines(int fd, shared_ptr<Renderer> renderer) {
  FILE *file_handle = fdopen(fd, "r");

  char *line_buf = nullptr;
  size_t line_buf_size = 0;
  ssize_t line_size = getline(&line_buf, &line_buf_size, file_handle);

  while (line_size >= 0) {
    renderer->renderLogLine(line_buf);
    line_size = getline(&line_buf, &line_buf_size, file_handle);
  }

  delete line_buf;
  fclose(file_handle);
}

int main(int argc, char *argv[]) {
  vector<string> cmd = {"adb", "logcat", "-v", "long", "-T", "100"};

  // Apply additional adb arguments
  for (int i = 1; i < argc; i++) {
    cmd.insert(cmd.begin() + i, string(argv[i]));
  }

  auto adb_proc = std::make_shared<Subprocess>(
      cmd, Subprocess::Options().pipeStdout().usePath());

  auto thread = std::thread([adb_proc]() {
    EventBase eventBase;
    SignalHandler handler(&eventBase, adb_proc);
    handler.registerSignalHandler(SIGINT);
    eventBase.loopForever();
  });

  auto renderer = std::make_shared<Renderer>();

  outputLines(adb_proc->stdoutFd(), renderer);

  adb_proc->wait();
  thread.join();

  return 0;
}
