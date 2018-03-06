
#include <windows.h>
#include "hud.h"
#include "r_studioint.h"
#include "ref_params.h"
#include "vr_helper.h"
#include "vr_gl.h"
#include "vr_input.h"
#include "vr_system_openvr.h"
#include "vr_system_fake.h"

#include "glm/matrix.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/matrix_decompose.hpp"

#include "cl_util.h"

#include <string>

#ifndef MAX_COMMAND_SIZE
#define MAX_COMMAND_SIZE 256
#endif

extern engine_studio_api_t IEngineStudio;

const glm::vec3 VR_TO_HL(64.0f / 1.22f, 64.0f / 1.22f, 64.0f / 1.22f); // 1.22m = 64 units
const glm::vec3 HL_TO_VR(1.0f / VR_TO_HL.x, 1.0f / VR_TO_HL.y, 1.0f / VR_TO_HL.z);
const float FLOOR_OFFSET = 10;

VRHelper gVRHelper;

cvar_t *vr_weapontilt;
cvar_t *vr_roomcrouch;
cvar_t* vr_systemType;
cvar_t* vr_showPlayer;
cvar_t* vr_renderEyeInWindow;

void VR_Recenter()
{
	gVRHelper.Recenter();
}

VRHelper::VRHelper() :
	invertCenterTransform(1.0f)
{
	for (int i = 0; i < VRMaxTrackedDeviceCount; ++i)
	{
		hlSpaceVRTransforms.deviceModelViews[i] = glm::mat4(1.0f);
	}
}

VRHelper::~VRHelper()
{

}

void VRHelper::Init()
{
	gEngfuncs.pfnAddCommand("vr_recenter", VR_Recenter);

	//Register Helper convars
	vr_weapontilt = gEngfuncs.pfnRegisterVariable("vr_weapontilt", "-25", FCVAR_ARCHIVE);
	vr_roomcrouch = gEngfuncs.pfnRegisterVariable("vr_roomcrouch", "1", FCVAR_ARCHIVE);
	vr_systemType = gEngfuncs.pfnRegisterVariable("vr_systemType", "0", FCVAR_ARCHIVE);
	vr_showPlayer = gEngfuncs.pfnRegisterVariable("vr_showPlayer", "0", 0);
	vr_renderEyeInWindow = gEngfuncs.pfnRegisterVariable("vr_renderEyeInWindow", "0", 0); // 0 = Classic preview - 1 = Left eye - 2 = Right eye

	//Register Input convars 
	g_vrInput.vr_control_alwaysforward = gEngfuncs.pfnRegisterVariable("vr_control_alwaysforward", "0", FCVAR_ARCHIVE);
	g_vrInput.vr_control_deadzone = gEngfuncs.pfnRegisterVariable("vr_control_deadzone", "0.5", FCVAR_ARCHIVE);
	g_vrInput.vr_control_teleport = gEngfuncs.pfnRegisterVariable("vr_control_teleport", "0", FCVAR_ARCHIVE);
	g_vrInput.vr_control_hand = gEngfuncs.pfnRegisterVariable("vr_control_hand", "1", FCVAR_ARCHIVE);
	g_vrInput.vr_control_scheme = gEngfuncs.pfnRegisterVariable("vr_control_scheme", "1", FCVAR_ARCHIVE);

	/*if (!AcceptsDisclaimer())
	{
		Exit();
	}*/
	if (!IEngineStudio.IsHardware())
	{
		Exit(TEXT("Software mode not supported. Please start this game with OpenGL renderer. (Start Half-Life, open the Options menu, select Video, chose OpenGL as Renderer, save, quit Half-Life, then start Half-Life: VR)"));
	}
	else if (!InitAdditionalGLFunctions())
	{
		Exit(TEXT("Failed to load necessary OpenGL functions. Make sure you have a graphics card that can handle VR and up-to-date drivers, and make sure you are running the game in OpenGL mode."));
	}
	else if (!InitGLMatrixOverrideFunctions())
	{
		Exit(TEXT("Failed to load custom opengl32.dll. Make sure you launch this game through HLVRLauncher.exe, not the Steam menu."));
	}
	else
	{
		if (vr_systemType->value == 0.0f)
		{
			vrSystem = &VRSystem_OpenVR::Instance();
		}
		else
		{
			vrSystem = &VRSystem_Fake::Instance();
		}

		if (!vrSystem->Init())
		{
			Exit(TEXT("Failed to initialize VR enviroment. Make sure your headset is properly connected and SteamVR is running."));
		}
		else
		{
			rawTrackedDevicePoses.resize(vrSystem->GetMaxTrackedDevices());
			vrSystem->GetRecommendedRenderTargetSize(vrRenderWidth, vrRenderHeight);
			CreateGLTexture(&vrGLLeftEyeTexture, vrRenderWidth, vrRenderHeight);
			CreateGLTexture(&vrGLRightEyeTexture, vrRenderWidth, vrRenderHeight);
			int viewport[4];
			glGetIntegerv(GL_VIEWPORT, viewport);
			CreateGLTexture(&vrGLMenuTexture, viewport[2], viewport[3]);
			CreateGLTexture(&vrGLHUDTexture, viewport[2], viewport[3]);
			if (vrGLLeftEyeTexture == 0 || vrGLRightEyeTexture == 0 || vrGLMenuTexture == 0 || vrGLHUDTexture == 0)
			{
				Exit(TEXT("Failed to initialize OpenGL textures for VR enviroment. Make sure you have a graphics card that can handle VR and up-to-date drivers."));
			}
			else
			{
				CreateGLFrameBuffer(&vrGLLeftEyeFrameBuffer, vrGLLeftEyeTexture, vrRenderWidth, vrRenderHeight);
				CreateGLFrameBuffer(&vrGLRightEyeFrameBuffer, vrGLRightEyeTexture, vrRenderWidth, vrRenderHeight);
				if (vrGLLeftEyeFrameBuffer == 0 || vrGLRightEyeFrameBuffer == 0)
				{
					Exit(TEXT("Failed to initialize OpenGL framebuffers for VR enviroment. Make sure you have a graphics card that can handle VR and up-to-date drivers."));
				}
			}
		}
	}
}

