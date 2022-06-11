#include "TrackerDevice.hpp"
#include "VRDriver.hpp"
#include <iostream>
#include <fstream>
#include <string>
#define WIN32_LEAN_AND_MEAN
// Windows Header Files:
#include <windows.h>

VRTri::TrackerDevice::TrackerDevice(std::string serial) :
	serial_(serial)
{
}

std::string VRTri::TrackerDevice::GetSerial()
{
	return this->serial_;
}

void VRTri::TrackerDevice::Update()
{
	//vr::VRDriverLog()->Log("Updating!");
	if (this->device_index_ == vr::k_unTrackedDeviceIndexInvalid)
		return;

	// Check if this device was asked to be identified
	auto events = GetDriver()->GetOpenVREvents();
	for (auto event : events) {
		// Note here, event.trackedDeviceIndex does not necissarily equal this->device_index_, not sure why, but the component handle will match so we can just use that instead
		//if (event.trackedDeviceIndex == this->device_index_) {
		if (event.eventType == vr::EVREventType::VREvent_Input_HapticVibration) {
			if (event.data.hapticVibration.componentHandle == this->haptic_component_) {
				this->did_vibrate_ = true;
			}
		}
		//}
	}

	// Setup pose for this frame
	auto pose = IVRDevice::MakeDefaultPose();
	// Find a HMD
	auto devices = GetDriver()->GetDevices();
	auto trckr = std::find_if(devices.begin(), devices.end(), [](const std::shared_ptr<IVRDevice>& device_ptr) {return device_ptr->GetDeviceType() == DeviceType::TRACKER; });
	//IO
	//vr::VRDriverLog()->Log("GOT TO I/O");
	std::string rawdata;
	std::ifstream dataFile;
	std::vector<double> data;
	try
	{
		dataFile.open("C:/temp/File.txt");
		int i = 1;
		data.clear();
		while (i <= 8) {
			std::getline(dataFile, rawdata);
			//vr::VRDriverLog()->Log("GOT LINE DATA: "+ i);
			data.push_back(std::stod(rawdata));
			i++;
		}
		dataFile.close();
	}
	catch (const std::exception&)
	{
		vr::VRDriverLog()->Log("Caught File Exception");
		
	}

	//vr::VRDriverLog()->Log("FILE OPENED");
	//vr::VRDriverLog()->Log("FILE CLOSED");
	//order = [pelvis],[right_ankle],[left_ankle],frame_id
	//Process Inbound Data
	if (trckr != devices.end()) {
		//vr::DriverPose_t trckr_pose = (*trckr)->GetPose();

		if (this->GetSerial() == "TrackerDevice1") { //hip  
			pose.vecPosition[0] = data[0];
			pose.vecPosition[1] = data[1];
			pose.vecPosition[2] = data[2];
			//vr::VRDriverLog()->Log("Updating Tracker 1");
		}
		else if (this->GetSerial() == "TrackerDevice2") { //right ankle
			pose.vecPosition[0] = data[3];
			pose.vecPosition[1] = data[4];
			pose.vecPosition[2] = data[5];
			//vr::VRDriverLog()->Log("Updating Tracker 2");
		}
		else if (this->GetSerial() == "TrackerDevice3") { //left ankle
			pose.vecPosition[0] = data[6];
			pose.vecPosition[1] = data[7];
			pose.vecPosition[2] = data[8];
			//vr::VRDriverLog()->Log("Updating Tracker 3");
		}
		else {
			linalg::vec<float, 3> trckr_position{ (1, 1, 1) };
			pose.vecPosition[0] = trckr_position.x;
			pose.vecPosition[1] = trckr_position.y;
			pose.vecPosition[2] = trckr_position.z;
			vr::VRDriverLog()->Log("Error : Pose stalled");
		}

		pose.qRotation.w = 0;
		pose.qRotation.x = 0;
		pose.qRotation.y = 0;
		pose.qRotation.z = 0;
	}
	else {
		vr::VRDriverLog()->Log("devices.end Error");
	}
	// Post pose
	GetDriver()->GetDriverHost()->TrackedDevicePoseUpdated(this->device_index_, pose, sizeof(vr::DriverPose_t));
	this->last_pose_ = pose;
}
//
DeviceType VRTri::TrackerDevice::GetDeviceType()
{
	return DeviceType::TRACKER;
}

vr::TrackedDeviceIndex_t VRTri::TrackerDevice::GetDeviceIndex()
{
	return this->device_index_;
}

vr::EVRInitError VRTri::TrackerDevice::Activate(uint32_t unObjectId)
{
	this->device_index_ = unObjectId;

	GetDriver()->Log("Activating tracker " + this->serial_);

	// Get the properties handle
	auto props = GetDriver()->GetProperties()->TrackedDeviceToPropertyContainer(this->device_index_);

	// Set some universe ID (Must be 2 or higher)
	GetDriver()->GetProperties()->SetUint64Property(props, vr::Prop_CurrentUniverseId_Uint64, 2);

	// Set up a model "number" (not needed but good to have)
	GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_ModelNumber_String, "VRomp");

	// Opt out of hand selection
	GetDriver()->GetProperties()->SetInt32Property(props, vr::Prop_ControllerRoleHint_Int32, vr::ETrackedControllerRole::TrackedControllerRole_OptOut);

	// Set up a render model path
	GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_RenderModelName_String, "vr_controller_05_wireless_b");

	// Set controller profile
	GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_InputProfilePath_String, "{VRomp}/input/VRomp_tracker_bindings.json");

	// Set the icon
	GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceReady_String, "{VRomp}/icons/tracker_ready.png");

	GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceOff_String, "{VRomp}/icons/tracker_not_ready.png");
	GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceSearching_String, "{VRomp}/icons/tracker_not_ready.png");
	GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceSearchingAlert_String, "{VRomp}/icons/tracker_not_ready.png");
	GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceReadyAlert_String, "{VRomp}/icons/tracker_not_ready.png");
	GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceNotReady_String, "{VRomp}/icons/tracker_not_ready.png");
	GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceStandby_String, "{VRomp}/icons/tracker_not_ready.png");
	GetDriver()->GetProperties()->SetStringProperty(props, vr::Prop_NamedIconPathDeviceAlertLow_String, "{VRomp}/icons/tracker_not_ready.png");
	return vr::EVRInitError::VRInitError_None;
}

void VRTri::TrackerDevice::Deactivate()
{
	this->device_index_ = vr::k_unTrackedDeviceIndexInvalid;
}

void VRTri::TrackerDevice::EnterStandby()
{
}

void* VRTri::TrackerDevice::GetComponent(const char* pchComponentNameAndVersion)
{
	return nullptr;
}

void VRTri::TrackerDevice::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize)
{
	if (unResponseBufferSize >= 1)
		pchResponseBuffer[0] = 0;
}

vr::DriverPose_t VRTri::TrackerDevice::GetPose()
{
	return last_pose_;
}