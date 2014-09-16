// msgutil.cpp : tool to send windows messages.
// --generate ..\utests\ut_core.cpp --catalog=..\catalog --pch

#include "stdafx.h"

#include <string>
#include <vector>
#include <unordered_map>

#include <SDKDDKVer.h>

#include <stdio.h>
#include <tchar.h>
#include <Windows.h>

#include <stdlib.h>


plx::File OpenConfigFile() {
  auto exe_path = plx::GetExePath();
  auto path = exe_path.append(L"config.json");
  plx::FileParams fparams = plx::FileParams::Read_SharedRead();
  return plx::File::Create(path, fparams, plx::FileSecurity());
}

plx::JsonValue ReadConfig(plx::File& cfile) {
  if (!cfile.is_valid())
    return false;
  auto size = cfile.size_in_bytes();
  if (size < 10)
    return false;
  plx::Range<uint8_t> r(0, plx::To<size_t>(size));
  auto mem = plx::HeapRange(r);
  if (cfile.read(r, 0) != size)
    return false;
  plx::Range<const char> json(reinterpret_cast<char*>(r.start()),
                              reinterpret_cast<char*>(r.end()));
  return plx::ParseJsonValue(json);
}

enum Method {
  sent_message,
  posted_message
};

struct Message {
  Method method;
  UINT message;
  UINT wparam;
  UINT lparam;
};

typedef std::unordered_map<std::string, long> Mappings;
typedef std::unordered_map<std::string, Message> Messages;

bool LoadMappings(plx::JsonValue& json, Mappings& mappings) {
  const auto& cm = json["constant_mappings"];
  if (cm.type() != plx::JsonType::OBJECT)
    return false;
  // expecting  <string> : number.
  auto kv = cm.get_iterator();
  while (kv.first != kv.second) {
    const auto& key = kv.first->first;
    const auto& val = kv.first->second;
    if (val.type() != plx::JsonType::INT64)
      return false;
    mappings.insert(std::make_pair(key, plx::To<long>(val.get_int64())));
    ++kv.first;
  }
  return true;
}

bool ResolveValue(plx::JsonValue& json_val, Mappings& mappings, UINT* value) {
  if (json_val.type() == plx::JsonType::STRING) {
    auto it = mappings.find(json_val.get_string());
    if (it != mappings.end())
      *value = plx::To<UINT>(it->second);
  } else if (json_val.type() == plx::JsonType::INT64) {
    *value = plx::To<UINT>(json_val.get_int64());
  } else {
    return false;
  }
  return true;
}

bool LoadMessages(plx::JsonValue& json, Messages& messages) {
  Mappings mappings;
  if (!LoadMappings(json, mappings))
    return false;

  auto& msg_array = json["messages"];
  if (msg_array.type() != plx::JsonType::ARRAY)
    return false;
  
  for (size_t ix = 0; ix != msg_array.size(); ++ix) {
    auto& msg = msg_array[ix];
    auto& name = msg["name"];
    if (name.type() != plx::JsonType::STRING)
      return false;
    auto& method = msg["method"];
    if (name.type() != plx::JsonType::STRING)
      return false;
    
    Message m;
    m.method =  method.get_string() == "SEND" ?
        sent_message : posted_message;
    if (!ResolveValue(msg["message"], mappings, &m.message))
      return false;
    if (!ResolveValue(msg["wparam"], mappings, &m.wparam))
      return false;
    if (!ResolveValue(msg["lparam"], mappings, &m.lparam))
      return false;

    messages[name.get_string()] = m;
  }
  return true;
}

int wmain(int argc, wchar_t* argv[]) {
  auto config = ReadConfig(OpenConfigFile());

  Messages messages;
  if (!LoadMessages(config, messages))
    return 1;

  return 0;
}
