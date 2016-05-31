/*
	(C) Alex Caston 2016
	-----------------------
	-- Mod made by Aex12 --
	-----------------------

	------------------------------------
	--     Full list of natives:      --
	-- http://www.dev-c.com/nativedb/ --
	------------------------------------
*/

#include "script.h"
#include "keyboard.h"
#include <string.h>
#include "utils.h"
#include "iniReader.h"
#include "logIt.h"
#include <sstream>
#include "logIt.h"
using namespace std;

CIniReader config(".\\flashback.ini");
int kb_flashback = config.ReadInteger("Keyboard", "flashback", 76);
int gp_flashback_1 = config.ReadInteger("Gamepad", "flashback_1", 44);
int gp_flashback_2 = config.ReadInteger("Gamepad", "flashback_2", 10);
int gp_flashback_3 = config.ReadInteger("Gamepad", "flashback_3", 11);
bool restoreVehicleSpeed = config.ReadBoolean("Settings", "restoreVehicleSpeed", true);


struct vehicleFlashbackStruct {
	Vehicle vehicle;
	Vector3 position;
	Vector3 rotation;
};

struct playerFlashbackStruct {
	bool isInVehicle;
	float vehicleEngineHealth; /// VEHICLE::GET/SET_VEHICLE_ENGINE_HEALTH
	float vehicleBodyHealth; /// VEHICLE::GET/SET_VEHICLE_BODY_HEALTH
	float vehicleDirtLevel; /// VEHICLE::GET/SET_VEHICLE_DIRT_LEVEL
	Vehicle vehicle;
	Hash vehicleModel; /// ENTITY::GET_ENTITY_MODEL

	int wantedLevel; /// PLAYER::GET_PLAYER_WANTED_LEVEL - PLAYER::SET_PLAYER_WANTED_LEVEL
	Vector3 wantedCircle; /// PLAYER::GET_PLAYER_WANTED_CENTRE_POSITION - PLAYER::SET_PLAYER_WANTED_CENTRE_POSITION
	int playerHealth; /// ENTITY::GET/SET_ENTITY_HEALTH

	Vector3 camPos;
	Vector3 camRot;
	float   camFov;
	Vector3 position;
	Vector3 rotation;
	Vector3 speed;
	Vector3 rotationSpeed;
};

logIt f_log;
const int maxframes = 9000;
const int numVehiclesSaved = 12;
const int arrSizeSavedVehicles = numVehiclesSaved * 2 + 2;
int nearbyVehicles[arrSizeSavedVehicles];
vehicleFlashbackStruct vehiclesFlashback[maxframes][numVehiclesSaved];
playerFlashbackStruct playerFlashback[maxframes];
int fcount = 0;
bool fxactive = false;

void draw_text(char *text) {
	UI::SET_TEXT_FONT(1);
	UI::SET_TEXT_SCALE(1.2, 1.2);
	UI::SET_TEXT_COLOUR(255, 255, 255, 255);
	UI::SET_TEXT_WRAP(0.0, 1.0);
	UI::SET_TEXT_CENTRE(true);
	UI::SET_TEXT_DROPSHADOW(4, 4, 0, 0, 0);
	UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
	UI::_SET_TEXT_ENTRY("STRING");
	UI::_ADD_TEXT_COMPONENT_STRING(text);
	UI::_DRAW_TEXT(0.48, 0.5);
}

int draw_debug(string title, int value, int line) {
	std::ostringstream dstream;
	dstream << title << ": " << value;

	UI::SET_TEXT_FONT(0);
	UI::SET_TEXT_SCALE(0.32, 0.32);
	UI::SET_TEXT_COLOUR(225, 0, 225, 255);
	UI::SET_TEXT_WRAP(0.0, 1.0);
	UI::SET_TEXT_CENTRE(false);
	UI::_SET_TEXT_ENTRY("STRING");
	UI::_ADD_TEXT_COMPONENT_STRING(&dstream.str()[0]);
	UI::_DRAW_TEXT(0.005, 0.021*(line+0.05));
	return line + 1;
}

