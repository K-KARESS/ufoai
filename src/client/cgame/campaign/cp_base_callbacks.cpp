/**
 * @file
 * @brief Menu related console command callbacks
 */

/*
Copyright (C) 2002-2024 UFO: Alien Invasion.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "../../cl_shared.h"
#include "cp_campaign.h"
#include "cp_base_callbacks.h"
#include "cp_base.h"
#include "cp_capacity.h"
#include "cp_geoscape.h"
#include "cp_popup.h"
#include "cp_time.h"
#include "cp_ufo.h"
#include "../../ui/ui_dataids.h"
#include "aliencontainment.h"

/** @brief Used from menu scripts as parameter for mn_base_select */
#define CREATE_NEW_BASE_ID -1

static cvar_t* mn_base_title;

/**
 * @brief onDestroy Callback for Antimatter Storage
 */
static void B_Destroy_AntimaterStorage_f (void)
{
	base_t* base;
	const float prob = frand();

	if (cgi->Cmd_Argc() < 4) {	/** note: third parameter not used but we must be sure we have probability parameter */
		cgi->Com_Printf("Usage: %s <probability> <baseID> <buildingType>\n", cgi->Cmd_Argv(0));
		return;
	}

	base = B_GetFoundedBaseByIDX(atoi(cgi->Cmd_Argv(2)));
	if (!base)
		return;
	if (CAP_GetCurrent(base, CAP_ANTIMATTER) <= 0)
		return;

	CAP_RemoveAntimatterExceedingCapacity(base);

	if (base->baseStatus != BASE_WORKING)
		return;

	if (prob < atof(cgi->Cmd_Argv(1))) {
		MS_AddNewMessage(_("Notice"), va(_("%s has been destroyed by an antimatter storage breach."), base->name));
		cgi->UI_PopWindow(false);
		B_Destroy(base);
	}
}

/**
 * @brief Called when a base is opened or a new base is created on geoscape.
 * For a new base the baseID is -1.
 */
static void B_SelectBase_f (void)
{
	int baseID;

	if (cgi->Cmd_Argc() < 2) {
		cgi->Com_Printf("Usage: %s <baseID>\n", cgi->Cmd_Argv(0));
		return;
	}
	baseID = atoi(cgi->Cmd_Argv(1));
	/* check against MAX_BASES here! - only -1 will create a new base
	 * if we would check against ccs.numBases here, a click on the base selector
	 * base nodes would try to select unfounded bases */
	if (baseID >= 0 && baseID < MAX_BASES) {
		const base_t* base = B_GetFoundedBaseByIDX(baseID);
		/* don't create a new base if the index was valid */
		if (base)
			B_SelectBase(base);
	} else if (baseID == CREATE_NEW_BASE_ID) {
		/* create a new base */
		B_SelectBase(nullptr);
	}
}

/**
 * @brief Sets the title of the base to a cvar to prepare the rename menu.
 */
static void B_SetBaseTitle_f (void)
{
	int baseCount = B_GetCount();

	if (baseCount < MAX_BASES) {
		char baseName[MAX_VAR];

		if (baseCount > 0) {
			int j;
			int i = 2;
			do {
				j = 0;
				Com_sprintf(baseName, lengthof(baseName), _("Base #%i"), i);
				while (j <= baseCount && !Q_streq(baseName, ccs.bases[j].name)) {
					j++;
				}
			} while (i++ <= baseCount && j <= baseCount);
		} else {
			Q_strncpyz(baseName, _("Home"), lengthof(baseName));
		}

		cgi->Cvar_Set("mn_base_title", "%s", baseName);
	} else {
		MS_AddNewMessage(_("Notice"), _("You've reached the base limit."));
		cgi->UI_PopWindow(false);		/* remove the new base popup */
	}
}

/**
 * @brief Constructs a new base.
 * @sa B_NewBase
 */
