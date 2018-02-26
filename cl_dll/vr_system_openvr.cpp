#include "Matrices.h"
#include "hud.h"
#include "cl_util.h"
#include "r_studioint.h"
#include "ref_params.h"

#include "vr_system_openvr.h"

#include <cassert>

static vr::ETrackingUniverseOrigin openVRTrackingUniverseOrigin[] = {
	vr::TrackingUniverseSeated, // VRTrackingSpace::Seated
	vr::TrackingUniverseStanding, // VRTrackingSpace::Standing
	vr::TrackingUniverseRawAndUncalibrated // VRTrackingSpace::RawAndUncalibrated
};

static vr::EVREye openVREye[] = {
	vr::Eye_Left, // VREye::Left
	vr::Eye_Right // VREye::Right
};

static vr::ETrackedControllerRole openVRTrackedControllerRole[] = {
	vr::TrackedControllerRole_Invalid, // VRTrackedControllerRole::Invalid
	vr::TrackedControllerRole_LeftHand, // VRTrackedControllerRole::LeftHand
	vr::TrackedControllerRole_RightHand // VRTrackedControllerRole::RightHand
};

static vr::EVREventType openVREventType[] = {
	vr::VREvent_None, // VREventType::None
	vr::VREvent_Quit, // VREventType::Quit
	vr::VREvent_ProcessQuit, // VREventType::ProcessQuit
	vr::VREvent_QuitAborted_UserPrompt, // VREventType::QuitAborted_UserPrompt
	vr::VREvent_QuitAcknowledged, // VREventType::QuitAcknowledged
	vr::VREvent_DriverRequestedQuit, // VREventType::DriverRequestedQuit
	vr::VREvent_ButtonPress, // VREventType::ButtonPress
	vr::VREvent_ButtonUnpress, // VREventType::ButtonUnpress
};

Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &mat)
{
	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0f,
		mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0f,
		mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0f,
		mat.m[0][3], mat.m[1][3], mat.m[2][3], 0.1f
	);
}

Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix44_t &mat)
{
	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}

VRButton ConvertSteamVRButtonToVRButton(uint32_t button)
{
	switch (button)
	{
		case vr::k_EButton_System:
			return VRButton::System;
		case vr::k_EButton_ApplicationMenu:
			return VRButton::ApplicationMenu;
		case vr::k_EButton_Grip:
			return VRButton::Grip;
		case vr::k_EButton_DPad_Left:
			return VRButton::DPad_Left;
		case vr::k_EButton_DPad_Up:
			return VRButton::DPad_Up;
		case vr::k_EButton_DPad_Right:
			return VRButton::DPad_Right;
		case vr::k_EButton_DPad_Down:
			return VRButton::DPad_Down;
		case vr::k_EButton_A:
			return VRButton::A;

		case vr::k_EButton_ProximitySensor:
			return VRButton::ProximitySensor;

		case vr::k_EButton_Axis0:
			return VRButton::Axis0;
		case vr::k_EButton_Axis1:
			return VRButton::Axis1;
		case vr::k_EButton_Axis2:
			return VRButton::Axis2;
		case vr::k_EButton_Axis3:
			return VRButton::Axis4;
		case vr::k_EButton_Axis4:
			return VRButton::Axis4;

		default:
		case vr::k_EButton_Max:
			return VRButton::Max;
	}
}

VRTrackedControllerRole ConvertSteamVRControllerRole(vr::ETrackedControllerRole controllerRole)
{
	switch (controllerRole)
	{
		case vr::TrackedControllerRole_LeftHand:
			return VRTrackedControllerRole::LeftHand;
		case vr::TrackedControllerRole_RightHand:
			return VRTrackedControllerRole::RightHand;
		case vr::TrackedControllerRole_Invalid:
		default:
			return VRTrackedControllerRole::Invalid;
	}
}

VRSystem_OpenVR::VRSystem_OpenVR() :
	vrSystem(nullptr),
	vrCompositor(nullptr)
{

}

