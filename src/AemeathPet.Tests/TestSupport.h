#pragma once

#include <cstdio>
#include <exception>
#include <string>
#include <vector>

namespace Aemeath::Tests {

using TestFunction = void (*)();

struct TestCase {
    const char* name;
    TestFunction function;
};

std::vector<TestCase>& Registry();
void Register(const char* name, TestFunction function);
void ReportFailure(const char* file, int line, const std::string& message);
int FailureCount();

/// 断言宏：失败不中断，继续跑完整个用例。
#define AE_EXPECT(condition)                                                          \
    do {                                                                              \
        if (!(condition)) {                                                           \
            ::Aemeath::Tests::ReportFailure(__FILE__, __LINE__, "期望成立: " #condition); \
        }                                                                             \
    } while (false)

#define AE_EXPECT_EQ(actual, expected)                                                \
    do {                                                                              \
        const auto& ae_a = (actual);                                                   \
        const auto& ae_b = (expected);                                                 \
        if (!(ae_a == ae_b)) {                                                         \
            ::Aemeath::Tests::ReportFailure(                                            \
                __FILE__, __LINE__, "期望 " #actual " == " #expected);                  \
        }                                                                             \
    } while (false)

#define AE_EXPECT_NE(actual, unexpected)                                              \
    do {                                                                              \
        const auto& ae_a = (actual);                                                   \
        const auto& ae_b = (unexpected);                                               \
        if (ae_a == ae_b) {                                                            \
            ::Aemeath::Tests::ReportFailure(                                            \
                __FILE__, __LINE__, "期望 " #actual " != " #unexpected);                \
        }                                                                             \
    } while (false)

#define AE_TEST(name)                                                                 \
    static void name();                                                               \
    static const bool name##_registered =                                             \
        (::Aemeath::Tests::Register(#name, &name), true);                             \
    static void name()

} // namespace Aemeath::Tests