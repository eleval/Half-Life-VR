#ifndef __VR_SYSTEM_OPENVR_H__
#define __VR_SYSTEM_OPENVR_H__

#include "vr_system.h"

#include "openvr/openvr.h"

class VRSystem_OpenVR final : public IVRSystem
{
private:
	VRSystem_OpenVR();

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
	static VRSystem_OpenVR& Instance();

private:
	vr::IVRSystem* vrSystem;
	vr::IVRCompositor* vrCompositor;
};

#endif