
#include <windows.h>
#include "Matrices.h"
#include "hud.h"
#include "cl_util.h"
#include "r_studioint.h"
#include "ref_params.h"
#include "vr_helper.h"
#include "vr_gl.h"
#include "vr_input.h"
#include "vr_system_openvr.h"
#include "vr_system_fake.h"

#ifndef MAX_COMMAND_SIZE
#define MAX_COMMAND_SIZE 256
#endif

extern engine_studio_api_t IEngineStudio;

const Vector3 HL_TO_VR(2.3f / 10.f, 2.2f / 10.f, 2.3f / 10.f);
const Vector3 VR_TO_HL(1.f / HL_TO_VR.x, 1.f / HL_TO_VR.y, 1.f / HL_TO_VR.z);
const float FLOOR_OFFSET = 10;

VRHelper gVRHelper;

cvar_t *vr_weapontilt;
cvar_t *vr_roomcrouch;
cvar_t* vr_systemType;
cvar_t* vr_showPlayer;

void VR_Recenter()
{
	gVRHelper.Recenter();
}

VRHelper::VRHelper()
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

Matrix4 VRHelper::GetHMDMatrixProjectionEye(VREye eEye)
{
	float fNearZ = 0.01f;
	float fFarZ = 8192.f;
	return vrSystem->GetProjectionMatrix(eEye, fNearZ, fFarZ);
}

Matrix4 VRHelper::GetHMDMatrixPoseEye(VREye nEye)
{
	return vrSystem->GetEyeToHeadTransform(nEye).invert();
}

extern void VectorAngles(const float *forward, float *angles);

Vector VRHelper::GetHLViewAnglesFromVRMatrix(const Matrix4 &mat)
{
	Vector4 v1 = mat * Vector4(1, 0, 0, 0);
	Vector4 v2 = mat * Vector4(0, 1, 0, 0);
	Vector4 v3 = mat * Vector4(0, 0, 1, 0);
	v1.normalize();
	v2.normalize();
	v3.normalize();
	Vector angles;
	VectorAngles(Vector(-v1.z, -v2.z, -v3.z), angles);
	angles.x = 360.f - angles.x;	// viewangles pitch is inverted
	return angles;
}

