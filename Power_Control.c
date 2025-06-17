//Power_Control.c - Routines handling the control point logic

#include "g_local.h"

// MH: separated from edict_s to avoid bloating all entities
Power_Control_Struct Power_Control[POWER_MAX_CONTROL_POINTS];

char* Null_Marker_Name = "No name specified";

char* Control_Point_Model_Names[4] = {
	"models/controls/neutral.md2",
	"models/controls/team_1.md2",
	"models/controls/team_2.md2",
	"models/controls/team_3.md2" // MH: 3rd team
};

void Power_Overlay_Generate(void)
{
	//Combine the overlay data from each control for transmission
	//Build the overlay
	switch (Power_Game.num_control_points)
	{
		default:
		{
			return;
			break;
		}
		case 1:
		{
			// MH:
			//Com_sprintf(Power_Game.Overlay, POWER_OVERLAY_BUFFER_SIZE, "%s ", Power_Game.markers[0]->Power_Control.Overlay);
			Com_sprintf(Power_Game.Overlay, POWER_OVERLAY_BUFFER_SIZE, 
				"xl 5 %s ", 
				Power_Control[0].Overlay);
			break;
		}
		case 2:
		{
			// MH:
			//Com_sprintf(Power_Game.Overlay, POWER_OVERLAY_BUFFER_SIZE, "%s %s ", Power_Game.markers[0]->Power_Control.Overlay, Power_Game.markers[1]->Power_Control.Overlay);
			Com_sprintf(Power_Game.Overlay, POWER_OVERLAY_BUFFER_SIZE, 
				"xl 5 %s %s ", 
				Power_Control[0].Overlay, Power_Control[1].Overlay);
			break;
		}
		case 3:
		{
			// MH:
			//Com_sprintf(Power_Game.Overlay, POWER_OVERLAY_BUFFER_SIZE, "%s %s %s ", Power_Game.markers[0]->Power_Control.Overlay, Power_Game.markers[1]->Power_Control.Overlay, Power_Game.markers[2]->Power_Control.Overlay);
			Com_sprintf(Power_Game.Overlay, POWER_OVERLAY_BUFFER_SIZE, 
				"xl 5 %s %s %s ", 
				Power_Control[0].Overlay, Power_Control[1].Overlay, Power_Control[2].Overlay);
			break;
		}
	}
	Power_Game.Overlay_Length = strlen(Power_Game.Overlay);
	//	gi.dprintf("Power_Overlay_Generate() called\n");
}

void Power_Control_Overlay_Generate(edict_t* ent)
{//This is now generated directly by the control when something changes
	//long Counter;
	int Y_Coordinate;
	//int Time;

	char* Model_Target;
	char* Colour_Target;

	//Target coordinate
	Y_Coordinate = 150; //offset under radar
	Y_Coordinate += (ent->count * 56); //hypov8 smaller image used

	switch (ent->style)
	{
		default:
		{
			Model_Target = "/pics/m0.tga";
			Colour_Target = "888";
			break;
		}
		case 1:
		{
			Model_Target = "/pics/m1.tga";
			Colour_Target = "933";
			break;
		}
		case 2:
		{
			Model_Target = "/pics/m2.tga";
			Colour_Target = "993";
			break;
		}
		// MH: 3rd team
		case 3:
		{
			Model_Target = "/pics/m3.tga";
			Colour_Target = "339";
			break;
		}
	}
	//Build the string
	sprintf(ent->Power_Control->Overlay, "yt %d picn %s yt %d string \"%s\"", 
		Y_Coordinate, Model_Target, Y_Coordinate + 38, ent->name);

	//Clear the string if in the correct phase
	if (ent->Power_Control->Notify_Time > 0)
	{
		//The marker can only be cleared if within the possible flash time, and the flash state is false
		//This means regardless of flash state, the overlay marker will always be displayed when its flash timer expires or when the control is initialised
		if (Power_Game.HUD_Flash_State == false)
		{
			//Always clear the string if required. This will prevent flash 'glitches' if another control point is captured within another controls flash time
			*ent->Power_Control->Overlay = 0x00;

			//Check the last overlay state
			if (ent->Power_Control->Is_Overlay_Empty == false)
			{
				ent->Power_Control->Is_Overlay_Empty = true;
				//Update to all clients
				Power_Game.Overlay_Updated = true;
			}
		}
		else
		{
			//Overlay will be displayed
			//Check the last overlay state
			if (ent->Power_Control->Is_Overlay_Empty == true)
			{
				ent->Power_Control->Is_Overlay_Empty = false;
				//Update to all clients
				Power_Game.Overlay_Updated = true;
			}
		}
	}
	else
	{
		//Overlay will be displayed. This is the initialised and timer expire state
		ent->Power_Control->Is_Overlay_Empty = false;
		//Update to all clients
		Power_Game.Overlay_Updated = true;
	}

	/*
	The overlay should be transmitted:
	1) Immediately if a control point is captured
	2) On flash state change

	This might require different calls for touch and think, although can probably just set
	Power_Game.Overlay_Updated = true; after the generate call if a control was captured

	This means remove the existing Power_Game.Overlay_Updated = true; and move into the code that detects the state change

	The goal is to only transmit the overlay if either of the 2 events above are true.
	*/
}

