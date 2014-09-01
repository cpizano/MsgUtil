// msgutil.cpp : tool to send windows messages.
// --generate ..\utests\ut_core.cpp --catalog=..\catalog --pch

#include "stdafx.h"

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
  plx::Range<char> r(0, size);
  auto mem = plx::HeapRange(r);
  if (cfile.read(r, 0) != size)
    return false;
  plx::Range<const char> json(r);
  return plx::ParseJsonValue(json);
}


int wmain(int argc, wchar_t* argv[]) {
  auto config = ReadConfig(OpenConfigFile());

  return 0;
}