void player_save_flashback(int fbcount)
{
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	Player player = PLAYER::PLAYER_ID();
	if (PED::IS_PED_IN_ANY_VEHICLE(PLAYER::PLAYER_PED_ID(), false))
	{
		Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_USING(playerPed);
		playerFlashback[fbcount].isInVehicle = true;
		playerFlashback[fbcount].vehicle = playerVehicle;
		playerFlashback[fbcount].vehicleEngineHealth = VEHICLE::GET_VEHICLE_ENGINE_HEALTH(playerVehicle);
		playerFlashback[fbcount].vehicleBodyHealth = VEHICLE::GET_VEHICLE_BODY_HEALTH(playerVehicle);
		playerFlashback[fbcount].vehicleDirtLevel = VEHICLE::GET_VEHICLE_DIRT_LEVEL(playerVehicle);
		playerFlashback[fbcount].vehicleModel = ENTITY::GET_ENTITY_MODEL(playerVehicle);
		playerFlashback[fbcount].speed = ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, false);
		playerFlashback[fbcount].rotationSpeed = ENTITY::GET_ENTITY_ROTATION_VELOCITY(playerVehicle);
		playerFlashback[fbcount].position = ENTITY::GET_ENTITY_COORDS(playerVehicle, true);
		playerFlashback[fbcount].rotation = ENTITY::GET_ENTITY_ROTATION(playerVehicle, true);
	}
	else
	{
		playerFlashback[fbcount].isInVehicle = false;
		playerFlashback[fbcount].position = ENTITY::GET_ENTITY_COORDS(playerPed, true);
		playerFlashback[fbcount].rotation = ENTITY::GET_ENTITY_ROTATION(playerPed, true);
	}
	playerFlashback[fbcount].playerHealth = ENTITY::GET_ENTITY_HEALTH(playerPed);
	Cam camera;
	playerFlashback[fbcount].camPos = CAM::GET_GAMEPLAY_CAM_COORD();
	playerFlashback[fbcount].camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
	playerFlashback[fbcount].camFov = CAM::GET_GAMEPLAY_CAM_FOV();
	playerFlashback[fbcount].wantedLevel = PLAYER::GET_PLAYER_WANTED_LEVEL(player);
	if (playerFlashback[fbcount].wantedLevel > 1)
	{
		playerFlashback[fbcount].wantedCircle = PLAYER::GET_PLAYER_WANTED_CENTRE_POSITION(player);
	}
}

void player_load_after_flashback(playerFlashbackStruct fplayer)
{
	if (fplayer.isInVehicle)
	{
		ENTITY::APPLY_FORCE_TO_ENTITY(fplayer.vehicle, 3, 0, 0, 0, fplayer.rotationSpeed.x, fplayer.rotationSpeed.y, fplayer.rotationSpeed.z, false, true, false, true, false, false);
		ENTITY::SET_ENTITY_VELOCITY(fplayer.vehicle, fplayer.speed.x, fplayer.speed.y, fplayer.speed.z);
	
	}
	CAM::RENDER_SCRIPT_CAMS(0, 1, 3000, 1, 0);
}

