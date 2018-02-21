#pragma once

#include "vr_system.h"

#include <vector>

class Positions
{
public:
	std::vector<VRTrackedDevicePose> m_rTrackedDevicePose;

	Matrix4 m_mat4LeftProjection;
	Matrix4 m_mat4RightProjection;

	Matrix4 m_mat4LeftModelView;
	Matrix4 m_mat4RightModelView;
};

class VRHelper
{
public:
	VRHelper();
	~VRHelper();

	void Init();

	void PollEvents();
	bool UpdatePositions(struct ref_params_s* pparams);
	void SubmitImages();
	void PrepareVRScene(VREye eEye, struct ref_params_s* pparams);
	void FinishVRScene(struct ref_params_s* pparams);

	void GetViewAngles(float * angles);
	void GetViewOrg(float * angles);

	void GetWalkAngles(float * angles);

	void TestRenderControllerPosition(bool leftOrRight);

private:

	bool AcceptsDisclaimer();
	void Exit(const char* lpErrorMessage = nullptr);

	void UpdateGunPosition(struct ref_params_s* pparams);
	void SendPositionUpdateToServer();

	Matrix4 GetHMDMatrixProjectionEye(VREye eEye);
	Matrix4 GetHMDMatrixPoseEye(VREye eEye);

	Vector GetHLViewAnglesFromVRMatrix(const Matrix4 &mat);
	Vector GetHLAnglesFromVRMatrix(const Matrix4 &mat);

	Positions positions;

	Vector walkAngles;

	bool isVRRoomScale = true;

	IVRSystem* vrSystem = nullptr;

	unsigned int vrGLLeftEyeFrameBuffer = 0;
	unsigned int vrGLRightEyeFrameBuffer = 0;

	unsigned int vrGLLeftEyeTexture = 0;
	unsigned int vrGLRightEyeTexture = 0;

	unsigned int vrGLMenuTexture = 0;
	unsigned int vrGLHUDTexture = 0;

	unsigned int vrRenderWidth = 0;
	unsigned int vrRenderHeight = 0;
};
