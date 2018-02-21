#ifndef __VR_INTERFACE_H__
#define __VR_INTERFACE_H__

#include "Matrices.h"

#include <vector>
#include <cstdint>

enum class VRTrackingSpace
{
	Seated,
	Standing,
	RawAndUncalibrated
};

enum class VREye
{
	Left,
	Right
};

enum class VRTrackedControllerRole
{
	Invalid = 0,					// Invalid value for controller type
	LeftHand = 1,					// Tracked device associated with the left hand
	RightHand = 2,				// Tracked device associated with the right hand
};

inline VRTrackedControllerRole operator++(VRTrackedControllerRole &c)
{
	int i = static_cast<int>(c);
	++i;
	c = static_cast<VRTrackedControllerRole>(i);
	return c;
}

struct VRTrackedDevicePose
{
	Matrix4 transform;
	Vector velocity;
	bool isValid;
};

using VRTrackedDeviceIndex = uint32_t;

enum class VREventType
{
	None,

	Quit,
	ProcessQuit,
	QuitAborted_UserPrompt,
	QuitAcknowledged,
	DriverRequestedQuit,
	ButtonPress,
	ButtonUnpress,
	ButtonTouch,
	ButtonUntouch
};

enum class VRButton
{
	System = 0,
	ApplicationMenu = 1,
	Grip = 2,
	DPad_Left = 3,
	DPad_Up = 4,
	DPad_Right = 5,
	DPad_Down = 6,
	A = 7,

	ProximitySensor = 31,

	Axis0 = 32,
	Axis1 = 33,
	Axis2 = 34,
	Axis3 = 35,
	Axis4 = 36,

	// aliases for well known controllers
	SteamVR_Touchpad = Axis0,
	SteamVR_Trigger = Axis1,

	Dashboard_Back = Grip,

	Max = 64
};

struct VREvent_Controller
{
	VRButton button;
};

union VREventData
{
	VREvent_Controller controller;
};

struct VREvent
{
	VREventType eventType;
	VRTrackedDeviceIndex trackedDeviceIndex;
	float eventAgeSeconds;

	VREventData data;
};

using VRTrackedDeviceIndex = uint32_t;
static const VRTrackedDeviceIndex VRTrackedDeviceIndex_Hmd = 0;
static const VRTrackedDeviceIndex VRMaxTrackedDeviceCount = 16;
static const VRTrackedDeviceIndex VRTrackedDeviceIndexOther = 0xFFFFFFFE;
static const VRTrackedDeviceIndex VRTrackedDeviceIndexInvalid = 0xFFFFFFFF;

class IVRInterface
{
public:
	virtual bool Init() = 0;
	virtual void Shutdown() = 0;
	virtual void SetTrackingSpace(VRTrackingSpace trackingSpace) = 0;
	virtual void WaitGetPoses(std::vector<VRTrackedDevicePose>& trackedPoses) = 0;
	virtual int GetMaxTrackedDevices() = 0;
	virtual void SubmitImage(VREye eye, uint32_t textureHandle) = 0;
	virtual void PostPresentHandoff() = 0;
	virtual void GetRecommendedRenderTargetSize(uint32_t& outWidth, uint32_t& outHeight) = 0;
	virtual Matrix4 GetProjectionMatrix(VREye eye, float nearZ, float farZ) = 0;
	virtual Matrix4 GetEyeToHeadTransform(VREye eye) = 0;
	virtual Matrix4 GetRawZeroPoseToStandingAbsoluteTrackingPose() = 0;
	virtual VRTrackedDeviceIndex GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole role) = 0;
	virtual bool PollNextEvent(VREvent& outEvent) = 0;
	virtual VRTrackedControllerRole GetControllerRoleForTrackedDeviceIndex(VRTrackedDeviceIndex deviceIndex) = 0;
};

#endif