void player_load_flashback(playerFlashbackStruct fplayer)
{
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	Player player = PLAYER::PLAYER_ID();
	CAM::DESTROY_ALL_CAMS(true);
	Cam camera;
	camera = CAM::CREATE_CAM_WITH_PARAMS("DEFAULT_SCRIPTED_CAMERA", fplayer.camPos.x, fplayer.camPos.y, fplayer.camPos.z, fplayer.camRot.x, fplayer.camRot.y, fplayer.camRot.z, fplayer.camFov, true, 1);
	CAM::RENDER_SCRIPT_CAMS(1, 0, camera, 1, 1);
	if (fplayer.isInVehicle)
	{
		if (PED::IS_PED_IN_ANY_VEHICLE(PLAYER::PLAYER_PED_ID(), false))
		{
			Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_USING(playerPed);
			if (ENTITY::GET_ENTITY_MODEL(playerVehicle) != fplayer.vehicleModel)
			{
				fplayer.vehicle = VEHICLE::CREATE_VEHICLE(fplayer.vehicleModel, fplayer.position.x, fplayer.position.y, fplayer.position.z, 0.0, 1, 1);
				PED::SET_PED_INTO_VEHICLE(playerPed, fplayer.vehicle, -1);
			}
		}
		else
		{
			if (ENTITY::DOES_ENTITY_EXIST(fplayer.vehicle) && ((VEHICLE::GET_PED_IN_VEHICLE_SEAT(fplayer.vehicle, -1) != playerPed) || VEHICLE::IS_VEHICLE_SEAT_FREE(fplayer.vehicle, -1)))
			{
				PED::SET_PED_INTO_VEHICLE(playerPed, fplayer.vehicle, -1);
			}
			else
			{
				fplayer.vehicle = VEHICLE::CREATE_VEHICLE(fplayer.vehicleModel, fplayer.position.x, fplayer.position.y, fplayer.position.z, 0.0, 1, 1);
				PED::SET_PED_INTO_VEHICLE(playerPed, fplayer.vehicle, -1);
			}
		}

		ENTITY::SET_ENTITY_COORDS(fplayer.vehicle, fplayer.position.x, fplayer.position.y, fplayer.position.z, false, false, true, false);
		ENTITY::SET_ENTITY_ROTATION(fplayer.vehicle, fplayer.rotation.x, fplayer.rotation.y, fplayer.rotation.z, 1, false);
		VEHICLE::SET_VEHICLE_ENGINE_HEALTH(fplayer.vehicle, fplayer.vehicleEngineHealth);
		VEHICLE::SET_VEHICLE_BODY_HEALTH(fplayer.vehicle, fplayer.vehicleBodyHealth);
		VEHICLE::SET_VEHICLE_DIRT_LEVEL(fplayer.vehicle, fplayer.vehicleDirtLevel);
	}
	else
	{
		ENTITY::SET_ENTITY_COORDS(playerPed, fplayer.position.x, fplayer.position.y, fplayer.position.z, false, false, true, false);
		ENTITY::SET_ENTITY_ROTATION(playerPed, fplayer.rotation.x, fplayer.rotation.y, fplayer.rotation.z, 1, false);
	}
	ENTITY::SET_ENTITY_HEALTH(playerPed, fplayer.playerHealth);
	PLAYER::SET_PLAYER_WANTED_LEVEL(player, fplayer.wantedLevel, false);
	if(fplayer.wantedLevel > 1)
	{
		PLAYER::SET_PLAYER_WANTED_CENTRE_POSITION(player, fplayer.wantedCircle.x, fplayer.wantedCircle.y, fplayer.wantedCircle.z);
	}
}

void vehicle_save_flashback(Vehicle vehicle, int fcount, int i) 
{
	vehiclesFlashback[fcount][i].vehicle = vehicle;
	vehiclesFlashback[fcount][i].position = ENTITY::GET_ENTITY_COORDS(vehicle, true);
	vehiclesFlashback[fcount][i].rotation = ENTITY::GET_ENTITY_ROTATION(vehicle, true);
}

void vehicle_load_flashback(vehicleFlashbackStruct fvehicle)
{
	ENTITY::SET_ENTITY_COORDS(fvehicle.vehicle, fvehicle.position.x, fvehicle.position.y, fvehicle.position.z, false, false, true, false);
	ENTITY::SET_ENTITY_ROTATION(fvehicle.vehicle, fvehicle.rotation.x, fvehicle.rotation.y, fvehicle.rotation.z, 1, false);
}

