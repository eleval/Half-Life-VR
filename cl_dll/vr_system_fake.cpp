#include "Matrices.h"
#include "hud.h"
#include "cl_util.h"
#include "r_studioint.h"
#include "ref_params.h"
#include "kbutton.h"

#include "vr_system_fake.h"

#include <cassert>
#include <algorithm>
#include <windows.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846f
#endif

#undef min
#undef max

extern float CL_KeyState(kbutton_t *key);
extern kbutton_t in_forward;
extern int mouseactive;

VRSystem_Fake::VRSystem_Fake() :
	controlledDeviceIdx(0),
	justChangedControlledDevice(false)
{
	memset(fakeDevices.data(), 0, sizeof(VRFakeDevice));
	memset(fakeControllers.data(), 0, sizeof(VRControllerState));
}

bool VRSystem_Fake::Init()
{
	fakeDevices[FakeHeadsetIdx].position += Vector3(0.0f, 1.5f, 0.0f);
	fakeDevices[FakeLeftHandControllerIdx].position += Vector3(-0.5f, 1.0f, -0.5f);
	fakeDevices[FakeRightHandControllerIdx].position += Vector3(0.5f, 1.0f, -0.5f);

	gEngfuncs.pfnSetMousePos(gEngfuncs.GetWindowCenterX(), gEngfuncs.GetWindowCenterY());
	prevTime = gEngfuncs.GetClientTime();

	return true;
}

void VRSystem_Fake::Shutdown()
{

}

void VRSystem_Fake::Update()
{
	float deltaTime = gEngfuncs.GetClientTime() - prevTime;
	prevTime = gEngfuncs.GetClientTime();

	if (!mouseactive)
	{
		return;
	}

	int mouseX = 0;
	int mouseY = 0;
	gEngfuncs.GetMousePosition(&mouseX, &mouseY);

	const int deltaMouseX = mouseX - gEngfuncs.GetWindowCenterX();
	const int deltaMouseY = mouseY - gEngfuncs.GetWindowCenterY();

	VRFakeDevice& fakeDevice = fakeDevices[controlledDeviceIdx];
	fakeDevice.rotation.y -= deltaMouseX * M_PI * 0.001f;
	fakeDevice.rotation.x -= deltaMouseY * M_PI * 0.001f;

	// Prevent x rotation from going too far up or down
	fakeDevice.rotation.x = std::min(std::max(-M_PI * 0.5f + 0.1f, fakeDevice.rotation.x), M_PI * 0.5f - 0.1f);

	Vector3 forward(-sinf(fakeDevice.rotation.y), 0.0f, -cosf(fakeDevice.rotation.y));
	Vector3 right(-sinf(fakeDevice.rotation.y - M_PI * 0.5f), 0.0f, -cosf(fakeDevice.rotation.y - M_PI * 0.5f));
	Vector3 up(0.0f, 1.0f, 0.0f);

	gEngfuncs.pfnSetMousePos(gEngfuncs.GetWindowCenterX(), gEngfuncs.GetWindowCenterY());

	BYTE keyboardState[256];
	GetKeyboardState(keyboardState);

	if (keyboardState[VK_UP] >> 4)
	{
		fakeDevice.position += forward * deltaTime;
	}

	if (keyboardState[VK_DOWN] >> 4)
	{
		fakeDevice.position -= forward * deltaTime;
	}

	if (keyboardState[VK_LEFT] >> 4)
	{
		fakeDevice.position -= right * deltaTime;
	}

	if (keyboardState[VK_RIGHT] >> 4)
	{
		fakeDevice.position += right * deltaTime;
	}

	if (keyboardState[VK_RIGHT] >> 4)
	{
		fakeDevice.position += right * deltaTime;
	}

	if (keyboardState[VK_PRIOR] >> 4)
	{
		fakeDevice.position += up * deltaTime;
	}

	if (keyboardState[VK_NEXT] >> 4)
	{
		fakeDevice.position -= up * deltaTime;
	}

	// Update controller buttons
	// Mapping goes as follow:
	// System = 0, // F1
	// ApplicationMenu = 1, // F2
	// Grip = 2, // Mouse2
	// Axis0 = 32, // WASD
	// Axis1 = 33, // Mouse 1
	// Others aren't set as they are not present on the Vive. Feel free to implement them if you wish
	VRControllerState& controllerState = fakeControllers[controlledDeviceIdx];
	SetControllerButtonState(controllerState, VRButton::System, keyboardState[VK_F1] >> 4);
	SetControllerButtonState(controllerState, VRButton::ApplicationMenu, keyboardState[VK_F2] >> 4);
	SetControllerButtonState(controllerState, VRButton::Grip, keyboardState[VK_RBUTTON] >> 4);

	const uint64_t axis0 = 0;
	const uint64_t axis1 = 1;

	if (keyboardState['W'] >> 4)
	{
		controllerState.axis[axis0].x = 0.0f;
		controllerState.axis[axis0].y = 1.0f;
		SetControllerButtonState(controllerState, VRButton::Axis0, true);
	}
	else if (keyboardState['S'] >> 4)
	{
		controllerState.axis[axis0].x = 0.0f;
		controllerState.axis[axis0].y = -1.0f;
		SetControllerButtonState(controllerState, VRButton::Axis0, true);
	}
	else if (keyboardState['D'] >> 4)
	{
		controllerState.axis[axis0].x = 1.0f;
		controllerState.axis[axis0].y = 0.0f;
		SetControllerButtonState(controllerState, VRButton::Axis0, true);
	}
	else if (keyboardState['A'] >> 4)
	{
		controllerState.axis[axis0].x = -1.0f;
		controllerState.axis[axis0].y = 0.0f;
		SetControllerButtonState(controllerState, VRButton::Axis0, true);
	}
	else
	{
		SetControllerButtonState(controllerState, VRButton::Axis0, false);
	}

	if (keyboardState[VK_LBUTTON] >> 4)
	{
		controllerState.axis[axis1].x = 1.0f;
		SetControllerButtonState(controllerState, VRButton::Axis1, true);
	}
	else
	{
		controllerState.axis[axis1].x = 0.0f;
		SetControllerButtonState(controllerState, VRButton::Axis1, false);
	}
	
	if (keyboardState[VK_END] >> 4)
	{
		if (!justChangedControlledDevice)
		{
			controlledDeviceIdx = (controlledDeviceIdx + 1) % FakeDevicesCount;
			justChangedControlledDevice = true;
		}
	}
	else
	{
		justChangedControlledDevice = false;
	}
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
		
		trackedPose.transform.rotateX(fakeDevice.rotation.x);
		trackedPose.transform.rotateY(fakeDevice.rotation.y);
		trackedPose.transform.rotateZ(fakeDevice.rotation.z);

		trackedPose.transform.translate(fakeDevice.position);
		
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
	Matrix4 identity;
	return identity;
}

