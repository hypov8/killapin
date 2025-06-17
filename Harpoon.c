//Harpoon.c - routines for the harpoon weapon
/*
CODE TAKEN FROM Q2CTF grapple and modified for Kingpin.

Changed name to harpoon
Added variable distance limit
Added variable fly & pull speeds
Added variable damage
*/

#include "g_local.h"

//Speed harpoon flies at
#define HARPOON_SPEED_FLY_MINIMUM	650
#define HARPOON_SPEED_FLY_MAXIMUM	2500

//Speed player is pulled at
#define HARPOON_SPEED_PULL_MINIMUM	650
#define HARPOON_SPEED_PULL_MAXIMUM	3000

//Damage inflicted on a player hit with the harpoon
#define HARPOON_DAMAGE_MINIMUM 0
#define HARPOON_DAMAGE_MAXIMUM 500	//Blow the player up

cvar_t *harpoon_enable;
cvar_t *harpoon_max_distance;
cvar_t* harpoon_cable;
cvar_t *harpoon_speed_fly;
cvar_t *harpoon_speed_pull;
cvar_t *harpoon_damage;

Harpoon_Parameters_Def Harpoon_Parameters;

//Logic of the reliable below is that looping sounds should never be reliable.
//One-shot sounds should always be reliable so that they override any looping sounds that might have made it through to be heard.

void Cmd_Set_Harpoon_Speed_Fly_f(edict_t *ent, char *value)
{
	int i;
	if (ent->client->pers.admin == NOT_ADMIN)
	{
		gi.cprintf(ent, PRINT_HIGH, "You do not have admin\n");
		return;
	}
//Player has admin
//Check that a value was actually entered
	if (*value == 0x00)
	{
		gi.cprintf(ent, PRINT_HIGH, "Usage: setharpoonspeedfly <value>\n");
		return;
	}
	i = atoi(value);
	if ((i < HARPOON_SPEED_FLY_MINIMUM) || (i > HARPOON_SPEED_FLY_MAXIMUM))
	{
		gi.cprintf(ent, PRINT_HIGH, "Please choose a value between %d and %d\n", HARPOON_SPEED_FLY_MINIMUM, HARPOON_SPEED_FLY_MAXIMUM);
		return;
	}
	gi.bprintf(PRINT_HIGH, "harpoon_speed_fly has been changed to %d\n", i);
//Force the change as the cvar is latched
	gi.cvar_forceset("harpoon_speed_fly", value);
	Harpoon_Parameters.Speed_Fly = i;
}

void Cmd_Set_Harpoon_Speed_Pull_f(edict_t *ent, char *value)
{
	int i;
	if (ent->client->pers.admin == NOT_ADMIN)
	{
		gi.cprintf(ent, PRINT_HIGH, "You do not have admin\n");
		return;
	}
	//Player has admin
	//Check that a value was actually entered
	if (*value == 0x00)
	{
		gi.cprintf(ent, PRINT_HIGH, "Usage: setharpoonspeedpull <value>\n");
		return;
	}
	i = atoi(value);
	if ((i < HARPOON_SPEED_PULL_MINIMUM) || (i > HARPOON_SPEED_PULL_MAXIMUM))
	{
		gi.cprintf(ent, PRINT_HIGH, "Please choose a value between %d and %d\n", HARPOON_SPEED_PULL_MINIMUM, HARPOON_SPEED_PULL_MAXIMUM);
		return;
	}
	gi.bprintf(PRINT_HIGH, "harpoon_speed_fly has been changed to %d\n", i);
//Force the change as the cvar is latched
	gi.cvar_forceset("harpoon_speed_pull", value);
	Harpoon_Parameters.Speed_Pull = i;
}

