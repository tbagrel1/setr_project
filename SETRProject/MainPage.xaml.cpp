#include "pch.h"

//
// MainPage.xaml.cpp
// Implémentation de la classe MainPage.
//

#include <ctime>
#include <iostream>
#include <limits>
#include "MainPage.xaml.h"
#include "Mote.h"
#include "ComputeResult.h"
#include "Mutexed.h"
#include "ReadSensorsResult.h"
#include "UserPositionResult.h"
#include "TimeUtils.h"
#include "json.hpp"
#include "StringUtils.h"

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
using namespace Windows::UI::Xaml::Controls::Maps;
using namespace Windows::Web::Http;
using namespace Windows::Devices::Geolocation;
using namespace Windows::Storage::Streams;

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
	Mote(L"9.138", GeoPosition(48.669422, 6.155112), L"Amphi Nord"),
	Mote(L"111.130", GeoPosition(48.668837, 6.154990), L"Amphi Sud"),
	Mote(L"151.105", GeoPosition(48.668922, 6.155363), L"Salle E-1.22"),
	Mote(L"32.131", GeoPosition(48.669400, 6.155340), L"Salle N-0.3"),
	Mote(L"97.145", GeoPosition(48.669439, 6.155265), L"Bureau 2.6"),
	Mote(L"120.99", GeoPosition(48.669419, 6.155269), L"Bureau 2.7"),
	Mote(L"200.124", GeoPosition(48.669394, 6.155287), L"Bureau 2.8"),
	Mote(L"53.105", GeoPosition(48.669350, 6.155310), L"Bureau 2.9"),
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
			errorMessage = L"Unable to read API response body";
			// Read body exception
		}
	}
	catch (Platform::Exception ^ex) {
		isSuccess = false;
		errorMessage = L"Unable to join the server";
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
	GeoPosition userPosition = GeoPosition(userPositionRes.position);
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
	bool isSuccess;
	std::wstring errorMessage;
	if (!userPositionRes.isSuccess) {
		isSuccess = false;
		errorMessage = std::wstring(userPositionRes.errorMessage);
	}
	else if (!readSensorsRes.isSuccess) {
		isSuccess = false;
		errorMessage = std::wstring(readSensorsRes.errorMessage);
	}
	else {
		isSuccess = found;
		errorMessage = found ? L"" : L"No active mote, cannot compute the nearest one";
	}
	return ComputeResult(userPositionRes.instant, readSensorsRes.instant, timestampNow(), isSuccess, errorMessage, motes, userPosition, nearestActiveMote);
}

void readSensorsThreadFunc() {
	while (readSensorsRunning.read()) {
		ReadSensorsResult newResult = readSensors(apiUrl);
		readSensorsResult.update([newResult](ReadSensorsResult prevResult) { return newResult; });
		std::this_thread::sleep_for(std::chrono::milliseconds(READ_SENSORS_SLEEP_MILLISECONDS));
	}
}

void computeThreadFunc() {
	while (computeRunning.read()) {
		ComputeResult newResult = compute(computeResult.read());
		computeResult.update([newResult](ComputeResult prevResult) { return newResult; });
		std::this_thread::sleep_for(std::chrono::milliseconds(COMPUTE_SLEEP_MILLISECONDS));
	}
}
	
PositionStatus convertToPositionStatus(GeolocationAccessStatus accessStatus) {
	switch (accessStatus) {
	case GeolocationAccessStatus::Allowed:
		return PositionStatus::Initializing;
	case GeolocationAccessStatus::Denied:
		return PositionStatus::Disabled;
	default:
		return PositionStatus::NotAvailable;
	}
}