void GetChaseMessage(edict_t *ent, char *entry);
void Power_Overlay_Display(edict_t* ent)
{
	char Holder[1024];
	char Final[1024];
	char* Final_Overlay;
	int Chase_Length;

	//Add in the spectator display if required
	if (ent->client->pers.spectator == SPECTATING && (level.modeset == PUBLIC || level.modeset == MATCH))
	{
		GetChaseMessage(ent, Holder);
		Chase_Length = strlen(Holder);
		if ((Power_Game.Overlay_Length + Chase_Length) < 1024)
		{
			sprintf(Final, "%s%s", Power_Game.Overlay, Holder);
			Final_Overlay = Final;
		}
		else
		{
			//Not enough space. Just return the power overlay
			Final_Overlay = Power_Game.Overlay;
		}
	}
	else
	{
		//Write the base overlay
		Final_Overlay = Power_Game.Overlay;
	}
	gi.WriteByte(svc_layout);
	gi.WriteString(Final_Overlay);

	//	gi.dprintf("Power_Overlay_Display() called\n");
}

void Power_Control_Think(edict_t *self)
{
	//Set the next think time
	self->nextthink = level.time + POWER_CONTROL_THINK_TIME;

	//Delay before recapture
	if (self->Power_Control->Capture_Delay > 0)
	{
		self->Power_Control->Capture_Delay--;
		//If reaching zero, restore the rotate effect
		if (self->Power_Control->Capture_Delay == 0)
			self->s.effects |= EF_ROTATE;
	}
	//Check not in intermission
	if (level.intermissiontime)
		return;
	//Check a game is in progress
	if ((level.modeset != MATCH) && (level.modeset != PUBLIC))
		return;
	//Notify time
	if (self->Power_Control->Notify_Time > 0)
	{
		self->Power_Control->Notify_Time--;
		//Update the overlay
		Power_Control_Overlay_Generate(self);
	}
	//Check for points allocation
	if (self->Power_Control->Points_Delay > 0)
		self->Power_Control->Points_Delay--;
	//Do the check again as we do not want an extra think before allocating points
	if (self->Power_Control->Points_Delay == 0)
	{
		switch (self->style)
		{
			case 1:
			{
				/*	Power_Game.team_score[0]++;
					if (Power_Game.team_score[0] > 9999)
					Power_Game.team_score[0] = 9999;*/
				// MH: use existing team score array
				team_cash[1]++;
				if (team_cash[1] > 9999)
					team_cash[1] = 9999;
				break;
			}
			case 2:
			{
				/*	Power_Game.team_score[1]++;
					if (Power_Game.team_score[1] > 9999)
					Power_Game.team_score[1] = 9999;*/
				// MH: use existing team score array
				team_cash[2]++;
				if (team_cash[2] > 9999)
					team_cash[2] = 9999;
				break;
			}
			// MH: 3rd team
			case 3:
			{
				/*	Power_Game.team_score[1]++;
					if (Power_Game.team_score[1] > 9999)
					Power_Game.team_score[1] = 9999;*/
				// MH: use existing team score array
				team_cash[3]++;
				if (team_cash[3] > 9999)
					team_cash[3] = 9999;
				break;
			}
		}
		//Reset the points award delay (in thinks)
		self->Power_Control->Points_Delay = POWER_CONTROL_POINTS_DELAY;
		UpdateScore();
	}
}

