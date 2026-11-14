#include "TestSupport.h"

#include "Aemeath/Infrastructure/Json/SimpleJson.h"

#include <string>

namespace J = Aemeath::Infrastructure::Json;

namespace {

AE_TEST(Json_ParseSimpleObject) {
    const auto value = J::Parse(R"({"name":"aemeath","version":1,"enabled":true})");
    AE_EXPECT(value.IsObject());
    AE_EXPECT_EQ(J::String(value, "name"), std::string("aemeath"));
    AE_EXPECT_EQ(J::Int(value, "version"), static_cast<std::int64_t>(1));
    AE_EXPECT_EQ(J::Bool(value, "enabled", false), true);
}

AE_TEST(Json_ParseNestedStructure) {
    const auto value = J::Parse(R"({"data":{"posts":[{"id":1,"title":"hello"},{"id":2}]}})");
    const auto* data = value.Find("data");
    AE_EXPECT(data != nullptr);
    const auto* posts = data->Find("posts");
    AE_EXPECT(posts != nullptr);
    AE_EXPECT(posts->IsArray());
    AE_EXPECT_EQ(posts->AsArray().size(), static_cast<std::size_t>(2));
    AE_EXPECT_EQ(J::Int(posts->AsArray()[0], "id"), static_cast<std::int64_t>(1));
    AE_EXPECT_EQ(J::String(posts->AsArray()[0], "title"), std::string("hello"));
}

AE_TEST(Json_StringifyRoundTrip) {
    J::Value::Object object {
        { "model", "test-model" },
        { "temperature", 0.7 },
        { "stream", true },
    };
    J::Value::Array messages;
    messages.emplace_back(J::Value::Object { { "role", "user" }, { "content", "\u4f60\u597d" } });
    object["messages"] = J::Value(std::move(messages));

    const auto text = J::Stringify(J::Value(std::move(object)));
    const auto parsed = J::Parse(text);

    AE_EXPECT_EQ(J::String(parsed, "model"), std::string("test-model"));
    AE_EXPECT_EQ(J::Bool(parsed, "stream", false), true);
    const auto* parsedMessages = parsed.Find("messages");
    AE_EXPECT(parsedMessages != nullptr && parsedMessages->IsArray());
    AE_EXPECT_EQ(J::String(parsedMessages->AsArray()[0], "content"), std::string("\u4f60\u597d"));
}

AE_TEST(Json_DefaultsWhenMissing) {
    const auto value = J::Parse(R"({})");
    AE_EXPECT_EQ(J::String(value, "missing", "fallback"), std::string("fallback"));
    AE_EXPECT_EQ(J::Int(value, "missing", 5), static_cast<std::int64_t>(5));
    AE_EXPECT_EQ(J::Bool(value, "missing", true), true);
    AE_EXPECT(value.Find("missing") == nullptr);
}

AE_TEST(Json_InvalidInputThrows) {
    bool threw = false;
    try {
        (void)J::Parse("{not json");
    } catch (const std::exception&) {
        threw = true;
    }
    AE_EXPECT(threw);
}

AE_TEST(Json_ArrayRoot) {
    const auto value = J::Parse(R"([1,2,3])");
    AE_EXPECT(value.IsArray());
    AE_EXPECT_EQ(value.AsArray().size(), static_cast<std::size_t>(3));
    AE_EXPECT_EQ(value.AsArray()[2].AsNumber(), 3.0);
}

} // namespace
