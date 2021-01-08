#include "pch.h"
#include "TimeUtils.h"

#include <chrono>
#include <string>
#include <ctime>

uint64_t timestampNow()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

Platform::String^ timestampToString(uint64_t timestamp) {
	wchar_t* buffer = new wchar_t[64];
	time_t instant = timestamp / 1000UL;
	struct tm timeObj;
	localtime_s(&timeObj, &instant);
	wcsftime(buffer, 64, L"%Y-%m-%d %H:%M:%S", &timeObj);
	buffer[63] = L'\0';
	Platform::String^ result = ref new Platform::String(buffer, (unsigned int) (wcslen(buffer)));
	delete[] buffer;
	return result;
}
