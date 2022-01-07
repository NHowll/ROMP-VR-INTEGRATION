#pragma once

#include <chrono>
#include <cmath>
#include <linalg.h>
#include <Driver/IVRDevice.hpp>
#include <Native/DriverFactory.hpp>
//Socket
#include<WinSock2.h>

namespace VRTri {
    class TrackerDevice : public IVRDevice {
    public:

        TrackerDevice(std::string serial);
        ~TrackerDevice() = default;

        // Inherited via IVRDevice
        virtual std::string GetSerial() override;
        virtual void Update() override;
        virtual vr::TrackedDeviceIndex_t GetDeviceIndex() override;
        virtual DeviceType GetDeviceType() override;

        virtual vr::EVRInitError Activate(uint32_t unObjectId) override;

        virtual int Socket();
        virtual void Deactivate() override;
        virtual void EnterStandby() override;
        virtual void* GetComponent(const char* pchComponentNameAndVersion) override;
        virtual void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;
        virtual vr::DriverPose_t GetPose() override;

    private:
        vr::TrackedDeviceIndex_t device_index_ = vr::k_unTrackedDeviceIndexInvalid;
        std::string serial_;
        vr::DriverPose_t last_pose_ = IVRDevice::MakeDefaultPose();
        //Private socket stuff
        int clientsocket;
		int data;
        char buffer[1000];
        int serverSock = socket(AF_INET, SOCK_STREAM, 0);
        //Unused Haptics
        bool did_vibrate_ = false;
        float vibrate_anim_state_ = 0.f;
        vr::VRInputComponentHandle_t haptic_component_ = 0;
        vr::VRInputComponentHandle_t system_click_component_ = 0;
        vr::VRInputComponentHandle_t system_touch_component_ = 0;
    };
};