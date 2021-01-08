#pragma once

#include <string>

#include "GeoPoint.h"

struct FindNearestResult
{
	uint64_t instant;
	bool isSuccess;
	std::wstring errorMessage;
	std::wstring moteId;
	GeoPoint position;

	FindNearestResult() : instant(0), isSuccess(false), errorMessage(L"No result yet"), moteId(L"") {}
	FindNearestResult(const FindNearestResult& that) : instant(that.instant), isSuccess(that.isSuccess), errorMessage(that.errorMessage), moteId(that.moteId), position(that.position) {}
	FindNearestResult(uint64_t _instant, bool _isSuccess, std::wstring _errorMessage, std::wstring _moteId, GeoPoint _position) : instant(_instant), isSuccess(_isSuccess), errorMessage(_errorMessage), moteId(_moteId), position(_position) {}
};
