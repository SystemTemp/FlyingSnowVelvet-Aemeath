#pragma once
#include <cctype>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
namespace Aemeath::Infrastructure::Json {
class Value {
public:
 using Array=std::vector<Value>; using Object=std::map<std::string,Value>; using Storage=std::variant<std::nullptr_t,bool,double,std::string,Array,Object>;
 Value():v(nullptr){} Value(std::nullptr_t):v(nullptr){} Value(bool x):v(x){} Value(double x):v(x){} Value(int x):v(double(x)){} Value(std::int64_t x):v(double(x)){} Value(std::string x):v(std::move(x)){} Value(const char*x):v(std::string(x)){} Value(Array x):v(std::move(x)){} Value(Object x):v(std::move(x)){}
 bool IsObject()const{return std::holds_alternative<Object>(v);} bool IsArray()const{return std::holds_alternative<Array>(v);} bool IsString()const{return std::holds_alternative<std::string>(v);} bool IsNumber()const{return std::holds_alternative<double>(v);} bool IsBool()const{return std::holds_alternative<bool>(v);}
 const Object& AsObject()const{return std::get<Object>(v);} const Array& AsArray()const{return std::get<Array>(v);} const std::string& AsString()const{return std::get<std::string>(v);} double AsNumber()const{return std::get<double>(v);} bool AsBool()const{return std::get<bool>(v);}
 const Value* Find(std::string_view key)const{if(!IsObject())return nullptr;auto&i=AsObject();auto p=i.find(std::string(key));return p==i.end()?nullptr:&p->second;}
 Storage v;
};
Value Parse(std::string_view text); std::string Stringify(const Value& value);
inline std::string String(const Value&v,std::string_view k,std::string d={}){auto*p=v.Find(k);return p&&p->IsString()?p->AsString():d;}
inline std::int64_t Int(const Value&v,std::string_view k,std::int64_t d=0){auto*p=v.Find(k);return p&&p->IsNumber()?static_cast<std::int64_t>(p->AsNumber()):d;}
inline bool Bool(const Value&v,std::string_view k,bool d=false){auto*p=v.Find(k);return p&&p->IsBool()?p->AsBool():d;}
}
