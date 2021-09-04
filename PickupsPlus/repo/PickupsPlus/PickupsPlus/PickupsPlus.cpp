#include "plugin.h"

//>>>
#include "IniReader-master/IniReader.h"
#include "TestCheat.h"
#include "CHud.h"
#include "CTimer.h"
#include "CPickups.h"
#include "CCoronas.h"
#include "CShadows.h"
#include "CPointLights.h"
//<<<
using namespace plugin;
//>>>
using namespace std;

CIniReader inifile;
void loadINI() {
	(&inifile)->~CIniReader();

	new(&inifile) CIniReader("PickupsPlus.ini");
}

int pickupsstart;
float blinktime;

class pickupData {
public:
	bool nodraw = false;
	int colorred = 255;
	int colorgreen = 255;
	int colorblue = 255;
	int coloralpha = 255;
	float coronaradius = 1.0f;
	bool innercorona = true;
	int innertype = 0;
	bool outercorona = true;
	int outertype = 9;
	bool sidecorona = false;
	int sidetype = 0;
	bool groundshadow = true;
	bool pointlight = true;
};
void readINI(pickupData &pickupdata, string datatype, int dataid) {
	string datastring = datatype + to_string(dataid);

	pickupdata.nodraw = inifile.ReadBoolean("PickupsPlus", datastring + " ND", pickupdata.nodraw);
	pickupdata.colorred = inifile.ReadInteger("PickupsPlus", datastring + " RE", pickupdata.colorred);
	pickupdata.colorgreen = inifile.ReadInteger("PickupsPlus", datastring + " GR", pickupdata.colorgreen);
	pickupdata.colorblue = inifile.ReadInteger("PickupsPlus", datastring + " BL", pickupdata.colorblue);
	pickupdata.coloralpha = inifile.ReadInteger("PickupsPlus", datastring + " AL", pickupdata.coloralpha);
	pickupdata.coronaradius = inifile.ReadFloat("PickupsPlus", datastring + " CR", pickupdata.coronaradius);
	pickupdata.innercorona = inifile.ReadBoolean("PickupsPlus", datastring + " IC", pickupdata.innercorona);
	pickupdata.innertype = inifile.ReadInteger("PickupsPlus", datastring + " IT", pickupdata.innertype);
	pickupdata.outercorona = inifile.ReadBoolean("PickupsPlus", datastring + " OC", pickupdata.outercorona);
	pickupdata.outertype = inifile.ReadInteger("PickupsPlus", datastring + " OT", pickupdata.outertype);
	pickupdata.sidecorona = inifile.ReadBoolean("PickupsPlus", datastring + " SC", pickupdata.sidecorona);
	pickupdata.sidetype = inifile.ReadInteger("PickupsPlus", datastring + " ST", pickupdata.sidetype);
	pickupdata.groundshadow = inifile.ReadBoolean("PickupsPlus", datastring + " GS", pickupdata.groundshadow);
	pickupdata.pointlight = inifile.ReadBoolean("PickupsPlus", datastring + " PL", pickupdata.pointlight);
}
//<<<

class PickupsPlus {
public:
    PickupsPlus() {
        // Initialise your plugin here
        
//>>>
		loadINI();

		pickupsstart = patch::GetInt(0x48ADC3);

		blinktime = 0.0f;

		CdeclEvent <AddressList<
			// void __cdecl CPickups::Update()
			0x53C0AD, H_CALL // void __cdecl CGame::Process()
		>, PRIORITY_AFTER, ArgPickNone, void()> pickupsUpdateEvent;
		pickupsUpdateEvent += [] {
			if (TestCheat("RSPKP")) {
				loadINI();

				CHud::SetHelpMessage("PickupsPlus Reloaded.", false, false, false);
			}

			blinktime += CTimer::ms_fTimeStep;

			float offtime = inifile.ReadFloat("PickupsPlus", "fOff", 0.5f);

			if (blinktime > ((60.0f * (inifile.ReadFloat("PickupsPlus", "fOn", 1.0f) + offtime)))) {
				blinktime = 0.0f;
			}

			if (blinktime > (60.0f * offtime)) {
				float alphamultiplier = inifile.ReadFloat("PickupsPlus", "fAlpha", 0.5f);

				for (CObject *objectpointer : CPools::ms_pObjectPool) {
					CPickup *pickuppointer = CPickups::FindPickUpForThisObject(objectpointer);
					if ((int)pickuppointer != pickupsstart) {
						pickupData pickupdata;

						readINI(pickupdata, "PT ", pickuppointer->m_nPickupType);

						int objectmodel = objectpointer->m_nModelIndex;
						if (objectmodel != 0) {
							readINI(pickupdata, "OM ", objectmodel);

							int weapontype = CPickups::WeaponForModel(objectmodel);
							if (weapontype != 0
								&& weapontype != 47
								&& weapontype != 48
							) {
								CWeaponInfo *weaponinfo = CWeaponInfo::GetWeaponInfo((eWeaponType)weapontype, 1);
								if (weaponinfo) {
									readINI(pickupdata, "WF ", weaponinfo->m_nWeaponFire);
								}
							}
						}

						if (!pickupdata.nodraw) {
							int coronaid = (int)objectpointer + 2959;

							CVector &objectposition = objectpointer->GetPosition();

							int colorred = pickupdata.colorred;
							int colorgreen = pickupdata.colorgreen;
							int colorblue = pickupdata.colorblue;
							int coloralpha = pickupdata.coloralpha * alphamultiplier;
							float coronaradius = pickupdata.coronaradius;

							if (pickupdata.innercorona) {
								CCoronas::RegisterCorona(coronaid, nullptr, colorred, colorgreen, colorblue, coloralpha, objectposition, coronaradius, 100.0f, (eCoronaType)pickupdata.innertype, eCoronaFlareType::FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.0f, 0, 99999.0f, false, true);
							}

							if (pickupdata.outercorona) {
								CCoronas::RegisterCorona(coronaid + 1, nullptr, colorred, colorgreen, colorblue, coloralpha, objectposition, coronaradius, 100.0f, (eCoronaType)pickupdata.outertype, eCoronaFlareType::FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.0f, 0, 99999.0f, false, true);
							}

							if (pickupdata.sidecorona) {
								float sideradius = coronaradius * 0.5f;
								CCoronas::RegisterCorona(coronaid + 2, nullptr, colorred, colorgreen, colorblue, coloralpha, objectpointer->TransformFromObjectSpace(CVector::CVector(0.0f, sideradius * 0.5f, 0.0f)), sideradius, 100.0f, (eCoronaType)pickupdata.sidetype, eCoronaFlareType::FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.0f, 0, 99999.0f, false, true);
							}

							if (pickupdata.groundshadow) {
								CShadows::StoreStaticShadow(coronaid, eShadowType::SHADOW_ADDITIVE, gpShadowExplosionTex, &objectposition, coronaradius, 0.0f, 0.0f, -coronaradius, coloralpha * 0.5f, colorred, colorgreen, colorblue, 4.0f, 1.0f, 100.0f, false, 0.0f);
							}

							if (pickupdata.pointlight) {
								CPointLights::AddLight(ePointLightType::PLTYPE_POINTLIGHT, objectposition, CVector(), coronaradius + 2.0f, (int)(colorred * (coloralpha / 255.0f)), (int)(colorgreen * (coloralpha / 255.0f)), (int)(colorblue * (coloralpha / 255.0f)), RwFogType::rwFOGTYPENAFOGTYPE, true, nullptr);
							}
						}

						(&pickupdata)->~pickupData();
					}
				}
			}
		};
//<<<
    }
} pickupsPlus;

//>>>
//<<<