static void B_BuildBase_f (void)
{
	const campaign_t* campaign = ccs.curCampaign;

	if (ccs.mapAction == MA_NEWBASE)
		ccs.mapAction = MA_NONE;

	if (ccs.credits - campaign->basecost > 0) {
		const nation_t* nation;
		const char* baseName = mn_base_title->string;
		base_t* base;
		/* there may be no " in the base name */
		if (!Com_IsValidName(baseName))
			baseName = _("Base");

		base = B_Build(campaign, ccs.newBasePos, baseName);
		if (!base)
			cgi->Com_Error(ERR_DROP, "Cannot build base");

		CP_UpdateCredits(ccs.credits - campaign->basecost);
		nation = GEO_GetNation(base->pos);
		if (nation)
			Com_sprintf(cp_messageBuffer, sizeof(cp_messageBuffer), _("A new base has been built: %s (nation: %s)"), mn_base_title->string, _(nation->name));
		else
			Com_sprintf(cp_messageBuffer, sizeof(cp_messageBuffer), _("A new base has been built: %s"), mn_base_title->string);
		MS_AddNewMessage(_("Base built"), cp_messageBuffer, MSG_CONSTRUCTION);

		/* First base */
		if (ccs.campaignStats.basesBuilt == 1)
			B_SetUpFirstBase(campaign, base);

		cgi->Cvar_SetValue("mn_base_count", B_GetCount());
		B_SelectBase(base);
	} else {
		/** @todo Why is this needed? Also see bug #5401 */
		if (GEO_IsRadarOverlayActivated())
			GEO_SetOverlay("radar", 0);

		CP_GameTimeStop();
		CP_Popup(_("Notice"), "%s", _("Not enough credits to set up a new base."));
	}
}

/**
 * @brief Creates console command to change the name of a base.
 */
static void B_ChangeBaseName_f (void)
{
	if (cgi->Cmd_Argc() < 3) {
		cgi->Com_Printf("Usage: %s <baseIDX> \"<new base name>\"\n", cgi->Cmd_Argv(0));
		return;
	}
	base_t* base = B_GetFoundedBaseByIDX(atoi(cgi->Cmd_Argv(1)));
	if (!base) {
		cgi->Com_Printf("Invalid base idx: %s\n", cgi->Cmd_Argv(1));
		return;
	}

	/* basename should not contain double-quote character */
	if (!Com_IsValidName(cgi->Cmd_Argv(2))) {
		/* Cancel update, set the cvar to the original name */
		cgi->Cvar_Set("mn_base_title", "%s", base->name);
		return;
	}

	B_SetName(base, cgi->Cmd_Argv(2));
}

/**
 * @brief Initialises base.
 * @note This command is executed in the init node of the base menu.
 * It is called everytime the base menu pops up and sets the cvars.
 * @todo integrate building status tooltips from B_CheckBuildingStatusForMenu_f into this one
 */
static void B_BaseInit_f (void)
{
	if (cgi->Cmd_Argc() < 2) {
#if 0
		cgi->Com_Printf("Usage: %s <baseIDX>\n", cgi->Cmd_Argv(0));
		return;
#endif
	}
	base_t* base = B_GetFoundedBaseByIDX(atoi(cgi->Cmd_Argv(1)));
	if (!base)
		base = B_GetCurrentSelectedBase();
	if (!base) {
		cgi->Com_Printf("Invalid base idx: %s\n", cgi->Cmd_Argv(1));
		return;
	}

	/* make sure the credits cvar is up-to-date */
	CP_UpdateCredits(ccs.credits);

	/* activate or deactivate the aircraft button */
	if (AIR_AircraftAllowed(base)) {
		if (AIR_BaseHasAircraft(base))
			cgi->UI_ExecuteConfunc("update_basebutton aircraft false \"%s\"", _("Aircraft management and crew equipment"));
		else
			cgi->UI_ExecuteConfunc("update_basebutton aircraft true \"%s\"", _("Buy or produce at least one aircraft first."));
	} else {
		cgi->UI_ExecuteConfunc("update_basebutton aircraft true \"%s\"", _("No Hangar operating at this base."));
	}

	if (BS_BuySellAllowed(base))
		cgi->UI_ExecuteConfunc("update_basebutton buysell false \"%s\"", _("Buy/Sell equipment, aircraft and UGV"));
	else
		cgi->UI_ExecuteConfunc("update_basebutton buysell true \"%s\"", _("No Storage operating at this base."));

	if (B_GetCount() > 1)
		cgi->UI_ExecuteConfunc("update_basebutton transfer false \"%s\"", _("Transfer equipment, vehicles, aliens and employees to other bases"));
	else
		cgi->UI_ExecuteConfunc("update_basebutton transfer true \"%s\"", _("Build at least a second base to transfer equipment or personnel"));

	if (RS_ResearchAllowed(base))
		cgi->UI_ExecuteConfunc("update_basebutton research false \"%s\"", _("Research new technology"));
	else
		cgi->UI_ExecuteConfunc("update_basebutton research true \"%s\"", _("No Laboratory operating at this base."));

	if (PR_ProductionAllowed(base))
		cgi->UI_ExecuteConfunc("update_basebutton production false \"%s\"", _("Produce equipment, aircraft and UGV"));
	else
		cgi->UI_ExecuteConfunc("update_basebutton production true \"%s\"", _("No Workshop operating at this base."));

	if (E_HireAllowed(base))
		cgi->UI_ExecuteConfunc("update_basebutton hire false \"%s\"", _("Hire or dismiss employees"));
	else
		cgi->UI_ExecuteConfunc("update_basebutton hire true \"%s\"", _("No Living Quarters operating at this base."));

	if (AC_ContainmentAllowed(base))
		cgi->UI_ExecuteConfunc("update_basebutton containment false \"%s\"", _("Manage captured aliens"));
	else
		cgi->UI_ExecuteConfunc("update_basebutton containment true \"%s\"", _("No Containment operating at this base."));

	if (HOS_HospitalAllowed(base))
		cgi->UI_ExecuteConfunc("update_basebutton hospital false \"%s\"", _("Treat wounded soldiers and perform implant surgery"));
	else
		cgi->UI_ExecuteConfunc("update_basebutton hospital true \"%s\"", _("No Hospital operating at this in base."));
}

