#pragma once

#include "vr_system.h"

#include "glm/matrix.hpp"

#include <vector>

class VRPositions
{
public:
	std::vector<VRTrackedDevicePose> m_rTrackedDevicePose;

	glm::mat4 headsetLeftEyeProjection;
	glm::mat4 headsetRightEyeProjection;

	glm::mat4 headsetModelView;

	glm::mat4 headsetLeftEyeModelView;
	glm::mat4 headsetRightEyeModelView;
};

class VRHelper
{
public:
	VRHelper();
	~VRHelper();

	void Init();

	void PollEvents();
	bool UpdatePositions();
	void SubmitImages();
	void PrepareVRScene(VREye eEye, struct ref_params_s* pparams);
	void FinishVRScene(struct ref_params_s* pparams);

	void GetViewAngles(float * angles);
	void GetViewOrg(float * angles);

	void GetWalkAngles(float * angles);

	glm::mat4 GetDeviceTransform(VRTrackedDeviceIndex deviceIndex);
	glm::mat4 GetDeviceAbsoluteTransform(VRTrackedDeviceIndex deviceIndex);

	void Recenter();

	void TestRenderControllerPosition(bool leftOrRight);
private:

	bool AcceptsDisclaimer();
	void Exit(const char* lpErrorMessage = nullptr);

	void UpdateGunPosition();
	void SendPositionUpdateToServer();

	glm::mat4 GetHMDMatrixProjectionEye(VREye eEye);

	Vector GetPlayerViewOrg();
	glm::vec3 GetTranslationFromTransform(const glm::mat4& transform);
	glm::quat GetRotationFromTransform(const glm::mat4& transform);
	glm::mat4 TransformVRSpaceToHLSpace(const glm::mat4 &absoluteTrackingMatrix, Vector translate);

	Vector GetAnglesFromMatrix(const glm::mat4 &mat);
	Vector GetHLAnglesFromVRMatrix(const glm::mat4 &mat);

	VRPositions positions;

	Vector walkAngles;

	glm::mat4 centerTransform;
	glm::mat4 invertCenterTransform;

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

extern VRHelper gVRHelper;