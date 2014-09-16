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
  WPARAM wparam;
  LPARAM lparam;
  std::string info;
};

typedef std::unordered_map<std::string, long> Mappings;

bool LoadMappings(plx::JsonValue& json, Mappings& mappings) {
  auto cm = json["constant_mappings"];
  if (cm.type != plx::JsonType::ARRAY)
    return false;

  for (size_t ix = 0; ix != cm.size(); ++ix) {
    auto entry = cm[ix];
    if (entry.type != plx::JsonType::OBJECT)
      return false;
    // !!!! need to enumerate the keys.
  }
  return true;
}

int wmain(int argc, wchar_t* argv[]) {
  auto config = ReadConfig(OpenConfigFile());

  Mappings mappings;
  std::vector<Message> messages;

  if (!LoadMappings(config, mappings))
    return 1;

  return 0;
}