/**
 * @brief Destroy a base building
 * @sa B_BuildingDestroy
 */
static void B_BuildingDestroy_f (void)
{
	if (cgi->Cmd_Argc() < 4) {
		cgi->Com_Printf("Usage: %s <baseIDX> <column> <row> [confirmed]\n", cgi->Cmd_Argv(0));
		return;
	}

	base_t* base = B_GetFoundedBaseByIDX(atoi(cgi->Cmd_Argv(1)));
	if (base == nullptr) {
		cgi->Com_Printf("B_BuildingDestroy_f: Invalid base IDX: %s\n", cgi->Cmd_Argv(1));
		return;
	}

	building_t* building = B_GetBuildingAt(base, atoi(cgi->Cmd_Argv(2)), atoi(cgi->Cmd_Argv(3)));
	if (building == nullptr) {
		cgi->Com_Printf("B_BuildingDestroy_f: No valid building at base %i, position (%s, %s)\n",
			base->idx, cgi->Cmd_Argv(2), cgi->Cmd_Argv(3));
		return;
	}

	if (cgi->Cmd_Argc() == 5 && Q_streq(cgi->Cmd_Argv(4), "confirmed")) {
		B_BuildingDestroy(building);
		return;
	}

	/* you can't destroy buildings if base is under attack */
	if (B_IsUnderAttack(base)) {
		CP_Popup(_("Notice"), _("Base is under attack, you can't destroy buildings!"));
		return;
	}

	baseCapacities_t cap = B_GetCapacityFromBuildingType(building->buildingType);
	/** @todo: make base destroyable by destroying entrance */
	if (building->buildingType == B_ENTRANCE) {
		CP_Popup(_("Destroy Entrance"), _("You can't destroy the entrance of the base!"));
		return;
	}

	if (!B_IsBuildingDestroyable(building)) {
			CP_Popup(_("Notice"), _("You can't destroy this building! It is the only connection to other buildings!"));
			return;
	}

	if (building->buildingStatus == B_STATUS_WORKING) {
		const bool hasMoreBases = B_GetCount() > 1;
		switch (building->buildingType) {
		case B_ALIEN_CONTAINMENT:
			if (CAP_GetFreeCapacity(base, cap) < building->capacity) {
				cgi->UI_PopupButton(_("Destroy Alien Containment"), _("If you destroy this building, you will also kill the aliens inside.\nAre you sure you want to destroy this building?"),
					"ui_pop;ui_push aliencont;", _("Containment"), _("Go to the Alien Containment without destroying building"),
					va("ui_pop;building_destroy %i %i %i confirmed;", base->idx, int(building->pos[0]), int(building->pos[1])), _("Destroy"), _("Destroy the building"),
					hasMoreBases ? "ui_pop;ui_push transfer;" : nullptr, hasMoreBases ? _("Transfer") : nullptr,
					_("Go to transfer menu without destroying the building"));
				return;
			}
			break;
		case B_HANGAR:
		case B_SMALL_HANGAR:
			if (CAP_GetFreeCapacity(base, cap) <= 0) {
				cgi->UI_PopupButton(_("Destroy Hangar"), _("If you destroy this hangar, you will also destroy the aircraft inside.\nAre you sure you want to destroy this building?"),
					"ui_pop;ui_push aircraft_equip;aircraft_select;", _("Go to hangar"), _("Go to hangar without destroying building"),
					va("ui_pop;building_destroy %i %i %i confirmed;", base->idx, int(building->pos[0]), int(building->pos[1])), _("Destroy"), _("Destroy the building"),
					hasMoreBases ? "ui_pop;ui_push transfer;" : nullptr, hasMoreBases ? _("Transfer") : nullptr,
					_("Go to transfer menu without destroying the building"));
				return;
			}
			break;
		case B_QUARTERS:
			if (CAP_GetFreeCapacity(base, cap) < building->capacity) {
				cgi->UI_PopupButton(_("Destroy Quarter"), _("If you destroy this Quarters, every employee inside will be killed.\nAre you sure you want to destroy this building?"),
					"ui_pop;ui_push employees;employee_list 0;", _("Dismiss"), _("Go to hiring menu without destroying building"),
					va("ui_pop;building_destroy %i %i %i confirmed;", base->idx, int(building->pos[0]), int(building->pos[1])), _("Destroy"), _("Destroy the building"),
					hasMoreBases ? "ui_pop;ui_push transfer;" : nullptr, hasMoreBases ? _("Transfer") : nullptr,
					_("Go to transfer menu without destroying the building"));
				return;
			}
			break;
		case B_STORAGE:
			if (CAP_GetFreeCapacity(base, cap) < building->capacity) {
				cgi->UI_PopupButton(_("Destroy Storage"), _("If you destroy this Storage, every items inside will be destroyed.\nAre you sure you want to destroy this building?"),
					"ui_pop;ui_push market;buy_type *mn_itemtype", _("Go to storage"), _("Go to buy/sell menu without destroying building"),
					va("ui_pop;building_destroy %i %i %i confirmed;", base->idx, int(building->pos[0]), int(building->pos[1])), _("Destroy"), _("Destroy the building"),
					hasMoreBases ? "ui_pop;ui_push transfer;" : nullptr, hasMoreBases ? _("Transfer") : nullptr,
					_("Go to transfer menu without destroying the building"));
				return;
			}
			break;
		default:
			break;
		}
	}

	cgi->UI_PopupButton(_("Destroy building"), _("Are you sure you want to destroy this building?"),
		nullptr, nullptr, nullptr,
		va("ui_pop;building_destroy %i %i %i confirmed;", base->idx, int(building->pos[0]), int(building->pos[1])), _("Destroy"), _("Destroy the building"),
		nullptr, nullptr, nullptr);
}