void Cmd_Set_Harpoon_Damage_f(edict_t *ent, char *value)
{
	int i;
	if (ent->client->pers.admin == NOT_ADMIN)
	{
		gi.cprintf(ent, PRINT_HIGH, "You do not have admin\n");
		return;
	}
//Player has admin
//Check that a value was actually entered
	if (*value == 0x00)
	{
		gi.cprintf(ent, PRINT_HIGH, "Usage: setharpoondamage <value>\n");
		return;
	}
	i = atoi(value);
	if ((i < HARPOON_DAMAGE_MINIMUM) || (i > HARPOON_DAMAGE_MAXIMUM))
	{
		gi.cprintf(ent, PRINT_HIGH, "Please choose a value between %d and %d\n", HARPOON_DAMAGE_MINIMUM, HARPOON_DAMAGE_MAXIMUM);
		return;
	}
	gi.bprintf(PRINT_HIGH, "harpoon_damage has been changed to %d\n", i);
//Force the change as the cvar is latched
	gi.cvar_forceset("harpoon_damage", value);
	Harpoon_Parameters.Damage = i;
}

void Harpoon_Initialise(void)
{
	qboolean Invalid_Fly, Invalid_Pull, Invalid_Damage;
	char Buffer[16];

	Invalid_Fly = false;
	Invalid_Pull = false;
	Invalid_Damage = false;

//Ensure the harpoon parameters are within valid ranges
//Check fly speed
	if (harpoon_speed_fly->value < HARPOON_SPEED_FLY_MINIMUM)
	{
		Com_sprintf(Buffer, 16, "%s", HARPOON_SPEED_FLY_MINIMUM);
		gi.cvar_forceset("harpoon_speed_fly", Buffer);
		Invalid_Fly = true;
	}
	else
	if (harpoon_speed_fly->value > HARPOON_SPEED_FLY_MAXIMUM)
	{
		Com_sprintf(Buffer, 16, "%s", HARPOON_SPEED_FLY_MAXIMUM);
		gi.cvar_forceset("harpoon_speed_fly", Buffer);
		Invalid_Fly = true;
	}
//Check pull speed
	if (harpoon_speed_pull->value < HARPOON_SPEED_PULL_MINIMUM)
	{
		Com_sprintf(Buffer, 16, "%s", HARPOON_SPEED_PULL_MINIMUM);
		gi.cvar_forceset("harpoon_speed_pull", Buffer);
		Invalid_Pull = true;
	}
	else
	if (harpoon_speed_pull->value > HARPOON_SPEED_PULL_MAXIMUM)
	{
		Com_sprintf(Buffer, 16, "%s", HARPOON_SPEED_PULL_MAXIMUM);
		gi.cvar_forceset("harpoon_speed_pull", Buffer);
		Invalid_Pull = true;
	}
//Check damage
	if (harpoon_damage->value < HARPOON_DAMAGE_MINIMUM)
	{
		Com_sprintf(Buffer, 16, "%s", HARPOON_DAMAGE_MINIMUM);
		gi.cvar_forceset("harpoon_damage", Buffer);
		Invalid_Damage = true;
	}
	else
	if (harpoon_damage->value > HARPOON_DAMAGE_MAXIMUM)
	{
		Com_sprintf(Buffer, 16, "%s", HARPOON_DAMAGE_MAXIMUM);
		gi.cvar_forceset("harpoon_damage", Buffer);
		Invalid_Damage = true;
	}
//Update the parameters
	Harpoon_Parameters.Damage = (int)harpoon_damage->value;
	Harpoon_Parameters.Speed_Fly = (int)harpoon_speed_fly->value;
	Harpoon_Parameters.Speed_Pull = harpoon_speed_pull->value;

//Display errors if required
	if (Invalid_Fly == true)
		gi.dprintf("Invalid harpoon fly speed. Fixed\n");
	if (Invalid_Pull == true)
		gi.dprintf("Invalid harpoon pull speed. Fixed\n");
	if (Invalid_Damage == true)
		gi.dprintf("Invalid harpoon damage. Fixed\n");

//Precache the model
	Harpoon_Parameters.Model_Index = gi.modelindex("models/weapons/v_harpoon/arrow.mdx");
//Precache the sounds
	Harpoon_Parameters.Sound_Fire = gi.soundindex("weapons/harpoon/fire.wav");
	Harpoon_Parameters.Sound_Pull = gi.soundindex("weapons/harpoon/pull.wav");
	Harpoon_Parameters.Sound_Reset = gi.soundindex("weapons/harpoon/reset.wav");
	Harpoon_Parameters.Sound_Hit = gi.soundindex("weapons/harpoon/hit.wav");
	Harpoon_Parameters.Sound_Hang = gi.soundindex("weapons/harpoon/hang.wav");
//This sample does not exist but is referenced in the code. It does not exist in the q2ctf pak either. KP handles hurt sounds itself.
//	Harpoon_Parameters.Sound_Hurt = gi.soundindex("weapons/harpoon/hurt.wav");
}