/*bool VRHelper::AcceptsDisclaimer()
{
	return MessageBox(WindowFromDC(wglGetCurrentDC()), TEXT("This software is provided as is with no warranties whatsoever. This mod uses a custom opengl32.dll that might get you VAC banned. The VR experience might be unpleasant and nauseating. Only continue if you're aware of the risks and know what you are doing.\n\nDo you want to continue?"), TEXT("Disclaimer"), MB_YESNO) == IDYES;
}*/

void VRHelper::Exit(const char* lpErrorMessage)
{
	if (lpErrorMessage != nullptr)
	{
		MessageBox(NULL, lpErrorMessage, TEXT("Error starting Half-Life: VR"), MB_OK);
	}
	vrSystem->Shutdown();
	vrSystem = nullptr;
	gEngfuncs.pfnClientCmd("quit");
	std::exit(lpErrorMessage != nullptr ? 1 : 0);
}

glm::mat4 VRHelper::GetHMDMatrixProjectionEye(VREye eEye)
{
	float fNearZ = 0.01f;
	float fFarZ = 8192.f;
	return vrSystem->GetProjectionMatrix(eEye, fNearZ, fFarZ);
}

extern void VectorAngles(const float *forward, float *angles);

Vector VRHelper::GetAnglesFromMatrix(const glm::mat4 &mat)
{
	glm::mat4 rotationOnly = mat;
	rotationOnly[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	glm::vec4 v1 = rotationOnly * glm::vec4(1, 0, 0, 0);
	glm::vec4 v2 = rotationOnly * glm::vec4(0, 1, 0, 0);
	glm::vec4 v3 = rotationOnly * glm::vec4(0, 0, 1, 0);

	v1 = glm::normalize(v1);
	v2 = glm::normalize(v2);
	v3 = glm::normalize(v3);

	Vector forward(-v1.z, -v2.z, -v3.z);

	Vector angles;
	VectorAngles(forward, angles);
	angles.x = 360.f - angles.x;	// viewangles pitch is inverted
	return angles;
}

Vector VRHelper::GetHLAnglesFromVRMatrix(const glm::mat4 &mat)
{
	glm::vec4 forwardInVRSpace = mat * glm::vec4(0, 0, -1, 0);
	glm::vec4 rightInVRSpace = mat * glm::vec4(1, 0, 0, 0);
	glm::vec4 upInVRSpace = mat * glm::vec4(0, 1, 0, 0);

	Vector forward(forwardInVRSpace.x, -forwardInVRSpace.z, forwardInVRSpace.y);
	Vector right(rightInVRSpace.x, -rightInVRSpace.z, rightInVRSpace.y);
	Vector up(upInVRSpace.x, -upInVRSpace.z, upInVRSpace.y);

	forward.Normalize();
	right.Normalize();
	up.Normalize();

	Vector angles;
	GetAnglesFromVectors(forward, right, up, angles);
	angles.x = 360.f - angles.x;
	return angles;
}

Vector VRHelper::GetPlayerViewOrg()
{
	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
	return localPlayer->curstate.origin;
}

glm::vec3 VRHelper::GetTranslationFromTransform(const glm::mat4& transform)
{
	glm::vec3 scale;
	glm::quat rot;
	glm::vec3 trans;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transform, scale, rot, trans, skew, perspective);

	return trans;
}