/**
 * @brief Builds a base map for tactical combat.
 * @sa CP_BaseAttackChooseBase
 */
static void B_AssembleMap_f (void)
{
	const base_t* base;

	if (cgi->Cmd_Argc() < 2) {
		cgi->Com_Printf("Usage: %s <baseIDX>\n", cgi->Cmd_Argv(0));
		return;
	}
	base = B_GetFoundedBaseByIDX(atoi(cgi->Cmd_Argv(1)));
	if (base == nullptr) {
		cgi->Com_Printf("B_AssembleMap_f: Invalid base IDX: %s\n", cgi->Cmd_Argv(1));
		return;
	}

	char maps[2048];
	char coords[2048];
	B_AssembleMap(maps, sizeof(maps), coords, sizeof(coords), base);
	cgi->Cbuf_AddText("map %s \"%s\" \"%s\"\n", (GEO_IsNight(base->pos) ? "night" : "day"), maps, coords);
}

/**
 * @brief Makes a mapshot - called by basemapshot script command
 * @note Load a basemap and execute 'basemapshot' in console
 */
static void B_MakeBaseMapShot_f (void)
{
	if (!cgi->Com_ServerState()) {
		cgi->Com_Printf("Load the base map before you try to use this function\n");
		return;
	}

	cgi->Cmd_ExecuteString("camsetangles %i %i", 60, 90);
	cgi->Cvar_SetValue("r_isometric", 1);
	/* we are interested in the second level only */
	cgi->Cvar_SetValue("cl_worldlevel", 1);
	cgi->UI_PushWindow("hud_nohud");
	/* hide any active console */
	cgi->Cmd_ExecuteString("toggleconsole");
	cgi->Cmd_ExecuteString("r_screenshot tga");
}