// self is harpoon, not player
void Harpoon_Reset(edict_t *self, qboolean Silent)
{
	float volume;
	gclient_t *cl;
//Sanity
	if (self == NULL)
	{
//Debug message
//		gi.dprintf("DEBUG: Harpoon_Reset(): self is NULL\n");
		return;
	}
	if (self->owner == NULL)
	{
//Debug message
//		gi.dprintf("DEBUG: Harpoon_Reset(): self->owner is NULL\n");
//Release the harpoon entity
		G_FreeEdict(self);
		return;
	}
	if (self->owner->client == NULL)
	{
//Debug message
//		gi.dprintf("DEBUG: Harpoon_Reset(): self->owner->client is NULL\n");
//Release the harpoon entity
		G_FreeEdict(self);
		return;
	}
	if (self->owner->client->Harpoon.Weapon == NULL)
	{
//Debug message
//		gi.dprintf("DEBUG: Harpoon_Reset(): self->owner->client->Harpoon.Weapon is NULL\n");
//Release the harpoon entity
		G_FreeEdict(self);
		return;
	}
//OK to do full harpoon release

//CDEATH - Always want to play the sound here even if it is silent so that the looping hanging sound is killed.
	if (Silent == true)
		volume = 0.0f;
	else
		volume = 1.0f;
	gi.sound(self->owner, CHAN_RELIABLE + CHAN_WEAPON, Harpoon_Parameters.Sound_Reset, volume, ATTN_NORM, 0);
	cl = self->owner->client;
	cl->Harpoon.Weapon = NULL;
	cl->Harpoon.Release_Time = level.time;
	cl->Harpoon.State = HARPOON_STATE_FLY; // we're firing, not on hook
	cl->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
//Debug message
//	gi.dprintf("DEBUG: Harpoon_Reset(): Harpoon deleted\n");
//Release the harpoon entity
	G_FreeEdict(self);
	return;
}

// ent is player
void Harpoon_Player_Initialise(edict_t* ent)
{
	ent->client->Harpoon.Weapon = NULL;
	ent->client->Harpoon.Release_Time = level.time;
	ent->client->Harpoon.State = HARPOON_STATE_FLY;
}

void Harpoon_Player_Reset(edict_t *ent)
{
	if (ent->client && ent->client->Harpoon.Weapon)
	{
		Harpoon_Reset(ent->client->Harpoon.Weapon, true);
//Just in case...
		Harpoon_Player_Initialise(ent);
	}
/*	else
	{
//Debug
		if (ent->client == NULL)
		{
			gi.dprintf("DEBUG: Harpoon_Player_Reset(): ent->client is NULL\n");
			return;
		}
	}
*/
}

void Harpoon_Touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	float volume = 1.0;

	if (other == self->owner)
		return;

	if (self->owner->client->Harpoon.State != HARPOON_STATE_FLY)
		return;

	if ((surf) && (surf->flags & SURF_SKY))
	{
		Harpoon_Reset(self, false);
		return;
	}
//Check if touching another entity
	if (other)
	{
//Check if touching a cloud (elements_raincloud or elements_snowcloud)
		if (Q_strncasecmp(other->classname, "elements_", 9) == 0)
		{
			Harpoon_Reset(self, false);
			return;
		}
	}

	VectorCopy(vec3_origin, self->velocity);

//CDEATH - Disable PlayerNoise call as MM does not allow NPC ents
//	PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);
//END CDEATH

	if (other->takedamage) {
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, Harpoon_Parameters.Damage, 0, MOD_HARPOON);
		Harpoon_Reset(self, false);
		return;
	}
//Hit something that is not another player
	self->owner->client->Harpoon.State = HARPOON_STATE_PULL; // we're on hook
	self->enemy = other;

	self->solid = SOLID_NOT;

//CDEATH - Swapped the order and pull sound is now not reliable
	gi.sound(self, CHAN_RELIABLE + CHAN_WEAPON, Harpoon_Parameters.Sound_Hit, volume, ATTN_NORM, 0);

