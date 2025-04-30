#include "g_local.h"
#include "m_player.h"
/*
todo
crowbar env light?

code commented/expanded for killa

*/


void Killapin_SetTeamScore_PlayerDied(edict_t *self, edict_t *attacker)
{
	int *score_deadPlyr = &team_cash[self->client->pers.team] ;
	int *score_enemy = &team_cash[attacker->client->pers.team];

	//player died was a boss
	if (self->client->resp.is_boss)
	{
		//boss died while BoB mode enabled
		if (level.invincible_boss > level.framenum)
		{
			//*score_deadPlyr -= 10; //subtract from own team scores
			*score_enemy += 25; //attacker team gets +25?
		}
		else
		{
			//boss died in normal mode
			*score_deadPlyr -= 10; //subtract from own team scores
		}
	}
	else
	{ 
		//non boss death
		*score_deadPlyr -= 1; //subtract from own team scores
		*score_enemy += 1; //attacker team gets +1
	}

	UpdateScore();
}

edict_t *Killapin_GetTeamBoss(int team)
{
	if (level.team_boss[team - 1])
	{
		edict_t *boss = g_edicts + level.team_boss[team - 1];
		if (boss->inuse && boss->client->pers.team == team && boss->client->resp.is_boss)
			return boss;
	}
	return NULL;
}

edict_t *Killapin_NewTeamBoss(int team)
{
	int			i, n, best=0;
	edict_t		*doot;
	int kills = 0, deaths = 999999, time = 0;

	n = level.team_boss[team - 1];
	if (!n)
	{
		// randomize first boss
		for (i = 0; i < (int)maxclients->value; i++)
		{
			doot = g_edicts + 1 + i;
			if (doot->inuse && doot->client->pers.team == team)
				n = i;
		}
		n = rand() % (n + 1);

		for (i = 0; i < (int)maxclients->value; i++)
		{
			if (++n > (int)maxclients->value)
				n = 1;
			doot = g_edicts + n;
			if (doot->inuse && doot->client->pers.team == team)
				return doot;
		}
	}
	else
	{
		for (i = 0; i < (int)maxclients->value; i++)
		{
			doot = g_edicts + 1 + i;
			if (doot->inuse && doot->client->pers.team == team)
			{
				//simple select best player
				if (doot->client->resp.score > kills || 
					(doot->client->resp.score >= kills && doot->client->resp.deposited < deaths) /*|| 
					(doot->client->resp.score >= kills && doot->client->resp.bossTime > time)*/)
				{
					//highest kills/lowest deaths/longest time being boss
					kills = doot->client->resp.score;
					deaths = doot->client->resp.deposited;
					time = doot->client->resp.deposited; //todo
					best = i+1;
				}
			}
		}
		if (best)
			return g_edicts +  best;
	}

	return NULL;
}

void Killapin_KillTeam(int team)
{
	int i;
	edict_t *doot;
	edict_t *boss = Killapin_GetTeamBoss(team);
	gi.bprintf(PRINT_HIGH, boss ? "The %s boss has fallen!\n" : "The %s boss fled!\n", team_names[team]);
	for_each_player (doot, i)
	{
		if (doot->client->pers.team == team)
		{
			if (!doot->deadflag)
			{
				doot->health = 0; // no obituary
				player_die(doot, doot, doot, 0, vec3_origin, 0, 0);
			}
			if (boss && doot != boss && doot->client->chase_target != boss)
			{
				if (!(doot->svflags & SVF_NOCLIENT))
				{
					doot->flags |= FL_SPAWNED_BLOODPOOL; // prevent blood
					CopyToBodyQue(doot);
				}
				doot->movetype = MOVETYPE_NOCLIP;
				doot->solid = SOLID_NOT;
				doot->svflags |= SVF_NOCLIENT;
				VectorClear(doot->velocity);
				doot->client->chase_target = boss;
			}
			doot->client->resp.is_boss = false;
			doot->client->respawn_time = 0;
		}
		Com_sprintf(doot->client->resp.message, sizeof(doot->client->resp.message) - 1, boss ? "The %s boss has fallen!" : "The %s boss fled!", team_names[team]);
		doot->client->resp.message_frame = level.framenum + 30;
		if (!doot->client->showscores)
			doot->client->resp.scoreboard_frame = 0;
	}
	level.next_spawn[team - 1] = level.framenum + 30; // respawn team in 3s
}