/**
 * @brief Script function for clicking the building list text field.
 */
static void B_FillBuildingInfo_f (void)
{
	if (cgi->Cmd_Argc() < 3) {
		cgi->Com_Printf("Usage: %s <baseIDX> <buildingID>\n", cgi->Cmd_Argv(0));
		return;
	}
	base_t* base = B_GetBaseByIDX(atoi(cgi->Cmd_Argv(1)));
	if (!base) {
		cgi->Com_Printf("Invalid base idx\n");
		return;
	}
	building_t* building = B_GetBuildingTemplateSilent(cgi->Cmd_Argv(2));
	if (!building) {
		cgi->Com_Printf("Invalid building id\n");
		return;
	}

	cgi->UI_ExecuteConfunc("show_buildinginfo %s \"%s\" \"%s\" %i %i %i \"%s\" \"%s\"",
		building->id,
		building->name,
		building->image,
		building->fixCosts,
		building->varCosts,
		building->buildTime,
		va(ngettext("%i day", "%i days", building->buildTime), building->buildTime),
		building->dependsBuilding ? building->dependsBuilding->name : ""
	);

	/* Prevent building more if we reached the limit */
	if (building->maxCount >= 0 && B_GetNumberOfBuildingsInBaseByTemplate(base, building) >= building->maxCount)
		return;
}

/**
 * @brief Update the facilities list
 */
static void B_ListBuildings_f (void)
{
	if (cgi->Cmd_Argc() < 2) {
		cgi->Com_Printf("Usage: %s <baseID>\n", cgi->Cmd_Argv(0));
		return;
	}
	const base_t* const base = B_GetBaseByIDX(atoi(cgi->Cmd_Argv(1)));
	if (!base)
		return;

	for (int i = 0; i < ccs.numBuildingTemplates; i++) {
		const building_t* const building = &ccs.buildingTemplates[i];

		/* skip mandatory buildings (like Entrance) which are built automatically */
		if (building->mandatory)
			continue;
		/* only show already researched buildings */
		if (!RS_IsResearched_ptr(building->tech))
			continue;

		const baseCapacities_t capType = B_GetCapacityFromBuildingType(building->buildingType);
		capacities_t cap;
		if (capType != MAX_CAP)
			cap = *CAP_Get(base, capType);
		else
			OBJZERO(cap);

		assert(building->tpl);
		const int count = B_GetNumberOfBuildingsInBaseByTemplate(base, building->tpl);

		cgi->UI_ExecuteConfunc("show_building \"%s\" \"%s\" %i %i %i %i %i %i %i", _(building->name),
			building->id, building->tpl->capacity, cap.cur, cap.max, count,
			building->tpl->maxCount, int(building->size[0]), int(building->size[1]));
	}
}

/**
 * @brief Lists building tiles for the Basescape UI
 */
static void B_FillMap_f (void)
{
	if (cgi->Cmd_Argc() < 2) {
		return;
	}

	const int baseIdx = atoi(cgi->Cmd_Argv(1));

	const base_t* const base = B_GetFoundedBaseByIDX(baseIdx);
	if (base == nullptr)
		return;

	for (int baseRow = 0; baseRow < BASE_SIZE; baseRow++) {
		for (int baseCol = 0; baseCol < BASE_SIZE; baseCol++) {
			const building_t* const building = B_GetBuildingAt(base, baseCol, baseRow);
			if (B_IsTileBlocked(base, baseCol, baseRow)) {
				cgi->UI_ExecuteConfunc("base_building_add %d %d %d %d \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
					baseCol, baseRow, 1, 1, "", "", "base/invalid", _("Blocked tile"), "");
				continue;
			}
			if (building == nullptr) {
				cgi->UI_ExecuteConfunc("base_building_add %d %d %d %d \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
					baseCol, baseRow, 1, 1, "", "", "base/grid", _("Free tile"), "");
				continue;
			}
			if (building->pos[0] != baseCol || building->pos[1] != baseRow)
				continue;
			cgi->UI_ExecuteConfunc("base_building_add %d %d %d %d \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
				baseCol, baseRow, int(building->size[0]), int(building->size[1]),
				building->id, building->name, building->image, "",
				!B_IsBuildingBuiltUp(building) ? va(ngettext("%d day", "%d days", int(B_GetConstructionTimeRemain(building))),
					int(B_GetConstructionTimeRemain(building))) : "");
		}
	}
}