Vector VRHelper::GetHLAnglesFromVRMatrix(const Matrix4 &mat)
{
	Vector4 forwardInVRSpace = mat * Vector4(0, 0, -1, 0);
	Vector4 rightInVRSpace = mat * Vector4(1, 0, 0, 0);
	Vector4 upInVRSpace = mat * Vector4(0, 1, 0, 0);

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

Matrix4 GetModelViewMatrixFromAbsoluteTrackingMatrix(Matrix4 &absoluteTrackingMatrix, Vector translate)
{
	Matrix4 hlToVRScaleMatrix;
	hlToVRScaleMatrix[0] = HL_TO_VR.x;
	hlToVRScaleMatrix[5] = HL_TO_VR.y;
	hlToVRScaleMatrix[10] = HL_TO_VR.z;

	Matrix4 hlToVRTranslateMatrix;
	hlToVRTranslateMatrix[12] = translate.x;
	hlToVRTranslateMatrix[13] = translate.y;
	hlToVRTranslateMatrix[14] = translate.z;

	Matrix4 switchYAndZTransitionMatrix;
	switchYAndZTransitionMatrix[5] = 0;
	switchYAndZTransitionMatrix[6] = -1;
	switchYAndZTransitionMatrix[9] = 1;
	switchYAndZTransitionMatrix[10] = 0;

	Matrix4 modelViewMatrix = absoluteTrackingMatrix * hlToVRScaleMatrix *switchYAndZTransitionMatrix * hlToVRTranslateMatrix;
	modelViewMatrix.scale(13);
	return modelViewMatrix;
}

Vector GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(const Matrix4 & absoluteTrackingMatrix)
{
	Vector4 originInVRSpace = absoluteTrackingMatrix * Vector4(0, 0, .10, 1);
	return Vector(originInVRSpace.x * VR_TO_HL.x * 10, -originInVRSpace.z * VR_TO_HL.z * 10, originInVRSpace.y * VR_TO_HL.y * 10);
}

Vector GetPositionInHLSpaceFromAbsoluteTrackingMatrix(const Matrix4 & absoluteTrackingMatrix)
{
	Vector originInRelativeHLSpace = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(absoluteTrackingMatrix);

	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
	Vector clientGroundPosition = localPlayer->curstate.origin;
	clientGroundPosition.z += localPlayer->curstate.mins.z;

	return clientGroundPosition + originInRelativeHLSpace;
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

bool VRHelper::UpdatePositions(struct ref_params_s* pparams)
{
	if (vrSystem != nullptr)
	{
		//vrCompositor->GetCurrentSceneFocusProcess();
		vrSystem->SetTrackingSpace(isVRRoomScale ? VRTrackingSpace::Standing : VRTrackingSpace::Seated);
		vrSystem->WaitGetPoses(positions.m_rTrackedDevicePose);

		if (positions.m_rTrackedDevicePose[VRTrackedDeviceIndex_Hmd].isValid)
		{
			//Matrix4 m_mat4SeatedPose = ConvertSteamVRMatrixToMatrix4(vrSystem->GetSeatedZeroPoseToStandingAbsoluteTrackingPose()).invert();
			//Matrix4 m_mat4StandingPose = vrInterface->GetRawZeroPoseToStandingAbsoluteTrackingPose().invert();

			Matrix4 m_mat4HMDPose = GetDeviceTransform(VRTrackedDeviceIndex_Hmd);
			m_mat4HMDPose.invert();

			positions.m_mat4LeftProjection = GetHMDMatrixProjectionEye(VREye::Left);
			positions.m_mat4RightProjection = GetHMDMatrixProjectionEye(VREye::Right);

			cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
			Vector clientGroundPosition = localPlayer->curstate.origin;
			clientGroundPosition.z += localPlayer->curstate.mins.z;
			positions.m_mat4LeftModelView = GetHMDMatrixPoseEye(VREye::Left) * GetModelViewMatrixFromAbsoluteTrackingMatrix(m_mat4HMDPose, -clientGroundPosition);
			positions.m_mat4RightModelView = GetHMDMatrixPoseEye(VREye::Right) * GetModelViewMatrixFromAbsoluteTrackingMatrix(m_mat4HMDPose, -clientGroundPosition);

			UpdateGunPosition(pparams);

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
	glLoadMatrixf(eEye == VREye::Left ? positions.m_mat4LeftProjection.get() : positions.m_mat4RightProjection.get());

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glLoadMatrixf(eEye == VREye::Left ? positions.m_mat4LeftModelView.get() : positions.m_mat4RightModelView.get());

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
	GetHLViewAnglesFromVRMatrix(positions.m_mat4LeftModelView).CopyToArray(angles);
}

void VRHelper::GetWalkAngles(float * angles)
{
	walkAngles.CopyToArray(angles);
}

void VRHelper::GetViewOrg(float * org)
{
	GetPositionInHLSpaceFromAbsoluteTrackingMatrix(GetDeviceTransform(VRTrackedDeviceIndex_Hmd)).CopyToArray(org);
}

void VRHelper::UpdateGunPosition(struct ref_params_s* pparams)
{
	cl_entity_t *viewent = gEngfuncs.GetViewModel();
	if (viewent != nullptr)
	{
		VRTrackedDeviceIndex controllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(VRTrackedControllerRole::RightHand);

		if (controllerIndex > 0 && controllerIndex != VRTrackedDeviceIndexInvalid && positions.m_rTrackedDevicePose[controllerIndex].isValid)
		{
			Matrix4 controllerAbsoluteTrackingMatrix = GetDeviceTransform(controllerIndex);

			Vector4 originInVRSpace = controllerAbsoluteTrackingMatrix * Vector4(0, 0, 0, 1);
			Vector originInRelativeHLSpace(originInVRSpace.x * VR_TO_HL.x * 10, -originInVRSpace.z * VR_TO_HL.z * 10, originInVRSpace.y * VR_TO_HL.y * 10);

			cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
			Vector clientGroundPosition = localPlayer->curstate.origin;
			clientGroundPosition.z += localPlayer->curstate.mins.z;
			Vector originInHLSpace = clientGroundPosition + originInRelativeHLSpace;

			VectorCopy(originInHLSpace, viewent->origin);
			VectorCopy(originInHLSpace, viewent->curstate.origin);
			VectorCopy(originInHLSpace, viewent->latched.prevorigin);


			viewent->angles = GetHLAnglesFromVRMatrix(controllerAbsoluteTrackingMatrix);
			viewent->angles.x += vr_weapontilt->value;
			VectorCopy(viewent->angles, viewent->curstate.angles);
			VectorCopy(viewent->angles, viewent->latched.prevangles);


			Vector velocityInVRSpace = positions.m_rTrackedDevicePose[controllerIndex].velocity;
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
		Matrix4 leftControllerAbsoluteTrackingMatrix = GetDeviceTransform(leftControllerIndex);
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

Matrix4 VRHelper::GetDeviceTransform(VRTrackedDeviceIndex deviceIndex)
{
	return invertCenterTransform * positions.m_rTrackedDevicePose[deviceIndex].transform;
}

Matrix4 VRHelper::GetDeviceAbsoluteTransform(VRTrackedDeviceIndex deviceIndex)
{
	return positions.m_rTrackedDevicePose[deviceIndex].transform;
}

void VRHelper::Recenter()
{
	Matrix4 hmdTransform = GetDeviceAbsoluteTransform(VRTrackedDeviceIndex_Hmd);
	centerTransform.identity();
	Vector3 centerTranslation;
	centerTranslation = hmdTransform.getTranslation();
	centerTranslation.y = 0.0f; // Ignore height difference
	centerTransform.translate(centerTranslation);
	invertCenterTransform = centerTransform;
	invertCenterTransform.invert();
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
		Matrix4 controllerAbsoluteTrackingMatrix = GetDeviceTransform(controllerIndex);

		Vector originInHLSpace = GetPositionInHLSpaceFromAbsoluteTrackingMatrix(controllerAbsoluteTrackingMatrix);
		
		Vector4 forwardInVRSpace = controllerAbsoluteTrackingMatrix * Vector4(0, 0, -1, 0);
		Vector4 rightInVRSpace = controllerAbsoluteTrackingMatrix * Vector4(-1, 0, 0, 0);
		Vector4 upInVRSpace = controllerAbsoluteTrackingMatrix * Vector4(0, 1, 0, 0);

		Vector forward(forwardInVRSpace.x * VR_TO_HL.x * 3, -forwardInVRSpace.z * VR_TO_HL.z * 3, forwardInVRSpace.y * VR_TO_HL.y * 3);
		Vector right(rightInVRSpace.x * VR_TO_HL.x * 3, -rightInVRSpace.z * VR_TO_HL.z * 3, rightInVRSpace.y * VR_TO_HL.y * 3);
		Vector up(upInVRSpace.x * VR_TO_HL.x * 3, -upInVRSpace.z * VR_TO_HL.z * 3, upInVRSpace.y * VR_TO_HL.y * 3);
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