glm::quat VRHelper::GetRotationFromTransform(const glm::mat4& transform)
{
	glm::vec3 scale;
	glm::quat rot;
	glm::vec3 trans;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transform, scale, rot, trans, skew, perspective);

	return rot;
}

glm::mat4 VRHelper::TransformVRSpaceToHLSpace(const glm::mat4 &absoluteTrackingMatrix, Vector translate)
{
	glm::mat4 hlSpaceMat;
	glm::mat4 inverseVR = glm::inverse(absoluteTrackingMatrix);
	inverseVR[3] = glm::vec4(0.0, 0.0, 0.0, 1.0f);

	hlSpaceMat[0] = -inverseVR[2];
	hlSpaceMat[1] = -inverseVR[0];
	hlSpaceMat[2] = inverseVR[1];
	hlSpaceMat[3].w = 1.0f;

	glm::mat4 hlTrans(1.0f);

	hlTrans[3].x = absoluteTrackingMatrix[3].z * VR_TO_HL.z - translate.x;
	hlTrans[3].y = absoluteTrackingMatrix[3].x * VR_TO_HL.x - translate.y;
	hlTrans[3].z = -absoluteTrackingMatrix[3].y * VR_TO_HL.y - translate.z;
	hlTrans[3].w = 1.0f;

	return hlSpaceMat * hlTrans;
}

Vector GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(const glm::mat4 & absoluteTrackingMatrix)
{
	glm::vec4 originInVRSpace = absoluteTrackingMatrix * glm::vec4(0, 0, .10, 1);
	return Vector(originInVRSpace.x * VR_TO_HL.x * 10, -originInVRSpace.z * VR_TO_HL.z * 10, originInVRSpace.y * VR_TO_HL.y * 10);
}

Vector GetPositionInHLSpaceFromAbsoluteTrackingMatrix(const glm::mat4 & absoluteTrackingMatrix)
{
	Vector originInRelativeHLSpace = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(absoluteTrackingMatrix);

	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
	Vector clientGroundPosition = localPlayer->curstate.origin;

	return localPlayer->curstate.origin + originInRelativeHLSpace;
}

