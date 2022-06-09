#include "VRDriver.hpp"
#include <Driver/TrackerDevice.hpp>

#pragma comment(lib,"ws2_32.lib")
#include <Ws2tcpip.h>
#define SERVER_PORT htons(8887)

vr::EVRInitError VRTri::VRDriver::Init(vr::IVRDriverContext* pDriverContext)
{
	// Perform driver context initialisation
	if (vr::EVRInitError init_error = vr::InitServerDriverContext(pDriverContext); init_error != vr::EVRInitError::VRInitError_None) {
		return init_error;
	}

	Log("Activating VRomp...");
	// Add a tracker

	this->AddDevice(std::make_shared<TrackerDevice>("TrackerDevice1"));
	this->AddDevice(std::make_shared<TrackerDevice>("TrackerDevice2"));
	this->AddDevice(std::make_shared<TrackerDevice>("TrackerDevice3"));
	
	Log("VRomp Loaded Successfully");

	return vr::VRInitError_None;
}

void VRTri::VRDriver::Cleanup()
{
}

void VRTri::VRDriver::RunFrame()
{
	// Collect events
	vr::VREvent_t event;
	std::vector<vr::VREvent_t> events;
	while (vr::VRServerDriverHost()->PollNextEvent(&event, sizeof(event)))
	{
		events.push_back(event);
	}
	this->openvr_events_ = events;

	// Update frame timing
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	this->frame_timing_ = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->last_frame_time_);
	this->last_frame_time_ = now;

	// Update devices
	for (auto& device : this->devices_)
		device->Update();
}

bool VRTri::VRDriver::ShouldBlockStandbyMode()
{
	return false;
}

void VRTri::VRDriver::EnterStandby()
{
}

void VRTri::VRDriver::LeaveStandby()
{
}

std::vector<std::shared_ptr<VRTri::IVRDevice>> VRTri::VRDriver::GetDevices()
{
	return this->devices_;
}

std::vector<vr::VREvent_t> VRTri::VRDriver::GetOpenVREvents()
{
	return this->openvr_events_;
}

std::chrono::milliseconds VRTri::VRDriver::GetLastFrameTime()
{
	return this->frame_timing_;
}

bool VRTri::VRDriver::AddDevice(std::shared_ptr<IVRDevice> device)
{
	vr::ETrackedDeviceClass openvr_device_class;
	// Remember to update this switch when new device types are added
	switch (device->GetDeviceType()) {
	case DeviceType::CONTROLLER:
		openvr_device_class = vr::ETrackedDeviceClass::TrackedDeviceClass_Controller;
		break;
	case DeviceType::HMD:
		openvr_device_class = vr::ETrackedDeviceClass::TrackedDeviceClass_HMD;
		break;
	case DeviceType::TRACKER:
		openvr_device_class = vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker;
		break;
	case DeviceType::TRACKING_REFERENCE:
		openvr_device_class = vr::ETrackedDeviceClass::TrackedDeviceClass_TrackingReference;
		break;
	default:
		return false;
	}
	bool result = vr::VRServerDriverHost()->TrackedDeviceAdded(device->GetSerial().c_str(), openvr_device_class, device.get());
	if (result)
		this->devices_.push_back(device);
	return result;
}

VRTri::SettingsValue VRTri::VRDriver::GetSettingsValue(std::string key)
{
	vr::EVRSettingsError err = vr::EVRSettingsError::VRSettingsError_None;
	int int_value = vr::VRSettings()->GetInt32(settings_key_.c_str(), key.c_str(), &err);
	if (err == vr::EVRSettingsError::VRSettingsError_None) {
		return int_value;
	}
	err = vr::EVRSettingsError::VRSettingsError_None;
	float float_value = vr::VRSettings()->GetFloat(settings_key_.c_str(), key.c_str(), &err);
	if (err == vr::EVRSettingsError::VRSettingsError_None) {
		return float_value;
	}
	err = vr::EVRSettingsError::VRSettingsError_None;
	bool bool_value = vr::VRSettings()->GetBool(settings_key_.c_str(), key.c_str(), &err);
	if (err == vr::EVRSettingsError::VRSettingsError_None) {
		return bool_value;
	}
	std::string str_value;
	str_value.reserve(1024);
	vr::VRSettings()->GetString(settings_key_.c_str(), key.c_str(), str_value.data(), 1024, &err);
	if (err == vr::EVRSettingsError::VRSettingsError_None) {
		return str_value;
	}
	err = vr::EVRSettingsError::VRSettingsError_None;

	return SettingsValue();
}

void VRTri::VRDriver::Log(std::string message)
{
	std::string message_endl = message + "\n";
	vr::VRDriverLog()->Log(message_endl.c_str());
}

vr::IVRDriverInput* VRTri::VRDriver::GetInput()
{
	return vr::VRDriverInput();
}

vr::CVRPropertyHelpers* VRTri::VRDriver::GetProperties()
{
	return vr::VRProperties();
}

vr::IVRServerDriverHost* VRTri::VRDriver::GetDriverHost()
{
	return vr::VRServerDriverHost();
}