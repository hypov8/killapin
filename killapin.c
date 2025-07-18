#include "g_local.h"
#include "m_player.h"
/*
todo
crowbar env light?
hud radar. show team boss+minions. also ctrl points
harpoon color

code commented/expanded for killa

*/

//killapin
cvar_t	*bosshp;
cvar_t	*bossbest; //best player as boss
cvar_t	*bossonce; //let each player be boss once
cvar_t	*bossonly; //only consider a boss for spawpoint distance/locations
cvar_t	*showspawns; //show dm locations

cvar_t	*bosstimetotal;
cvar_t	*bosstimebonus;

#define BOSS_ALIVE_TIME_TOTAL   45 //if boss dies after this time, they get to respawn as boss
#define BOSS_ALIVE_TIME_BONUS   15 //time in seconds if the boss is live and will recieve points.
#define BOSS_ALIVE_BONUS_POINTS  1 //score to add if alive	 
#define BOSS_START_HP          250 //BOSS START hp

#define CHAT_COLOR_RED    (1 << 2) //RED
#define CHAT_COLOR_GREEN  (2 << 2) //GREEN
#define CHAT_COLOR_YELLOW (3 << 2)
#define CHAT_COLOR_PURPLE (4 << 2)
#define CHAT_COLOR_PINK   (5 << 2)
#define CHAT_COLOR_AQUA   (6 << 2)



void SP_info_player_show(edict_t *self, int skin)
{
	if (!showspawns->value)
		return;

	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 48);

	self->s.skinnum = skin;
	self->model = "models/bot/spawn.md2";
	self->s.modelindex = gi.modelindex(self->model);
	self->s.scale = 0;
	self->s.renderfx2 = RF2_NOSHADOW;
	self->s.renderfx2 |= RF2_SURF_ALPHA;
	self->s.renderfx = RF_FULLBRIGHT; // || RF_TRANSLUCENT;
	gi.linkentity (self);
}

//also used for match setup
void Killapin_ResetPlayer(edict_t *ent)
{
	//reset player scores
	ent->client->resp.time = 0;
	ent->client->resp.scoreboard_frame = 0;
	ent->client->pers.bagcash = 0;
	ent->client->resp.deposited = 0;
	ent->client->resp.score = 0;
	ent->client->pers.currentcash = 0;
	ent->client->resp.acchit = 0;
	ent->client->resp.accshot = 0;

	memset(ent->client->resp.fav, 0, sizeof(ent->client->resp.fav));
	if (ent->client->pers.spectator == PLAYING)
		ent->client->showscores = NO_SCOREBOARD;

	//killapin specific
	ent->client->resp.boss_time = 0;
	ent->client->resp.wep_level = 0; //reset weapons. todo
	ent->client->resp.hasBeenBoss = 0;
}

void Killapin_ResetRound()
{
	int i;
	edict_t *player;

	if (level.modeset >= MATCHSETUP)
	{
		MatchStart();
		return;
	}

	Start_Pub(); //local to tourney.c

	team_cash[1] = 0; //reset team scores
	team_cash[2] = 0;
	team_cash[3] = 0;
	UpdateScore();

	G_ClearUp ();

	//reset player scores
	for_each_player(player, i)
	{
		Killapin_ResetPlayer(player);

		if (player->health > 0) //alive
			player->health = player->client->pers.max_health; 
	}
}

//respawn health
void Killapin_SetBossMaxHealth(gclient_t *boss)
{
	if (boss->resp.is_boss)
	{
		boss->pers.max_health = (int)bosshp->value; //max hp
		boss->pers.health = (int)bosshp->value; //start hp
	}
}

//damage multiplyer
void Killapin_AdjustDamage(edict_t *attacker, edict_t *target, float *dmg, int mod)
{
	if (mod == MOD_CROWBAR && 
		attacker->client && attacker->client->resp.is_boss && //boss attacker
		target->client && !target->client->resp.is_boss)      //minion enemy
	{
		*dmg *= 2;
	}
}

