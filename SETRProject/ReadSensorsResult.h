#pragma once

#include <string>
#include <vector>

#include "TempPoint.h"

struct ReadSensorsResult
{
	uint64_t instant;
	bool isSuccess;
	std::wstring errorMessage;
	std::vector<TempPoint> points;

	ReadSensorsResult() : instant(0), isSuccess(false), errorMessage(L"No result yet") {}
	ReadSensorsResult(const ReadSensorsResult& that) : instant(that.instant), isSuccess(that.isSuccess), errorMessage(that.errorMessage), points(that.points) {}
	ReadSensorsResult(uint64_t _instant, bool _isSuccess, std::wstring _errorMessage, std::vector<TempPoint> _points) : instant(_instant), isSuccess(_isSuccess), errorMessage(_errorMessage), points(_points) {}
};
