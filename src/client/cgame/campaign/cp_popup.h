/**
 * @file
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

#pragma once

bool CL_DisplayHomebasePopup(aircraft_t* aircraft, bool alwaysDisplay);
void CL_PopupInit(void);
void CL_DisplayPopupInterceptUFO(aircraft_t* ufo);
void CL_DisplayPopupInterceptMission(struct mission_s* mission);
void CP_Popup(const char* title, const char* text, ...) __attribute__((format(__printf__,2,3)));
