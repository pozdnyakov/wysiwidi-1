#ifndef CONTEXT_MANAGER_H_
#define CONTEXT_MANAGER_H_

#include <string>

namespace wfd {

class ContextManager {
 public:  
  virtual ~ContextManager() {}
  virtual void Play() = 0;
  virtual void Pause() = 0;
  virtual void Teardown() = 0;
  virtual bool IsPaused() const = 0;
  virtual void SetRtpPorts(int port1, int port2) = 0;

  virtual void SendRtspMessage(const std::string& message) = 0;
};

}  // namespace wfd

#endif // CONTEXT_MANAGER_H_