//disable protection between boss/boss attacks
void Killapin_Add_NoDamageProtection(edict_t *attacker, edict_t *target, int *dflags)
{
	if (attacker->client && attacker->client->resp.is_boss && //boss attacker
		target->client && target->client->resp.is_boss)        //boss enemy
	{
		*dflags |= DAMAGE_NO_PROTECTION;
	}
}

static qboolean Killapin_IsBoBMode_Active()
{
	//Bob Timer active
	if (level.invincible_boss > level.framenum)
	{
		//check if countdown has ended
		if ((level.framenum + BOSS_TIME_TOTAL) > level.invincible_boss) 
			return true;
	}

	return false;
}

void Killapin_sendChatMsg(edict_t *player, char *msg, int color)
{
	int chatColor = PRINT_CHAT;

	//patched. add color
	if (player->client->pers.patched >= 7)
		chatColor |= color;

	gi.cprintf(player, chatColor, "%s\n", msg);
}

void Killapin_SetTeamScore_PlayerDied(edict_t *self, edict_t *attacker)
{
	int *score_deadPlyr = &team_cash[self->client->pers.team];
	int *score_enemy = &team_cash[attacker->client->pers.team];
	int diedAsBoss = self->client->resp.is_boss;
	int atackerIsBoss = attacker->client->resp.is_boss;
	int *attackerScore = &attacker->client->resp.deposited;

	//player died was a boss
	if (diedAsBoss)
	{
		if (atackerIsBoss)
		{
			//boss died while BoB mode enabled
			if (Killapin_IsBoBMode_Active())
			{
				*score_enemy += 25; //attacker team gets +25?
				Killapin_sendChatMsg(attacker, "Killing a Boss scored your team 25 points!", CHAT_COLOR_GREEN);
				*attackerScore += 25; //add score for killer. used to decide next boss
			}
			else
			{
				//boss died in normal mode
				*score_deadPlyr -= 10; //subtract from own team scores
				Killapin_sendChatMsg(self, "Dying cost your team 10 points!", CHAT_COLOR_RED);
				*attackerScore += 15; //add score for killer. used to decide next boss
			}
		}
		else //attacker is minion. (no BvB mode)
		{
			//boss died from minion
			*score_deadPlyr -= 2; //subtract from own team scores
			*score_enemy += 2; //attacker team gets +2
			*attackerScore += 5; //add score for killer. used to decide next boss
		}
	}
	else
	{ 
		//non boss death
		*score_deadPlyr -= 1; //subtract from own team scores
		*score_enemy += 1; //attacker team gets +1
		*attackerScore += 1; //add score for killer. used to decide next boss
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

static edict_t *Killapin_FindTeamPlayerRand(int team, qboolean checkPrevBoss)
{
	int i, playerID;
	int bossOnce = (int)bossonce->value;
	int maxPlayers = (int)maxclients->value;
	edict_t *player;

	// randomize first boss
	for (i = 0; i < maxPlayers; i++)
	{
		player = g_edicts + 1 + i;
		if (player->inuse && player->client->pers.team == team)
			playerID = i;
	}
	playerID = rand() % (playerID + 1);

	for (i = 0; i < maxPlayers; i++)
	{
		if (++playerID > maxPlayers)
			playerID = 1; //loop back to start of player list
		player = g_edicts + playerID;

		if (!player->inuse || player->client->pers.team != team)
			continue;

		//conditional rand boss
		if (checkPrevBoss)
		{
			//BoB mode. this lets a rand player be chosen if a boss dies in BoB mode
			if (Killapin_IsBoBMode_Active())
			{
				//dont pic prev boss
				if (player->client->resp.boss_time)
					continue;
			}
			else //non BoB mode
			{
				//ignore previous boss
				if (bossOnce)
				{
					if (player->client->resp.hasBeenBoss)
						continue; //skip player
				}
				else
					continue; //skip all players
			}
		}

		//valid next player
		player->client->resp.boss_time = 0;
		return player;
	}

	//invalid
	return NULL;
}

static edict_t *Killapin_FindTeamPlayerBest(int team)
{
	int i, bestIsBoss = 0;
	int kills = -99999, points = -999999, time = 0;
	edict_t *player, *old_boss = NULL, *most_kills = NULL;
	gclient_t *cl;

	for_each_player(player, i)
	{
		cl = player->client;

		//search for players on same team
		if (cl->pers.team == team)
		{
			//respawn same boss. alive for more then x seconds
			if (cl->resp.boss_time > (int)bosstimetotal->value)
			{
				old_boss = player;
				cl->resp.boss_time = 0;
				break;
			}

			//simple select best player, usng hit points.
			if (cl->resp.deposited > points ||
				(cl->resp.deposited == points && !bestIsBoss && cl->resp.score > kills)) //compare kills if not boss
			{
				//highest kills/lowest deaths
				kills = cl->resp.score;
				points = cl->resp.deposited;
				time = cl->resp.deposited; //todo
				most_kills = player;

				//prevent minion with same kills becomming boss
				bestIsBoss = (cl->resp.boss_time) ? 1: 0; 
			}
			//reset bosstimer
			cl->resp.boss_time = 0;
		}
	}

	//found best player
	if (old_boss)
		return old_boss;

	else if (most_kills)
		return most_kills;

	//invalid
	return NULL;
}

edict_t *Killapin_NewTeamBoss(int team)
{
	//level has never spawned a boss for this team. or rand cvar
	if (!level.next_spawn[team - 1] || !bossbest->value)
	{
		//random pick a new boss.
		return Killapin_FindTeamPlayerRand(team, FALSE);
	}
	else 
	{
		edict_t * boss = Killapin_FindTeamPlayerRand(team, TRUE);

		//let every player be boss once. or BoB mode and not the prev boss.
		if (boss) //cvar?
			return boss;

		//pick next/best player as boss
		return Killapin_FindTeamPlayerBest(team);	
	}
}

void CopyToBodyQue(edict_t *ent);
void Killapin_KillTeam(int team)
{
	int i;
	edict_t *doot;
	edict_t *boss = Killapin_GetTeamBoss(team);
	gi.bprintf(PRINT_HIGH, 
		boss ? "The %s boss has fallen!\n" : "The %s boss fled!\n", team_names[team]);
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
		Com_sprintf(doot->client->resp.message, sizeof(doot->client->resp.message) - 1, 
			boss ? "The %s boss has fallen!" : "The %s boss fled!", team_names[team]);
		doot->client->resp.message_frame = level.framenum + 30;
		if (!doot->client->showscores)
			doot->client->resp.scoreboard_frame = 0;
	}
	level.next_spawn[team - 1] = level.framenum + 30; // respawn team in 3s
}

void Killapin_ShowShellColors(edict_t *self)
{
	//boss. 
	if (self->client->resp.is_boss)
	{
		//BoB mode
		//if (level.invincible_boss > level.framenum 
		///	&& (level.invincible_boss -BOSS_TIME_TOTAL) < level.framenum)
		if (Killapin_IsBoBMode_Active())
		{	
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
			//non BoB mode
		}
	}
	else
	{ 
		//non boss
	}

}

//this is run every server frame (10 fps)
void Killapin_GiveItems(edict_t *ent, gclient_t *client)
{
	int isBoB_Mode = Killapin_IsBoBMode_Active();
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

		//reset players timer
		*refill_time = 0;
	}
}


