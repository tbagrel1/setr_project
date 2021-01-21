#pragma once

#include <string>
#include <vector>

#include "Mote.h"

struct ReadSensorsResult
{
	uint64_t instant;
	bool isSuccess;
	std::wstring errorMessage;
	std::map<std::wstring, Mote> motes;

	ReadSensorsResult() : instant(0), isSuccess(false), errorMessage(L"No result yet") {}
	ReadSensorsResult(const ReadSensorsResult& that) : instant(that.instant), isSuccess(that.isSuccess), errorMessage(that.errorMessage), motes(that.motes) {}
	ReadSensorsResult(uint64_t _instant, bool _isSuccess, std::wstring _errorMessage, std::map<std::wstring, Mote> _motes) : instant(_instant), isSuccess(_isSuccess), errorMessage(_errorMessage), motes(_motes) {}
};
