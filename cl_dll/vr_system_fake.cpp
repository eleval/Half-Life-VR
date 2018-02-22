#include "Matrices.h"
#include "hud.h"
#include "cl_util.h"
#include "r_studioint.h"
#include "ref_params.h"

#include "vr_system_fake.h"

#include <cassert>

VRSystem_Fake::VRSystem_Fake()
{

}

bool VRSystem_Fake::Init()
{
	return true;
}

void VRSystem_Fake::Shutdown()
{

}

void VRSystem_Fake::SetTrackingSpace(VRTrackingSpace trackingSpace)
{
	// We don't care about that in fake mode
}

void VRSystem_Fake::WaitGetPoses(std::vector<VRTrackedDevicePose>& trackedPoses)
{
	assert(trackedPoses.size() >= FakeDevicesCount);

	for (int i = 0; i < FakeDevicesCount; ++i)
	{
		VRTrackedDevicePose& trackedPose = trackedPoses[i];
		VRFakeDevice& fakeDevice = fakeDevices[i];

		trackedPose.isValid = true;
		trackedPose.transform.identity();
		trackedPose.transform.translate(fakeDevice.position);
		trackedPose.transform.rotateX(fakeDevice.rotation.x);
		trackedPose.transform.rotateY(fakeDevice.rotation.y);
		trackedPose.transform.rotateZ(fakeDevice.rotation.z);
		trackedPose.velocity.x = fakeDevice.velocity.x;
		trackedPose.velocity.y = fakeDevice.velocity.y;
		trackedPose.velocity.z = fakeDevice.velocity.z;
	}
}

int VRSystem_Fake::GetMaxTrackedDevices()
{
	return FakeDevicesCount;
}

void VRSystem_Fake::SubmitImage(VREye eye, uint32_t textureHandle)
{
	// No image to submit since we have no headset
}

void VRSystem_Fake::PostPresentHandoff()
{
	// Nothing to present since no headset
}

void VRSystem_Fake::GetRecommendedRenderTargetSize(uint32_t& outWidth, uint32_t& outHeight)
{
	// Return dummy size that make sense (same as HTC Vive)
	outWidth = 1200;
	outHeight = 1080;
}

Matrix4 VRSystem_Fake::GetProjectionMatrix(VREye eye, float nearZ, float farZ)
{
	// Since we have nothing to render, we don't care about that, we will just return an identity matrix
	Matrix4 dummy;
	return dummy;
}

Matrix4 VRSystem_Fake::GetEyeToHeadTransform(VREye eye)
{
	VRFakeDevice& fakeDevice = fakeDevices[FakeHeadsetIdx];

	Matrix4 transform;
	transform.translate(fakeDevice.position);
	transform.rotateX(fakeDevice.rotation.x);
	transform.rotateY(fakeDevice.rotation.y);
	transform.rotateZ(fakeDevice.rotation.z);

	return transform;
}

Matrix4 VRSystem_Fake::GetRawZeroPoseToStandingAbsoluteTrackingPose()
{
	// TODO but isn't used
	Matrix4 dummy;
	return dummy;
}

VRTrackedDeviceIndex VRSystem_Fake::GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole role)
{
	// TODO
	return 0;
}

bool VRSystem_Fake::PollNextEvent(VREvent& outEvent)
{
	// TODO
	return false;
}

VRTrackedControllerRole VRSystem_Fake::GetControllerRoleForTrackedDeviceIndex(VRTrackedDeviceIndex deviceIndex)
{
	// TODO
	return VRTrackedControllerRole::LeftHand;
}

bool VRSystem_Fake::GetControllerState(VRTrackedDeviceIndex controllerDeviceIndex, VRControllerState& outControllerState)
{
	// TODO
	return false;
}

//////////////////////////////////////////////////////////////////////////

VRSystem_Fake& VRSystem_Fake::Instance()
{
	static VRSystem_Fake instance;
	return instance;
}