//hypov8 cleanup scoreboard code
int HUD_AppendMessage(char *baseStr, char *addStr, int *stringlength, int maxLength)
{
	int sLen = strlen(addStr);

	if (sLen > 0)
	{
		if (*stringlength + sLen < maxLength)
		{
			strcpy(baseStr + *stringlength, addStr);
			*stringlength += sLen;
		}
		else
		{
			gi.dprintf("scoreboard overflowed");
			return 0; // failed/overflowed
		}
	}

	return 1;
}

#define RADAR_RAD 54 //radious (mid point) pic is 64x. resized to 54 ingame...
#define RADAR_DIAM RADAR_RAD*2
#define RADAR_PIC_WIDTH 64 //pic is 64x. resized to 54 ingame...
#define RADAR_SHIFT_X 10
#define RADAR_SHIFT_Y 30 //down below chat
#define ROUND_2_INT(f) ((int)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))

void Killapin_Build_RadarMessage(edict_t *ent, char *string, int *stringlength, int maxLen)
{
	char     entry[1024], *color;
	float    vecLen;
	int      i, newX, newY, len, count = 0;
	edict_t *src;
	vec3_t   vFlat;
	vec3_t   newVec, normal = {0,0,-1}, pos, norm;
	int      team = ent->client->pers.team;

	if (ent->client->pers.spectator != PLAYING)
	{
		return;
	}

	//count players
	for_each_player(src, i)
	{
		if (src->client->pers.team == team
			&& src->client->pers.spectator == PLAYING)
		{
			count++;
		}
	}
	//dont show if only 1 player. todo: not needed for ctrl points
	if (count < 2)
		return;

	Com_sprintf(entry, sizeof(entry),
		"xl %i yt %i picn /pics/rad2.tga ",
		RADAR_SHIFT_X, RADAR_SHIFT_Y); 
	HUD_AppendMessage(string, entry, stringlength, maxLen);

	//todo ctrl points

	for_each_player(src, i)
	{
		if (src->client->pers.team == team
			&& src->client->pers.spectator == PLAYING)
		{
			if (!src)
				continue;
			if (src->health <= 0)
				continue;
			if (src->client && src->client->pers.spectator != PLAYING)
				continue;
			if (ent == src)
				continue;

			VectorCopy(ent->s.origin, vFlat);
			vFlat[2] = src->s.origin[2];
			VectorSubtract(vFlat, src->s.origin, vFlat);
			len = VectorLength (vFlat);

			RotatePointAroundVector( newVec, normal, vFlat, ent->s.angles[1]);

			VectorCopy(newVec, norm);
			vecLen = VectorNormalize(norm);
			vecLen *= 0.11f;
			if (vecLen > RADAR_RAD+2)
				vecLen = RADAR_RAD+2;
			VectorScale(norm, vecLen, pos);

			newX = ROUND_2_INT(pos[1]);
			newY = ROUND_2_INT(pos[0]);

			//move to centre
			newX += RADAR_RAD;
			newY += RADAR_RAD;

			if (src->client->resp.is_boss)
				newY += 2; //shift star down
			else
				newY -= 4; //shift dot up

			//shift into position
			newX += RADAR_SHIFT_X;
			newY += RADAR_SHIFT_Y;

			switch (src->client->pers.team)
			{
				case 1:  color = "900"; break; //red
				case 2:  color = "990"; break; //yellow
				case 3:  color = "309"; break; //purple/blue
				default: color = "999"; break; //white
			}
			if (src->client->resp.is_boss)
			{
				Com_sprintf(entry, sizeof(entry),
					"xl %i yt %i dmstr %s \"%s\" ", newX, newY, color, "*");
			}
			else
			{
				Com_sprintf(entry, sizeof(entry),
					"xl %i yt %i dmstr 999 \"%s\" ", newX, newY, ".");
			}
			//}
			HUD_AppendMessage(string, entry, stringlength, maxLen);
		}
	}
}

