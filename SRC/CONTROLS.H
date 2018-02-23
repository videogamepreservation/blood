#ifndef __CONTROLS_H
#define __CONTROLS_H

#include "typedefs.h"
#include "player.h"

enum {
	kForward1 = 0,
	kForward2,
	kBackward1,
	kBackward2,
	kLeft1,
	kLeft2,
	kRight1,
	kRight2,
	kJump1,
	kJump2,
	kCrouch1,
	kCrouch2,
	kStrafeOn1,
	kStrafeOn2,
	kStrafeLeft1,
	kStrafeLeft2,
	kStrafeRight1,
	kStrafeRight2,
	kLookUp1,
	kLookUp2,
	kLookDown1,
	kLookDown2,
	kAimUp1,
	kAimUp2,
	kAimDown1,
	kAimDown2,
	kLookCenter1,
	kLookCenter2,
	kAction1,
	kAction2,
	kFire1,
	kFire2,
	kAltFire1,
	kAltFire2,
	kRunOn1,
	kRunOn2,
	kRunToggle1,
	kRunToggle2,
	kSelectWeapon1,
	kSelectWeapon2,
	kSelectWeapon3,
	kSelectWeapon4,
	kSelectWeapon5,
	kSelectWeapon6,
	kSelectWeapon7,
	kSelectWeapon8,
	kSelectWeapon9,
	kSelectWeapon0,
	kAutomapToggle,
	kItemLeft1,
	kItemLeft2,
	kItemRight1,
	kItemRight2,
	kItemDrop1,
	kItemDrop2,
	kItemUse1,
	kItemUse2,
	kPause,
	kMaxControls
};


extern INPUT gInput;

extern volatile BYTE *control[ kMaxControls ];

void ctrlStrobeKey( void );
void ctrlGetInput( void );
void ctrlInit( void );
void ctrlTerm( void );

#endif //__CONTROLS_H

