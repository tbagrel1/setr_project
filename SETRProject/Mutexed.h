#pragma once

#include <mutex>

template<typename T>
class Mutexed
{
private:
	std::mutex mutex;
	T value;
public:
	Mutexed() {}
	Mutexed(T initialValue) : value(initialValue) {}
	T read() {
		this->mutex.lock();
		T readValue = T(value);
		this->mutex.unlock();
		return readValue;
	}
	template<typename F> void update(F &lambda) {
		this->mutex.lock();
		value = lambda(value);
		this->mutex.unlock();
	}
};