static void Killapin_CheckBoB_Mode()
{
	int i;
	edict_t *player;
	// enable invincible boss mode. rand
	if (level.framenum - level.invincible_boss >= 1200 && !(rand() & 127))
	{	
		//add additional time for countdown
		level.invincible_boss = level.framenum + BOSS_TIME_TOTAL + BOSS_TIME_BEGIN;// 542
		gi.WriteByte(svc_stufftext);
		gi.WriteString("play killapin/bstart\n");
		gi.multicast(vec3_origin, MULTICAST_ALL);
		for_each_player(player, i)
		{
			Killapin_sendChatMsg(player, "Boss timer is starting!", CHAT_COLOR_GREEN);
		}
	}

	//end boss mode
	if (level.framenum == level.invincible_boss - 1)
	{
		gi.WriteByte(svc_stufftext);
		gi.WriteString("play killapin/bend\n");
		gi.multicast(vec3_origin, MULTICAST_ALL);
		for_each_player(player, i)
		{
			Killapin_sendChatMsg(player, "Boss timer ended!", CHAT_COLOR_RED);
		}
	}

	//begin BoB mode. countdown warning finished
	if (level.framenum == (level.invincible_boss - BOSS_TIME_TOTAL))
	{
		for_each_player (player, i)
		{
			strcpy(player->client->resp.message, "BOSS TIME HAS STARTED!");
			player->client->resp.message_frame = level.framenum + 30;
			if (!player->client->showscores)
				player->client->resp.scoreboard_frame = 0;
			
			//reset boss time
			if (player->client->resp.is_boss)
				player->client->resp.boss_time = 1; //prevents respawning as boss again
			else
				player->client->resp.boss_time = 0;

			Killapin_sendChatMsg(player, "Boss immunity!", CHAT_COLOR_GREEN);
		}
	}
}