void Power_Control_Touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (self->Power_Control->Capture_Delay > 0)
		return;

	if (other->leader)
		other = other->leader;

	if (!other->client)
		return;
	//Check if already captured by this team
	if (self->style == other->client->pers.team)
		return;
	//Update the capture team parameter
	self->style = other->client->pers.team;
	//DEBUG
	//	self->style++;
	//	self->style = self->style % 3;
	//END DEBUG
	/* // MH: disabled
	//Captured
	//	gi.bprintf(PRINT_HIGH, "%s captured %s and receives a frag bonus.\n", team_names[other->client->pers.team], self->name);
	gi.bprintf(PRINT_HIGH, "%s captured %s and receives a frag bonus.\n", other->client->pers.netname, self->name);
	//Give frag bonus for capturing the point
	other->client->resp.score += POWER_POINTS_CONTROL_CAPTURE_BONUS;
	//Give team frag bonus as well
	team_cash[other->client->pers.team] += POWER_POINTS_CONTROL_CAPTURE_BONUS;
	//	UpdateScore();	//Serverinfo scores show the capture count
	*/
	//Make the team score flash for a while
	Power_Game.capture_time[other->client->pers.team - 1] = level.time + POWER_CAPTURE_FLASH_TIME;
	//Update the model
	self->s.modelindex = Power_Game.Control_Model_Index[other->client->pers.team];
	//Clear the rotate effect
	self->s.effects = 0;

	// MH: glow in team colour
	if (self->style == 1)
		self->s.effects |= EF_FLAG1;
	else if (self->style == 2)
		self->s.effects |= EF_BLASTER;
	else
		self->s.effects |= EF_FLAG2;

	//Reset the think time
	self->nextthink = level.time + POWER_CONTROL_THINK_TIME;
	//Reset the points award delay (in thinks)
	self->Power_Control->Points_Delay = POWER_CONTROL_POINTS_DELAY;
	//Reset the delay before recapture (in thinks)
	self->Power_Control->Capture_Delay = POWER_CONTROL_CAPTURE_DELAY;
	//Set the notify time
	self->Power_Control->Notify_Time = Power_Game.HUD_Flash_Time;
	//Play a sound
	gi.sound(self, CHAN_NO_PHS_ADD + CHAN_VOICE, Power_Game.Control_Capture_Sound, 0.75f, ATTN_NONE, 0);

	//Update the overlay
	Power_Control_Overlay_Generate(self);
	//Force an update to all clients
	Power_Game.Overlay_Updated = true;
}