Matrix4 VRSystem_Fake::GetEyeToHeadTransform(VREye eye)
{
	// Since the eye is at the same place as the headset in fake VR, return an indentity Matrix
	Matrix4 identity;
	return identity;
}

Matrix4 VRSystem_Fake::GetRawZeroPoseToStandingAbsoluteTrackingPose()
{
	// TODO but isn't used
	Matrix4 identity;
	return identity;
}

VRTrackedDeviceIndex VRSystem_Fake::GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole role)
{
	switch (role)
	{
		case VRTrackedControllerRole::LeftHand:
			return FakeLeftHandControllerIdx;
		case VRTrackedControllerRole::RightHand:
			return FakeRightHandControllerIdx;
		default:
			return -1;
	}
}

bool VRSystem_Fake::PollNextEvent(VREvent& outEvent)
{
	if (!eventQueue.empty())
	{
		outEvent = eventQueue.front();
		eventQueue.pop();
		return true;
	}
	return false;
}

VRTrackedControllerRole VRSystem_Fake::GetControllerRoleForTrackedDeviceIndex(VRTrackedDeviceIndex deviceIndex)
{
	switch (deviceIndex)
	{
		case 1:
			return VRTrackedControllerRole::LeftHand;
		case 2:
			return VRTrackedControllerRole::RightHand;
		default:
			return VRTrackedControllerRole::Invalid;
	}
}

bool VRSystem_Fake::GetControllerState(VRTrackedDeviceIndex controllerDeviceIndex, VRControllerState& outControllerState)
{
	assert(controllerDeviceIndex >= 0 && controlledDeviceIdx < FakeDevicesCount);
	assert(controllerDeviceIndex != FakeHeadsetIdx);

	outControllerState = fakeControllers[controllerDeviceIndex];
	return true;
}

//////////////////////////////////////////////////////////////////////////

VRSystem_Fake& VRSystem_Fake::Instance()
{
	static VRSystem_Fake instance;
	return instance;
}

//////////////////////////////////////////////////////////////////////////

void VRSystem_Fake::SetControllerButtonState(VRControllerState& controller, VRButton button, bool pressed)
{
	const uint64_t iButton = 1ull << static_cast<uint64_t>(button);
	if (pressed)
	{
		if ((controller.buttonPressed & iButton) == 0)
		{
			VREvent event;
			event.eventType = VREventType::ButtonPress;
			event.data.controller.button = button;
			event.trackedDeviceIndex = controlledDeviceIdx;
			eventQueue.push(event);
		}
		controller.buttonPressed |= iButton;
	}
	else
	{
		if (controller.buttonPressed & iButton)
		{
			VREvent event;
			event.eventType = VREventType::ButtonUnpress;
			event.data.controller.button = button;
			event.trackedDeviceIndex = controlledDeviceIdx;
			eventQueue.push(event);
		}
		controller.buttonPressed &= ~iButton;
	}
}