static void Killapin_Radar()
{
		int i;
		edict_t *player;

	//update radar.. non very efficent
	for_each_player(player, i)
	{
		if (player->client->showscores == NO_SCOREBOARD && player->client->pers.spectator == PLAYING && level.framenum %10 == 0)
			player->client->resp.scoreboard_frame = 0;
	}
}

int Killapin_AdjustSpawnpoint(gclient_t *client, float *playerDistance)
{
	if (client->resp.is_boss)
	{
		// give greater priority to distance from other bosses
		*playerDistance *= 0.16; //make boss seem closer
	}
	else if (bossonly->value /* && !client->resp.is_boss */)	//not boss
	{
		//skip minions in range checking
		return 0;
	}

	return 1;
}

static void Killapin_SpawnPlayers()
{
	int			i, team;
	edict_t		*boss, *player;

	// respawn a player from each team
	for (team = 1; team <= (int)teams->value; team++)
	{
		if (level.framenum >= level.next_spawn[team - 1])
		{
			boss = Killapin_GetTeamBoss(team);
			if (!boss)
			{
				boss = Killapin_NewTeamBoss(team);
				if (boss)
				{
					boss->client->resp.hasBeenBoss = 1; //mark boss
					respawn(boss);
				}
				continue;
			}
			for_each_player (player, i)
			{
				if (player->client->pers.team == team 
					&& (player->deadflag || !player->solid) 
					&& level.time >= player->client->respawn_time)
				{
					respawn(player);
					break;
				}
			}
		}
	}
}

static void Killapin_UpdateCounters()
{
	if (level.framenum % 10 == 0)
	{
		int i, aliveTime = (int)bosstimebonus->value;
		edict_t *player;

		//BoB mode. dont add time/bonus
		if (level.invincible_boss > level.framenum)
			return;

		for_each_player(player, i)
		{
			if (player->client->pers.spectator == PLAYING)
			{
				if (player->client->resp.is_boss)
				{
					player->client->resp.boss_time += 1; //add 1 second to boss

					//give score to boss that lives past 15 seconds
					if (player->client->resp.boss_time % aliveTime == 0)
						team_cash[player->client->pers.team] += BOSS_ALIVE_BONUS_POINTS;
				}
			}
		}
	}
}