void VRHelper::PollEvents()
{
	if (vrSystem != nullptr)
	{
		vrSystem->Update();
	}

	VREvent vrEvent;
	while (vrSystem != nullptr && vrSystem->PollNextEvent(vrEvent))
	{
		switch (vrEvent.eventType)
		{
			case VREventType::Quit:
			case VREventType::ProcessQuit:
			case VREventType::QuitAborted_UserPrompt:
			case VREventType::QuitAcknowledged:
			case VREventType::DriverRequestedQuit:
				Exit();
				return;
			case VREventType::ButtonPress:
			case VREventType::ButtonUnpress:
			{
				VRTrackedControllerRole controllerRole = vrSystem->GetControllerRoleForTrackedDeviceIndex(vrEvent.trackedDeviceIndex);
				if (controllerRole != VRTrackedControllerRole::Invalid)
				{
					VRControllerState controllerState;
					vrSystem->GetControllerState(vrEvent.trackedDeviceIndex, controllerState);
					g_vrInput.HandleButtonPress(vrEvent.data.controller.button, controllerState, controllerRole == VRTrackedControllerRole::LeftHand, vrEvent.eventType == VREventType::ButtonPress);
				}
			}
			break;
			case VREventType::ButtonTouch:
			case VREventType::ButtonUntouch:
			default:
				break;
		}
	}

	for (VRTrackedControllerRole controllerRole = VRTrackedControllerRole::LeftHand; controllerRole <= VRTrackedControllerRole::RightHand; ++controllerRole)
	{
		VRControllerState controllerState;
		vrSystem->GetControllerState(vrSystem->GetTrackedDeviceIndexForControllerRole(controllerRole), controllerState);
		g_vrInput.HandleTrackpad(VRButton::SteamVR_Touchpad, controllerState, controllerRole == VRTrackedControllerRole::LeftHand, false);
	}
}

bool VRHelper::UpdatePositions()
{
	if (vrSystem != nullptr)
	{
		const Vector playerOrigin = GetPlayerViewOrg();

		vrSystem->SetTrackingSpace(isVRRoomScale ? VRTrackingSpace::Standing : VRTrackingSpace::Seated);
		vrSystem->WaitGetPoses(rawTrackedDevicePoses);

		const VRTrackedDeviceIndex controllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole::RightHand);
		for (VRTrackedDeviceIndex deviceIndex = 0; deviceIndex < rawTrackedDevicePoses.size(); ++deviceIndex)
		{
			if (rawTrackedDevicePoses[deviceIndex].isValid)
			{
				hlSpaceVRTransforms.deviceModelViews[deviceIndex] = TransformVRSpaceToHLSpace(GetCenteredRawDeviceTransform(deviceIndex), playerOrigin);
			}
		}

		if (rawTrackedDevicePoses[VRTrackedDeviceIndex_Hmd].isValid)
		{
			const glm::mat4 headsetTransform = GetCenteredRawDeviceTransform(VRTrackedDeviceIndex_Hmd);

			hlSpaceVRTransforms.headsetLeftEyeProjection = GetHMDMatrixProjectionEye(VREye::Left);
			hlSpaceVRTransforms.headsetRightEyeProjection = GetHMDMatrixProjectionEye(VREye::Right);

			hlSpaceVRTransforms.headsetLeftEyeModelView = TransformVRSpaceToHLSpace(headsetTransform * vrSystem->GetEyeToHeadTransform(VREye::Left), playerOrigin);
			hlSpaceVRTransforms.headsetRightEyeModelView = TransformVRSpaceToHLSpace(headsetTransform * vrSystem->GetEyeToHeadTransform(VREye::Right), playerOrigin);

			UpdateGunPosition();

			SendPositionUpdateToServer();

			return true;
		}
	}

	return false;
}

void VRHelper::PrepareVRScene(VREye eEye, struct ref_params_s* pparams)
{
	GetAnglesFromMatrix(hlSpaceVRTransforms.headsetLeftEyeModelView).CopyToArray(pparams->viewangles);

	glBindFramebuffer(GL_FRAMEBUFFER, eEye == VREye::Left ? vrGLLeftEyeFrameBuffer : vrGLRightEyeFrameBuffer);

	glViewport(0, 0, vrRenderWidth, vrRenderHeight);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glLoadMatrixf(eEye == VREye::Left ? glm::value_ptr((hlSpaceVRTransforms.headsetLeftEyeProjection)) : glm::value_ptr((hlSpaceVRTransforms.headsetRightEyeProjection)));

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	const glm::mat4& modelViewMat = eEye == VREye::Left ? hlSpaceVRTransforms.headsetLeftEyeModelView : hlSpaceVRTransforms.headsetRightEyeModelView;
	glLoadMatrixf(glm::value_ptr(modelViewMat));

	glDisable(GL_CULL_FACE);
	glCullFace(GL_NONE);

	hlvrLockGLMatrices();
}

