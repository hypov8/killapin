
// g_teamplay.c - teamplay oriented code

#include "g_local.h"


#define	CASH_ROLL			10
#define	CASH_BAG			25

#define	MAX_CASH_ITEMS		10		// never spawn more than this many cash items at once
int		num_cash_items;

char *team_names[] = {
	"(spectator)",
	"Dragons",
	"Nikki's Boyz",
	"Jokers",
	NULL
};


int	team_cash[4];	// cash per team, 0 is neutral so just ignore

//=====================================================================
// Entity spawn functions

/*QUAKED dm_cashspawn (0.5 0 1) (-16 -16 -16) (16 16 16)
Spawn location for cash during "Grab da Loot" games

  angle - direction to project cash upon spawning
  speed - speed of projection
  type	- "cashroll" or "cashbag" (more money, longer delay)
*/
void SP_dm_cashspawn( edict_t *self )
{
	return;
}

/*QUAKED dm_safebag (0.5 0 1) (-12 -12 -16) (12 12 12)
Bag that holds the money in the safe.

  style - team that this bag belongs to (1 or 2)
*/
void SP_dm_safebag( edict_t *self )
{
	G_FreeEdict( self );
	return;
}


/*QUAKED dm_props_banner (.5 0 1) (-4 -4 -4) (4 4 4)
Temp banner for teamplay

 style = team (1 / 2)
 scale = scale the size up/down (2 = double size)

model="models\props\temp\triangle\small.md2"
*/
void SP_dm_props_banner (edict_t *self)
{
//	vec3_t	end, bestnorm, bestend;
//	float bestdist;
//	int	x,y;
//	trace_t tr;

	if (!deathmatch_value || !teamplay->value)
	{	// remove
		G_FreeEdict (self);
		return;
	}
	
	if (!self->style)
	{
		gi.dprintf( "%s has invalid style (should be 1 or 2) at %s\n", self->classname, vtos(self->s.origin) );
		G_FreeEdict (self);
		return;
	}
/*
	// trace a line back, to get the wall, then go out
	{
		bestdist = 9999;

		for (x=-256; x<300; x+= 256)
		{
			VectorCopy( self->s.origin, end );
			end[0] = self->s.origin[0] + x;
			tr = gi.trace( self->s.origin, NULL, NULL, end, NULL, MASK_SOLID );
			if (tr.fraction < bestdist)
			{
				VectorCopy( tr.plane.normal, bestnorm );
				VectorCopy( tr.endpos, bestend );
				bestdist = tr.fraction;
			}
		}
		for (y=-256; y<300; y+= 256)
		{
			VectorCopy( self->s.origin, end );
			end[1] = self->s.origin[1] + y;
			tr = gi.trace( self->s.origin, NULL, NULL, end, NULL, MASK_SOLID );
			if (tr.fraction < bestdist)
			{
				VectorCopy( tr.plane.normal, bestnorm );
				VectorCopy( tr.endpos, bestend );
				bestdist = tr.fraction;
			}
		}

		vectoangles( bestnorm, self->s.angles );

		VectorMA( bestend, 40 * self->cast_info.scale, bestnorm, self->s.origin );
	}

*/
// Ridah, 1-jun-99, use flag models for now
#if 1
	{
		void think_flag (edict_t *self);

//		self->solid = SOLID_BBOX;
		self->movetype = MOVETYPE_NONE;

		if (self->style == 2)
		{
			self->model = "models/props/flag/flag1.md2";
		}
		else
		{
			self->model = "models/props/flag/flag3.md2";
		}

		self->s.modelindex = gi.modelindex (self->model);

		self->s.renderfx2 |= RF2_NOSHADOW;
		self->s.renderfx |= RF_MINLIGHT;

		if (!self->cast_info.scale)
			self->cast_info.scale = 1;

		self->s.scale = (self->cast_info.scale - 1);

//		VectorMA( bestend, 40 * self->cast_info.scale, bestnorm, self->s.origin );

		self->cast_info.scale *= 0.3;

		gi.linkentity (self);

		self->s.effects |= EF_ANIM_ALLFAST_NEW;
		self->s.renderfx2 |= RF2_MODULUS_FRAME;
		self->s.renderfx2 |= RDF_NOLERP;

// Disabled, doesn't animate much, and uses bandwidth
//		self->nextthink = level.time + FRAMETIME *2;
//		self->think = think_flag;
	}

#else // TRIANGULAR ROTATING ICONS

	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;

	self->s.skinnum = self->style - 1;

	self->s.renderfx2 |= RF2_NOSHADOW;	
	self->s.renderfx |= RF_MINLIGHT;

	if (!self->cast_info.scale)
		self->cast_info.scale = 1;

	self->s.scale = self->cast_info.scale - 1;

	self->s.modelindex = gi.modelindex ("models/props/temp/triangle/small.md2");

	gi.linkentity (self);

	{
		edict_t *arm;

		arm = G_Spawn();
		arm->solid = self->solid;
		arm->movetype = self->movetype;
		arm->s.renderfx2 |= RF2_NOSHADOW;
		arm->s.scale = self->s.scale;

		VectorCopy( self->s.origin, arm->s.origin );
		VectorCopy( self->s.angles, arm->s.angles );

		arm->s.modelindex = gi.modelindex ("models/props/temp/triangle/arm.md2");
		gi.linkentity (arm);
	}

	VectorCopy( self->s.angles, self->last_step_pos );
	VectorClear( self->move_angles );
#endif
}

