#include "keyboard.h"

namespace gef
{
	static bool key_printable[gef::Keyboard::NUM_KEY_CODES] =
	{
		true,//KC_0 = 0,
		true,//KC_1,
		true,//KC_2,
		true,//KC_3,
		true,//KC_4,
		true,//KC_5,
		true,//KC_6,
		true,//KC_7,
		true,//KC_8,
		true,//KC_9,
		true,//KC_A,
		true,//KC_B,
		true,//KC_C,
		true,//KC_D,
		true,//KC_E,
		true,//KC_F,
		true,//KC_G,
		true,//KC_H,
		true,//KC_I,
		true,//KC_J,
		true,//KC_K,
		true,//KC_L,
		true,//KC_M,
		true,//KC_N,
		true,//KC_O,
		true,//KC_P,
		true,//KC_Q,
		true,//KC_R,
		true,//KC_S,
		true,//KC_T,
		true,//KC_U,
		true,//KC_V,
		true,//KC_W,
		true,//KC_X,
		true,//KC_Y,
		true,//KC_Z,
		false,//KC_F1,
		false,//KC_F2,
		false,//KC_F3,
		false,//KC_F4,
		false,//KC_F5,
		false,//KC_F6,
		false,//KC_F7,
		false,//KC_F8,
		false,//KC_F9,
		false,//KC_F10,
		false,//KC_F11,
		false,//KC_F12,
		false,//KC_F13,
		false,//KC_F14,
		false,//KC_F15,
		true,//KC_NUMPAD0,
		true,//KC_NUMPAD1,
		true,//KC_NUMPAD2,
		true,//KC_NUMPAD3,
		true,//KC_NUMPAD4,
		true,//KC_NUMPAD5,
		true,//KC_NUMPAD6,
		true,//KC_NUMPAD7,
		true,//KC_NUMPAD8,
		true,//KC_NUMPAD9,
		true,//KC_NUMPADENTER,
		true,//KC_NUMPADSTAR,
		true,//KC_NUMPADEQUALS,
		true,//KC_NUMPADMINUS,
		true,//KC_NUMPADPLUS,
		true,//KC_NUMPADPERIOD,
		true,//KC_NUMPADSLASH,
		true,//KC_ESCAPE,
		true,//KC_TAB,
		false,//KC_LSHIFT,
		false,//KC_RSHIFT,
		false,//KC_LCONTROL,
		false,//KC_RCONTROL,
		false,//KC_BACKSPACE,
		true,//KC_RETURN,
		false,//KC_LALT,
		true,//KC_SPACE,
		false,//KC_CAPSLOCK,
		false,//KC_NUMLOCK,
		false,//KC_SCROLL,
		false,//KC_RALT,
		true,//KC_AT,
		true,//KC_COLON,
		true,//KC_UNDERLINE,
		true,//KC_MINUS,
		true,//KC_EQUALS,
		true,//KC_LBRACKET,
		true,//KC_RBRACKET,
		true,//KC_SEMICOLON,
		true,//KC_APOSTROPHE,
		true,//KC_GRAVE,
		true,//KC_BACKSLASH,
		true,//KC_COMMA,
		true,//KC_PERIOD,
		true,//KC_SLASH,
		false,//KC_UP,
		false,//KC_DOWN,
		false,//KC_LEFT,
		false,//KC_RIGHT,
		false,//KC_PGUP,
		false//KC_PGDN
	};

	Keyboard::~Keyboard()
	{

	}

	bool Keyboard::IsKeyDown(KeyCode key) const
	{
		return false;
	}

	bool Keyboard::IsKeyPressed(KeyCode key) const
	{
		return false;
	}

	bool Keyboard::IsKeyReleased(KeyCode key) const
	{
		return false;
	}

	bool Keyboard::IsKeyPrintable(KeyCode key) const
	{
		return key_printable[key];
	}
}
