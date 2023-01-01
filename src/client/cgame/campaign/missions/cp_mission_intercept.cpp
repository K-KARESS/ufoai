/**
 * @file
 * @brief Campaign mission
 */

/*
Copyright (C) 2002-2023 UFO: Alien Invasion.

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

#include "../../../DateTime.h"
#include "../../../cl_shared.h"
#include "../cp_campaign.h"
#include "../cp_ufo.h"
#include "../cp_missions.h"
#include "../cp_time.h"
#include "../cp_alien_interest.h"
#include "../cp_xvi.h"
#include "cp_mission_intercept.h"

/**
 * @brief Intercept mission is over and is a success: change interest values.
 * @note Intercept mission
 */
void CP_InterceptMissionIsSuccess (mission_t* mission)
{
	INT_ChangeIndividualInterest(0.3f, INTERESTCATEGORY_RECON);
	INT_ChangeIndividualInterest(-0.3f, INTERESTCATEGORY_INTERCEPT);
	INT_ChangeIndividualInterest(0.1f, INTERESTCATEGORY_HARVEST);
	if (CP_IsXVIStarted())
		INT_ChangeIndividualInterest(0.1f, INTERESTCATEGORY_XVI);

	CP_MissionRemove(mission);
}

/**
 * @brief Intercept mission is over and is a failure: change interest values.
 * @note Intercept mission
 */
void CP_InterceptMissionIsFailure (mission_t* mission)
{
	INT_ChangeIndividualInterest(0.1f, INTERESTCATEGORY_INTERCEPT);
	INT_ChangeIndividualInterest(0.05f, INTERESTCATEGORY_BUILDING);
	INT_ChangeIndividualInterest(0.05f, INTERESTCATEGORY_BASE_ATTACK);
	INT_ChangeIndividualInterest(0.05f, INTERESTCATEGORY_TERROR_ATTACK);

	CP_MissionRemove(mission);
}

#define	UFO_EPSILON 0.00001f

/**
 * @brief Intercept mission ends: UFO leave earth.
 * @param[in] mission Pointer to the mission
 * @param[in] destroyed true if the UFO actually destroyed the installation, false else
 * @note Intercept mission -- Stage 3
 */
void CP_InterceptMissionLeave (mission_t* mission, bool destroyed)
{
	installation_t* installation;

	assert(mission->ufo);

	mission->stage = STAGE_RETURN_TO_ORBIT;

	/* if the mission was an attack of an installation, destroy it */
	installation = mission->data.installation;
	if (installation) {
		vec3_t missionPos;

		Vector2Copy(mission->pos, missionPos);
		missionPos[2] = installation->pos[2];
		if (destroyed && VectorCompareEps(missionPos, installation->pos, UFO_EPSILON))
			INS_DestroyInstallation(installation);
	}

	CP_MissionDisableTimeLimit(mission);
	UFO_SetRandomDest(mission->ufo);
	CP_MissionRemoveFromGeoscape(mission);
	/* Display UFO on geoscape if it is detected */
	mission->ufo->landed = false;
}

/**
 * @brief UFO starts to attack the installation.
 * @note Intercept mission -- Stage 2
 */
static void CP_InterceptAttackInstallation (mission_t* mission)
{
	const DateTime minAttackDelay(0, 3600);
	const DateTime maxAttackDelay(0, 21600);		/* How long the UFO should stay on earth */
	installation_t* installation;
	vec3_t missionPos;

	mission->stage = STAGE_INTERCEPT;

	installation = mission->data.installation;
	Vector2Copy(mission->pos, missionPos);
	if (!VectorCompareEps(missionPos, installation->pos, UFO_EPSILON)) {
		mission->finalDate = ccs.date;
		return;
	}

	/* Make round around the position of the mission */
	UFO_SetRandomDestAround(mission->ufo, mission->pos);
	mission->finalDate = ccs.date + Date_Random(minAttackDelay, maxAttackDelay);
}

/**
 * @brief Set Intercept mission: UFO looks for new aircraft target.
 * @note Intercept mission -- Stage 1
 */
void CP_InterceptAircraftMissionSet (mission_t* mission)
{
	const DateTime minReconDelay(3, 0);
	const DateTime maxReconDelay(6, 0);		/* How long the UFO should stay on earth */

	mission->stage = STAGE_INTERCEPT;
	mission->finalDate = ccs.date + Date_Random(minReconDelay, maxReconDelay);
}

