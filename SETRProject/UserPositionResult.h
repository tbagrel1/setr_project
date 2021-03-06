#pragma once

#include <string>

#include "GeoPosition.h"

struct UserPositionResult {
	uint64_t instant;
	bool isSuccess;
	std::wstring errorMessage;
	GeoPosition position;

	UserPositionResult() : instant(0), isSuccess(false), errorMessage(L"No result yet") {}
	UserPositionResult(const UserPositionResult& that) : instant(that.instant), isSuccess(that.isSuccess), errorMessage(that.errorMessage), position(that.position) {}
	UserPositionResult(uint64_t _instant, bool _isSuccess, std::wstring _errorMessage, GeoPosition _position) : instant(_instant), isSuccess(_isSuccess), errorMessage(_errorMessage), position(_position) {}
};