bool VRSystem_OpenVR::Init()
{
	vr::EVRInitError vrInitError;
	vrSystem = vr::VR_Init(&vrInitError, vr::EVRApplicationType::VRApplication_Scene);
	vrCompositor = vr::VRCompositor();

	return vrInitError == vr::EVRInitError::VRInitError_None && vrSystem != nullptr && vrCompositor != nullptr;
}

void VRSystem_OpenVR::Shutdown()
{
	vr::VR_Shutdown();
}

void VRSystem_OpenVR::Update()
{

}

void VRSystem_OpenVR::SetTrackingSpace(VRTrackingSpace trackingSpace)
{
	vrCompositor->GetCurrentSceneFocusProcess();
	vrCompositor->SetTrackingSpace(openVRTrackingUniverseOrigin[static_cast<int>(trackingSpace)]);
}

void VRSystem_OpenVR::WaitGetPoses(std::vector<VRTrackedDevicePose>& trackedPoses)
{
	assert(trackedPoses.size() <= vr::k_unMaxTrackedDeviceCount);

	vr::TrackedDevicePose_t openVRTrackedDevicesPoses[vr::k_unMaxTrackedDeviceCount];
	vrCompositor->WaitGetPoses(openVRTrackedDevicesPoses, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
	
	for (size_t i = 0; i < trackedPoses.size(); ++i)
	{
		vr::TrackedDevicePose_t& openVRTrackedDevicePose = openVRTrackedDevicesPoses[i];
		VRTrackedDevicePose& trackedDevicePose = trackedPoses[i];

		trackedDevicePose.isValid = openVRTrackedDevicePose.bDeviceIsConnected && openVRTrackedDevicePose.bPoseIsValid && openVRTrackedDevicePose.eTrackingResult == vr::TrackingResult_Running_OK;
		trackedDevicePose.transform = ConvertSteamVRMatrixToMatrix4(openVRTrackedDevicePose.mDeviceToAbsoluteTracking);
		trackedDevicePose.velocity = Vector(openVRTrackedDevicePose.vVelocity.v);
	}
}

int VRSystem_OpenVR::GetMaxTrackedDevices()
{
	return vr::k_unMaxTrackedDeviceCount;
}

void VRSystem_OpenVR::SubmitImage(VREye eye, uint32_t textureHandle)
{
	vr::Texture_t vrTexture;
	vrTexture.eType = vr::ETextureType::TextureType_OpenGL;
	vrTexture.eColorSpace = vr::EColorSpace::ColorSpace_Auto;
	vrTexture.handle = reinterpret_cast<void*>(textureHandle);

	vrCompositor->Submit(openVREye[static_cast<int>(eye)], &vrTexture);
}

void VRSystem_OpenVR::PostPresentHandoff()
{
	vrCompositor->PostPresentHandoff();
}

void VRSystem_OpenVR::GetRecommendedRenderTargetSize(uint32_t& outWidth, uint32_t& outHeight)
{
	vrSystem->GetRecommendedRenderTargetSize(&outWidth, &outHeight);
}

Matrix4 VRSystem_OpenVR::GetProjectionMatrix(VREye eye, float nearZ, float farZ)
{
	return ConvertSteamVRMatrixToMatrix4(vrSystem->GetProjectionMatrix(openVREye[static_cast<int>(eye)], nearZ, farZ));
}

Matrix4 VRSystem_OpenVR::GetEyeToHeadTransform(VREye eye)
{
	return ConvertSteamVRMatrixToMatrix4(vrSystem->GetEyeToHeadTransform(openVREye[static_cast<int>(eye)]));
}

Matrix4 VRSystem_OpenVR::GetRawZeroPoseToStandingAbsoluteTrackingPose()
{
	return ConvertSteamVRMatrixToMatrix4(vrSystem->GetRawZeroPoseToStandingAbsoluteTrackingPose());
}

VRTrackedDeviceIndex VRSystem_OpenVR::GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole role)
{
	return vrSystem->GetTrackedDeviceIndexForControllerRole(openVRTrackedControllerRole[static_cast<int>(role)]);
}