void Gps::init(GpsStatusChangedCallback^ _statusChangedCallback) {
	statusChangedCallback = _statusChangedCallback;
	geolocator = ref new Geolocator();
	geolocator->DesiredAccuracyInMeters = GPS_SLEEP_METERS;
	geolocator->ReportInterval = GPS_SLEEP_MILLISECONDS;
	concurrency::create_task(geolocator->RequestAccessAsync()).then([this](GeolocationAccessStatus status) { this->statusChanged(convertToPositionStatus(status)); });
	// geolocator->AllowFallbackToConsentlessPositions();
	geolocator->PositionChanged += ref new TypedEventHandler<Geolocator^, PositionChangedEventArgs^>(this, &Gps::positionUpdated);
	geolocator->StatusChanged += ref new TypedEventHandler<Geolocator^, StatusChangedEventArgs^>(this, &Gps::_statusChanged);
}

double SETRProject::Gps::getLastLatitude()
{
	return userPositionRes.position.latitude;
}

double SETRProject::Gps::getLastLongitude()
{
	return userPositionRes.position.longitude;
}


void Gps::positionUpdated(Geolocator^ geolocator, PositionChangedEventArgs^ args)
{
	userPositionRes.position.latitude = args->Position->Coordinate->Point->Position.Latitude;
	userPositionRes.position.longitude = args->Position->Coordinate->Point->Position.Longitude;
	userPositionRes.instant = timestampNow();
	userPositionRes.isSuccess = true;
	userPositionRes.errorMessage = L"";
	if (useGps.read()) {
		userPositionResult.update([this](UserPositionResult prevResult) { return UserPositionResult(userPositionRes); });
	}
}

void SETRProject::Gps::_statusChanged(Windows::Devices::Geolocation::Geolocator ^ geolocator, Windows::Devices::Geolocation::StatusChangedEventArgs ^ args)
{
	statusChanged(args->Status);
}

void SETRProject::Gps::statusChanged(PositionStatus newStatus)
{
	bool isSuccess;
	std::wstring errorMessage;
	switch (newStatus)
	{
	case PositionStatus::Ready:
		// Location platform is providing valid data.
		isSuccess = true;
		errorMessage = L"";
		break;

	case PositionStatus::Initializing:
		// Location platform is attempting to acquire a fix.
		isSuccess = true;
		errorMessage = L"";
		break;

	case PositionStatus::NoData:
		// Location platform could not obtain location data.
		isSuccess = false;
		errorMessage = L"Not able to determine the location.";
		break;

	case PositionStatus::Disabled:
		// The permission to access location data is denied by the user or other policies.
		isSuccess = false;
		errorMessage = L"Access to location is denied.";
		break;

	case PositionStatus::NotInitialized:
		// The location platform is not initialized. This indicates that the application
		// has not made a request for location data.
		isSuccess = false;
		errorMessage = L"No request for location is made yet.";
		break;

	case PositionStatus::NotAvailable:
		// The location platform is not available on this version of the OS.
		isSuccess = false;
		errorMessage = L"Location is not available on this version of the OS.";
		break;

	default:
		isSuccess = false;
		errorMessage = L"Unknown GPS status";
		break;
	}
	userPositionRes.instant = timestampNow();
	userPositionRes.isSuccess = isSuccess;
	userPositionRes.errorMessage = errorMessage;
	statusChangedCallback(newStatus);
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

	gps = ref new Gps();
	gps->init(ref new GpsStatusChangedCallback([this](PositionStatus newStatus) { this->gpsStatusChanged(newStatus); }));
	hideDisabledLocationMessages();
	computeInstant = 0;
	useGpsCheckBox->IsChecked = false;
	useGps.update([](bool prev) { return false; });
	switchToManualGeolocation();
	updateUi();
}

