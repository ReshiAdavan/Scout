// Entry point for the test binary. Each test_*.cpp file registers its own
// TEST(...) cases via static initializers; this just runs them.
#include "test_framework.h"

int main() {
    return testing::run_all();
}