/**
 * @brief Choose Base that will be attacked, and add it to mission description.
 * @note Base attack mission -- Stage 1
 * @return Pointer to the base, nullptr if no base set
 */
static installation_t* CP_InterceptChooseInstallation (const mission_t* mission)
{
	float randomNumber, sum = 0.0f;
	installation_t* installation = nullptr;

	assert(mission);

	/* Choose randomly a base depending on alienInterest values for those bases */
	INS_Foreach(i) {
		sum += i->alienInterest;
	}
	randomNumber = frand() * sum;
	INS_Foreach(i) {
		randomNumber -= i->alienInterest;
		if (randomNumber < 0) {
			installation = i;
			break;
		}
	}

	/* Make sure we have a base */
	assert(installation && (randomNumber < 0));

	return installation;
}

/**
 * @brief Set Intercept mission: UFO chooses an installation an flies to it.
 * @note Intercept mission -- Stage 1
 */
void CP_InterceptGoToInstallation (mission_t* mission)
{
	installation_t* installation;
	assert(mission->ufo);

	mission->stage = STAGE_MISSION_GOTO;

	installation = CP_InterceptChooseInstallation(mission);
	if (!installation) {
		cgi->Com_Printf("CP_InterceptGoToInstallation: no installation found\n");
		CP_MissionRemove(mission);
		return;
	}
	mission->data.installation = installation;

	Vector2Copy(installation->pos, mission->pos);
	mission->posAssigned = true;

	CP_MissionDisableTimeLimit(mission);
	UFO_SendToDestination(mission->ufo, mission->pos);
}

/**
 * @brief Set Intercept mission: choose between attacking aircraft or installations.
 * @note Intercept mission -- Stage 1
 */
static void CP_InterceptMissionSet (mission_t* mission)
{
	assert(mission->ufo);

	/* Only large UFOs can attack installations -- if there are installations to attack */
	if (UFO_CanDoMission(mission->ufo->getUfoType(), "interceptbombing"))
		if (INS_HasAny()) {
			/* Probability to get a UFO that targets installations. */
			const float TARGET_INS_PROBABILITY = 0.25;
			/* don't make attack on installation happen too often */
			if (frand() < TARGET_INS_PROBABILITY)
				CP_InterceptGoToInstallation(mission);
		}

	/* if not attacking an installation */
	if (mission->stage != STAGE_MISSION_GOTO)
		CP_InterceptAircraftMissionSet(mission);
}

/**
 * @brief Determine what action should be performed when a Intercept mission stage ends.
 * @param[in] mission Pointer to the mission which stage ended.
 */
void CP_InterceptNextStage (mission_t* mission)
{
	switch (mission->stage) {
	case STAGE_NOT_ACTIVE:
		/* Create Intercept mission */
		CP_MissionBegin(mission);
		break;
	case STAGE_COME_FROM_ORBIT:
		/* UFO start looking for target */
		CP_InterceptMissionSet(mission);
		break;
	case STAGE_MISSION_GOTO:
		CP_InterceptAttackInstallation(mission);
		break;
	case STAGE_INTERCEPT:
		assert(mission->ufo);
		/* Leave earth */
		if (AIRFIGHT_ChooseWeapon(mission->ufo->weapons, mission->ufo->maxWeapons, mission->ufo->pos, mission->ufo->pos) !=
			AIRFIGHT_WEAPON_CAN_NEVER_SHOOT && mission->ufo->status == AIR_UFO && !mission->data.installation) {
			/* UFO is fighting and has still ammo, wait a little bit before leaving (UFO is not attacking an installation) */
			const DateTime additionalDelay(0, 3600);	/* check every hour if there is still ammos */
			mission->finalDate = ccs.date + additionalDelay;
		} else
			CP_InterceptMissionLeave(mission, true);
		break;
	case STAGE_RETURN_TO_ORBIT:
		/* mission is over, remove mission */
		CP_InterceptMissionIsSuccess(mission);
		break;
	default:
		cgi->Com_Printf("CP_InterceptNextStage: Unknown stage: %i, removing mission.\n", mission->stage);
		CP_MissionRemove(mission);
		break;
	}
}
