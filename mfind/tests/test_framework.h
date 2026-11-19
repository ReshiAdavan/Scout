#ifndef SCOUT_TEST_FRAMEWORK_H
#define SCOUT_TEST_FRAMEWORK_H

// Minimal self-contained C++17 test framework. No external deps.
// Use TEST(name) { ... } to register a test; tests_main.cpp runs all of them.

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <sstream>
#include <exception>

namespace testing {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

struct AssertionFailure : std::exception {
    std::string msg;
    explicit AssertionFailure(std::string m) : msg(std::move(m)) {}
    const char* what() const noexcept override { return msg.c_str(); }
};

inline int run_all() {
    int passed = 0, failed = 0;
    for (auto& t : registry()) {
        try {
            t.fn();
            std::cout << "[PASS] " << t.name << "\n";
            ++passed;
        } catch (const AssertionFailure& e) {
            std::cout << "[FAIL] " << t.name << ": " << e.what() << "\n";
            ++failed;
        } catch (const std::exception& e) {
            std::cout << "[ERROR] " << t.name << ": " << e.what() << "\n";
            ++failed;
        } catch (...) {
            std::cout << "[ERROR] " << t.name << ": unknown exception\n";
            ++failed;
        }
    }
    std::cout << "\n" << passed << " passed, " << failed << " failed.\n";
    return failed == 0 ? 0 : 1;
}

} // namespace testing

#define TEST(name) \
    static void scout_test_##name(); \
    static testing::Registrar scout_reg_##name(#name, scout_test_##name); \
    static void scout_test_##name()

#define EXPECT_EQ(a, b) do { \
    auto _va = (a); auto _vb = (b); \
    if (!(_va == _vb)) { \
        std::stringstream _ss; \
        _ss << "EXPECT_EQ failed: " #a " == " #b \
            << " (" << _va << " vs " << _vb << ") at " \
            << __FILE__ << ":" << __LINE__; \
        throw testing::AssertionFailure(_ss.str()); \
    } \
} while (0)

#define EXPECT_TRUE(cond) do { \
    if (!(cond)) { \
        std::stringstream _ss; \
        _ss << "EXPECT_TRUE failed: " #cond \
            << " at " << __FILE__ << ":" << __LINE__; \
        throw testing::AssertionFailure(_ss.str()); \
    } \
} while (0)

#define EXPECT_FALSE(cond) EXPECT_TRUE(!(cond))

#endif // SCOUT_TEST_FRAMEWORK_H
