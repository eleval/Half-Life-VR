
#include <windows.h>
#include "hud.h"
#include "vr_input.h"
#include "vr_helper.h"

#include "cl_util.h"

#include "glm/vec3.hpp"
#include "glm/trigonometric.hpp"

VRInput g_vrInput;

VRInput::VRInput()
{

}

VRInput::~VRInput()
{

}

void VRInput::HandleButtonPress(VRButton button, VRControllerState controllerState, bool isLeftController, bool buttonDown)
{
	if (vr_control_scheme->value == 0.0f)
	{
		HandleButtonPressOld(button, controllerState, isLeftController, buttonDown);
	}
	else if(vr_control_scheme->value == 1.0f)
	{
		HandleButtonPressNew(button, controllerState, isLeftController, buttonDown);
	}
}

void VRInput::HandleTrackpad(VRButton button, VRControllerState controllerState, bool isLeftController, bool buttonDown)
{
	if ( vr_control_scheme->value == 0.0f )
	{
		HandleTrackpadOld(button, controllerState, isLeftController, buttonDown);
	} else if ( vr_control_scheme->value == 1.0f )
	{
		HandleTrackpadNew(button, controllerState, isLeftController, buttonDown);
	}
}

//////////////////////////////////////////////////////////////////////////

void VRInput::HandleButtonPressOld(VRButton button, VRControllerState controllerState, bool isLeftController, bool buttonDown)
{
	if (isLeftController)
	{				// Left controller button presses start here.
		switch (button)
		{
			case VRButton::ApplicationMenu:
			{
				ClientCmd("save quick");
			}
			break;
			case VRButton::SteamVR_Trigger:
			{
				buttonDown ? ClientCmd("+use") : ClientCmd("-use");
			}
			break;
			case VRButton::SteamVR_Touchpad:
			{
				if (vr_control_teleport->value == 1.f)
				{
					if (buttonDown && isTeleActive) {
						ServerCmd("vrtele 0");
						isTeleActive = false;
					}
					} else
					buttonDown ? ClientCmd("cl_forwardspeed 400") : ClientCmd("cl_forwardspeed 175");
					buttonDown ? ClientCmd("cl_backwardspeed 400") : ClientCmd("cl_backwardspeed 175");
					buttonDown ? ClientCmd("cl_sidespeed 400") : ClientCmd("cl_sidespeed 175");
			}
			break;
			case VRButton::Grip:
			{
			
				buttonDown ? ClientCmd("Impulse 100"): ClientCmd("Impulse");
				//downOrUp ? ServerCmd("vr_grabweapon 1") : ServerCmd("vr_grabweapon 0");

				//I decided not to atempt this now. A quick and dirty way to make two handed
				//	weapons is a quaternion/matrix lookat function. The weapon rotation is in
				//  Euler angles though, so a little bit of conversion is necessary. Needs to
				//	be done in UpdateGunPosition.
			}
			break;
		}
	} else
	{			// Right controller button presses start here & left controller input end.
		switch (button)
		{
			case VRButton::ApplicationMenu:
			{
				buttonDown ? ClientCmd("+jump") : ClientCmd("-jump");
			}
			break;
			case VRButton::Grip:
			{
				buttonDown ? ClientCmd("+attack2") : ClientCmd("-attack2");
			}
			break;
			case VRButton::SteamVR_Trigger:
			{
				buttonDown ? ClientCmd("+attack") : ClientCmd("-attack");
			}
			break;
			case VRButton::SteamVR_Touchpad:
			{
				//ServerCmd(downOrUp ? "vrtele 1" : "vrtele 0");
				const VRControllerAxis& touchPadAxis = controllerState.axis[0];

				if (touchPadAxis.x < -0.5f && !buttonDown)
				{
					gHUD.m_Ammo.UserCmd_NextWeapon();
					gHUD.m_iKeyBits |= IN_ATTACK;
					gHUD.m_Ammo.Think();
					} else if ( touchPadAxis.x > 0.5f && !buttonDown )
				{
					gHUD.m_Ammo.UserCmd_PrevWeapon();
					gHUD.m_iKeyBits |= IN_ATTACK;
					gHUD.m_Ammo.Think();
				}

				if (touchPadAxis.y < -0.5f && buttonDown)
				{
					ClientCmd("+reload");				
					} else
				{
					ClientCmd("-reload");				
				}

				if (touchPadAxis.y > 0.5f && buttonDown)
				{
					ServerCmd("vrtele 1");
					} else
				{
					ServerCmd("vrtele 0");
				}

			}
			break;
		}
	}
}

void VRInput::HandleTrackpadOld(VRButton button, VRControllerState controllerState, bool isLeftController, bool buttonDown)
{
	const VRControllerAxis& touchPadAxis = controllerState.axis[0];
	buttonDown = (
		touchPadAxis.x < -vr_control_deadzone->value ||
		touchPadAxis.x > vr_control_deadzone->value ||
		touchPadAxis.y < -vr_control_deadzone->value ||
		touchPadAxis.y > vr_control_deadzone->value
		);

	if (isLeftController && vr_control_teleport->value != 1.f)
	{
		if (vr_control_alwaysforward->value == 1.f)
			buttonDown ? ClientCmd("+forward") : ClientCmd("-forward");
		else
		{
			if (touchPadAxis.x < -vr_control_deadzone->value)
			{
				ClientCmd("+moveleft");
			} else
			{
				ClientCmd("-moveleft");
			}

			if (touchPadAxis.x > vr_control_deadzone->value)
			{
				ClientCmd("+moveright");
			} else
			{
				ClientCmd("-moveright");
			}

			if (touchPadAxis.y > vr_control_deadzone->value)
			{
				ClientCmd("+forward");
			} else
			{
				ClientCmd("-forward");
			}

			if (touchPadAxis.y < -vr_control_deadzone->value)
			{
				ClientCmd("+back");
			} else
			{
				ClientCmd("-back");
			}
		}
	} else if ( isLeftController && vr_control_teleport->value == 1.f )
	{
		if (buttonDown && !isTeleActive)
		{
			ServerCmd("vrtele 1");
			isTeleActive = true;
		} else if ( !buttonDown && isTeleActive )
		{
			ServerCmd("vrtele 2");
			isTeleActive = false;
		}
	}
}