void VRHelper::FinishVRScene(struct ref_params_s* pparams)
{
	hlvrUnlockGLMatrices();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, pparams->viewport[2], pparams->viewport[3]);
}

void VRHelper::SubmitImages()
{
	vrSystem->SubmitImage(VREye::Left, vrGLLeftEyeTexture);
	vrSystem->SubmitImage(VREye::Right, vrGLRightEyeTexture);
	vrSystem->PostPresentHandoff();
}

void VRHelper::GetViewAngles(float * angles)
{
	GetAnglesFromMatrix(hlSpaceVRTransforms.deviceModelViews[VRTrackedDeviceIndex_Hmd]).CopyToArray(angles);
}

void VRHelper::GetWalkAngles(float * angles)
{
	Vector walkAngles;

	if (g_vrInput.vr_control_hand->value == 0.0f)
	{
		walkAngles = GetAnglesFromMatrix(GetDeviceHLSpaceTransform(0));
	}
	else if (g_vrInput.vr_control_hand->value == 1.0f)
	{
		walkAngles = GetAnglesFromMatrix(GetDeviceHLSpaceTransform(1));
	}
	else if (g_vrInput.vr_control_hand->value == 2.0f)
	{
		walkAngles = GetAnglesFromMatrix(GetDeviceHLSpaceTransform(2));
	}

	walkAngles.CopyToArray(angles);
}

void VRHelper::GetViewOrg(float * org)
{
	glm::vec3 vrTranslation;
	if (vr_renderEyeInWindow->value == 0.0f)
	{
		vrTranslation = GetDeviceHLSpaceTranslation(VRTrackedDeviceIndex_Hmd);
	}
	else if (vr_renderEyeInWindow->value == 1.0f)
	{
		// TODO
	}
	else
	{
		// TODO
	}

	Vector playerOrg = GetPlayerViewOrg();
	Vector hlTrans;

	hlTrans.x = vrTranslation.x;
	hlTrans.y = vrTranslation.y;
	hlTrans.z = vrTranslation.z;

	hlTrans.CopyToArray(org);
}

void VRHelper::UpdateGunPosition()
{
	cl_entity_t *viewent = gEngfuncs.GetViewModel();
	if (viewent != nullptr)
	{
		VRTrackedDeviceIndex controllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole::RightHand);

		if (controllerIndex > 0 && controllerIndex != VRTrackedDeviceIndexInvalid && rawTrackedDevicePoses[controllerIndex].isValid)
		{
			const glm::mat4 controllerTransform = GetDeviceHLSpaceTransform(controllerIndex);
			const glm::vec3 controllerPosition = GetDeviceHLSpaceTranslation(controllerIndex);
			const glm::vec3 controllerVelocity = GetDeviceHLSpaceVelocity(controllerIndex);

			const Vector controllerPositionHL(controllerPosition.x, controllerPosition.y, controllerPosition.z);
			VectorCopy(controllerPositionHL, viewent->origin);
			VectorCopy(controllerPositionHL, viewent->curstate.origin);
			VectorCopy(controllerPositionHL, viewent->latched.prevorigin);

			viewent->angles = GetAnglesFromMatrix(controllerTransform);
			viewent->angles.x = 360.0f - viewent->angles.x;
			VectorCopy(viewent->angles, viewent->curstate.angles);
			VectorCopy(viewent->angles, viewent->latched.prevangles);

			viewent->curstate.velocity = Vector(controllerVelocity.x, controllerVelocity.y, controllerVelocity.z);
		}
		else
		{
			viewent->model = NULL;
		}
	}
}

