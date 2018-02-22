#ifndef __VR_SYSTEM_FAKE_H__
#define __VR_SYSTEM_FAKE_H__

#include "vr_system.h"

#include "Matrices.h"

#include <array>

class VRSystem_Fake final : public IVRSystem
{
	struct VRFakeDevice
	{
		Vector3 position;
		Vector3 rotation;
		Vector3 velocity;
	};

	static const int FakeDevicesCount = 3; // Headset and two controllers
	static const int FakeHeadsetIdx = 0;
	static const int FakePrimaryControllerIdx = 1;
	static const int FakeSecondaryControllerIdx = 2;

private:
	VRSystem_Fake();

public:
	bool Init() override;
	void Shutdown() override;
	void SetTrackingSpace(VRTrackingSpace trackingSpace) override;
	void WaitGetPoses(std::vector<VRTrackedDevicePose>& trackedPoses) override;
	int GetMaxTrackedDevices() override;
	void SubmitImage(VREye eye, uint32_t textureHandle) override;
	void PostPresentHandoff() override;
	void GetRecommendedRenderTargetSize(uint32_t& outWidth, uint32_t& outHeight) override;
	Matrix4 GetProjectionMatrix(VREye eye, float nearZ, float farZ) override;
	Matrix4 GetEyeToHeadTransform(VREye eye) override;
	Matrix4 GetRawZeroPoseToStandingAbsoluteTrackingPose() override;
	VRTrackedDeviceIndex GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole role) override;
	bool PollNextEvent(VREvent& outEvent) override;
	VRTrackedControllerRole GetControllerRoleForTrackedDeviceIndex(VRTrackedDeviceIndex deviceIndex) override;
	bool GetControllerState(VRTrackedDeviceIndex controllerDeviceIndex, VRControllerState& outControllerState) override;

public:
	static VRSystem_Fake& Instance();

private:
	std::array<VRFakeDevice, FakeDevicesCount> fakeDevices;
};

#endif