#include "pch.h"
#include "StringUtils.h"

#include <codecvt>
#include <locale>

std::wstring toWstring(std::string source) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(source);
}