
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

#ifndef MAX_COMMAND_SIZE
#define MAX_COMMAND_SIZE 256
#endif

extern engine_studio_api_t IEngineStudio;

const glm::vec3 HL_TO_VR(2.3f / 10.f, 2.2f / 10.f, 2.3f / 10.f);
const glm::vec3 VR_TO_HL(1.f / HL_TO_VR.x, 1.f / HL_TO_VR.y, 1.f / HL_TO_VR.z);
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
	vr_systemType = gEngfuncs.pfnRegisterVariable("vr_systemType", "1", FCVAR_ARCHIVE);
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
			positions.m_rTrackedDevicePose.resize(vrSystem->GetMaxTrackedDevices());
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

Vector VRHelper::GetHLViewAnglesFromVRMatrix(const glm::mat4 &mat)
{
	const glm::vec3 forward(mat[2][0], mat[2][1], mat[2][2]);

	Vector angles;
	VectorAngles(Vector(forward.x, -forward.z, forward.y), angles);
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

glm::mat4 VRHelper::TransformVRSpaceToHLSpace(const glm::mat4& vrSpaceMatrix)
{
	glm::mat4 hlToVRScaleMatrix(1.0f);
	hlToVRScaleMatrix[0][0] = HL_TO_VR.x * 10.0f;
	hlToVRScaleMatrix[1][1] = HL_TO_VR.y * 10.0f;
	hlToVRScaleMatrix[2][2] = HL_TO_VR.z * 10.0f;

	glm::mat4 switchYAndZTransitionMatrix(1.0f);
	switchYAndZTransitionMatrix[1][1] = 0;
	switchYAndZTransitionMatrix[1][2] = -1;
	switchYAndZTransitionMatrix[2][1] = 1;
	switchYAndZTransitionMatrix[2][2] = 0;

	/*const glm::mat4 modelViewMatrix = vrSpaceMatrix * hlToVRScaleMatrix * switchYAndZTransitionMatrix;
	//modelViewMatrix = glm::scale(modelViewMatrix, glm::vec3(13.0f, 13.0f, 13.0f));
	return modelViewMatrix;*/

	glm::vec3 translation = GetTranslationFromTransform(vrSpaceMatrix);
	glm::quat rotation = GetRotationFromTransform(vrSpaceMatrix);

	translation = glm::vec3(translation.x * VR_TO_HL.x * 10.0f, -translation.z * VR_TO_HL.z * 10.0f, translation.y * VR_TO_HL.y * 10.0f); // This has side effects on controllers for some reasons, check later (and check with Vive, might be an issue related with the fake system)
	glm::vec3 euler = glm::eulerAngles(rotation);

	// Invert y & z (and negate z)
	std::swap(euler.y, euler.z);
	euler.z = -euler.z;

	const glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
	const glm::quat quat(euler);
	const glm::mat4 rotationMatrix = glm::mat4_cast(quat);

	return translationMatrix * rotationMatrix;

	/*glm::vec4 originInVRSpace = vrSpaceMatrix * glm::vec4(0, 0, .10, 1);
	return Vector(originInVRSpace.x * VR_TO_HL.x * 10, -originInVRSpace.z * VR_TO_HL.z * 10, originInVRSpace.y * VR_TO_HL.y * 10);*/
}

glm::mat4 VRHelper::TranslateToPlayerView(const glm::mat4 transform)
{
	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
	Vector playerOrigin = localPlayer->curstate.origin;

	return glm::translate(glm::mat4(1.0f), glm::vec3(playerOrigin.x, playerOrigin.y, playerOrigin.z)) * transform;
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

glm::mat4 VRHelper::GetModelViewMatrixFromAbsoluteTrackingMatrix(glm::mat4 &absoluteTrackingMatrix, Vector translate)
{
	glm::mat4 hlToVRScaleMatrix(1.0f);
	hlToVRScaleMatrix[0][0] = HL_TO_VR.x * 10.0f;
	hlToVRScaleMatrix[1][1] = HL_TO_VR.y * 10.0f;
	hlToVRScaleMatrix[2][2] = HL_TO_VR.z * 10.0f;

	glm::mat4 hlToVRTranslateMatrix(1.0f);
	hlToVRTranslateMatrix[3][0] = translate.x;
	hlToVRTranslateMatrix[3][1] = translate.y;
	hlToVRTranslateMatrix[3][2] = translate.z;

	glm::mat4 switchYAndZTransitionMatrix(1.0f);
	switchYAndZTransitionMatrix[1][1] = 0;
	switchYAndZTransitionMatrix[1][2] = -1;
	switchYAndZTransitionMatrix[2][1] = 1;
	switchYAndZTransitionMatrix[2][2] = 0;

	glm::mat4 modelViewMatrix = absoluteTrackingMatrix * hlToVRScaleMatrix *switchYAndZTransitionMatrix * hlToVRTranslateMatrix;
	//modelViewMatrix = glm::scale(modelViewMatrix, glm::vec3(13.0f, 13.0f, 13.0f));
	return modelViewMatrix;
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
		vrSystem->SetTrackingSpace(isVRRoomScale ? VRTrackingSpace::Standing : VRTrackingSpace::Seated);
		vrSystem->WaitGetPoses(positions.m_rTrackedDevicePose);

		if (positions.m_rTrackedDevicePose[VRTrackedDeviceIndex_Hmd].isValid)
		{
			positions.m_mat4HmdModelView = GetDeviceTransform(VRTrackedDeviceIndex_Hmd);

			positions.m_mat4LeftProjection = GetHMDMatrixProjectionEye(VREye::Left);
			positions.m_mat4RightProjection = GetHMDMatrixProjectionEye(VREye::Right);

			positions.m_mat4LeftModelView = vrSystem->GetEyeToHeadTransform(VREye::Left) * positions.m_mat4HmdModelView;
			positions.m_mat4RightModelView = vrSystem->GetEyeToHeadTransform(VREye::Right)* positions.m_mat4HmdModelView;

			/*cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
			Vector clientGroundPosition = localPlayer->curstate.origin;
			clientGroundPosition.z += localPlayer->curstate.mins.z;
			positions.m_mat4LeftModelView = GetHMDMatrixPoseEye(VREye::Left) * GetModelViewMatrixFromAbsoluteTrackingMatrix(m_mat4HMDPose, -clientGroundPosition);
			positions.m_mat4RightModelView = GetHMDMatrixPoseEye(VREye::Right) * GetModelViewMatrixFromAbsoluteTrackingMatrix(m_mat4HMDPose, -clientGroundPosition);*/

			UpdateGunPosition();

			SendPositionUpdateToServer();

			return true;
		}
	}

	return false;
}

void VRHelper::PrepareVRScene(VREye eEye, struct ref_params_s* pparams)
{
	GetHLViewAnglesFromVRMatrix(positions.m_mat4LeftModelView).CopyToArray(pparams->viewangles);

	glBindFramebuffer(GL_FRAMEBUFFER, eEye == VREye::Left ? vrGLLeftEyeFrameBuffer : vrGLRightEyeFrameBuffer);

	glViewport(0, 0, vrRenderWidth, vrRenderHeight);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glLoadMatrixf(eEye == VREye::Left ? glm::value_ptr((positions.m_mat4LeftProjection)) : glm::value_ptr((positions.m_mat4RightProjection)));

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glLoadMatrixf(eEye == VREye::Left ? glm::value_ptr((positions.m_mat4LeftModelView)) : glm::value_ptr((positions.m_mat4RightModelView)));

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
	GetHLViewAnglesFromVRMatrix(positions.m_mat4HmdModelView).CopyToArray(angles);
}

void VRHelper::GetWalkAngles(float * angles)
{
	walkAngles.CopyToArray(angles);
}

void VRHelper::GetViewOrg(float * org)
{
	glm::mat4 hlSpaceView(1.0f);
	if (vr_renderEyeInWindow->value == 0.0f)
	{
		//Vector trans = GetPositionInHLSpaceFromAbsoluteTrackingMatrix(GetDeviceTransform(VRTrackedDeviceIndex_Hmd));
		//trans.CopyToArray(org);
		hlSpaceView = TransformVRSpaceToHLSpace(positions.m_mat4HmdModelView);
	}
	else if ( vr_renderEyeInWindow->value == 1.0f)
	{
		/*glm::mat4 a = positions.m_mat4LeftModelView;
		a = glm::inverse(a);
		glm::vec3 scale;
		glm::quat rot;
		glm::vec3 trans;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(a, scale, rot, trans, skew, perspective);
		Vector trans2(trans.x, trans.y, trans.z);
		trans2.CopyToArray(org);*/
		hlSpaceView = TransformVRSpaceToHLSpace(positions.m_mat4LeftModelView);
	}
	else
	{
		/*glm::mat4 a = positions.m_mat4RightModelView;
		a = glm::inverse(a);
		glm::vec3 scale;
		glm::quat rot;
		glm::vec3 trans;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(a, scale, rot, trans, skew, perspective);
		Vector trans2(trans.x, trans.y, trans.z);
		trans2.CopyToArray(org);*/
		hlSpaceView = TransformVRSpaceToHLSpace(positions.m_mat4RightModelView);
	}

	const glm::mat4 playerSpaceView = TranslateToPlayerView(hlSpaceView);
	const glm::vec3 translation = GetTranslationFromTransform(playerSpaceView);
	Vector hlVectorTranslation(translation.x, translation.y, translation.z);
	hlVectorTranslation.CopyToArray(org);
}

void VRHelper::UpdateGunPosition()
{
	cl_entity_t *viewent = gEngfuncs.GetViewModel();
	if (viewent != nullptr)
	{
		VRTrackedDeviceIndex controllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole::RightHand);

		if (controllerIndex > 0 && controllerIndex != VRTrackedDeviceIndexInvalid && positions.m_rTrackedDevicePose[controllerIndex].isValid)
		{
			glm::mat4 controllerAbsoluteTrackingMatrix = GetDeviceTransform(controllerIndex);

			glm::vec4 originInVRSpace = controllerAbsoluteTrackingMatrix * glm::vec4(0, 0, 0, 1);
			Vector originInRelativeHLSpace(originInVRSpace.x * VR_TO_HL.x * 10, -originInVRSpace.z * VR_TO_HL.z * 10, originInVRSpace.y * VR_TO_HL.y * 10);

			cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
			Vector originInHLSpace = localPlayer->curstate.origin + originInRelativeHLSpace;

			VectorCopy(originInHLSpace, viewent->origin);
			VectorCopy(originInHLSpace, viewent->curstate.origin);
			VectorCopy(originInHLSpace, viewent->latched.prevorigin);


			viewent->angles = GetHLAnglesFromVRMatrix(controllerAbsoluteTrackingMatrix);
			viewent->angles.x += vr_weapontilt->value;
			VectorCopy(viewent->angles, viewent->curstate.angles);
			VectorCopy(viewent->angles, viewent->latched.prevangles);


			Vector velocityInVRSpace = Vector(positions.m_rTrackedDevicePose[controllerIndex].velocity.x, positions.m_rTrackedDevicePose[controllerIndex].velocity.y, positions.m_rTrackedDevicePose[controllerIndex].velocity.z);
			Vector velocityInHLSpace(velocityInVRSpace.x * VR_TO_HL.x * 10, -velocityInVRSpace.z * VR_TO_HL.z * 10, velocityInVRSpace.y * VR_TO_HL.y * 10);
			viewent->curstate.velocity = velocityInHLSpace;
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

	Vector hmdOffset = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(GetDeviceTransform(VRTrackedDeviceIndex_Hmd));
	hmdOffset.z += localPlayer->curstate.mins.z;

	VRTrackedDeviceIndex leftControllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole::LeftHand);
	bool isLeftControllerValid = leftControllerIndex > 0 && leftControllerIndex != VRTrackedDeviceIndexInvalid && positions.m_rTrackedDevicePose[leftControllerIndex].isValid;

	VRTrackedDeviceIndex rightControllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole::RightHand);
	bool isRightControllerValid = leftControllerIndex > 0 && leftControllerIndex != VRTrackedDeviceIndexInvalid && positions.m_rTrackedDevicePose[leftControllerIndex].isValid;

	Vector leftControllerOffset(0, 0, 0);
	Vector leftControllerAngles(0, 0, 0);
	if (isLeftControllerValid)
	{
		glm::mat4 leftControllerAbsoluteTrackingMatrix = GetDeviceTransform(leftControllerIndex);
		leftControllerOffset = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(leftControllerAbsoluteTrackingMatrix);
		leftControllerOffset.z += localPlayer->curstate.mins.z;
		leftControllerAngles = GetHLAnglesFromVRMatrix(leftControllerAbsoluteTrackingMatrix);
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

	GetViewAngles(walkAngles);
	walkAngles = g_vrInput.vr_control_hand->value == 1.f ? leftControllerAngles : walkAngles;
	walkAngles = g_vrInput.vr_control_hand->value == 2.f ? weaponAngles : walkAngles;

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

glm::mat4 VRHelper::GetDeviceTransform(VRTrackedDeviceIndex deviceIndex)
{
	return invertCenterTransform * positions.m_rTrackedDevicePose[deviceIndex].transform;
}

glm::mat4 VRHelper::GetDeviceAbsoluteTransform(VRTrackedDeviceIndex deviceIndex)
{
	return positions.m_rTrackedDevicePose[deviceIndex].transform;
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
	centerTranslation.y += (localPlayer->curstate.mins.z * HL_TO_VR.z * 0.1f);
	centerTransform = glm::translate(centerTransform, centerTranslation);
	invertCenterTransform = glm::inverse(centerTransform);
}

void RenderLine(Vector v1, Vector v2, Vector color)
{
	glLineWidth(3.0f);
	glColor4f(color.x, color.y, color.z, 1.0f);
	glBegin(GL_LINES);
	glVertex3f(v1.x, v1.y, v1.z);
	glVertex3f(v2.x, v2.y, v2.z);
	glEnd();
}


void VRHelper::TestRenderControllerPosition(bool leftOrRight)
{
 	VRTrackedDeviceIndex controllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(leftOrRight ? VRTrackedControllerRole::LeftHand : VRTrackedControllerRole::RightHand);

	if (controllerIndex > 0 && controllerIndex != VRTrackedDeviceIndexInvalid && positions.m_rTrackedDevicePose[controllerIndex].isValid)
	{
		const glm::mat4 controllerAbsoluteTrackingMatrix = GetDeviceTransform(controllerIndex);
		const glm::mat4 controllerHLMatrix = TranslateToPlayerView(TransformVRSpaceToHLSpace(controllerAbsoluteTrackingMatrix));
		const glm::vec3 controllerTranslation = GetTranslationFromTransform(controllerHLMatrix);
		Vector originInHLSpace = Vector(controllerTranslation.x, controllerTranslation.y, controllerTranslation.z);
		
		glm::vec4 forwardInVRSpace = controllerHLMatrix * glm::vec4(0, 10, 0, 0);
		glm::vec4 rightInVRSpace = controllerHLMatrix * glm::vec4(10, 0, 0, 0);
		glm::vec4 upInVRSpace = controllerHLMatrix * glm::vec4(0, 0, 10, 0);

		Vector forward(forwardInVRSpace.x, forwardInVRSpace.y, forwardInVRSpace.z);
		Vector right(rightInVRSpace.x, rightInVRSpace.y, rightInVRSpace.z);
		Vector up(upInVRSpace.x, upInVRSpace.y, upInVRSpace.z);

		/*Vector forward(forwardInVRSpace.x * VR_TO_HL.x * 3, -forwardInVRSpace.z * VR_TO_HL.z * 3, forwardInVRSpace.y * VR_TO_HL.y * 3);
		Vector right(rightInVRSpace.x * VR_TO_HL.x * 3, -rightInVRSpace.z * VR_TO_HL.z * 3, rightInVRSpace.y * VR_TO_HL.y * 3);
		Vector up(upInVRSpace.x * VR_TO_HL.x * 3, -upInVRSpace.z * VR_TO_HL.z * 3, upInVRSpace.y * VR_TO_HL.y * 3);*/
/*
		if (leftOrRight)
		{
			right = -right; // left
		}
*/	
		RenderLine(originInHLSpace, originInHLSpace + forward, Vector(1, 0, 0));
		RenderLine(originInHLSpace, originInHLSpace + right, Vector(0, 1, 0));
		RenderLine(originInHLSpace, originInHLSpace + up, Vector(0, 0, 1));
	}
}