void MainPage::updateUi()
{
	ComputeResult computeRes = computeResult.read();
	if (computeRes.instant == computeInstant) {
		return;
	}
	computeInstant = computeRes.instant;
	if (useGps.read()) {
		userLatitudeTextBlock->Text = computeRes.userPosition.latitude.ToString();
		userLongitudeTextBlock->Text = computeRes.userPosition.longitude.ToString();
	}
	nearestMoteNameTextBlock->Text = Platform::String::Concat("Mote active la plus proche : ",  ref new Platform::String(computeRes.nearestActiveMote.moteId.c_str()));
	nearestMoteLocationNameTextBlock->Text = ref new Platform::String(computeRes.nearestActiveMote.positionName.c_str());
	nearestMoteLatitudeTextBlock->Text = computeRes.nearestActiveMote.position.latitude.ToString();
	nearestMoteLongitudeTextBlock->Text = computeRes.nearestActiveMote.position.longitude.ToString();
	nearestMoteTemperatureTextBlock->Text = Platform::String::Concat(computeRes.nearestActiveMote.temperature.ToString(),"°C");
	errorTextBlock->Text = ref new Platform::String(computeRes.errorMessage.c_str());
	updateMap();
}

void MainPage::onTick(Platform::Object^ sender, Platform::Object^ args)
{
	this->updateUi();
}

void MainPage::switchToGps() {
	refreshButton->IsEnabled = false;
	userLatitudeTextBlock->IsEnabled = false;
	userLongitudeTextBlock->IsEnabled = false;
	useGps.update([](bool prev) { return true; });
	double lastLatitude = gps->getLastLatitude();
	double lastLongitude = gps->getLastLongitude();
	if (lastLatitude != 0.0 || lastLongitude != 0.0) {
		userPositionResult.update([lastLatitude, lastLongitude](UserPositionResult prevResult) { return UserPositionResult(timestampNow(), true, L"", GeoPosition(lastLatitude, lastLongitude)); });
	}
}

void MainPage::switchToManualGeolocation() {
	useGps.update([](bool prev) { return false; });
	refreshButton->IsEnabled = true;
	userLatitudeTextBlock->IsEnabled = true;
	userLongitudeTextBlock->IsEnabled = true;
}

void SETRProject::MainPage::showDisabledLocationMessages()
{
	locationDisabledMessage1->Visibility = Windows::UI::Xaml::Visibility::Visible;
	locationDisabledMessage2->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

void SETRProject::MainPage::hideDisabledLocationMessages()
{
	locationDisabledMessage1->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	locationDisabledMessage2->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void SETRProject::MainPage::gpsStatusChanged(Windows::Devices::Geolocation::PositionStatus newStatus)
{
	locationDisabledMessage1->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this, newStatus] {
		switch (newStatus)
		{
		case PositionStatus::NoData:
		case PositionStatus::Disabled:
		case PositionStatus::NotAvailable:
			this->showDisabledLocationMessages();
			break;
		default:
			this->hideDisabledLocationMessages();
			break;
		}
	}));
}

MapIcon^ createPin(Mote mote, bool isNearest) {
	BasicGeoposition pos = BasicGeoposition();
	pos.Latitude = mote.position.latitude;
	pos.Longitude = mote.position.longitude;
	Geopoint^ point = ref new Geopoint(pos);
	MapIcon^ pin = ref new MapIcon();
	pin->Location = point;
	pin->NormalizedAnchorPoint = Point(0.5, 1.0);
	pin->ZIndex = isNearest ? 3 : mote.active ? 2 : 1;
	pin->Title = mote.active ? Platform::String::Concat(mote.temperature.ToString(), "°C")  : ref new Platform::String();
	pin->Image = isNearest ? RandomAccessStreamReference::CreateFromUri(ref new Uri{ L"ms-appx:///Assets/pin_nearest.png" }) : mote.active ? RandomAccessStreamReference::CreateFromUri(ref new Uri{ L"ms-appx:///Assets/pin_default.png" }) : RandomAccessStreamReference::CreateFromUri(ref new Uri{ L"ms-appx:///Assets/pin_disabled.png" });
	return pin;
}

