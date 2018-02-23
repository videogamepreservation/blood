int actSpawnEffect( short nSector, int x, int y, int z, int nStatus, int nEffectType )
{
	int nSpawned = actSpawnSprite( nSector, x, y, z, kStatThing, TRUE );

	switch( nType )
	{
		case kEffectBloodSquib:
			break;
		case kEffectWaterSquib:
			break;
		case kEffectSparkSquib:
			break;
		case kEffectSmokeSquib:
			break;
		case kEffectWoodSquib:
			break;
		case kEffectPlantSquib:
			break;
		case kEffectDirtSquib:
			break;
		case kEffectSnowSquib:
			break;
	}
}

enum {
	kBlossom0 = 0,
};



struct GIB
{
	ushort	chance;		// likelihood (out of 256)
	ushort	tile;		// tile number
	ushort	gibType;	//

	// sequences? short sequenceID;
};

struct DEBRIS
{
	ushort	min;
	ushort	range;
	ushort	picnum;
	schar	shade;
	uchar	pal;
	uchar	xrepeat;
	uchar	yrepeat;
};

struct GIBTABLE
{
	GIB		gib[8];
	DEBRIS	debris[8];
};

enum {
	kGibGlassShards,
	kGibWoodShards,
	kGibMetalShards,


	kGibTommyCultist,
	kGibShotgunCultist,
	kGibAxeZombie,
	kGibFatZombie,
	kGibEarthZombie,
	kGibFleshGargoyle,
	kGibStoneGargoyle,
	kGibFleshStatue,
	kGibStoneStatue,
	kGibPhantasm,
	kGibHound,
	kGibHand,
	kGibBrownSpider,
	kGibRedSpider,
	kGibBlackSpider,
	kGibMotherSpider,
	kGibGillBeast,
	kGibEel,
	kGibBat,
	kGibRat,
	kGibGreenPod,
	kGibGreenTentacle,
	kGibFirePod,
	kGibFireTentacle,
	kGibMotherPod,
	kGibMotherTentacle,
	kGibCerberus,
	kGibCerberus2,
	kGibTchernobog,
	kGibRachel,
};


extern int actGenerateGibs( GIB *pGibList, short nSector, int x, int y, int z, int nBlossom=0 );

int actGenerateGibs( GIB *pGibList, short nSector, int x, int y, int z, int nBlossom )
{
	dassert(pGibList != NULL);

	int angle, velocity = 120;

	for (int i = 0; i < kGibMax && pGibList[i].chance > 0; i++)
	{
		if ( Random(256) < pGibList[i].chance )
		{
			int nGib = actSpawnSprite( nSector, x, y, z, kStatThing, FALSE );
			SPRITE *pGib = &sprite[nGib];

			angle = Random(kAngle360);
			pGib->type = kThingGibSmall;
			pGib->picnum = pGibList[i].tile;
			pGib->xvel += mulscale30(velocity, Cos(angle));
			pGib->yvel += mulscale30(velocity, Sin(angle));
			pGib->zvel -= 128;	// toss it in the air a bit
			pGib->cstat &= ~kSpriteBlocking & ~kSpriteHitscan;
			pGib->flags = kAttrMove | kAttrGravity;
			pGib->pal = kPLUNormal;
		}
	}
}
