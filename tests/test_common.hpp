#pragma once

#include <cmath>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>

inline void assert_true_impl(
    const bool condition,
    const char* expression,
    const char* file,
    const int line) {
    if (!condition) {
        std::ostringstream message;
        message << file << ":" << line << " assertion failed: " << expression;
        throw std::runtime_error(message.str());
    }
}

template <typename T, typename U>
inline void assert_equal_impl(
    const T& expected,
    const U& actual,
    const char* expected_expression,
    const char* actual_expression,
    const char* file,
    const int line) {
    if (!(expected == actual)) {
        std::ostringstream message;
        message << file << ":" << line << " expected " << expected_expression
                << " == " << actual_expression
                << " but got [" << expected << "] vs [" << actual << "]";
        throw std::runtime_error(message.str());
    }
}

inline void assert_near_impl(
    const double expected,
    const double actual,
    const double tolerance,
    const char* file,
    const int line) {
    if (std::fabs(expected - actual) > tolerance) {
        std::ostringstream message;
        message << file << ":" << line << " expected " << expected
                << " near " << actual << " (tolerance " << tolerance << ")";
        throw std::runtime_error(message.str());
    }
}

inline void run_test(const std::string&, const std::function<void()>& test) {
    test();
}

#define ASSERT_TRUE(expr) assert_true_impl((expr), #expr, __FILE__, __LINE__)
#define ASSERT_FALSE(expr) assert_true_impl(!(expr), "!(" #expr ")", __FILE__, __LINE__)
#define ASSERT_EQ(expected, actual) assert_equal_impl((expected), (actual), #expected, #actual, __FILE__, __LINE__)
#define ASSERT_NEAR(expected, actual, tolerance) assert_near_impl((expected), (actual), (tolerance), __FILE__, __LINE__)
