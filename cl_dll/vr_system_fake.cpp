#include "hud.h"

#include "vr_system_fake.h"

#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cassert>
#include <algorithm>
#include <windows.h>
#include <gl\gl.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846f
#endif

#undef min
#undef max

extern int mouseactive;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, message, wParam, lParam);
}

VRSystem_Fake::VREyeRenderWindow::VREyeRenderWindow() :
	m_hdc(NULL),
	m_hrc(NULL),
	m_hinstance(NULL),
	m_window(NULL)
{

}

bool VRSystem_Fake::VREyeRenderWindow::Init(VREye eye)
{
	GLuint pixelFormat;
	WNDCLASS wc;
	DWORD exStyle;
	DWORD style;
	RECT windowRect;

	m_className = eye == VREye::Left ? "VRFake_Left" : "VRFake_Right",

	windowRect.left = (long)0;
	windowRect.right = (long)800;
	windowRect.top = (long)0;
	windowRect.bottom = (long)600;

	m_hinstance = GetModuleHandle(NULL);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hinstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = m_className.c_str();

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, "Failed To Register The Window Class.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	exStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	style = WS_OVERLAPPEDWINDOW;

	AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);

	if (!(m_window = CreateWindowEx(exStyle,
		m_className.c_str(),
		eye == VREye::Left ? "Left Eye" : "Right Eye",
		style |
		WS_CLIPSIBLINGS |
		WS_CLIPCHILDREN,
		0, 0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		m_hinstance,
		NULL)))
	{
		Shutdown();
		MessageBox(NULL, "Window Creation Error.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	static	PIXELFORMATDESCRIPTOR pfd =	
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		16,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	if (!(m_hdc = GetDC(m_window)))
	{
		Shutdown();
		MessageBox(NULL, "Can't Create A GL Device Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if (!(pixelFormat = ChoosePixelFormat(m_hdc, &pfd)))
	{
		Shutdown();
		MessageBox(NULL, "Can't Find A Suitable PixelFormat.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if (!SetPixelFormat(m_hdc, pixelFormat, &pfd))
	{
		Shutdown();
		MessageBox(NULL, "Can't Set The PixelFormat.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if (!(m_hrc = wglCreateContext(m_hdc)))
	{
		Shutdown();
		MessageBox(NULL, "Can't Create A GL Rendering Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	HGLRC currentContext = wglGetCurrentContext();
	HDC currentDC = wglGetCurrentDC();

	wglShareLists(currentContext, m_hrc);

	ShowWindow(m_window, SW_SHOW);

	return true;
}

void VRSystem_Fake::VREyeRenderWindow::Shutdown()
{
	if (m_hrc)
	{
		if (!wglDeleteContext(m_hrc))
		{
			MessageBox(NULL, "Release Rendering Context Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		}
		m_hrc = NULL;
	}

	if (m_hdc && !ReleaseDC(m_window, m_hdc))
	{
		MessageBox(NULL, "Release Device Context Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		m_hdc = NULL;
	}

	if (m_window && !DestroyWindow(m_window))
	{
		MessageBox(NULL, "Could Not Release hWnd.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		m_window = NULL;
	}

	if (!UnregisterClass(m_className.c_str(), m_hinstance))
	{
		MessageBox(NULL, "Could Not Unregister Class.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		m_hinstance = NULL;
	}
}

void VRSystem_Fake::VREyeRenderWindow::Render(uint32_t textureHandle)
{
	HGLRC prevContext = wglGetCurrentContext();
	HDC prevDC = wglGetCurrentDC();

	wglMakeCurrent(m_hdc, m_hrc);

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-1.0f, -1.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(-1.0f, 1.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(1.0f, 1.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(1.0f, -1.0f);
	glEnd();

	SwapBuffers(m_hdc);

	wglMakeCurrent(prevDC, prevContext);
}

//////////////////////////////////////////////////////////////////////////

VRSystem_Fake::VRSystem_Fake() :
	controlledDeviceIdx(0),
	justChangedControlledDevice(false)
{
	memset(fakeDevices.data(), 0, sizeof(VRFakeDevice));
	memset(fakeControllers.data(), 0, sizeof(VRControllerState));
}

bool VRSystem_Fake::Init()
{
	fakeDevices[FakeHeadsetIdx].position += glm::vec3(0.0f, 1.5f, 0.0f);
	fakeDevices[FakeLeftHandControllerIdx].position += glm::vec3(0.5f, 1.0f, -0.5f);
	fakeDevices[FakeRightHandControllerIdx].position += glm::vec3(-0.5f, 1.0f, -0.5f);

	gEngfuncs.pfnSetMousePos(gEngfuncs.GetWindowCenterX(), gEngfuncs.GetWindowCenterY());
	prevTime = gEngfuncs.GetClientTime();

	leftEyeRender.Init(VREye::Left);
	rightEyeRender.Init(VREye::Right);

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

	if (mouseX == 0 || mouseY == 0) // GetMousePosition will always return 0 until the player moves its mouse at least once.
	{
		return;
	}

	const int deltaMouseX = mouseX - gEngfuncs.GetWindowCenterX();
	const int deltaMouseY = mouseY - gEngfuncs.GetWindowCenterY();

	VRFakeDevice& fakeDevice = fakeDevices[controlledDeviceIdx];
	fakeDevice.rotation.y -= deltaMouseX * M_PI * 0.001f;
	fakeDevice.rotation.x -= deltaMouseY * M_PI * 0.001f;

	// Prevent x rotation from going too far up or down
	fakeDevice.rotation.x = std::min(std::max(-M_PI * 0.5f + 0.1f, fakeDevice.rotation.x), M_PI * 0.5f - 0.1f);

	glm::vec3 forward(-sinf(fakeDevice.rotation.y), 0.0f, -cosf(fakeDevice.rotation.y));
	glm::vec3 left(sinf(fakeDevice.rotation.y - M_PI * 0.5f), 0.0f, cosf(fakeDevice.rotation.y - M_PI * 0.5f));
	glm::vec3 up(0.0f, 1.0f, 0.0f);

	gEngfuncs.pfnSetMousePos(gEngfuncs.GetWindowCenterX(), gEngfuncs.GetWindowCenterY());

	BYTE keyboardState[256];
	GetKeyboardState(keyboardState);

	if (keyboardState[VK_UP] >> 4)
	{
		fakeDevice.position += glm::vec3(forward) * deltaTime;
	}

	if (keyboardState[VK_DOWN] >> 4)
	{
		fakeDevice.position -= glm::vec3(forward) * deltaTime;
	}

	if (keyboardState[VK_LEFT] >> 4)
	{
		fakeDevice.position += glm::vec3(left) * deltaTime;
	}

	if (keyboardState[VK_RIGHT] >> 4)
	{
		fakeDevice.position -= glm::vec3(left) * deltaTime;
	}

	if (keyboardState[VK_PRIOR] >> 4)
	{
		fakeDevice.position += glm::vec3(up) * deltaTime;
	}

	if (keyboardState[VK_NEXT] >> 4)
	{
		fakeDevice.position -= glm::vec3(up) * deltaTime;
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
	SetControllerButtonState(controllerState, VRButton::System, (keyboardState[VK_F1] >> 4) != 0);
	SetControllerButtonState(controllerState, VRButton::ApplicationMenu, (keyboardState[VK_F2] >> 4) != 0);
	SetControllerButtonState(controllerState, VRButton::Grip, (keyboardState[VK_RBUTTON] >> 4) != 0);

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

		const glm::mat4 translation = glm::translate(glm::mat4(1.0f), fakeDevice.position);

		const glm::vec3 forward(0.0f, 0.0f, -1.0f);
		const glm::vec3 right(1.0f, 0.0f, 0.0f);
		const glm::vec3 up(0.0f, 1.0f, 0.0f);

		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), fakeDevice.rotation.y, up);
		rotation = glm::rotate(rotation, fakeDevice.rotation.x, right);
		rotation = glm::rotate(rotation, fakeDevice.rotation.z, forward);

		const glm::mat4 lookAt = glm::lookAtRH(glm::vec3(0.0f, 0.0f, 0.0f), forward, up);

		trackedPose.transform = translation * lookAt * rotation;

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
	switch (eye)
	{
		case VREye::Left:
			leftEyeRender.Render(textureHandle);
			break;
		case VREye::Right:
			rightEyeRender.Render(textureHandle);
			break;
	}
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

glm::mat4 VRSystem_Fake::GetProjectionMatrix(VREye eye, float nearZ, float farZ)
{
	// Since we have nothing to render, we don't care about that, we will just return an identity matrix
	glm::mat4 identity(1.0f);
	return glm::perspectiveRH(M_PI * 0.5f, 4.0f / 3.0f, nearZ, farZ);
}

glm::mat4 VRSystem_Fake::GetEyeToHeadTransform(VREye eye)
{
	// Since the eye is at the same place as the headset in fake VR, return an identity Matrix
	// TODO : We could return a real eye to head transform now since we have a eye render
	glm::mat4 identity(1.0f);
	return identity;
}

glm::mat4 VRSystem_Fake::GetRawZeroPoseToStandingAbsoluteTrackingPose()
{
	// TODO but isn't used
	glm::mat4 identity(1.0f);
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