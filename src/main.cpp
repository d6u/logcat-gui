#include "renderer.h"
#include "signal_handler.h"

#include <folly/Subprocess.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include <folly/io/async/EventBase.h>
#include <memory>
#include <string>
#include <vector>

using namespace std;
using namespace folly;

int main(int argc, char *argv[]) {
  vector<string> cmd = {"adb", "logcat", "-v", "long", "-T", "1000"};

  int i = 1;
  bool is_debug = false;

  if (argc > 1 && strcmp(argv[1], "-D") == 0) {
    is_debug = true;
    i = 2;
  }

  vector<string> filters;
  for (; i < argc; i++) {
    filters.push_back(string(argv[i]));
  }

  auto adb_proc = std::make_shared<Subprocess>(
      cmd, Subprocess::Options().pipeStdout().usePath());

  auto renderer = std::make_shared<Renderer>(is_debug);
  renderer->init();
  renderer->start(adb_proc, filters);
  renderer->stop();

  adb_proc->terminate();
  adb_proc->wait();

  return 0;
}