/**
 * @brief Opens menu on clicking a building in Baseview
 * @todo Move it to Lua
 */
static void B_BuildingOpenAfterClick_f (void)
{
	if (cgi->Cmd_Argc() < 4) {
		cgi->Com_Printf("Usage: %s <baseIDX> <column> <row>\n", cgi->Cmd_Argv(0));
		return;
	}

	base_t* base = B_GetFoundedBaseByIDX(atoi(cgi->Cmd_Argv(1)));
	if (base == nullptr) {
		cgi->Com_Printf("B_BuildingOpenAfterClick_f: Invalid base IDX: %s\n", cgi->Cmd_Argv(1));
		return;
	}

	building_t* building = B_GetBuildingAt(base, atoi(cgi->Cmd_Argv(2)), atoi(cgi->Cmd_Argv(3)));
	if (building == nullptr) {
		cgi->Com_Printf("B_BuildingOpenAfterClick_f: No valid building at base %i, position (%s, %s)\n",
			base->idx, cgi->Cmd_Argv(2), cgi->Cmd_Argv(3));
		return;
	}

	if (!B_GetBuildingStatus(base, building->buildingType)) {
		UP_OpenWith(building->pedia);
		return;
	}

	switch (building->buildingType) {
	case B_LAB:
		if (RS_ResearchAllowed(base))
			cgi->UI_PushWindow("research");
		else
			UP_OpenWith(building->pedia);
		break;
	case B_HOSPITAL:
		if (HOS_HospitalAllowed(base))
			cgi->Cmd_ExecuteString("ui_push hospital %d", base->idx);
		else
			UP_OpenWith(building->pedia);
		break;
	case B_ALIEN_CONTAINMENT:
		if (AC_ContainmentAllowed(base))
			cgi->UI_PushWindow("aliencont");
		else
			UP_OpenWith(building->pedia);
		break;
	case B_QUARTERS:
		if (E_HireAllowed(base))
			cgi->UI_PushWindow("employees");
		else
			UP_OpenWith(building->pedia);
		break;
	case B_WORKSHOP:
		if (PR_ProductionAllowed(base))
			cgi->UI_PushWindow("production");
		else
			UP_OpenWith(building->pedia);
		break;
	case B_DEFENCE_LASER:
	case B_DEFENCE_MISSILE:
		cgi->UI_PushWindow("basedefence");
		break;
	case B_HANGAR:
	case B_SMALL_HANGAR:
		if (!AIR_AircraftAllowed(base)) {
			UP_OpenWith(building->pedia);
		} else if (AIR_BaseHasAircraft(base)) {
			cgi->UI_PushWindow("aircraft_equip");
		} else {
			cgi->UI_PushWindow("buyaircraft");
			/* transfer is only possible when there are at least two bases */
			if (B_GetCount() > 1)
				CP_Popup(_("Note"), _("No aircraft in this base - You first have to purchase or transfer an aircraft\n"));
			else
				CP_Popup(_("Note"), _("No aircraft in this base - You first have to purchase an aircraft\n"));
		}
		break;
	case B_STORAGE:
		if (BS_BuySellAllowed(base))
			cgi->UI_PushWindow("market");
		else
			UP_OpenWith(building->pedia);
		break;
	case B_ANTIMATTER:
		CP_Popup(_("Information"), "%s %d/%d", _("Antimatter (current/max):"), CAP_GetCurrent(base, CAP_ANTIMATTER), CAP_GetMax(base, CAP_ANTIMATTER));
		break;
	default:
		UP_OpenWith(building->pedia);
		break;
	}
}