//Looks like alt-tab can cause this sample to be played if it is the last sound and its looping
	gi.sound(self->owner, CHAN_WEAPON, Harpoon_Parameters.Sound_Pull, volume, ATTN_NORM, 0);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_SPARKS);
	gi.WritePosition (self->s.origin);
	if (!plane)
		gi.WriteDir (vec3_origin);
	else
		gi.WriteDir (plane->normal);
	gi.multicast (self->s.origin, MULTICAST_PVS);
}

// draw beam between harpoon and self
void Harpoon_Draw_Cable(edict_t *self)
{
	vec3_t	offset, start, end, f, r;
	vec3_t	dir;
	float	distance;

	AngleVectors (self->owner->client->v_angle, f, r, NULL);
	VectorSet(offset, 16, 16, self->owner->viewheight - 13); // MH: moved a bit lower
	P_ProjectSource (self->owner->client, self->owner->s.origin, offset, f, r, start);

	VectorSubtract(start, self->owner->s.origin, offset);

	VectorSubtract (start, self->s.origin, dir);
	distance = VectorLength(dir);
	// don't draw cable if close
	if (distance < 64)
	{
		self->s.frame = 0;
		self->s.skinnum = 0;
		self->s.renderfx &= ~RF_BEAM;
		return;
	}
#if 1
	self->s.frame = 10;
	self->s.renderfx |= (RF_BEAM|RF_FRAMELERP);
	//self->s.renderfx2 |= RDF_NOLERP;
	switch (self->owner->client->pers.team)
	{
		default: self->s.skinnum = 0xf2f2f0f0; break;
		case 1:  self->s.skinnum = 0xE787E787; break;//red
		case 2:  self->s.skinnum = 0xCDC8CDC8; break;//yellow
		case 3:  self->s.skinnum = 0xE2E3E3E2; break;//blue
	}
#endif
// adjust start for beam origin being in middle of a segment
//	VectorMA (start, 8, f, start);
	VectorCopy (self->s.origin, end);


	//VectorCopy(start, end);
	//end[2] += self->owner->viewheight - 13;
	//lazerbeam source
	VectorCopy(start, self->s.old_origin); //

// adjust end z for end spot since the monster is currently dead
//	end[2] = self->absmin[2] + self->size[2] / 2;

	if ((int)harpoon_cable->value != 0)
	{
		// MH: send multiple times to solidify the beam since called only once per frame now (in ClientBeginServerFrame instead of ClientThink)
		//int i;

		//G_SetMovedir (self->owner->s.angles, self->movedir);

		/*for (i = 0; i < 4; i++)
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_BFG_LASER);//TE_GRAPPLE_CABLE
			gi.WritePosition(start);
			gi.WritePosition(end);
			gi.multicast(self->s.origin, MULTICAST_PVS);
		}*/
	}
}

