#include <stdlib.h>
#include <i86.h>
#include <string.h>

#include "controls.h"
#include "engine.h"
#include "globals.h"
#include "key.h"
#include "debug4g.h"
#include "misc.h"
#include "timer.h"
#include "mouse.h"

#include <memcheck.h>


// virtual key scan codes
#define MOUSE_LBUTTON	0xE0
#define MOUSE_RBUTTON	0xE1
#define MOUSE_MBUTTON	0xE2

#define kAngAccel1	12
#define kAngAccel2	256 //12

// number of ticks at which turning switches to fast turn
#define kTurnThresh	30

//#define KEYS DOOM2
#define DOOM1KEYS

#ifdef DOOM2KEYS
#define kAngSpeed1	92		// Doom II speed
#define kAngSpeed2	184		// Doom II speed
#endif

#ifdef DOOM1KEYS
#define kAngSpeed1	93		// Doom I speed
#define kAngSpeed2	186		// Doom I speed
#endif


INPUT gInput;

// these variables store the keyboard analog input accelerations
static volatile schar iForward, iStrafe;
static volatile uchar iTicks;
static volatile uchar iTurnL = 0, iTurnR = 0;
static volatile int iTurnCount = 0;

static BOOL useMouse = FALSE;

struct {
	char *iniKeyName;
	BYTE  defScanCode;
} controlInfo[] = {
	"Forward1", 	KEY_UP,
	"Forward2", 	KEY_PADUP,
	"Backward1", 	KEY_DOWN,
	"Backward2", 	KEY_PADDOWN,
	"Left1", 		KEY_LEFT,
	"Left2", 		KEY_PADLEFT,
	"Right1", 		KEY_RIGHT,
	"Right2", 		KEY_PADRIGHT,
	"Jump1", 		KEY_X,
	"Jump2", 		0,
	"Crouch1", 		KEY_C,
	"Crouch2", 		0,
	"StrafeOn1", 	KEY_LALT,
	"StrafeOn2", 	KEY_RALT,
	"StrafeLeft1", 	KEY_COMMA,
	"StrafeLeft2", 	0,
	"StrafeRight1", KEY_PERIOD,
	"StrafeRight2", 0,
	"LookUp1", 		KEY_PAGEUP,
	"LookUp2", 		KEY_PADPAGEUP,
	"LookDown1", 	KEY_PAGEDN,
	"LookDown2", 	KEY_PADPAGEDN,
	"AimUp1", 		KEY_HOME,
	"AimUp2", 		KEY_PADHOME,
	"AimDown1", 	KEY_END,
	"AimDown2", 	KEY_PADEND,
	"LookCenter1",	KEY_PAD5,
	"LookCenter2",	0,
	"Action1", 		KEY_SPACE,
	"Action2", 		0,
	"Fire1", 		KEY_LCTRL,
	"Fire2", 		KEY_RCTRL,
	"AltFire1", 	KEY_Z,
	"AltFire2", 	0,
	"RunOn1", 		KEY_LSHIFT,
	"RunOn2", 		KEY_RSHIFT,
	"RunToggle1", 	KEY_SCROLLLOCK,
	"RunToggle2", 	0,
	"Weapon1", 		KEY_1,
	"Weapon2", 		KEY_2,
	"Weapon3", 		KEY_3,
	"Weapon4", 		KEY_4,
	"Weapon5", 		KEY_5,
	"Weapon6", 		KEY_6,
	"Weapon7", 		KEY_7,
	"Weapon8", 		KEY_8,
	"Weapon9", 		KEY_9,
	"Weapon0", 		KEY_0,
	"AutomapToggle", KEY_TAB,
	"ItemLeft1", 	KEY_LBRACE,
	"ItemLeft2", 	0,
	"ItemRight1", 	KEY_RBRACE,
	"ItemRight2", 	0,
	"ItemDrop1", 	KEY_BACKSPACE,
	"ItemDrop2", 	0,
	"ItemUse1", 	KEY_PADENTER,
	"ItemUse2", 	KEY_ENTER,
	"Pause",		KEY_PAUSE,
};

volatile BYTE *control[ kMaxControls ];

