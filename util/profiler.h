#pragma once

#include <fcntl.h>
#include <functional>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "absl/flags/flag.h"

#include "util/profile_map_generator.h"

ABSL_FLAG(std::string, perfpath, "perf output path", "Perf output path");

namespace kush::util {
struct Profiler {
  static void profile(std::function<void()> body) {
    std::string filename = FLAGS_perfpath.Get();

    // Launch profiler
    pid_t pid;
    std::stringstream s;
    s << getpid();
    pid = fork();
    if (pid == 0) {
      exit(execl("/usr/bin/perf", "perf", "record", "-F", "5000", "-k", "1",
                 "-g", "--call-graph", "fp", "-o", filename.c_str(), "-p",
                 s.str().c_str(), nullptr));
    }

    while (true) {
      // wait until perf is setup
      int status;
      if (waitpid(pid, &status, WNOHANG) == 0) {
        break;
      }
    }

    // Run body
    body();

    // Kill profiler
    kill(pid, SIGINT);
    waitpid(pid, nullptr, 0);
  }
};

}  // namespace kush::util