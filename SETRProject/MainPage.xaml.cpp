//
// MainPage.xaml.cpp
// Implémentation de la classe MainPage.
//

#include <ctime>

#include "pch.h"
#include "MainPage.xaml.h"
#include "FindNearestResult.h"
#include "Mutexed.h"
#include "ReadSensorsResult.h"
#include "UserPositionResult.h"
#include "TimeUtils.h"

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

// Pour plus d'informations sur le modèle d'élément Page vierge, consultez la page https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

#define READ_SENSORS_SLEEP_MILLISECONDS 10000L
#define FIND_NEAREST_SLEEP_MILLISECONDS 10000L
#define UI_REFRESH_SLEEP_MILLISECONDS 100L

Mutexed<ReadSensorsResult> readSensorsResult;
Mutexed<bool> readSensorsRunning(true);
Mutexed<FindNearestResult> findNearestResult;
Mutexed<bool> findNearestRunning(true);
Mutexed<UserPositionResult> userPositionResult;

void readSensorsThreadFunc() {
	while (readSensorsRunning.read()) {
		// TODO: read sensors
		readSensorsResult.update([](ReadSensorsResult prevResult) { return ReadSensorsResult(timestampNow(), true, L"", std::vector<TempPoint>()); });
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