void update()
{
	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if(!ENTITY::DOES_ENTITY_EXIST(playerPed)) { return; }
	if(!PLAYER::IS_PLAYER_CONTROL_ON(player)) { return; }
	if(ENTITY::IS_ENTITY_DEAD(playerPed)) { return; }
	if(PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE)) { return; }


	int dline;
	dline = draw_debug("Flashback! debug", 1, 0);
	dline = draw_debug("fcount", fcount, dline);
	dline = draw_debug("fxactive", fxactive, dline);

	if(((CONTROLS::IS_CONTROL_PRESSED(2, gp_flashback_1)) && (CONTROLS::IS_CONTROL_PRESSED(2, gp_flashback_2))) || (IsKeyDown(kb_flashback)))
	{
		if (!fxactive)
		{
			fxactive = true;
			AUDIO::PLAY_SOUND_FRONTEND(-1, "TENNIS_POINT_WON", "HUD_AWARDS", false); /// http://pastebin.com/0neZdsZ5
		}
		GAMEPLAY::SET_TIME_SCALE(0);
		draw_text("Flashback!");
		GRAPHICS::_START_SCREEN_EFFECT("HeistLocate", 0, true);
		

		/// 3x speed, use RB + Left Dpad(or LT) + RT
		if ((CONTROLS::IS_CONTROL_PRESSED(2, gp_flashback_3)) || (IsKeyDown(kb_flashback)))
		{
			fcount = fcount - 3;
		}
		else /// normal speed
		{
			fcount = fcount - 1;
		}

		if (fcount < 0) {
			if (playerFlashback[maxframes-1].position.x)
			{
				fcount = maxframes - 1;
			}
			else
			{
				fcount = 0;
			}
		}


		for (int i = 0; i < numVehiclesSaved+1; i++)
		{
			vehicle_load_flashback(vehiclesFlashback[fcount][i]);
		}

		if (!(playerFlashback[fcount].playerHealth == 0 && playerFlashback[fcount].position.x == 0 && playerFlashback[fcount].position.y == 0))
		{
			player_load_flashback(playerFlashback[fcount]);
		}

	} 
	else
	{
		if (fxactive == true) /// This will be execute inmediately after any flashback (I need to write == true because when fcount reach 1539, fxactive becomes a random number over 200, even if it's a boolean...
		{
			AUDIO::PLAY_SOUND_FRONTEND(-1, "TENNIS_POINT_WON", "HUD_AWARDS", false); /// http://pastebin.com/0neZdsZ5
			player_load_after_flashback(playerFlashback[fcount]);
			GAMEPLAY::SET_TIME_SCALE(1);
			GRAPHICS::_STOP_SCREEN_EFFECT("HeistLocate");
			fxactive = false;
		}
		nearbyVehicles[0] = numVehiclesSaved;
		int nearbyVehiclesCount = PED::GET_PED_NEARBY_VEHICLES(playerPed, nearbyVehicles);
		if (nearbyVehicles != NULL)
		{
			//Simple loop to go through results
			for (int i = 0; i < nearbyVehiclesCount; i++)
			{
				int offsettedID = i * 2 + 2;
				//Make sure it exists
				if (nearbyVehicles[offsettedID] != NULL && ENTITY::DOES_ENTITY_EXIST(nearbyVehicles[offsettedID]))
				{
					vehicle_save_flashback(nearbyVehicles[offsettedID], fcount, i);
				}
			}
		}

		player_save_flashback(fcount);
		fcount += 1;
		if (fcount >= maxframes)
		{
			fcount = 0;
		}
	}

}

void main()
{
	f_log.clear();
	f_log.writeStr("logIt initialized");
	f_log.writeStr("Peso en MB actual de playerFlashback");
	f_log.writeFloat(((sizeof(playerFlashback)/(float)1024)/(float)1024),5);
	f_log.writeStr("Peso en MB actual de vehiclesFlashback");
	f_log.writeFloat(((sizeof(vehiclesFlashback)/(float)1024)/(float)1024),5);
	while (true)
	{
		update();
		WAIT(0);
	}
}

void ScriptMain()
{
	main();
}