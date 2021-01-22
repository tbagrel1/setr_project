#pragma once

#include <string>

#include "GeoPosition.h"
#include "Mote.h"

struct ComputeResult
{
	uint64_t userPositionInstant;
	uint64_t readSensorsInstant;

	uint64_t instant;
	bool isSuccess;
	std::wstring errorMessage;

	std::vector<Mote> motes;
	GeoPosition userPosition;
	Mote nearestActiveMote;

	ComputeResult() : userPositionInstant(0), readSensorsInstant(0), instant(1), isSuccess(false), errorMessage(L"No result yet") {}
	ComputeResult(const ComputeResult& that) : userPositionInstant(that.userPositionInstant), readSensorsInstant(that.readSensorsInstant), instant(that.instant), isSuccess(that.isSuccess), errorMessage(that.errorMessage), motes(that.motes), userPosition(that.userPosition), nearestActiveMote(that.nearestActiveMote) {}
	ComputeResult(uint64_t _userPositionInstant, uint64_t _readSensorsInstant, uint64_t _instant, bool _isSuccess, std::wstring _errorMessage, std::vector<Mote> _motes, GeoPosition _userPosition, Mote _nearestActiveMote) : userPositionInstant(_userPositionInstant), readSensorsInstant(_readSensorsInstant), instant(_instant), isSuccess(_isSuccess), errorMessage(_errorMessage), motes(_motes), userPosition(_userPosition), nearestActiveMote(_nearestActiveMote) {}
};