// pull the player toward the harpoon
void Harpoon_Pull(edict_t *self)
{
	vec3_t hookdir, v;
	float vlen;

	if (strcmp(self->owner->client->pers.weapon->classname, "weapon_harpoon") == 0 &&
		!self->owner->client->newweapon &&
		self->owner->client->weaponstate != WEAPON_FIRING &&
		self->owner->client->weaponstate != WEAPON_ACTIVATING) {
		Harpoon_Reset(self, false);
		return;
	}

	if (self->enemy) {
		if (self->enemy->solid == SOLID_NOT) {
			Harpoon_Reset(self, false);
			return;
		}
		if (self->enemy->solid == SOLID_BBOX) {
			VectorScale(self->enemy->size, 0.5, v);
			VectorAdd(v, self->enemy->s.origin, v);
			VectorAdd(v, self->enemy->mins, self->s.origin);
			gi.linkentity (self);
		} else
			VectorCopy(self->enemy->velocity, self->velocity);
//CDEATH - CheckTeamDamage removed in mm2.0
		if (self->enemy->takedamage)// && !CheckTeamDamage (self->enemy, self->owner))
		{
			T_Damage(self->enemy, self, self->owner, self->velocity, self->s.origin, vec3_origin, Harpoon_Parameters.Damage, 1, 0, MOD_HARPOON);
//CDEATH - Do not play the hurt sound as it does not exist anyway.
//			float volume = 1.0;
//			gi.sound(self, CHAN_RELIABLE + CHAN_WEAPON, Harpoon_Parameters.Sound_Hurt, volume, ATTN_NORM, 0);
		}
		if (self->enemy->deadflag) { // he died
			Harpoon_Reset(self, false);
			return;
		}
	}

	if (self->owner->client->Harpoon.State == HARPOON_STATE_FLY)
	{
		if (harpoon_max_distance->value > 0)
		{
			vec3_t	offset, start, f, r;
			vec3_t	dir;
			float	distance;
			float Max_Distance;

			Max_Distance = harpoon_max_distance->value * 8;//Multiply by world units

			AngleVectors (self->owner->client->v_angle, f, r, NULL);
			VectorSet(offset, 16, 16, self->owner->viewheight - 13); // MH: moved a bit lower
			P_ProjectSource (self->owner->client, self->owner->s.origin, offset, f, r, start);

			VectorSubtract(start, self->owner->s.origin, offset);

			VectorSubtract (start, self->s.origin, dir);
			distance = VectorLength(dir);
//If harpoon distance is greater than the max harpoon distance, reset the harpoon
			if (distance > Max_Distance)
			{
				Harpoon_Reset(self, false);
				return;
			}
		}
	}

// MH: moved to ClientBeginServerFrame
//	Harpoon_Draw_Cable(self);

//	if (self->owner->client->Harpoon.State == HARPOON_STATE_PULL)// Should this be == HARPOON_STATE_PULL??
	if (self->owner->client->Harpoon.State > HARPOON_STATE_FLY)// Should this be == HARPOON_STATE_PULL??
	{
		// pull player toward harpoon
		// this causes icky stuff with prediction, we need to extend
		// the prediction layer to include two new fields in the player
		// move stuff: a point and a velocity.  The client should add
		// that velociy in the direction of the point
		vec3_t forward, up;

		AngleVectors (self->owner->client->v_angle, forward, NULL, up);
		VectorCopy(self->owner->s.origin, v);
		v[2] += self->owner->viewheight;
		VectorSubtract (self->s.origin, v, hookdir);

		vlen = VectorLength(hookdir);

		if (self->owner->client->Harpoon.State == HARPOON_STATE_PULL &&	vlen < 64)
		{
			float volume = 1.0;

			self->owner->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
//CDEATH MODIFIED. Do not make reliable as a release could fire immediately after and the priority sound is the harpoon retract sound
//			gi.sound(self->owner, CHAN_RELIABLE + CHAN_WEAPON, Harpoon_Parameters.Sound_Hang, volume, ATTN_NORM, 0);
			gi.sound(self->owner, CHAN_WEAPON, Harpoon_Parameters.Sound_Hang, volume, ATTN_NORM, 0);
			self->owner->client->Harpoon.State = HARPOON_STATE_HANG;
//Code must fall though here to keep the player attached to the harpoon
		}
		VectorNormalize (hookdir);
		VectorScale(hookdir, Harpoon_Parameters.Speed_Pull, hookdir);
		VectorCopy(hookdir, self->owner->velocity);
		SV_AddGravity(self->owner);
	}
}
extern void think_lighthouse_beam(edict_t *self);
void Harpoon_Fire(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{
	edict_t	*harpoon;
	trace_t	tr;

	VectorNormalize (dir);

	harpoon = G_Spawn();
//debug
	harpoon->classname = "harpoon class";

	VectorCopy (start, harpoon->s.origin);
	VectorCopy (start, harpoon->s.old_origin);
	vectoangles (dir, harpoon->s.angles);
	VectorScale (dir, speed, harpoon->velocity);
//CDEATH - Remove the shadow
	harpoon->s.renderfx2 |= RF2_NOSHADOW;
	harpoon->movetype = MOVETYPE_FLYMISSILE;
	harpoon->clipmask = MASK_SHOT;
	harpoon->solid = SOLID_BBOX;
	harpoon->s.effects |= effect;
	VectorClear (harpoon->mins);
	VectorClear (harpoon->maxs);
	harpoon->s.modelindex = Harpoon_Parameters.Model_Index;
//	harpoon->s.sound = gi.soundindex ("misc/lasfly.wav");
	harpoon->owner = self;
	harpoon->touch = Harpoon_Touch;
//	harpoon->nextthink = level.time + FRAMETIME;
//	harpoon->think = Harpoon_Think;
	harpoon->dmg = damage;
	self->client->Harpoon.Weapon = harpoon;
	self->client->Harpoon.State = HARPOON_STATE_FLY; // we're firing, not on hook

	gi.linkentity(harpoon);

	tr = gi.trace (self->s.origin, NULL, NULL, harpoon->s.origin, harpoon, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		VectorMA (harpoon->s.origin, -10, dir, harpoon->s.origin);
		harpoon->touch (harpoon, tr.ent, NULL, NULL);
	}
}	

void Harpoon_Fire_Start(edict_t *ent, vec3_t g_offset, int damage, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;
	float volume = 1.0;

	if (ent->client->Harpoon.State > HARPOON_STATE_FLY)
		return; // it's already out

	AngleVectors (ent->client->v_angle, forward, right, NULL);
//	VectorSet(offset, 24, 16, ent->viewheight-8+2);
	VectorSet(offset, 24, 8, ent->viewheight - 13); // MH: moved a bit lower
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

//CDEATH - Check if this should be reliable
//	gi.sound(ent, CHAN_RELIABLE + CHAN_WEAPON, Harpoon_Parameters.Sound_Fire, volume, ATTN_NORM, 0);
	gi.sound(ent, CHAN_WEAPON, Harpoon_Parameters.Sound_Fire, volume, ATTN_NORM, 0);
	Harpoon_Fire(ent, start, forward, damage, Harpoon_Parameters.Speed_Fly, effect);

#if 0
	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_BLASTER);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
#endif
//CDEATH - Disable PlayerNoise call as MM does not allow NPC ents
//	PlayerNoise(ent, start, PNOISE_WEAPON);
//END CDEATH
}