void SETRProject::MainPage::updateMap()
{
	// ComputeResult cannot be passed as parameter of this method because ComputeResult is not a managed class
	// So we are forced to lock again
	ComputeResult computeRes = computeResult.read();
	if (computeRes.motes.empty()) {
		return;
	}
	IVector<MapElement^>^ pins = ref new Platform::Collections::Vector<MapElement^>;
	std::wstring nearestModeId = computeRes.nearestActiveMote.moteId;
	double minLatitude = computeRes.motes.at(0).position.latitude;
	double maxLatitude = computeRes.motes.at(0).position.latitude;
	double minLongitude = computeRes.motes.at(0).position.longitude;
	double maxLongitude = computeRes.motes.at(0).position.longitude;
	myMap->MapElements->Clear();
	for (Mote mote : computeRes.motes) {
		if (mote.position.latitude > maxLatitude) { maxLatitude = mote.position.latitude; }
		if (mote.position.latitude < minLatitude) { minLatitude = mote.position.latitude; }
		if (mote.position.longitude > maxLongitude) { maxLongitude = mote.position.longitude; }
		if (mote.position.longitude < minLongitude) { minLongitude = mote.position.longitude; }
		MapIcon^ pin = createPin(mote, mote.moteId == nearestModeId);
		pins->Append(pin);
		myMap->MapElements->Append(pin);
	}
	if (computeRes.userPosition.latitude > maxLatitude) { maxLatitude = computeRes.userPosition.latitude; }
	if (computeRes.userPosition.latitude < minLatitude) { minLatitude = computeRes.userPosition.latitude; }
	if (computeRes.userPosition.longitude > maxLongitude) { maxLongitude = computeRes.userPosition.longitude; }
	if (computeRes.userPosition.longitude < minLongitude) { minLongitude = computeRes.userPosition.longitude; }
	// create user pin
	BasicGeoposition pos = BasicGeoposition();
	pos.Latitude = computeRes.userPosition.latitude;
	pos.Longitude = computeRes.userPosition.longitude;
	Geopoint^ point = ref new Geopoint(pos);
	MapIcon^ pin = ref new MapIcon();
	pin->Location = point;
	pin->NormalizedAnchorPoint = Point(0.5, 1.0);
	pin->ZIndex = 4;
	pin->Title = ref new Platform::String();
	pin->Image = RandomAccessStreamReference::CreateFromUri(ref new Uri{ L"ms-appx:///Assets/user.png" });
	pins->Append(pin);
	myMap->MapElements->Append(pin);
	double latitudeMargin = (maxLatitude - minLatitude) * 0.25;
	double longitudeMargin = (maxLongitude - minLongitude) * 0.25;
	BasicGeoposition northWestCorner;
	northWestCorner.Latitude = maxLatitude + latitudeMargin;
	northWestCorner.Longitude = minLongitude - longitudeMargin;
	BasicGeoposition southEastCorner;
	southEastCorner.Latitude = minLatitude - latitudeMargin;
	southEastCorner.Longitude = maxLongitude + longitudeMargin;
	GeoboundingBox^ area = ref new GeoboundingBox(northWestCorner, southEastCorner);
	myMap->TrySetSceneAsync(MapScene::CreateFromBoundingBox(area));
}

void SETRProject::MainPage::Button_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	double latitude = _wtof(userLatitudeTextBlock->Text->Data());
	double longitude = _wtof(userLongitudeTextBlock->Text->Data());
	userPositionResult.update([latitude, longitude](UserPositionResult prevResult) { return UserPositionResult(timestampNow(), true, L"", GeoPosition(latitude, longitude)); });
}


void SETRProject::MainPage::UserLongitudeTextBlock_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{

}


void SETRProject::MainPage::UserLatitudeTextBlock_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{

}


void SETRProject::MainPage::CheckBox_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	switchToGps();
}


void SETRProject::MainPage::CheckBox_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	switchToManualGeolocation();
}


void SETRProject::MainPage::MyMap_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
}
