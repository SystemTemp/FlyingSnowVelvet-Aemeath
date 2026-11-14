#include "TestSupport.h"

#include <cstdio>
#include <cstdlib>
#include <exception>

namespace Aemeath::Tests {

namespace {

std::vector<TestCase>& RegistryStorage() {
    static std::vector<TestCase> registry;
    return registry;
}

int g_failures = 0;

} // namespace

std::vector<TestCase>& Registry() {
    return RegistryStorage();
}

void Register(const char* name, TestFunction function) {
    RegistryStorage().push_back(TestCase { name, function });
}

void ReportFailure(const char* file, int line, const std::string& message) {
    ++g_failures;
    std::printf("    [FAIL] %s:%d  %s\n", file, line, message.c_str());
}

int FailureCount() {
    return g_failures;
}

} // namespace Aemeath::Tests

int main() {
    // 无缓冲输出：崩溃时也能看到最后跑到哪个用例。
    ::setvbuf(stdout, nullptr, _IONBF, 0);
    ::setvbuf(stderr, nullptr, _IONBF, 0);
    using namespace Aemeath::Tests;

    std::printf("AemeathPet C++ \u6d4b\u8bd5\uff0c\u5171 %zu \u4e2a\u7528\u4f8b\n\n", Registry().size());

    int failedCases = 0;
    for (const auto& testCase : Registry()) {
        const int before = FailureCount();
        std::printf("[ RUN  ] %s\n", testCase.name);

        try {
            testCase.function();
        } catch (const std::exception& ex) {
            ReportFailure(__FILE__, __LINE__, std::string("\u672a\u6355\u83b7\u5f02\u5e38\uff1a") + ex.what());
        } catch (...) {
            ReportFailure(__FILE__, __LINE__, "\u672a\u6355\u83b7\u7684\u672a\u77e5\u5f02\u5e38");
        }

        if (FailureCount() == before) {
            std::printf("[ PASS ] %s\n", testCase.name);
        } else {
            std::printf("[ FAIL ] %s\n", testCase.name);
            ++failedCases;
        }
    }

    std::printf("\n---- \u6c47\u603b ----\n");
    std::printf("\u7528\u4f8b\uff1a%zu\uff0c\u5931\u8d25\uff1a%d\uff0c\u65ad\u8a00\u9519\u8bef\uff1a%d\n",
        Registry().size(), failedCases, FailureCount());

    return failedCases == 0 ? 0 : 1;
}