void Harpoon_Weapon_Fire (edict_t *ent)
{
	Harpoon_Fire_Start(ent, vec3_origin, Harpoon_Parameters.Damage, 0);
	ent->client->ps.gunframe++;
}

void Harpoon_Weapon_Execute(edict_t *ent)
{
	static int	pause_frames[] = {0};
	static int	fire_frames[] = {6, 0};
	int prevstate;


	if (ent->client->Harpoon.Weapon)
	{
		if (ent->client->buttons & BUTTON_ATTACK)
		{	
			// if the the attack button is still down, stay in the firing frame(6)
			if (ent->client->weaponstate == WEAPON_FIRING)
				ent->client->ps.gunframe = 5;
		}
		else
		{
			Harpoon_Reset(ent->client->Harpoon.Weapon, false);
			if (ent->client->weaponstate == WEAPON_FIRING)
				ent->client->weaponstate = WEAPON_READY;
		}
	}

//CDEATH - If changing weapon, only keep harpoon if hanging somewhere
//	if (ent->client->newweapon && ent->client->Harpoon.State > HARPOON_STATE_FLY && ent->client->weaponstate == WEAPON_FIRING)
	if (ent->client->newweapon && ent->client->weaponstate == WEAPON_FIRING)
	{
//Change weapons while harpooned
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = 13;
//Check if hanging
		if (ent->client->Harpoon.State < HARPOON_STATE_HANG)
		{
//Not hanging. Terminate the harpoon
			ent->client->weaponstate = WEAPON_DROPPING;
			Harpoon_Reset(ent->client->Harpoon.Weapon, false);
			return;
		}
	}
//END CDEATH


	prevstate = ent->client->weaponstate;
	Weapon_Generic(ent, 4, 8, 12, 16, pause_frames, fire_frames, Harpoon_Weapon_Fire);

// if we just switched back to harpoon, immediately go to fire frame
	if (prevstate == WEAPON_ACTIVATING && ent->client->weaponstate == WEAPON_READY && ent->client->Harpoon.State > HARPOON_STATE_FLY)
	{
		if (!(ent->client->buttons & BUTTON_ATTACK))
			ent->client->ps.gunframe = 8;
		else
			ent->client->ps.gunframe = 4;
		ent->client->weaponstate = WEAPON_FIRING;
	}
}