void Killapin_ShowShellColors(edict_t *self)
{

	if (self->client->resp.is_boss)
	{
		//boss
		if (level.invincible_boss > level.framenum && (level.invincible_boss -500) < level.framenum)
		{	
			//BoB mode
			self->s.effects |= EF_COLOR_SHELL;
			switch (self->client->pers.team)
			{
				case 1: //team dragons
					self->s.renderfx |= RF_SHELL_RED;
					break;
				case 2: //team nikki
					self->s.renderfx |= (RF_SHELL_GREEN | RF_SHELL_RED);
					break;
				case 3:
				default: //team joker
					self->s.renderfx |= RF_SHELL_BLUE;
					break;
			}
		}
		else
		{
			//self->s.effects |= EF_COLOR_SHELL;
			//self->s.renderfx |= (RF_SHELL_RED|RF_SHELL_GREEN);
			//self->s.renderfx |= (RF_SHELL_GREEN | RF_SHELL_BLUE);
		}
	}
	else
	{ 
		//non boss
		//self->s.effects |= EF_COLOR_SHELL;
		//self->s.renderfx |= RF_SHELL_GREEN;
	}

}

void Killapin_GiveItems(edict_t *ent, gclient_t *client)
{
	int isBoB_Mode = (level.invincible_boss > level.framenum)? 1:0;
	int delay = 20; //delay before player recieves health/items
	int isBoss = client->resp.is_boss? 1 : 0;
	int isAttacking = (ent->client->buttons & BUTTON_ATTACK); //(client->weaponstate == WEAPON_FIRING)
	int *refill_time = &client->health_count;

	//BoB mode
	if (isBoB_Mode)
	{
		if (isBoss)
			delay = 10; //boss= 1.0 sec. (give boss more updates)
		else
			delay = 15; //minion = 1.5 sec
	}
	else //normal mode
	{
		if (isBoss)
			delay = 15; //boss = 1.5 sec. (give boss more updates)
		else
			delay = 20; //minion = 2.0 sec
	}

	//player is shooting, dont give items
	if (isAttacking)
	{
		*refill_time = 0; //dont give anything
	}
	else if (isBoss) //boss
	{
		*refill_time += 1; //always give to boss
	}
	else //minions
	{
		edict_t *boss_ent = Killapin_GetTeamBoss(client->pers.team);

		if (boss_ent)
		{	
			float dist = VectorDistance(ent->s.origin, boss_ent->s.origin);

			//is player within boss distance
			if (dist < 400)
				*refill_time += 1;
		}
		else
		{
			//missing boss? should not happen
			*refill_time = 0; //dont give anything
		}
	}


	//time reached. give items
	if (*refill_time > delay)
	{
		gitem_t *item;
		int sound = 0;

		//update non boss with items
		if (!isBoss)
		{
			if (Add_Ammo(ent, item = FindItem("Shells"), 4) && client->ammo_index == ITEM_INDEX(item))
				sound = gi.soundindex("world/pickups/ammo.wav");
			if (Add_Ammo(ent, item = FindItem("Bullets"), 25) && client->ammo_index == ITEM_INDEX(item))
				sound = gi.soundindex("world/pickups/ammo.wav");
			if (Add_Ammo(ent, item = FindItem("308cal"), 15) && client->ammo_index == ITEM_INDEX(item))
				sound = gi.soundindex("world/pickups/ammo.wav");
		}

		//give health (boss+minion)
		if (ent->health < ent->max_health)
		{
			ent->health += 15;
			if (ent->health > ent->max_health)
				ent->health = ent->max_health;
			sound = gi.soundindex("world/pickups/health.wav");
		}
		if (sound)
		{
			gi.WriteByte(11); // svc_sound
			gi.WriteByte(8); // SND_ENT
			gi.WriteByte(sound);
			gi.WriteShort(((ent - g_edicts) << 3) | CHAN_AUTO);
			gi.unicast(ent, true);
//			client->bonus_alpha = 0.25;
		}
		*refill_time = 0;
	}
}
