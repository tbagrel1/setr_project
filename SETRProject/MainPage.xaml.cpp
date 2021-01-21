#include "pch.h"

//
// MainPage.xaml.cpp
// Implémentation de la classe MainPage.
//

#include <ctime>
#include "MainPage.xaml.h"
#include "Mote.h"
#include "ComputeResult.h"
#include "Mutexed.h"
#include "ReadSensorsResult.h"
#include "UserPositionResult.h"
#include "TimeUtils.h"
#include "json.hpp"
#include "StringUtils.h"
#include <limits>

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
using namespace Windows::Devices::Geolocation;

// Pour plus d'informations sur le modèle d'élément Page vierge, consultez la page https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

#define READ_SENSORS_SLEEP_MILLISECONDS 20000L
#define COMPUTE_SLEEP_MILLISECONDS 50L
#define GPS_SLEEP_MILLISECONDS 1000L
#define GPS_SLEEP_METERS 1U
#define UI_REFRESH_SLEEP_MILLISECONDS 50L

Mutexed<ReadSensorsResult> readSensorsResult;
Mutexed<bool> readSensorsRunning(true);
Mutexed<UserPositionResult> userPositionResult;
Mutexed<bool> useGps(true);
Mutexed<ComputeResult> computeResult;
Mutexed<bool> computeRunning(true);

const std::vector<Mote> MOTES = {
	Mote(L"9.138", GeoPoint(48.669422, 6.155112), L"Amphi Nord"),
	Mote(L"111.130", GeoPoint(48.668837, 6.154990), L"Amphi Sud"),
	Mote(L"151.105", GeoPoint(48.668922, 6.155363), L"Salle E-1.22"),
	Mote(L"32.131", GeoPoint(48.669400, 6.155340), L"Salle N-0.3"),
	Mote(L"97.145", GeoPoint(48.669439, 6.155265), L"Bureau 2.6"),
	Mote(L"120.99", GeoPoint(48.669419, 6.155269), L"Bureau 2.7"),
	Mote(L"200.124", GeoPoint(48.669394, 6.155287), L"Bureau 2.8"),
	Mote(L"53.105", GeoPoint(48.669350, 6.155310), L"Bureau 2.9"),
};

const std::wstring apiUrl = L"http://iotlab.telecomnancy.eu:8080/iotlab/rest/data/1/temperature/last";
HttpClient httpClient;

std::map<std::wstring, Mote> parseJsonData(std::wstring const& jsonString) {
	std::map<std::wstring, Mote> result;
	nlohmann::json moteList = nlohmann::json::parse(jsonString).at("data");

	for (auto &moteObj : moteList) {
		Mote mote = Mote::fromJson(moteObj);
		result.insert(std::make_pair(std::wstring(mote.moteId), Mote::fromJson(moteObj)));
	}
	return result;
}

ReadSensorsResult readSensors(std::wstring apiUrl) {
	Platform::String^ apiUrlPlatform = ref new Platform::String(apiUrl.c_str());
	Uri^ requestUri = ref new Uri(apiUrlPlatform);
	std::map<std::wstring, Mote> motes;
	std::wstring errorMessage = L"";
	boolean isSuccess = true;
	try {
		HttpResponseMessage^ response = concurrency::create_task(httpClient.GetAsync(requestUri)).get();
		try {
			Platform::String^ responseBody = concurrency::create_task(response->Content->ReadAsStringAsync()).get();
			try {
				motes = parseJsonData(responseBody->Data());
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
	return ReadSensorsResult(timestampNow(), isSuccess, errorMessage, motes);
}

template<typename K, typename V>
bool has(std::map<K, V> dict, K const& key) {
	return dict.find(key) != dict.end();
}

ComputeResult compute(ComputeResult prevResult) {
	UserPositionResult userPositionRes = userPositionResult.read();
	ReadSensorsResult readSensorsRes = readSensorsResult.read();
	if (userPositionRes.instant == prevResult.userPositionInstant && readSensorsRes.instant == prevResult.readSensorsInstant) {
		// No change
		return ComputeResult(prevResult);
	}
	if (!userPositionRes.isSuccess) {
		ComputeResult result(prevResult);
		result.isSuccess = false;
		result.errorMessage = std::wstring(userPositionRes.errorMessage);
		return result;
	}
	if (!readSensorsRes.isSuccess) {
		ComputeResult result(prevResult);
		result.isSuccess = false;
		result.errorMessage = std::wstring(readSensorsRes.errorMessage);
		return result;
	}
	GeoPoint userPosition = GeoPoint(userPositionRes.position);
	bool found = false;
	double minDistance = (std::numeric_limits<double>::max)();
	Mote nearestActiveMote;
	std::vector<Mote> motes(MOTES);
	for (auto& mote : motes) {
		if (has(readSensorsRes.motes, mote.moteId)) {
			mote.updateWithSensorsData(readSensorsRes.motes.at(mote.moteId));
			double distance = mote.position.distanceWith(userPosition);
			if (distance < minDistance) {
				found = true;
				minDistance = distance;
				nearestActiveMote = Mote(mote);
			}
		}
	}
	bool isSuccess = found;
	std::wstring errorMessage = found ? L"" : L"No active mote, cannot compute the nearest one";
	return ComputeResult(userPositionRes.instant, readSensorsRes.instant, timestampNow(), isSuccess, errorMessage, motes, userPosition, nearestActiveMote);
}

void readSensorsThreadFunc() {
	while (readSensorsRunning.read()) {
		readSensorsResult.update([](ReadSensorsResult prevResult) { return readSensors(apiUrl); });
		std::this_thread::sleep_for(std::chrono::milliseconds(READ_SENSORS_SLEEP_MILLISECONDS));
	}
}

void computeThreadFunc() {
	while (computeRunning.read()) {
		computeResult.update([](ComputeResult prevResult) { return compute(prevResult); });
		std::this_thread::sleep_for(std::chrono::milliseconds(COMPUTE_SLEEP_MILLISECONDS));
	}
}
	

void Gps::init() {
	geolocator = ref new Geolocator();
	geolocator->DesiredAccuracyInMeters = GPS_SLEEP_METERS;
	geolocator->ReportInterval = GPS_SLEEP_MILLISECONDS;
	geolocator->AllowFallbackToConsentlessPositions();
	geolocator->PositionChanged += ref new TypedEventHandler<Geolocator^, PositionChangedEventArgs^>(this, &Gps::positionUpdated);
}

void Gps::positionUpdated(Geolocator^ geolocator, PositionChangedEventArgs^ args)
{
	double latitude = args->Position->Coordinate->Latitude;
	double longitude = args->Position->Coordinate->Longitude;
	userPositionRes = UserPositionResult(timestampNow(), true, L"", GeoPoint(latitude, longitude));
	if (useGps.read()) {
		userPositionResult.update([this](UserPositionResult prevResult) { return UserPositionResult(userPositionRes); });
	}
}

std::thread readSensorsThread(readSensorsThreadFunc);
std::thread computeThread(computeThreadFunc);

MainPage::MainPage()
{
	InitializeComponent();

	DispatcherTimer^ timer = ref new DispatcherTimer;
	TimeSpan timeSpan;
	timeSpan.Duration = 10000LL * UI_REFRESH_SLEEP_MILLISECONDS;
	timer->Interval = timeSpan;
	timer->Start();
	timer->Tick += ref new EventHandler<Object^>(this, &MainPage::onTick);

	gps.init();
	computeInstant = 0;
}

void MainPage::updateUi()
{

}

void MainPage::onTick(Platform::Object^ sender, Platform::Object^ args)
{
	this->updateUi();
}
