#include "signal_handler.h"

#include <folly/Subprocess.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include <folly/io/async/EventBase.h>
#include <iostream>
#include <ncurses.h>

using namespace folly;

SignalHandler::SignalHandler(EventBase *eb, std::shared_ptr<Subprocess> proc)
    : AsyncSignalHandler(eb), proc_(proc) {}

void SignalHandler::signalReceived(int /* signum */) noexcept {
  std::cout << std::endl << "Stopping process..." << std::endl;
  getEventBase()->terminateLoopSoon();
  proc_->terminate();
  endwin();
}