void VRInput::HandleButtonPressNew(VRButton button, VRControllerState controllerState, bool isLeftController, bool buttonDown)
{
	if (isLeftController)
	{
		HandleButtonPressLeft(button, controllerState, buttonDown);
	}
	else
	{
		HandleButtonPressRight(button, controllerState, buttonDown);
	}
}

void VRInput::HandleTrackpadNew(VRButton button, VRControllerState controllerState, bool isLeftController, bool buttonDown)
{
	// Trackpad isn't used (yet) in the new control scheme
}

void VRInput::HandleButtonPressLeft(VRButton button, VRControllerState controllerState, bool buttonDown)
{
	// Control scheme is as follow:
	// Trackpad up : Walk forward
	// Trackpad down : Run forward
	// Trackpad right : Rotate right (snap)
	// Trackpad left : Rotate left (snap)
	// Menu Button : Flashlight
	// Grip : QuickSave
	// Trigger : Use

	switch (button)
	{
		case VRButton::Grip:
		{
			ClientCmd("save quick");
		}
		break;
		case VRButton::SteamVR_Trigger:
		{
			buttonDown ? ClientCmd("+use") : ClientCmd("-use");
		}
		break;
		case VRButton::SteamVR_Touchpad:
		{
			const VRControllerAxis& touchPadAxis = controllerState.axis[ 0 ];

			if (buttonDown)
			{
				const float rotationAngle = glm::radians(vr_control_rotation_angle->value);
				const glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
				if (touchPadAxis.x <= -0.5f) // Touchpad left
				{
					gVRHelper.RotateCamera(rotationAngle, rotationAxis);
				}
				else if (touchPadAxis.x >= 0.5f) // Touchpad right
				{
					gVRHelper.RotateCamera(-rotationAngle, rotationAxis);
				}
				else if (touchPadAxis.y <= -0.5f) // Touchpad down
				{
					// Run forward
					ClientCmd("cl_forwardspeed 400");
					ClientCmd("cl_backwardspeed 400");
					ClientCmd("cl_sidespeed 400");
					ClientCmd("+forward");
				}
				else if (touchPadAxis.y >= 0.5f) // Touchpad up
				{
					// Walk forward
					ClientCmd("cl_forwardspeed 175");
					ClientCmd("cl_backwardspeed 175");
					ClientCmd("cl_sidespeed 175");
					ClientCmd("+forward");
				}
			}
			else
			{
				ClientCmd("-forward");
			}
		}
		break;
		case VRButton::ApplicationMenu:
		{
			buttonDown ? ClientCmd("Impulse 100") : ClientCmd("Impulse");
		}
		break;
	}
}

void VRInput::HandleButtonPressRight(VRButton button, VRControllerState controllerState, bool buttonDown)
{
	// Control scheme is as follow :
	// Trackpad up : Duck
	// Trackpad down : Reload
	// Trackpad right : Next Weapon
	// Trackpad left : Previous Weapon
	// Menu Button : Jump
	// Grip : Fire2
	// Trigger : Fire1

	switch (button)
	{
		case VRButton::Grip:
		{
			buttonDown ? ClientCmd("+attack2") : ClientCmd("-attack2");
		}
		break;
		case VRButton::SteamVR_Trigger:
		{
			buttonDown ? ClientCmd("+attack") : ClientCmd("-attack");
		}
		break;
		case VRButton::SteamVR_Touchpad:
		{
			const VRControllerAxis& touchPadAxis = controllerState.axis[ 0 ];

			if (touchPadAxis.x <= -0.5f && buttonDown) // Touchpad left
			{
				gHUD.m_Ammo.UserCmd_PrevWeapon();
				gHUD.m_iKeyBits |= IN_ATTACK;
				gHUD.m_Ammo.Think();
			}
			else if (touchPadAxis.x >= 0.5f && buttonDown) // Touchpad right
			{
				gHUD.m_Ammo.UserCmd_NextWeapon();
				gHUD.m_iKeyBits |= IN_ATTACK;
				gHUD.m_Ammo.Think();
			}
			else if (touchPadAxis.y <= -0.5f) // Touchpad down
			{
				buttonDown ? ClientCmd("+reload") : ClientCmd("-reload");
			}
			else if ( touchPadAxis.y >= 0.5f ) // Touchpad up
			{
				buttonDown ? ClientCmd("+duck") : ClientCmd("-duck");
			}
		}
		break;
		case VRButton::ApplicationMenu:
		{
			buttonDown ? ClientCmd("+jump") : ClientCmd("-jump");
		}
		break;
	}
}

void VRInput::HandleTrackpadLeft(VRButton button, VRControllerState controllerState, bool buttonDown)
{
	// Trackpad isn't used (yet) in the new control scheme
}

void VRInput::HandleTrackpadRight(VRButton button, VRControllerState controllerState, bool buttonDown)
{
	// Trackpad isn't used (yet) in the new control scheme
}