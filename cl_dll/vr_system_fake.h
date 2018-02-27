#ifndef __VR_SYSTEM_FAKE_H__
#define __VR_SYSTEM_FAKE_H__

#include "vr_system.h"

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

#include <array>
#include <queue>

class VRSystem_Fake final : public IVRSystem
{
	struct VRFakeDevice
	{
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 velocity;
	};

	static const int FakeDevicesCount = 3; // Headset and two controllers
	static const VRTrackedDeviceIndex FakeHeadsetIdx = 0;
	static const VRTrackedDeviceIndex FakeLeftHandControllerIdx = 1;
	static const VRTrackedDeviceIndex FakeRightHandControllerIdx = 2;

private:
	VRSystem_Fake();

public:
	bool Init() override;
	void Shutdown() override;
	void Update() override;
	void SetTrackingSpace(VRTrackingSpace trackingSpace) override;
	void WaitGetPoses(std::vector<VRTrackedDevicePose>& trackedPoses) override;
	int GetMaxTrackedDevices() override;
	void SubmitImage(VREye eye, uint32_t textureHandle) override;
	void PostPresentHandoff() override;
	void GetRecommendedRenderTargetSize(uint32_t& outWidth, uint32_t& outHeight) override;
	glm::mat4 GetProjectionMatrix(VREye eye, float nearZ, float farZ) override;
	glm::mat4 GetEyeToHeadTransform(VREye eye) override;
	glm::mat4 GetRawZeroPoseToStandingAbsoluteTrackingPose() override;
	VRTrackedDeviceIndex GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole role) override;
	bool PollNextEvent(VREvent& outEvent) override;
	VRTrackedControllerRole GetControllerRoleForTrackedDeviceIndex(VRTrackedDeviceIndex deviceIndex) override;
	bool GetControllerState(VRTrackedDeviceIndex controllerDeviceIndex, VRControllerState& outControllerState) override;

public:
	static VRSystem_Fake& Instance();

private:
	void SetControllerButtonState(VRControllerState& controller, VRButton button, bool pressed);

private:
	std::array<VRFakeDevice, FakeDevicesCount> fakeDevices;
	std::array<VRControllerState, FakeDevicesCount> fakeControllers;

	std::queue<VREvent> eventQueue;

	float prevTime;
	int controlledDeviceIdx;
	bool justChangedControlledDevice;
};

#endif