#include "dude.h"
#include "db.h"
#include "globals.h"
#include "trig.h"

#include "names.h"


DUDEINFO dudeInfo[kDudeMax - kDudeBase] =
{
	{ 0 },						// random dude not used

	// tommy cultist
	{
		0x100,		// start sequence ID
		40,			// start health
		70,			// mass
		48,			// clip distance
		41,			// eye above z
		M2X(20),	// hear distance
		M2X(100),	// seeing distance
		kAngle90,	// vision periphery
		0,			// melee distance
		10,			// flee health
		15,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		160,		// angSpeed
		{
			{ 0x40, kPicGibMeat1 },
			{ 0x40, kPicGibMeat2 },
			{ 0x40, kPicGibMeat3 },
			{ 0x40, kPicGibHand1 },
			{ 0x40, kPicGibHand1 },
			{ 0x20, kPicGibEye2 },
			{ 0x20, kPicGibEye2 },
		},
		{
			0,		// kDamagePummel
			0,      // kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			0,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},

	// shotgun cultist
	{
		0x2D0,		// start sequence ID
		40,			// start health
		70,			// mass
		48,			// clip distance
		41,			// eye above z
		M2X(20),	// hear distance
		M2X(100),	// seeing distance
		kAngle90,	// vision periphery
		0,			// melee distance
		10,			// flee health
		15,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		160,		// angSpeed
		{
			{ 0x40, kPicGibMeat1 },
			{ 0x40, kPicGibMeat2 },
			{ 0x40, kPicGibMeat3 },
			{ 0x40, kPicGibHand1 },
			{ 0x40, kPicGibHand1 },
			{ 0x20, kPicGibEye2 },
			{ 0x20, kPicGibEye2 },
		},
		{
			0,		// kDamagePummel
			0,      // kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			0,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},

	// axe zombie
	{
		0x110,		// start sequence ID
		60,			// start health
		70,			// mass
		48,			// clip distance
		46,			// eye above z
		M2X(20),	// hear distance
		M2X(100),	// seeing distance
		kAngle90,	// vision periphery
		0,			// melee distance
		10,			// flee health
		15,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		240,     	// frontSpeed	//220
		160,     	// sideSpeed	//150
		160,     	// backSpeed	//150
		160,		// angSpeed		//120
		{
			{ 0x40, kPicGibMeat1 },
			{ 0x40, kPicGibMeat2 },
			{ 0x40, kPicGibMeat3 },
			{ 0x40, kPicGibHand1 },
			{ 0x40, kPicGibHand1 },
			{ 0x20, kPicGibEye2 },
			{ 0x20, kPicGibEye2 },
		},
		{
			0,			// kDamagePummel
			0,      	// kDamageFall
			0,      	// kDamageBurn
			1,      	// kDamageBullet
			1,      	// kDamageStab
			0,      	// kDamageExplode
			kNoDamage,	// kDamageGas
			0,      	// kDamageDrown
			0,      	// kDamageSpirit
			3,			// kDamageVoodoo
		},
	},

	// fat zombie
	{
		0x120,		// start sequence ID
		30,			// start health
		120,		// mass
		48,			// clip distance
		44,			// eye above z
		M2X(20),	// hear distance
		M2X(100),	// seeing distance
		kAngle90,	// vision periphery
		0,			// melee distance
		10,			// flee health
		15,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		180,     	// frontSpeed
		120,     	// sideSpeed
		120,     	// backSpeed
		100,		// angSpeed
		{
			{ 0x80, kPicGibMeat1 },
			{ 0x80, kPicGibMeat2 },
			{ 0x40, kPicGibEye2 },
			{ 0x40, kPicGibEye2 },
			{ 0x80, kPicGibHand2 },
			{ 0x80, kPicGibHand2 },
			{ 0x80, kPicGibMeat3 },
		},
		{
			0,			// kDamagePummel
			0,      	// kDamageFall
			0,      	// kDamageBurn
			0,      	// kDamageBullet
			0,      	// kDamageStab
			0,      	// kDamageExplode
			kNoDamage,	// kDamageGas
			0,      	// kDamageDrown
			0,      	// kDamageSpirit
			3,			// kDamageVoodoo
		},
	},

	// earth zombie
	{
		0x110,		// start sequence ID
		40,			// start health
		70,			// mass
		48,			// clip distance
		46,			// eye above z
		M2X(10),	// hear distance
		M2X(0),		// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		10,			// flee health
		15,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		220,     	// frontSpeed	//150
		150,     	// sideSpeed	//100
		150,     	// backSpeed	//100
		120,		// angSpeed		//64
		{
			{ 0x40, kPicGibMeat1 },
			{ 0x40, kPicGibMeat2 },
			{ 0x40, kPicGibMeat3 },
			{ 0x40, kPicGibHand1 },
			{ 0x20, kPicGibEye2 },
		},
		{
			0,			// kDamagePummel
			0,      	// kDamageFall
			0,      	// kDamageBurn
			0,      	// kDamageBullet
			1,      	// kDamageStab
			0,      	// kDamageExplode
			kNoDamage,	// kDamageGas
			0,      	// kDamageDrown
			0,      	// kDamageSpirit
			3,			// kDamageVoodoo
		},
	},

	// flesh gargoyle
	{
		0x130,		// start sequence ID
		80,			// start health
		120,		// mass
		64,			// clip distance
		13,			// eye above z
		M2X(20),	// hear distance
		M2X(100),	// seeing distance
		kAngle90,	// vision periphery
		0,			// melee distance
		10,			// flee health
		15,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		5,			// sideAccel
		5,	    	// backAccel
		260,     	// frontSpeed
		210,     	// sideSpeed
		210,     	// backSpeed
		150,		// angSpeed
		{
			{ 0x100, kPicGibGargWing },
			{ 0x100, kPicGibGargWing },
			{ 0x40, kPicGibGargHead },
		},
		{
			0,		// kDamagePummel
			0,      // kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			0,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},

	// stone gargoyle
	{
		0x140,		// start sequence ID
		120,		// start health
		200,		// mass
		64,			// clip distance
		13,			// eye above z
		M2X(20),	// hear distance
		M2X(100),	// seeing distance
		kAngle90,	// vision periphery
		0,			// melee distance
		10,			// flee health
		20,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		15,       	// frontAccel
		5,			// sideAccel
		5,	    	// backAccel
		250,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		80,			// angSpeed
		{
			{0,0},
		},
		{	1,				// kDamagePummel
			0,				// kDamageFall
			kNoDamage,		// kDamageBurn
			2,				// kDamageBullet
			3,				// kDamageStab
			0,				// kDamageExplode
			kNoDamage,		// kDamageGas
			kNoDamage,		// kDamageDrown
			0,				// kDamageSpirit
			0 },			// kDamageVoodoo
	},

	// flesh statue
	{
		0x2B0,		// start sequence ID
		100,		// start health
		200,		// mass
		64,			// clip distance
		13,			// eye above z
		M2X(4),		// hear distance
		M2X(10),	// seeing distance
		kAngle90,	// vision periphery
		0,			// melee distance
		10,			// flee health
		15,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		0,       	// frontAccel
		0,			// sideAccel
		0,	    	// backAccel
		0,     		// frontSpeed
		0,     		// sideSpeed
		0,     		// backSpeed
		0,			// angSpeed
		{
			{0,0},
		},
		{
			kNoDamage,	// kDamagePummel
			0,      	// kDamageFall
			kNoDamage,	// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			kNoDamage,	// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	// stone statue
	{
		0x2C0,		// start sequence ID
		100,		// start health
		200,		// mass
		64,			// clip distance
		13,			// eye above z
		M2X(4),		// hear distance
		M2X(10),	// seeing distance
		kAngle90,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		0,       	// frontAccel
		0,			// sideAccel
		0,	    	// backAccel
		0,     		// frontSpeed
		0,     		// sideSpeed
		0,     		// backSpeed
		0,			// angSpeed
		{
			{0,0},
		},
		{
			kNoDamage,	// kDamagePummel
			0,      	// kDamageFall
			kNoDamage,	// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			kNoDamage,	// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	// kDudePhantasm,
	{
		0x150,		// start sequence ID
		100,		// start health
		70,			// mass
		64,			// clip distance
		25,			// eye above z
		M2X(20),	// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0000,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{
			{0,0},
		},
		{
			0,			// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			1,			// kDamageBullet
			1,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			0,			// kDamageSpirit
			4,			// kDamageVoodoo
		},
	},

	// hell hound
	{
		0x160,		// start sequence ID
		50,			// start health
		120,		// mass
		80,			// clip distance
		6,			// eye above z
		M2X(20),	// hear distance
		M2X(100),	// seeing distance
		kAngle120,	// vision periphery
		0,			// melee distance
		10,			// flee health
		20,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		300,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		160,		// angSpeed
		{
			{ 0x80, kPicGibHoundFoot },
			{ 0x80, kPicGibHoundFoot },
			{ 0x80, kPicGibHoundFoot },
			{ 0x80, kPicGibHoundFoot },
			{ 0x20, kPicGibEye1 },
			{ 0x20, kPicGibEye1 },
		},
		{
			0,			// kDamagePummel
			1,			// kDamageFall
			kNoDamage,	// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			0,			// kDamageGas
			0,			// kDamageDrown
			0,			// kDamageSpirit
			1,			// kDamageVoodoo
		},
	},

	// hand
	{
		0x170,		// start sequence ID
		10,			// start health
		2,			// mass
		32,			// clip distance
		0,			// eye above z
		M2X(10),	// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		200,		// angSpeed
		{
			{0,0},
		},
		{
			0,			// kDamagePummel
			2,			// kDamageFall
			0,			// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			0,			// kDamageSpirit
			2,			// kDamageVoodoo
		},
	},

	// brown spider
	{
		0x180,		// start sequence ID
		10,			// start health
		2,			// mass
		32,			// clip distance
		-5,			// eye above z
		M2X(10),	// hear distance
		M2X(100),	// seeing distance
		kAngle120,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		200,		// angSpeed
		{
			{0,0},
		},
		{
			0,		// kDamagePummel
			2,      // kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			0,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},

	// red spider
	{
		0x190,		// start sequence ID
		25,			// start health
		5,			// mass
		32,			// clip distance
		-5,			// eye above z
		M2X(10),	// hear distance
		M2X(100),	// seeing distance
		kAngle120,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		200,		// angSpeed
		{
			{0,0},
		},
		{
			0,		// kDamagePummel
			2,      // kDamageFall
			1,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			0,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},

	// black spider
	{
		0x1A0,		// start sequence ID
		50,			// start health
		10,			// mass
		32,			// clip distance
		-5,			// eye above z
		M2X(10),	// hear distance
		M2X(100),	// seeing distance
		kAngle120,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		200,		// angSpeed
		{
			{0,0},
		},
		{
			0,		// kDamagePummel
			1,      // kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			kNoDamage,      // kDamageSpirit
			kNoDamage,		// kDamageVoodoo
		},
	},

	// mother spider
	{
		0x1B0,		// start sequence ID
		100,		// start health
		20,			// mass
		32,			// clip distance
		-5,			// eye above z
		M2X(10),	// hear distance
		M2X(100),	// seeing distance
		kAngle120,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		180,		// angSpeed
		{
			{0,0},
		},
		{
			0,		// kDamagePummel
			1,      // kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			0,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},

	// gill beast
	{
		0x1C0,		// start sequence ID
		50,			// start health
		200,		// mass
		64,			// clip distance
		37,			// eye above z
		M2X(10),	// hear distance
		M2X(100),	// seeing distance
		kAngle120,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{
			{0,0},
		},
		{
			1,			// kDamagePummel
			0,			// kDamageFall
			0,      	// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			1,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			0,			// kDamageSpirit
			0,			// kDamageVoodoo
		},
	},

	// eel
	{
		0x1D0,		// start sequence ID
		25,			// start health
		30,			// mass
		32,			// clip distance
		4,			// eye above z
		M2X(10),	// hear distance
		M2X(100),	// seeing distance
		kAngle90,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{	{0,0},
		},
		{
			0,			// kDamagePummel
			0,			// kDamageFall
			0,			// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			0,			// kDamageSpirit
			0,			// kDamageVoodoo
		},
	},

	// bat
	{
		0x1E0,		// start sequence ID
		10,			// start health
		2,			// mass
		32,			// clip distance
		0,			// eye above z
		M2X(20),	// hear distance
		M2X(50),	// seeing distance
		kAngle90,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		128,		// angSpeed
		{
			{0,0},
		},
		{
			0,		// kDamagePummel
			kNoDamage,	// kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			0,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},

	// rat
	{
		0x1F0,		// start sequence ID
		10,			// start health
		5,			// mass
		32,			// clip distance
		3,			// eye above z
		M2X(25),	// hear distance
		M2X(100),	// seeing distance
		kAngle90,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		100,     	// frontSpeed
		100,     	// sideSpeed
		50,     	// backSpeed
		64,			// angSpeed
		{
			{0,0},
		},
		{
			0,		// kDamagePummel
			0,      // kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			0,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},

	// green pod
	{
		0x200,		// start sequence ID
		50,			// start health
		150,		// mass
		64,			// clip distance
		40,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle180,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		0,       	// frontAccel
		0,			// sideAccel
		0,	    	// backAccel
		0,     		// frontSpeed
		0,     		// sideSpeed
		0,     		// backSpeed
		0,			// angSpeed
		{
			{0,0},
		},
		{
			0,		// kDamagePummel
			0,      // kDamageFall
			0,      // kDamageBurn
			1,      // kDamageBullet
			1,      // kDamageStab
			0,      // kDamageExplode
			kNoDamage,      // kDamageGas
			kNoDamage,      // kDamageDrown
			kNoDamage,      // kDamageSpirit
			kNoDamage,		// kDamageVoodoo
		},
	},

	// green tentacle
	{
		0x210,		// start sequence ID
		10,			// start health
		65535,		// mass
		32,			// clip distance
		0,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle180,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		0,       	// frontAccel
		0,			// sideAccel
		0,	    	// backAccel
		0,     		// frontSpeed
		0,     		// sideSpeed
		0,     		// backSpeed
		0,			// angSpeed
		{
			{0,0},
		},
		{
			0,		// kDamagePummel
			0,      // kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			kNoDamage,      // kDamageGas
			kNoDamage,      // kDamageDrown
			kNoDamage,      // kDamageSpirit
			kNoDamage,		// kDamageVoodoo
		},
	},

	// fire pod
	{
		0x220,		// start sequence ID
		100,		// start health
		250,		// mass
		64,			// clip distance
		40,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle180,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		0,       	// frontAccel
		0,			// sideAccel
		0,	    	// backAccel
		0,     		// frontSpeed
		0,     		// sideSpeed
		0,     		// backSpeed
		0,			// angSpeed
		{
			{0,0},
		},
		{
			0,			// kDamagePummel
			0,			// kDamageFall
			kNoDamage,	// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			0,			// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	// fire tentacle
	{
		0x230,		// start sequence ID
		20,			// start health
		65535,		// mass
		32,			// clip distance
		0,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle180,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		0,       	// frontAccel
		0,			// sideAccel
		0,	    	// backAccel
		0,     		// frontSpeed
		0,     		// sideSpeed
		0,     		// backSpeed
		0,			// angSpeed
		{
			{0,0},
		},
		{
			0,			// kDamagePummel
			0,			// kDamageFall
			kNoDamage,	// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	// mother pod
	{
		0x240,		// start sequence ID
		200,		// start health
		400,		// mass
		64,			// clip distance
		40,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle180,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		0,       	// frontAccel
		0,			// sideAccel
		0,	    	// backAccel
		0,     		// frontSpeed
		0,     		// sideSpeed
		0,     		// backSpeed
		0,			// angSpeed
		{	{0,0},
		},
		{
			1,		// kDamagePummel
			0,      // kDamageFall
			0,      // kDamageBurn
			1,      // kDamageBullet
			1,      // kDamageStab
			0,      // kDamageExplode
			1,      // kDamageGas
			1,      // kDamageDrown
			1,      // kDamageSpirit
			1,		// kDamageVoodoo
		},
	},

	// mother tentacle
	{
		0x250,		// start sequence ID
		50,			// start health
		65535,		// mass
		32,			// clip distance
		0,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle180,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		0,       	// frontAccel
		0,			// sideAccel
		0,	    	// backAccel
		0,     		// frontSpeed
		0,     		// sideSpeed
		0,     		// backSpeed
		0,			// angSpeed
		{
			{0,0},
		},
		{
			0,		// kDamagePummel
			0,      // kDamageFall
			0,      // kDamageBurn
			1,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			1,      // kDamageGas
			1,      // kDamageDrown
			1,      // kDamageSpirit
			1,		// kDamageVoodoo
		},
	},

	// kDudeCerberus
	{
		0x260,		// start sequence ID
		200,		// start health
		200,		// mass
		64,			// clip distance
		29,			// eye above z
		M2X(80),	// hear distance
		M2X(200),	// seeing distance
		kAngle120,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{
			{0,0},
		},
		{
			0,			// kDamagePummel
			0,			// kDamageFall
			2,			// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			0,			// kDamageGas
			0,			// kDamageDrown
			1,			// kDamageSpirit
			1,			// kDamageVoodoo
		},
	},

	// kDudeCerberus2
	{
		0x270,		// start sequence ID
		100,		// start health
		200,		// mass
		64,			// clip distance
		29,			// eye above z
		M2X(40),	// hear distance
		M2X(100),	// seeing distance
		kAngle120,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{
			{0,0},
		},
		{
			0,		// kDamagePummel
			0,      // kDamageFall
			2,      // kDamageBurn
			1,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			1,      // kDamageGas
			0,      // kDamageDrown
			1,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},

	// tchernobog
	{
		0x280,		// start sequence ID
		200,		// start health
		400,		// mass
		128,		// clip distance
		0,			// eye above z
		M2X(50),	// hear distance
		M2X(100),	// seeing distance
		kAngle90,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{
			{0,0},
		},
		{
			1,		// kDamagePummel
			0,      // kDamageFall
			3,      // kDamageBurn
			0,      // kDamageBullet
			2,      // kDamageStab
			0,      // kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			2,			// kDamageSpirit
			4,			// kDamageVoodoo
		},
	},

	// rachel
	{
		0x290,		// start sequence ID
		25,			// start health
		20,			// mass
		32,			// clip distance
		0,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{
			{0,0},
		},
		{
			kNoDamage,		// kDamagePummel
			kNoDamage,      // kDamageFall
			kNoDamage,      // kDamageBurn
			kNoDamage,      // kDamageBullet
			kNoDamage,      // kDamageStab
			kNoDamage,      // kDamageExplode
			kNoDamage,      // kDamageGas
			kNoDamage,      // kDamageDrown
			kNoDamage,      // kDamageSpirit
			kNoDamage,		// kDamageVoodoo
		},
	},

	// player1..player8
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },

	// kDudePlayerBurning
	{
		0x100,		// start sequence ID
		25,			// start health
		70,			// mass
		48,			// clip distance
		41,			// eye above z
		M2X(20),	// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		100,		// flee health
		100,		// hinder damage
		0,			// change target chance
		0,			// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		160,		// angSpeed
		{
			{0,0},
		},
		{
			kNoDamage,	// kDamagePummel
			0,			// kDamageFall
			0,			// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	// kDudeCultistBurning
	{
		0x100,		// start sequence ID
		25,			// start health
		70,			// mass
		48,			// clip distance
		41,			// eye above z
		M2X(20),	// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		100,		// flee health
		100,		// hinder damage
		0,			// change target chance
		0,			// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		160,		// angSpeed
		{
			{0,0},
		},
		{
			kNoDamage,	// kDamagePummel
			0,			// kDamageFall
			0,			// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	// kDudeAxeZombieBurning
	{
		0x110,		// start sequence ID
		25,			// start health
		70,			// mass
		48,			// clip distance
		46,			// eye above z
		M2X(20),	// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		10,			// flee health
		15,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		240,     	// frontSpeed	//220
		160,     	// sideSpeed	//150
		160,     	// backSpeed	//150
		160,		// angSpeed		//120
		{
			{0,0},
		},
		{
			kNoDamage,	// kDamagePummel
			0,			// kDamageFall
			0,			// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	// kDudeFatZombieBurning
	{
		0x120,		// start sequence ID
		25,			// start health
		120,		// mass
		48,			// clip distance
		44,			// eye above z
		M2X(20),	// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		10,			// flee health
		15,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		180,     	// frontSpeed
		120,     	// sideSpeed
		120,     	// backSpeed
		100,		// angSpeed
		{
			{0,0},
		},
		{
			kNoDamage,	// kDamagePummel
			0,			// kDamageFall
			0,			// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	// kDudePlayer_Owned
	{
		0x100,		// start sequence ID
		100,		// start health
		70,			// mass
		64,			// clip distance
		38,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{
			{ 0x40, kPicGibMeat1 },
			{ 0x40, kPicGibMeat2 },
			{ 0x40, kPicGibMeat3 },
			{ 0x20, kPicGibEye2 },
			{ 0x20, kPicGibHand1 }
		},
		{
			0,		// kDamagePummel
			0,      // kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			0,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},

	// kDudeHound_Owned
	{
		0x160,		// start sequence ID
		25,			// start health
		70,			// mass
		32,			// clip distance
		6,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{
			{ 0x80, kPicGibHoundFoot },
			{ 0x80, kPicGibHoundFoot },
			{ 0x80, kPicGibHoundFoot },
			{ 0x80, kPicGibHoundFoot },
			{ 0x20, kPicGibEye1 },
			{ 0x20, kPicGibEye1 },
		},
		{
			0,			// kDamagePummel
			1,			// kDamageFall
			kNoDamage,	// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			0,			// kDamageGas
			0,			// kDamageDrown
			0,			// kDamageSpirit
			0,			// kDamageVoodoo
		},
	},

	// kDudeEel_Owned
	{
		0x1D0,		// start sequence ID
		25,			// start health
		70,			// mass
		32,			// clip distance
		4,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{
			{0,0},
		},
		{
			0,			// kDamagePummel
			0,			// kDamageFall
			0,			// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			0,			// kDamageSpirit
			0,			// kDamageVoodoo
		},
	},

	// kDudeSpider_Owned
	{
		0x180,		// start sequence ID
		25,			// start health
		70,			// mass
		32,			// clip distance
		-5,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		200,     	// frontSpeed
		200,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{
			{0,0},
		},
		{
			0,		// kDamagePummel
			0,      // kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			0,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},
};

DUDEINFO gPlayerTemplate[2] =
{
	// human
	{
		0x100,		// start sequence ID
		100,		// start health
		70,			// mass
		48,			// clip distance
		38,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		250,     	// frontSpeed
		250,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{
			{ 0x40, kPicGibMeat1 },
			{ 0x40, kPicGibMeat2 },
			{ 0x40, kPicGibMeat3 },
			{ 0x20, kPicGibEye2 },
			{ 0x20, kPicGibHand1 }
		},
		{
			0,		// kDamagePummel
			0,      // kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			0,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},

	// beast
	{
		0x2A0,		// start sequence ID
		100,		// start health
		70,			// mass
		48,			// clip distance
		36,			// eye above z
		M2X(4),		// hear distance
		M2X(100),	// seeing distance
		kAngle60,	// vision periphery
		0,			// melee distance
		10,			// flee health
		10,			// hinder damage
		0x0100,		// change target chance
		0x0010,		// change target to kin chance
		0x4000,		// alertChance
		10,       	// frontAccel
		25,			// sideAccel
		18,	    	// backAccel
		300,     	// frontSpeed
		300,     	// sideSpeed
		200,     	// backSpeed
		64,			// angSpeed
		{
			{0,0},
		},
		{
			0,		// kDamagePummel
			0,      // kDamageFall
			0,      // kDamageBurn
			0,      // kDamageBullet
			0,      // kDamageStab
			0,      // kDamageExplode
			0,      // kDamageGas
			0,      // kDamageDrown
			0,      // kDamageSpirit
			0,		// kDamageVoodoo
		},
	},
};
