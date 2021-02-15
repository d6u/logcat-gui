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
  vector<string> cmd = {"adb", "logcat", "-v", "long", "-T", "100"};

  // Apply additional adb arguments
  for (int i = 1; i < argc; i++) {
    cmd.insert(cmd.begin() + i, string(argv[i]));
  }

  auto adb_proc = std::make_shared<Subprocess>(
      cmd, Subprocess::Options().pipeStdout().usePath());

  auto renderer = std::make_shared<Renderer>();
  renderer->init();
  renderer->start(adb_proc);
  renderer->stop();

  adb_proc->terminate();
  adb_proc->wait();

  return 0;
}
