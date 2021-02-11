#include <folly/Subprocess.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include <folly/io/async/EventBase.h>

using namespace folly;

class SignalHandler : public AsyncSignalHandler {
public:
  SignalHandler(EventBase *eb, std::shared_ptr<Subprocess> proc);
  void signalReceived(int /* signum */) noexcept override;

private:
  std::shared_ptr<Subprocess> proc_;
};
