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
	std::vector<double> fallbackData;
	std::vector<double> data;
	fallbackData = data;
	try
	{
		dataFile.open("C:/temp/File.txt");
		if (!dataFile.is_open())
		{
			vr::VRDriverLog()->Log("File Unable to open");
			return;
		}
		int i = 0;
		data.clear();
		while (i < 19) {
			std::getline(dataFile, rawdata);
			//vr::VRDriverLog()->Log("GOT LINE DATA: "+ i);
			if (!rawdata.empty())
				data.push_back(std::stod(rawdata));
			i++;
		}
		dataFile.close();
	}
	//Known Exceptions
	//vrserver - VRomp : Caught File Exception [Fixed?]
	//vrserver - Exception c0000005 []
	//vrserver - VRomp : invalid stod argument [Fixed?]

	catch (const std::exception &exc)
	{
		vr::VRDriverLog()->Log("Caught File Exception");
		vr::VRDriverLog()->Log(exc.what());
		dataFile.close();
		data = fallbackData;

	}
	//int multConst = -3.28084;
	int multConst = -1;
	//OFFSETS
	//Tracker1
	double t1xoff = 0;
	double t1yoff = .65;
	double t1zoff = 0;
	//Tracker2
	double t2xoff = 0;
	double t2yoff = .65;
	double t2zoff = 0;
	//Tracker3
	double t3xoff = 0;
	double t3yoff = .65;
	double t3zoff = 0;
	//
	//order = [pelvis],[right_ankle],[left_ankle],frame_id
	//Process Inbound Data
	if (trckr != devices.end()) {
		//vr::DriverPose_t trckr_pose = (*trckr)->GetPose();

		if (this->GetSerial() == "TrackerDevice1") { //hip  
			pose.vecPosition[0] = (data[0] * multConst)+t1xoff;
			pose.vecPosition[1] = (data[1] * multConst)+t1yoff;
			pose.vecPosition[2] = (data[2] * multConst)+t1zoff;
			pose.qRotation.w = 0;
			pose.qRotation.x = data[9];
			pose.qRotation.y = 0;//data[10];
			pose.qRotation.z = data[11];
			//vr::VRDriverLog()->Log("Updating Tracker 1");
		}
		else if (this->GetSerial() == "TrackerDevice2") { //right ankle
			pose.vecPosition[0] = (data[3] * multConst)+t2xoff;
			pose.vecPosition[1] = (data[4] * multConst)+t2yoff;
			pose.vecPosition[2] = (data[5] * multConst)+t2zoff;
			pose.qRotation.w = 0;
			pose.qRotation.x = data[12];
			pose.qRotation.y = data[13];
			pose.qRotation.z = data[14];
			//vr::VRDriverLog()->Log("Updating Tracker 2");
		}
		else if (this->GetSerial() == "TrackerDevice3") { //left ankle
			pose.vecPosition[0] = (data[6] * multConst)+t3xoff;
			pose.vecPosition[1] = (data[7] * multConst)+t3yoff;
			pose.vecPosition[2] = (data[8] * multConst)+t3zoff;
			pose.qRotation.w = 0;
			pose.qRotation.x = data[15];
			pose.qRotation.y = data[16];
			pose.qRotation.z = data[17];
			//vr::VRDriverLog()->Log("Updating Tracker 3");
		}
		else {
			pose = this->last_pose_;
			vr::VRDriverLog()->Log("Error : Pose stalled");
		}
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