void ctrlStrobeKey( void )
{
	BYTE shift = *control[kRunOn1] || *control[kRunOn2];
	iTicks++;
	iTurnL = iTurnR = 0;

	if ( *control[kStrafeOn1] || *control[kStrafeOn2] )
	{
		if ( *control[kLeft1] || *control[kLeft2] )
			iStrafe += 1 + shift;

		if ( *control[kRight1] || *control[kRight2] )
			iStrafe -= 1 + shift;
	}
	else
	{
		if ( *control[kStrafeLeft1] || *control[kStrafeLeft2] )
			iStrafe += 1 + shift;

		if ( *control[kStrafeRight1] || *control[kStrafeRight2] )
			iStrafe -= 1 + shift;

		if ( *control[kLeft1] || *control[kLeft2] )
			iTurnL = 1;

		if ( *control[kRight1] || *control[kRight2] )
			iTurnR = 1;
	}

	if ( *control[kLeft1] || *control[kLeft2] || *control[kRight1] || *control[kRight2] )
		iTurnCount++;
	else
		iTurnCount = 0;

	if ( *control[kForward1] || *control[kForward2] )
		iForward += 1 + shift;

	if ( *control[kBackward1] || *control[kBackward2] )
		iForward -= 1 + shift;
}

void ctrlStrobeKey_END( void ) {};


void ctrlInit( void )
{
	char inputSettings[128];
	strcpy(inputSettings, BloodINI.GetKeyString("Options", "InputSettings", "KeyboardKeys"));

	for (int i = 0; i < kMaxControls; i++)
	{
		int scanId = BloodINI.GetKeyInt(inputSettings, controlInfo[i].iniKeyName, -1);
		if ( scanId < 0 )
			control[i] = (BYTE *)&keystatus[controlInfo[i].defScanCode];
		else
		{
			if (scanId < 0 || scanId > 255)
				scanId = 0;
			control[i] = (BYTE *)&keystatus[scanId];
		}
	}

	dpmiLockMemory(FP_OFF(&iForward), sizeof(schar));
	dpmiLockMemory(FP_OFF(&iTurnL), sizeof(uchar));
	dpmiLockMemory(FP_OFF(&iTurnR), sizeof(uchar));
	dpmiLockMemory(FP_OFF(&iTurnCount), sizeof(int));
	dpmiLockMemory(FP_OFF(&iStrafe), sizeof(schar));
	dpmiLockMemory(FP_OFF(&iTicks), sizeof(uchar));
	dpmiLockMemory(FP_OFF(control), sizeof(control));
	dpmiLockMemory(FP_OFF(&ctrlStrobeKey), FP_OFF(&ctrlStrobeKey_END) - FP_OFF(&ctrlStrobeKey));

	timerRegisterClient(ctrlStrobeKey, kTimerRate);

	useMouse = mouseInit() != 0;

	Mouse::speedX = BloodINI.GetKeyInt("Mouse", "HSensitivity", 30 );
	Mouse::speedY = BloodINI.GetKeyInt("Mouse", "VSensitivity", 10 );
}


void ctrlTerm( void )
{
	timerRemoveClient(ctrlStrobeKey);

	dpmiUnlockMemory(FP_OFF(&iForward), sizeof(schar));
	dpmiUnlockMemory(FP_OFF(&iTurnL), sizeof(uchar));
	dpmiUnlockMemory(FP_OFF(&iTurnR), sizeof(uchar));
	dpmiUnlockMemory(FP_OFF(&iTurnCount), sizeof(int));
	dpmiUnlockMemory(FP_OFF(&iStrafe), sizeof(schar));
	dpmiUnlockMemory(FP_OFF(&iTicks), sizeof(uchar));
	dpmiUnlockMemory(FP_OFF(control), sizeof(control));
	dpmiUnlockMemory(FP_OFF(&ctrlStrobeKey), FP_OFF(&ctrlStrobeKey_END) - FP_OFF(&ctrlStrobeKey));
}



