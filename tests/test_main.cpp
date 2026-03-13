#include <gtest/gtest.h>
#include <cstdlib>

// Main entry point for tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

#ifndef _WIN32
    // On Linux, the driver's global EnvShare destructor can cause 
    // "double free or corruption" during normal process exit.
    // Use _Exit() to skip static destructors and avoid the crash.
    // This is safe because GTest has already flushed all output.
    _Exit(result);
#endif

    return result;
}
