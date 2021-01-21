#include "pch.h"

//
// MainPage.xaml.cpp
// Implémentation de la classe MainPage.
//

#include <ctime>
#include <codecvt>
#include <locale>
#include <string>

#include "MainPage.xaml.h"
#include "FindNearestResult.h"
#include "Mutexed.h"
#include "ReadSensorsResult.h"
#include "UserPositionResult.h"
#include "TimeUtils.h"
#include "json.hpp"

using namespace SETRProject;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Web::Http;

// Pour plus d'informations sur le modèle d'élément Page vierge, consultez la page https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

#define READ_SENSORS_SLEEP_MILLISECONDS 10000L
#define FIND_NEAREST_SLEEP_MILLISECONDS 10000L
#define UI_REFRESH_SLEEP_MILLISECONDS 100L

Mutexed<ReadSensorsResult> readSensorsResult;
Mutexed<bool> readSensorsRunning(true);
Mutexed<FindNearestResult> findNearestResult;
Mutexed<bool> findNearestRunning(true);
Mutexed<UserPositionResult> userPositionResult;

std::wstring toWstring(std::string source) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(source);
}

const std::wstring apiUrl = L"http://iotlab.telecomnancy.eu:8080/iotlab/rest/data/1/temperature/last";
HttpClient httpClient;

std::vector<TempPoint> parseJsonData(std::wstring const& jsonString) {
	std::vector<TempPoint> result;
	nlohmann::json moteList = nlohmann::json::parse(jsonString);
	for (auto &moteObj : moteList) {
		result.push_back(TempPoint::fromJson(moteObj));
	}
	return result;
}

ReadSensorsResult readSensors(std::wstring apiUrl) {
	Platform::String^ apiUrlPlatform = ref new Platform::String(apiUrl.c_str());
	Uri^ requestUri = ref new Uri(apiUrlPlatform);
	std::vector<TempPoint> points;
	std::wstring errorMessage = L"";
	boolean isSuccess = true;
	try {
		HttpResponseMessage^ response = concurrency::create_task(httpClient.GetAsync(requestUri)).get();
		try {
			Platform::String^ responseBody = concurrency::create_task(response->Content->ReadAsStringAsync()).get();
			try {
				points = parseJsonData(responseBody->Data());
			}
			catch (nlohmann::json::exception& ex) {
				isSuccess = false;
				errorMessage = toWstring(ex.what());
				// Json exception
			}
		}
		catch (Platform::Exception ^ex) {
			isSuccess = false;
			errorMessage = std::wstring(ex->Message->Data());
			// Read body exception
		}
	}
	catch (Platform::Exception ^ex) {
		isSuccess = false;
		errorMessage = std::wstring(ex->Message->Data());
		// HTTP call exception
	}
	return ReadSensorsResult(timestampNow(), isSuccess, errorMessage, points);
}

void readSensorsThreadFunc() {
	while (readSensorsRunning.read()) {
		readSensorsResult.update([](ReadSensorsResult prevResult) { return readSensors(apiUrl); });
		std::this_thread::sleep_for(std::chrono::milliseconds(READ_SENSORS_SLEEP_MILLISECONDS));
	}
}

void findNearestThreadFunc() {
	while (findNearestRunning.read()) {
		// TODO: find nearest
		findNearestResult.update([](FindNearestResult prevResult) { return FindNearestResult(timestampNow(), true, L"", L"255.255", GeoPoint()); });
		std::this_thread::sleep_for(std::chrono::milliseconds(FIND_NEAREST_SLEEP_MILLISECONDS));
	}
}

std::thread readSensorsThread(readSensorsThreadFunc);
std::thread findNearestThread(findNearestThreadFunc);

MainPage::MainPage()
{
	InitializeComponent();

	DispatcherTimer^ timer = ref new DispatcherTimer;
	TimeSpan timeSpan;
	timeSpan.Duration = 10000LL * UI_REFRESH_SLEEP_MILLISECONDS;
	timer->Interval = timeSpan;
	timer->Start();
	timer->Tick += ref new EventHandler<Object^>(this, &MainPage::onTick);

	readSensorsInstant = 0;
	findNearestInstant = 0;
}

void MainPage::updateUi()
{
	ReadSensorsResult newReadSensorsResult = readSensorsResult.read();
	if (newReadSensorsResult.instant != readSensorsInstant) {
		// Update available
		readSensorsInstant = newReadSensorsResult.instant;
		readSensorsInstantTextBox->Text = timestampToString(readSensorsInstant);
	}
	FindNearestResult newFindNearestResult = findNearestResult.read();
	if (newFindNearestResult.instant != findNearestInstant) {
		// Update available
		findNearestInstant = newFindNearestResult.instant;
		findNearestInstantTextBox->Text = timestampToString(findNearestInstant);
	}
}

void MainPage::onTick(Platform::Object^ sender, Platform::Object^ args)
{
	this->updateUi();
}
