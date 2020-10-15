#include <dlfcn.h>

#include <exception>
#include <functional>
#include <string>
#include <type_traits>

using compute_fn = std::add_pointer<void()>::type;

int main() {
  void* handle = dlopen("./libtest.so", RTLD_LAZY);

  if (!handle) {
    throw std::runtime_error("Failed to open");
  }

  auto process_query = (compute_fn)(dlsym(handle, "compute"));
  if (!process_query) {
    dlclose(handle);
    throw std::runtime_error("Failed to get compute fn");
  }

  process_query();
  dlclose(handle);
}