/**
 * @brief Build a base building
 */
static void B_BuildBuilding_f (void)
{
	if (cgi->Cmd_Argc() < 5) {
		cgi->Com_Printf("Usage: %s <baseIDX> <buildingID> <column> <row>\n", cgi->Cmd_Argv(0));
		return;
	}

	base_t* base = B_GetBaseByIDX(atoi(cgi->Cmd_Argv(1)));
	if (!base) {
		cgi->Com_Printf("Invalid base idx\n");
		return;
	}

	building_t* building = B_GetBuildingTemplateSilent(cgi->Cmd_Argv(2));
	if (!building) {
		cgi->Com_Printf("Invalid building id\n");
		return;
	}

	const int column = atoi(cgi->Cmd_Argv(3));
	const int row = atoi(cgi->Cmd_Argv(4));
	if (column < 0 || row < 0 || column >= BASE_SIZE || row >= BASE_SIZE) {
		cgi->Com_Printf("Invalid building position (%s, %s)\n", cgi->Cmd_Argv(3), cgi->Cmd_Argv(4));
		return;
	}

	if (column + int(building->size[0]) > BASE_SIZE || row + int(building->size[1]) > BASE_SIZE) {
		cgi->Com_Printf("Building doesn't fit position (%s, %s), size (%d, %d)\n",
			cgi->Cmd_Argv(3), cgi->Cmd_Argv(4), int(building->size[0]), int(building->size[1]));
		return;
	}

	if (!CP_CheckCredits(building->fixCosts)) {
		CP_Popup(_("Notice"), _("Not enough credits to build this\n"));
		return;
	}

	if (B_BuildBuilding(base, building, column, row) != nullptr) {
		cgi->S_StartLocalSample("geoscape/build-place", 1.0f);
		cgi->Cmd_ExecuteString("ui_push bases %d", base->idx);
	}
}

/** Init/Shutdown functions */
static const cmdList_t baseCallbacks[] = {
	{"basemapshot", B_MakeBaseMapShot_f, "Command to make a screenshot for the baseview with the correct angles"},
	{"mn_base_select", B_SelectBase_f, "Select a founded base by index"},
	{"mn_base_build", B_BuildBase_f, nullptr},
	{"mn_set_base_title", B_SetBaseTitle_f, nullptr},
	{"base_changename", B_ChangeBaseName_f, "Called after editing the cvar base name"},
	{"base_init", B_BaseInit_f, nullptr},
	{"base_assemble", B_AssembleMap_f, "Called to assemble the current selected base"},
	{"building_destroy", B_BuildingDestroy_f, "Function to destroy a building (select via right click in baseview first)"},
	{"building_amdestroy", B_Destroy_AntimaterStorage_f, "Function called if antimatter storage destroyed"},
	{"base_selectbuilding", B_BuildingOpenAfterClick_f, nullptr},
	{"ui_list_buildings", B_ListBuildings_f, "Lists buildings built or can be built on a base and their capacities"},
	{"ui_show_buildinginfo", B_FillBuildingInfo_f, "Opens the building information window in construction mode"},
	{"ui_base_fillmap", B_FillMap_f, nullptr},
	{"ui_build_building", B_BuildBuilding_f, nullptr},
	{nullptr, nullptr, nullptr}
};

/** @todo unify the names into mn_base_* */
void B_InitCallbacks (void)
{
	mn_base_title = cgi->Cvar_Get("mn_base_title", "", 0, "The title of the current base");
	cgi->Cvar_Set("mn_base_cost", _("%i c"), ccs.curCampaign->basecost);
	cgi->Cvar_SetValue("mn_base_count", B_GetCount());
	cgi->Cvar_SetValue("mn_base_max", MAX_BASES);

	cgi->Cmd_TableAddList(baseCallbacks);
}

/** @todo unify the names into mn_base_* */
void B_ShutdownCallbacks (void)
{
	cgi->Cmd_TableRemoveList(baseCallbacks);

	cgi->Cvar_Delete("mn_base_max");
	cgi->Cvar_Delete("mn_base_cost");
	cgi->Cvar_Delete("mn_base_title");
	cgi->Cvar_Delete("mn_base_count");
}