void VRHelper::SendPositionUpdateToServer()
{
	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
	cl_entity_t *viewent = gEngfuncs.GetViewModel();

	Vector hmdOffset = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(GetCenteredRawDeviceTransform(VRTrackedDeviceIndex_Hmd));
	hmdOffset.z += localPlayer->curstate.mins.z;

	VRTrackedDeviceIndex leftControllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole::LeftHand);
	bool isLeftControllerValid = leftControllerIndex > 0 && leftControllerIndex != VRTrackedDeviceIndexInvalid && rawTrackedDevicePoses[leftControllerIndex].isValid;

	VRTrackedDeviceIndex rightControllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole::RightHand);
	bool isRightControllerValid = leftControllerIndex > 0 && leftControllerIndex != VRTrackedDeviceIndexInvalid && rawTrackedDevicePoses[leftControllerIndex].isValid;

	Vector leftControllerOffset(0, 0, 0);
	Vector leftControllerAngles(0, 0, 0);
	if (isLeftControllerValid)
	{
		glm::mat4 leftControllerAbsoluteTrackingMatrix = GetCenteredRawDeviceTransform(leftControllerIndex);
		leftControllerOffset = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(leftControllerAbsoluteTrackingMatrix);
		leftControllerOffset.z += localPlayer->curstate.mins.z;
		leftControllerAngles = GetAnglesFromMatrix(leftControllerAbsoluteTrackingMatrix);
		isLeftControllerValid = true;
	}

	Vector weaponOrigin(0, 0, 0);
	Vector weaponOffset(0, 0, 0);
	Vector weaponAngles(0, 0, 0);
	Vector weaponVelocity(0, 0, 0);
	if (isRightControllerValid)
	{
		weaponOrigin = viewent ? viewent->curstate.origin : Vector();
		weaponOffset = weaponOrigin - localPlayer->curstate.origin;
		weaponAngles = viewent ? viewent->curstate.angles : Vector();
		weaponVelocity = viewent ? viewent->curstate.velocity : Vector();
	}

	char cmd[MAX_COMMAND_SIZE] = { 0 };
	sprintf_s(cmd, "updatevr %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %i %i %.2f",
		hmdOffset.x, hmdOffset.y, hmdOffset.z,
		leftControllerOffset.x, leftControllerOffset.y, leftControllerOffset.z,
		leftControllerAngles.x, leftControllerAngles.y, leftControllerAngles.z,
		weaponOffset.x, weaponOffset.y, weaponOffset.z,
		weaponAngles.x, weaponAngles.y, weaponAngles.z,
		weaponVelocity.x, weaponVelocity.y, weaponVelocity.z,
		isLeftControllerValid ? 1 : 0,
		isRightControllerValid ? 1 : 0,
		vr_roomcrouch->value
	);
	gEngfuncs.pfnClientCmd(cmd);
}

glm::mat4 VRHelper::GetCenteredRawDeviceTransform(VRTrackedDeviceIndex deviceIndex)
{
	return invertCenterTransform * rawTrackedDevicePoses[deviceIndex].transform;
}

const glm::mat4& VRHelper::GetDeviceAbsoluteTransform(VRTrackedDeviceIndex deviceIndex)
{
	return rawTrackedDevicePoses[deviceIndex].transform;
}

const glm::mat4& VRHelper::GetDeviceHLSpaceTransform(VRTrackedDeviceIndex deviceIndex)
{
	return hlSpaceVRTransforms.deviceModelViews[deviceIndex];
}

glm::vec3 VRHelper::GetDeviceHLSpaceTranslation(VRTrackedDeviceIndex deviceIndex)
{
	const glm::vec3& vrTranslation = GetCenteredRawDeviceTransform(deviceIndex)[3];
	Vector playerOrg = GetPlayerViewOrg();
	glm::vec3 hlTrans;
	hlTrans.x = -vrTranslation.z * VR_TO_HL.z + playerOrg.x;
	hlTrans.y = -vrTranslation.x * VR_TO_HL.x + playerOrg.y;
	hlTrans.z = vrTranslation.y * VR_TO_HL.y + playerOrg.z;

	return hlTrans;
}

