#include "ExceptionPrinter.h"
#include "ConsoleApi.h"

ExceptionPrinter::ExceptionPrinter(const char* exceptionPrefix_)
  : exceptionPrefix(exceptionPrefix_)
{
}

void ExceptionPrinter::PrintException(const char* what)
{
  std::string tmp;

  auto console = RE::ConsoleLog::GetSingleton();
  if (!console) {
    return;
  }

  size_t i = 0;

  auto safePrint = [what, console, &i](std::string msg) {
    if (msg.size() > 128) {
      msg.resize(128);
      msg += '...';
    }
    const char* prefix = ConsoleApi::GetExceptionPrefix();
    console->Print("%s%s", (i ? "" : prefix), msg.data());
    ++i;
  };

  for (size_t i = 0, n = strlen(what); i < n; ++i) {
    if (what[i] == '\n') {
      safePrint(tmp);
      tmp.clear();
    } else {
      tmp += what[i];
    }
  }
  if (!tmp.empty()) {
    safePrint(tmp);
  }
}
