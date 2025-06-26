#pragma once
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string_view>

typedef void ProcessLine(const std::wstring_view line, void* data);

bool UnicodeProcessLine(const HANDLE hFile, _In_ UINT CodePage, ProcessLine ppl, void* data);