// ....................................................................

void Teamplay_ValidateSkin( edict_t *self )
{
	// TODO: we need color coded skins, for now, just use any skin
}

extern void ClientUserinfoChanged (edict_t *ent, char *userinfo);

// Papa - made some changes to this function

qboolean Teamplay_ValidateJoinTeam( edict_t *self, int teamindex )
{
	// NOTE: this is called by each player on level change, as well as when a player issues a "join XXX" command

	// TODO: player limit per team? cvar?


	Teamplay_ValidateSkin( self );

//	InitClientPersistant (self->client);
	self->client->resp.boss_time = 0; //killapin reset best player

	self->client->pers.team = teamindex;
	self->client->pers.spectator = PLAYING;
	if ((level.modeset != MATCHSPAWN) && (level.modeset != PUBLICSPAWN))
	{
		gi.bprintf( PRINT_HIGH, "%s joined %s\n", self->client->pers.netname, team_names[teamindex] );
		if (level.modeset == PREGAME || level.modeset == MATCHSETUP)
		{
			self->client->showscores = SCOREBOARD;
			self->client->resp.scoreboard_frame = 0;
		}
	}

	if ((level.modeset == PUBLIC) || (level.modeset == MATCH) || (level.modeset == MATCHSPAWN) || (level.modeset == PUBLICSPAWN))
	{
/*
	// Validate skins
	{
		char *str[256];

		strcpy( str, self->client->pers.userinfo );
		ClientUserinfoChanged ( self, str );
	}
*/
/*		if (self->client->resp.enterframe != level.framenum)
		{
			PutClientInServer( self );	// find a new spawn point
		}*/
	}
	return true;
}

void Teamplay_AutoJoinTeam( edict_t *self )
{
	int	team_count[2];
	int	i;

	// count number of players on each team, assign the team with least players

	team_count[0] = 0;
	team_count[1] = 0;

	for (i=1; i<maxclients->value; i++)
	{
		if (g_edicts[i].client && g_edicts[i].client->pers.team)
			team_count[g_edicts[i].client->pers.team - 1]++;
	}

	if (team_count[0] > team_count[1])
		self->client->pers.team = 2;
	else
		self->client->pers.team = 1;
	self->client->pers.spectator = PLAYING;

	Teamplay_ValidateJoinTeam( self, self->client->pers.team );
}


void Teamplay_InitTeamplay (void)
{
	num_cash_items = 0;
	memset( team_cash, 0, sizeof(team_cash) );

//CDEATH
	Power_Initialise_Level();
//END CDEATH
	if (teamplay->value)
		UpdateScore();
}
