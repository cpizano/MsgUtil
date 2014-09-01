// This is the plex precompiled header cc.
// Do not edit this file by hand.

#include "stdafx.h"



namespace plx {
ItRange<uint8_t*> RangeFromBytes(void* start, size_t count) {
  auto s = reinterpret_cast<uint8_t*>(start);
  return ItRange<uint8_t*>(s, s + count);
}
ItRange<const uint8_t*> RangeFromBytes(const void* start, size_t count) {
  auto s = reinterpret_cast<const uint8_t*>(start);
  return ItRange<const uint8_t*>(s, s + count);
}
plx::FilePath GetExePath() {
  wchar_t* pp = nullptr;
  _get_wpgmptr(&pp);
  return FilePath(pp).parent();
}
char* HexASCII(uint8_t byte, char* out) {
  *out++ = HexASCIITable[(byte >> 4) & 0x0F];
  *out++ = HexASCIITable[byte & 0x0F];
  return out;
}
std::string HexASCIIStr(const plx::Range<const uint8_t>& r, char separator) {
  if (r.empty())
    return std::string();

  std::string str((r.size() * 3) - 1, separator);
  char* data = &str[0];
  for (size_t ix = 0; ix != r.size(); ++ix) {
    data = plx::HexASCII(r[ix], data);
    ++data;
  }
  return str;
}
std::string DecodeString(plx::Range<const char>& range) {
  if (range.empty())
    return std::string();
  if (range[0] != '\"') {
    auto r = plx::RangeFromBytes(range.start(), 1);
    throw plx::CodecException(__LINE__, &r);
  }

  std::string s;
  for (;;) {
    auto text_start = range.start();
    while (range.advance(1) > 0) {
      auto c = range.front();
      if (c < 32) {
        throw plx::CodecException(__LINE__, nullptr);
      } else {
        switch (c) {
          case '\"' : break;
          case '\\' : goto escape;
          default: continue;
        }
      }
      s.append(++text_start, range.start());
      range.advance(1);
      return s;
    }
    // Reached the end of range before a (").
    throw plx::CodecException(__LINE__, nullptr);

  escape:
    s.append(++text_start, range.start());
    if (range.advance(1) <= 0)
      throw plx::CodecException(__LINE__, nullptr);

    switch (range.front()) {
      case '\"':  s.push_back('\"'); break;
      case '\\':  s.push_back('\\'); break;
      case '/':   s.push_back('/');  break;
      case 'b':   s.push_back('\b'); break;
      case 'f':   s.push_back('\f'); break;
      case 'n':   s.push_back('\n'); break;
      case 'r':   s.push_back('\r'); break;
      case 't':   s.push_back('\t'); break;   //$$ missing \u (unicode).
      default: {
        auto r = plx::RangeFromBytes(range.start() - 1, 2);
        throw plx::CodecException(__LINE__, &r);
      }
    }
  }
}
namespace JsonImp {
template <typename StrT>
bool Consume(plx::Range<const char>& r, StrT&& str) {
  auto c = r.starts_with(plx::RangeFromLitStr(str));
  if (c) {
    r.advance(c);
    return true;
  }
  else {
    return (c != 0);
  }
}

bool IsNumber(plx::Range<const char>&r) {
  if ((r.front() >= '0') && (r.front() <= '9'))
    return true;
  if ((r.front() == '-') || (r.front() == '+'))
    return true;
  if (r.front() == '.')
    return true;
  return false;
}

plx::JsonValue ParseArray(plx::Range<const char>& range) {
  if (range.empty())
    throw plx::CodecException(__LINE__, NULL);
  if (range.front() != '[')
    throw plx::CodecException(__LINE__, NULL);

  JsonValue value(plx::JsonType::ARRAY);
  range.advance(1);

  for (;!range.empty();) {
    range = plx::SkipWhitespace(range);

    if (range.front() == ',') {
      if (range.advance(1) <= 0)
        break;
      range = plx::SkipWhitespace(range);
    }

    if (range.front() == ']') {
      range.advance(1);
      return value;
    }

    value.push_back(ParseJsonValue(range));
  }

  auto r = plx::RangeFromBytes(range.start(), range.size());
  throw plx::CodecException(__LINE__, &r);
}

plx::JsonValue ParseNumber(plx::Range<const char>& range) {
  size_t pos = 0;
  auto num = plx::StringFromRange(range);

  auto iv = std::stoll(num, &pos);
  if ((range[pos] != 'e') && (range[pos] != 'E') && (range[pos] != '.')) {
    range.advance(pos);
    return iv;
  }

  auto dv = std::stod(num, &pos);
  range.advance(pos);
  return dv;
}

plx::JsonValue ParseObject(plx::Range<const char>& range) {
  if (range.empty())
    throw plx::CodecException(__LINE__, NULL);
  if (range.front() != '{')
    throw plx::CodecException(__LINE__, NULL);

  JsonValue obj(plx::JsonType::OBJECT);
  range.advance(1);

  for (;!range.empty();) {
    if (range.front() == '}') {
      range.advance(1);
      return obj;
    }

    range = plx::SkipWhitespace(range);
    auto key = plx::DecodeString(range);

    range = plx::SkipWhitespace(range);
    if (range.front() != ':')
      throw plx::CodecException(__LINE__, nullptr);
    if (range.advance(1) <= 0)
      throw plx::CodecException(__LINE__, nullptr);

    range = plx::SkipWhitespace(range);
    obj[key] = ParseJsonValue(range);

    range = plx::SkipWhitespace(range);
    if (range.front() == ',') {
      if (range.advance(1) <= 0)
        break;
      range = plx::SkipWhitespace(range);
    }
  }
  throw plx::CodecException(__LINE__, nullptr);
}

}
plx::JsonValue ParseJsonValue(plx::Range<const char>& range) {
  range = plx::SkipWhitespace(range);
  if (range.empty())
    throw plx::CodecException(__LINE__, NULL);

  if (range.front() == '{')
    return JsonImp::ParseObject(range);
  if (range.front() == '\"')
    return plx::DecodeString(range);
  if (range.front() == '[')
    return JsonImp::ParseArray(range);
  if (JsonImp::Consume(range, "true"))
    return true;
  if (JsonImp::Consume(range, "false"))
    return false;
  if (JsonImp::Consume(range, "null"))
    return nullptr;
  if (JsonImp::IsNumber(range))
    return JsonImp::ParseNumber(range);

  auto r = plx::RangeFromBytes(range.start(), range.size());
  throw plx::CodecException(__LINE__, &r);
}
}