/*
check for 2 teams with 2 members. 
restart game when first detected
*/
static void Killapin_CheckValidTeams()
{
	int i;
	static int validTeamCount = 0;
	static int restartFrame = 0;

	//count player while spawning
	if (level.modeset == PUBLIC || level.modeset == MATCH)
	{
		if (validTeamCount >= 0 && validTeamCount < (int)teams->value) //check for max teams
		{
			int teamCounts[4] = {0, 0, 0, 0}; 
			int ready[3] = {0, 0, 0}, readyCount;
			edict_t *player;

			for_each_player(player, i)
			{
				if (player->client->resp.is_boss)
					teamCounts[player->client->pers.team] |= 1; //is boss
				else
				{
					teamCounts[player->client->pers.team] |= 2; //is minion
					//todo count+cvar
				}
			}
			ready[0] = (teamCounts[1] & 3) == 3; //team1 has boss+minion?
			ready[1] = (teamCounts[2] & 3) == 3; //team2 has boss+minion?
			ready[2] = (teamCounts[3] & 3) == 3; //team3 has boss+minion?
			readyCount = ready[0] + ready[1] + ready[2];

			if (readyCount > validTeamCount)  //atleast 2 teams valid (2+ players)
			{
				if (level.framenum < level.startframe + 30)
				{
					validTeamCount = readyCount;
					return; //fresh game, with players. dont do anything
				}
				//restart round
				validTeamCount = -readyCount;

				//countdown. 4 seconds
				restartFrame = level.framenum + 41;

				//reset bosstime
				level.invincible_boss = 0;

				//countdown
				gi.WriteByte(svc_stufftext);
				gi.WriteString("play killapin/gstart\n");
				gi.multicast(vec3_origin, MULTICAST_ALL);
			}
		}
		else if (validTeamCount < 0)
		{
			//countdown hit?
			if (level.framenum > restartFrame)
			{
				Killapin_ResetRound();
				validTeamCount = abs(validTeamCount); //disable untill map change or more teams fill up
			}
		}
	}
	else
	{
		//reset game
		validTeamCount = 0;
		restartFrame = 0;
	}
}

void Killapin_RunFrame()
{
	Killapin_CheckValidTeams();

	if (level.modeset == PUBLIC || level.modeset == MATCH)
	{
		Killapin_CheckBoB_Mode();
		Killapin_SpawnPlayers();
		Killapin_UpdateCounters();
		Killapin_Radar();
	}
	else if (level.intermissiontime && level.time < level.intermissiontime + 20)
	{
		if (level.framenum == level.startframe + 10)
		{
			gi.WriteByte(svc_stufftext);
			gi.WriteString("play killapin/gend\n");
			gi.multicast(vec3_origin, MULTICAST_ALL);
		}
		else if (!((level.framenum - level.startframe - 25) % 150))
		{
			gi.WriteByte(svc_stufftext);
			gi.WriteString("play world/cypress4\n");
			gi.multicast(vec3_origin, MULTICAST_ALL);
		}
	}
}

void Killapin_Init()
{
	//new admin command "resetround". resets game/player stats

	bosshp = gi.cvar( "bosshp", va("%d", BOSS_START_HP), 0); //default spawn health. 250
	bossbest = gi.cvar( "bossbest", "1", 0); //best player as boss (0=random selected boss)
	bossonce = gi.cvar( "bossonce", "1", 0); //let each player become boss once. used with 'bossbest=1'
	bossonly = gi.cvar( "bossonly", "1", 0); //only consider a boss for spawpoint(0= considered minions to)
	showspawns = gi.cvar( "showspawns", "0", 0); //show dm spawn locations


	//if boss dies after this time, they get to respawn as boss
	bosstimetotal = gi.cvar( "bosstimetotal", va("%d", BOSS_ALIVE_TIME_TOTAL), 0); //defaul 45
	//time in seconds if the boss is live and will recieve points.
	bosstimebonus = gi.cvar( "bosstimebonus", va("%d", BOSS_ALIVE_TIME_BONUS), 0); //default 15
}