glm::vec3 VRHelper::GetDeviceHLSpaceVelocity(VRTrackedDeviceIndex deviceIndex)
{
	const glm::vec3& vrVelocity = rawTrackedDevicePoses[deviceIndex].velocity;
	glm::vec3 hlVelocity;
	hlVelocity.x = -vrVelocity.z * VR_TO_HL.z;
	hlVelocity.y = -vrVelocity.x * VR_TO_HL.x;
	hlVelocity.z = vrVelocity.y * VR_TO_HL.y;

	return hlVelocity;
}

void VRHelper::DecomposeHLSpaceTransform(const glm::mat4& mat, glm::vec3& outForward, glm::vec3& outLeft, glm::vec3& outUp)
{
	glm::mat4 rotationOnly = mat;
	rotationOnly[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	glm::vec4 v1 = rotationOnly * glm::vec4(1, 0, 0, 0);
	glm::vec4 v2 = rotationOnly * glm::vec4(0, 1, 0, 0);
	glm::vec4 v3 = rotationOnly * glm::vec4(0, 0, 1, 0);

	v1 = glm::normalize(v1);
	v2 = glm::normalize(v2);
	v3 = glm::normalize(v3);

	outForward = glm::vec3(-v1.z, -v2.z, -v3.z);
	outLeft = glm::vec3(-v1.x, -v2.x, -v3.x);
	outUp = glm::vec3(v1.y, v2.y, v3.y);
}

void VRHelper::Recenter()
{
	glm::mat4 hmdTransform = GetDeviceAbsoluteTransform(VRTrackedDeviceIndex_Hmd);
	centerTransform = glm::mat4(1.0f);
	glm::vec3 centerTranslation;
	glm::vec3 scale;
	glm::quat rot;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(hmdTransform, scale, rot, centerTranslation, skew, perspective);

	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
	centerTranslation.y += (localPlayer->curstate.mins.z * HL_TO_VR.z);
	centerTransform = glm::translate(centerTransform, centerTranslation);
	invertCenterTransform = glm::inverse(centerTransform);
}

void RenderLine(Vector v1, Vector v2, Vector color)
{
	glColor4f(color.x, color.y, color.z, 1.0f);
	glBegin(GL_LINES);
	glVertex3f(v1.x, v1.y, v1.z);
	glVertex3f(v2.x, v2.y, v2.z);
	glEnd();
}


void VRHelper::TestRenderControllerPosition(bool leftOrRight)
{
	VRTrackedDeviceIndex controllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(leftOrRight ? VRTrackedControllerRole::LeftHand : VRTrackedControllerRole::RightHand);

	if (controllerIndex > 0 && controllerIndex != VRTrackedDeviceIndexInvalid && rawTrackedDevicePoses[controllerIndex].isValid)
	{
		const glm::mat4 controllerHLSpaceMatrix = GetDeviceHLSpaceTransform(controllerIndex);
		const Vector playerPosition = GetPlayerViewOrg();
		glm::vec3 forward;
		glm::vec3 left;
		glm::vec3 up;
		const glm::vec3 position = GetDeviceHLSpaceTranslation(controllerIndex);

		DecomposeHLSpaceTransform(controllerHLSpaceMatrix, forward, left, up);

		const Vector forwardHL(forward.x, forward.y, forward.z);
		const Vector leftHL(left.x, left.y, left.z);
		const Vector upHL(up.x, up.y, up.z);
		Vector positionHL(position.x, position.y, position.z);

		RenderLine(positionHL, positionHL + forwardHL * 10.0f, Vector(1, 0, 0));
		RenderLine(positionHL, positionHL + leftHL * 10.0f, Vector(0, 1, 0));
		RenderLine(positionHL, positionHL + upHL * 10.0f, Vector(0, 0, 1));
	}

	// Render world axis at player's origin
	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
	Vector playerOrigin = localPlayer->curstate.origin;

	glLineWidth(1.0f);
	RenderLine(playerOrigin, playerOrigin + Vector(100, 0, 0), Vector(1, 0, 0));
	glLineWidth(5.0f);
	RenderLine(playerOrigin, playerOrigin + Vector(0, 100, 0), Vector(0, 1, 0));
	glLineWidth(10.0f);
	RenderLine(playerOrigin, playerOrigin + Vector(0, 0, 100), Vector(0, 0, 1));
}