void Power_Check_Award_Defend_Bonus(edict_t *target, edict_t *attacker)
{
	//target is enemy just killed, attacker is killer
	//Call this from death routine in P_Client.c
	float Distance;
	edict_t **Control = NULL;
	long Index;

	Control = Power_Game.markers;
	for (Index = 0; Index < Power_Game.num_control_points; Index++, Control++)
	{
		//Is it controlled by the killers team?
		if ((*Control)->style != attacker->client->pers.team)
			continue;
		/*
		//Is it visible to the killer?
		if (visible(*Control, attacker) == false)
		continue;
		gi.dprintf("Power_Check_Award_Defend_Bonus() - Control point visible to attacker: %s\n", (*Control)->name);
		Note that defend bonus should be awarded even if the attacker cannot see the control point.
		This is relevant when the killer cannot see the control point - eg camping or firing gl from hiding spot etc.
		*/

		//Is it visible to the victim? Must check this in case of within range but behind a wall where the control point is otherwise inaccessible, etc.
		if (visible(*Control, target) == false)
			continue;
		//Is it within bonus range?
		Distance = VectorDistance((*Control)->s.origin, target->s.origin);
		if (Distance >= 512.0f)
			continue;
		//Within range. Award the bonus
		gi.bprintf(PRINT_HIGH, "%s defends a %s control point and receives a frag bonus.\n", attacker->client->pers.netname, team_names[attacker->client->pers.team]);
		attacker->client->resp.score += POWER_POINTS_CONTROL_PROTECT_BONUS;
		//Give team frag bonus as well
		team_cash[attacker->client->pers.team] += POWER_POINTS_CONTROL_PROTECT_BONUS;
		//		UpdateScore();	//Serverinfo scores show the capture count
		//No further checking
		return;
	}
	//	gi.dprintf("Exiting Power_Check_Award_Defend_Bonus() - No bonus awarded\n");
}

void SP_Power_Control(edict_t *self)
{
	//	gi.dprintf("Control Spawned\n");

	if (Power_Game.num_control_points >= POWER_MAX_CONTROL_POINTS)
	{
		G_FreeEdict(self);
		gi.dprintf("Too many power_control_point entities. Deleted.\n");
		return;
	}

	// MH:
	self->Power_Control = &Power_Control[Power_Game.num_control_points];

	//Initialise to uncaptured mode
	self->model = Control_Point_Model_Names[0];
	self->s.modelindex = Power_Game.Control_Model_Index[0];

	//Set parameters
	self->s.effects = EF_ROTATE | EF_BLUEHYPERBLASTER; // MH: whitish glow
	VectorSet(self->mins, -16, -16, -24);// As big as player
	VectorSet(self->maxs, 16, 16, 48);
	self->viewheight = 20;	//20 is half the height of a player
	self->s.renderfx = RF_GLOW;
	self->touch = Power_Control_Touch;
	self->solid = SOLID_TRIGGER;
	self->think = Power_Control_Think;
	self->Power_Control->Capture_Delay = 0;
	self->Power_Control->Points_Delay = 0;
	self->Power_Control->Notify_Time = 0;
	//Clear the overlay text holder
	*self->Power_Control->Overlay = 0x00;
	self->nextthink = level.time + POWER_CONTROL_THINK_TIME;

	self->style = 0;	//Capture team index
	self->count = Power_Game.num_control_points;
	if (self->name == NULL)
	{
		//No name field entered
		self->name = Null_Marker_Name;
	}
	else
		if (*self->name == 0x00)
		{
			//Name value was empty
			self->name = Null_Marker_Name;
		}
	//Restrict name length for the HUD overlay
	if (strlen(self->name) > 32)
	{
		self->name[32] = 0x00;
	}
	Power_Game.markers[Power_Game.num_control_points++] = self;
	//Generate the overlay string for this control
	Power_Control_Overlay_Generate(self);
	//The overlay state is valid
	self->Power_Control->Is_Overlay_Empty = false;
	//Force an update to all clients
	Power_Game.Overlay_Updated = true;
	gi.linkentity(self);
}

void SP_Power_Dummy(edict_t *self)
{
	//Just delete the entity
	G_FreeEdict(self);
}

/*
New hud update code is working, but mr damage does not see the flash every time due to his ping
Update is 0.1 sec, but his ping is 0.3 sec. Nothing can be done about that.
HUD update always ends up with correct icon on though so no changes needed.
Possibly slow down the flash to 0.2 sec and extend the flash time to 3 sec or thereabouts
Flashing at an odd number would probably require an xor flag to be flipped during the think or the generate call,
and only when it flips should the update be made.
However, keeping to even time flashes (level.framenum & x != 0) -> clear the string
keeps the code simple. Need to reorganise code so that it either clears the string or builds, not build then test for clear.
*/