void ctrlGetInput( void )
{
	gInput.syncFlags.byte = 0;
	gInput.buttonFlags.byte = 0;
	gInput.keyFlags.byte = 0;
	gInput.newWeapon = 0;

	if ( useMouse )
	{
		Mouse::Read(iTicks);

		keystatus[MOUSE_LBUTTON] = (BYTE)(Mouse::buttons & 1);
		keystatus[MOUSE_RBUTTON] = (BYTE)(Mouse::buttons & 2);
		keystatus[MOUSE_MBUTTON] = (BYTE)(Mouse::buttons & 4);
	}

	// check buttons
	gInput.buttonFlags.jump = (*control[kJump1] || *control[kJump2]) ? 1 : 0;
	gInput.buttonFlags.crouch = (*control[kCrouch1] || *control[kCrouch2]) ? 1 : 0;
	gInput.buttonFlags.shoot = ( *control[kFire1] || *control[kFire2] ) ? 1 : 0;
	gInput.buttonFlags.shoot2 = ( *control[kAltFire1] || *control[kAltFire2] ) ? 1 : 0;

//	if (gInput.buttonFlags.byte != gMe->buttonFlags.byte)
		gInput.syncFlags.buttonChange = 1;

	if (keystatus[KEY_CAPSLOCK])
	{
		keystatus[KEY_CAPSLOCK] = 0;
		gInput.keyFlags.master = 1;
	}

	if ( *control[kAction1] || *control[kAction2] )
	{
		*control[kAction1] = 0;
		*control[kAction2] = 0;
		gInput.keyFlags.action = 1;
	}

	gInput.buttonFlags.lookup = 0;
	gInput.buttonFlags.lookdown = 0;
	if ( *control[kAimUp1] || *control[kAimUp2] )
	{
		gInput.buttonFlags.lookup = 1;
	}

	if ( *control[kAimDown1] || *control[kAimDown2] )
	{
		gInput.buttonFlags.lookdown = 1;
	}

	if ( *control[kLookUp1] || *control[kLookUp2] )
	{
		gInput.buttonFlags.lookup = 1;
		gInput.keyFlags.lookcenter = 1;
	}

	if ( *control[kLookDown1] || *control[kLookDown2] )
	{
		gInput.buttonFlags.lookdown = 1;
		gInput.keyFlags.lookcenter = 1;
	}

	if ( *control[kLookCenter1] || *control[kLookCenter2] )
	{
		*control[kLookCenter1] = 0;
		*control[kLookCenter2] = 0;
		gInput.keyFlags.lookcenter = 1;
	}

	if ( keystatus[KEY_F9] )
	{
		gInput.keyFlags.restart = 1;
		keystatus[KEY_F9] = 0;
	}

	if ( keystatus[KEY_PAUSE] )
	{
		dprintf("PAUSE PRESSED\n");
		gInput.keyFlags.pause = 1;
		keystatus[KEY_PAUSE] = 0;
	}

//	if ( gInput.keyFlags.byte )
		gInput.syncFlags.keyChange = 1;

	// check weapon change
	for (int i = KEY_1; i <= KEY_0; i++)
	{
		if (keystatus[i])
		{
			keystatus[i] = 0;
			gInput.newWeapon = (uchar)(i - KEY_1 + 1);
			gInput.syncFlags.weaponChange = 1;
		}
	}
	gInput.syncFlags.weaponChange = 1;

	_disable();	// prevent the values from changing while we're reading them

	gInput.forward = iForward;
//	if ( gInput.forward != gMe->forward )
		gInput.syncFlags.forwardChange = 1;

	gInput.turn = 0;
	BYTE shift = *control[kRunOn1] || *control[kRunOn2];

	if ( iTurnL )
	{
		if ( shift && iTurnCount > kTurnThresh )
			gInput.turn -= (sshort)ClipRange(iTurnCount * kAngAccel2, -kAngSpeed2, kAngSpeed2);
		else
			gInput.turn -= (sshort)ClipRange(iTurnCount * kAngAccel1, -kAngSpeed1, kAngSpeed1);
	}

	if ( iTurnR )
	{
		if ( shift && iTurnCount > kTurnThresh )
			gInput.turn += (sshort)ClipRange(iTurnCount * kAngAccel2, -kAngSpeed2, kAngSpeed2);
		else
			gInput.turn += (sshort)ClipRange(iTurnCount * kAngAccel1, -kAngSpeed1, kAngSpeed1);
	}

//	if ( gInput.turn != gMe->turn )
		gInput.syncFlags.turnChange = 1;

	gInput.strafe = iStrafe;
//	if ( gInput.strafe != gMe->strafe )
		gInput.syncFlags.strafeChange = 1;

	gInput.ticks = iTicks;

	iForward = iStrafe = iTicks = 0;
	_enable();

	if ( useMouse )
	{
		if ( *control[kStrafeOn1] || *control[kStrafeOn2] )
			gInput.strafe = (schar)ClipRange(gInput.strafe - Mouse::dX2, -16, 16);
		else
			gInput.turn = (sshort)ClipRange(gInput.turn + Mouse::dX2, -256, 256);

		gInput.forward = (schar)ClipRange(gInput.forward - Mouse::dY2, -16, 16);
	}

}