bool VRSystem_OpenVR::PollNextEvent(VREvent& outEvent)
{
	vr::VREvent_t vrEvent;
	if (vrSystem->PollNextEvent(&vrEvent, sizeof(vr::VREvent_t)))
	{
		switch (vrEvent.eventType)
		{
			case vr::VREvent_Quit:
				outEvent.eventType = VREventType::Quit;
				break;
			case vr::VREvent_ProcessQuit:
				outEvent.eventType = VREventType::ProcessQuit;
				break;
			case vr::VREvent_QuitAborted_UserPrompt:
				outEvent.eventType = VREventType::QuitAborted_UserPrompt;
				break;
			case vr::VREvent_QuitAcknowledged:
				outEvent.eventType = VREventType::QuitAcknowledged;
				break;
			case vr::VREvent_DriverRequestedQuit:
				outEvent.eventType = VREventType::DriverRequestedQuit;
				break;
			case vr::VREvent_ButtonPress:
				outEvent.eventType = VREventType::ButtonPress;
				outEvent.data.controller.button = ConvertSteamVRButtonToVRButton(vrEvent.data.controller.button);
				outEvent.eventAgeSeconds = vrEvent.eventAgeSeconds = vrEvent.eventAgeSeconds;
				outEvent.trackedDeviceIndex = vrEvent.trackedDeviceIndex;
				break;
			case vr::VREvent_ButtonUnpress:
				outEvent.eventType = VREventType::ButtonUnpress;
				outEvent.data.controller.button = ConvertSteamVRButtonToVRButton(vrEvent.data.controller.button);
				outEvent.eventAgeSeconds = vrEvent.eventAgeSeconds = vrEvent.eventAgeSeconds;
				outEvent.trackedDeviceIndex = vrEvent.trackedDeviceIndex;
				break;
			case vr::VREvent_ButtonTouch:
				outEvent.eventType = VREventType::ButtonUnpress;
				outEvent.data.controller.button = ConvertSteamVRButtonToVRButton(vrEvent.data.controller.button);
				outEvent.eventAgeSeconds = vrEvent.eventAgeSeconds = vrEvent.eventAgeSeconds;
				outEvent.trackedDeviceIndex = vrEvent.trackedDeviceIndex;
				break;
			case vr::VREvent_ButtonUntouch:
				outEvent.eventType = VREventType::ButtonUnpress;
				outEvent.data.controller.button = ConvertSteamVRButtonToVRButton(vrEvent.data.controller.button);
				outEvent.eventAgeSeconds = vrEvent.eventAgeSeconds = vrEvent.eventAgeSeconds;
				outEvent.trackedDeviceIndex = vrEvent.trackedDeviceIndex;
				break;
			default:
				outEvent.eventType = VREventType::None;
				break;
		}

		return true;
	}

	return false;
}

VRTrackedControllerRole VRSystem_OpenVR::GetControllerRoleForTrackedDeviceIndex(VRTrackedDeviceIndex deviceIndex)
{
	return ConvertSteamVRControllerRole(vrSystem->GetControllerRoleForTrackedDeviceIndex(deviceIndex));
}

bool VRSystem_OpenVR::GetControllerState(VRTrackedDeviceIndex controllerDeviceIndex, VRControllerState& outControllerState)
{
	vr::VRControllerState_t openVRControllerState;
	if(vrSystem->GetControllerState(controllerDeviceIndex, &openVRControllerState, sizeof(vr::VRControllerState_t)))
	{
		outControllerState.packetNum = openVRControllerState.unPacketNum;
		outControllerState.buttonPressed = openVRControllerState.ulButtonPressed;
		outControllerState.buttonTouched = openVRControllerState.ulButtonTouched;
		for (int i = 0; i < vr::k_unControllerStateAxisCount; ++i)
		{
			outControllerState.axis[i].x = openVRControllerState.rAxis[i].x;
			outControllerState.axis[i].y = openVRControllerState.rAxis[i].y;
		}
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

VRSystem_OpenVR& VRSystem_OpenVR::Instance()
{
	static VRSystem_OpenVR instance;
	return instance;
}