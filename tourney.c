#include "g_local.h"

char maplist[1024][32];

char admincode[16];		 // the admincode
char default_map[32];    // default settings
char default_teams[16];
char default_dmflags[16];
char default_password[16];
char default_timelimit[16];
char default_fraglimit[16];
char default_anti_spawncamp[16];
char default_dm_realmode[16];
char map_list_filename[32];
int allow_map_voting;
int wait_for_players;
int disable_admin_voting;
int pregameframes;
int num_maps;

int fixed_gametype;
int enable_password;
int keep_admin_status;
int default_random_map;
int disable_curse;
int unlimited_curse;
int enable_killerhealth;

char MOTD[20][80];
int num_MOTD_lines;

player_t playerlist[64];

char ban_name_filename[32];
ban_t ban_name[100];
int num_ban_names;
char ban_ip_filename[32];
ban_t ban_ip[100];
int num_ban_ips;

char rconx_file[32];
ban_t rconx_pass[100];
int num_rconx_pass;

int vote_set[9];
int num_vote_set;
qboolean vote_nopic[9];
int vote_winner;

int manual_tagset = 0;
int team_startcash[3];


edict_t *GetAdmin()
{
	int		i;
	edict_t	*doot;

	for_each_player(doot, i)
	{
		if (doot->client->pers.admin > NOT_ADMIN)
			return doot;
	}
	return NULL;
}

//==============================================================
//
// Papa - This file contains all the functions that control the 
//        modes a server may be in.
//
//===============================================================

// Places the server in prematch mode
void MatchSetup ()
{
	edict_t		*self;
	int			i;

	if (level.modeset == MATCHSETUP && !level.intermissiontime)
		return;

	level.intermissiontime = 0;
	level.modeset = MATCHSETUP;
	level.startframe = level.framenum;

	for_each_player (self,i)
	{
		self->client->showscores = SCOREBOARD;
		self->client->resp.scoreboard_frame = 0;
//CDEATH
		Harpoon_Player_Reset(self);
//END CDEATH
		ClientBeginDeathmatch( self );
	}

	gi.bprintf(PRINT_HIGH, "The server is now ready to setup a match.\n");
	gi.bprintf(PRINT_HIGH, "Players need to join the correct teams.\n");
	
}

// completely resets the server including map
qboolean ResetServer (qboolean ifneeded)
{
	char command[64];

	// refresh config
	LoadConfig();

	// these things don't need a restart
	if (default_dmflags[0])
		gi.cvar_set("dmflags", default_dmflags);
	if (default_timelimit[0])
		gi.cvar_set("timelimit", default_timelimit);
	if (default_fraglimit[0])
		gi.cvar_set("fraglimit", default_fraglimit);
	if (default_anti_spawncamp[0])
		gi.cvar_set("anti_spawncamp", default_anti_spawncamp);
	gi.cvar_set("password", default_password);
	gi.cvar_set("no_spec", "0");
	if (manual_tagset)
	{
		manual_tagset = 0;
		setTeamName(1, "Dragons");
		setTeamName(2, "Nikki's Boyz");
	}

	// these do need a restart
	if (ifneeded
		&& !(default_teams[0] && strcmp(teams->string, default_teams))
		&& !(default_dm_realmode[0] && strcmp(dm_realmode->string, default_dm_realmode)))
		return false;

	if (default_teams[0])
		gi.cvar_set("teams", default_teams);
	if (default_dm_realmode[0])
		gi.cvar_set("dm_realmode", default_dm_realmode);
	gi.cvar_set("cheats", "0");
	if (default_random_map && num_maps)
		Com_sprintf (command, sizeof(command), "map \"%s\"\n", maplist[rand() % num_maps]);
	else
		Com_sprintf (command, sizeof(command), "map \"%s\"\n", default_map[0] ? default_map : level.mapname);
	gi.AddCommandString (command);
	return true;
}

// start the match
void MatchStart()
{
	int			i;
	edict_t		*ent;
		
	level.intermissiontime = 0;
	level.player_num = 0;
	level.modeset = MATCHCOUNT;
	level.startframe = level.framenum;
	gi.bprintf (PRINT_HIGH,"COUNTDOWN STARTED. 15 SECONDS TO MATCH.\n");
//CDEATH
	Power_Initialise_Level();
//END CDEATH
	team_cash[1] = team_startcash[0];
	team_cash[2] = team_startcash[1];
	team_cash[3] = team_startcash[2];
	team_startcash[2] = team_startcash[1] = team_startcash[0] = 0;
	UpdateScore();

	G_ClearUp ();

	for_each_player (ent, i)
	{
		Killapin_ResetPlayer(ent); //killapin

		ClientBeginDeathmatch( ent );
	}

	gi.WriteByte(svc_stufftext);
	gi.WriteString("play world/cypress3\n");
	gi.multicast(vec3_origin, MULTICAST_ALL);

	// turn back on any sounds that were turned off during intermission
	for (i=0; i<globals.num_edicts; i++)
	{
		ent = g_edicts + i;
		if (ent->inuse && (ent->spawnflags&1) && ent->classname && !strcmp(ent->classname, "target_speaker"))
			ent->s.sound = ent->noise_index;
	}
}

// Starts the match
void Start_Match ()
{
	edict_t		*self;
	int			i;

	//CDEATH
	Power_Initialise_Level();
	//END CDEATH

	level.modeset = MATCH;
	level.startframe = level.framenum;
	level.invincible_boss = level.framenum;
	level.is_spawn = false;
	for_each_player(self, i)
	{
		gi.centerprintf(self, "The match has begun!");
		self->client->resp.is_spawn = false;
	}
	gi.dprintf("The match has begun!\n");

	gi.WriteByte(svc_stufftext);
	gi.WriteString("play killapin/buzz\n");
	gi.multicast(vec3_origin, MULTICAST_ALL);
}

// Starts a public game
void Start_Pub ()
{
	edict_t		*self;
	int			i;

//CDEATH
	Power_Initialise_Level();
//END CDEATH

	level.modeset = PUBLIC;
	level.startframe = level.framenum;
	level.invincible_boss = level.framenum;
	level.is_spawn = false;
	for_each_player(self,i)
	{
		gi.centerprintf(self, "Let the fun begin!");
		self->client->resp.is_spawn = false;
	}
	gi.dprintf("Let the fun begin!\n");

	gi.WriteByte(svc_stufftext);
	gi.WriteString("play killapin/buzz\n");
	gi.multicast(vec3_origin, MULTICAST_ALL);
}

// end of the match
void MatchEnd ()
{
	level.modeset = MATCHSETUP;
	level.startframe = level.framenum;

	BeginIntermission(NULL);
}

// restart the server if its empty in matchsetup mode
void CheckIdleMatchSetup ()
{
	int		count = 0;
	int		i;
	edict_t	*doot;

	for_each_player (doot, i)
		count++;
	if (count == 0)
		ResetServer (false);
}

// 15 countdown before matches
void CheckStartMatch ()
{
	int framenum = level.framenum - level.startframe;
	if (framenum >= 150)
	{
		Start_Match ();
		return;
	}

	if (level.framenum - level.startframe == 150 - 41)
	{
		gi.WriteByte(svc_stufftext);
		gi.WriteString("play killapin/gstart\n");
		gi.multicast(vec3_origin, MULTICAST_ALL);
	}
}

// countdown before public game starts
void CheckStartPub ()
{
#ifndef HYPODEBUG
	if (level.framenum >= level.pregameframes)
#endif
	{
		Start_Pub ();
		return;
	}

	// start sooner if all players are ready
	if (level.framenum < level.pregameframes - 41)
	{
		int i, ready = 0;
		for (i=1; i<=maxclients->value; i++)
		{
			edict_t	*ent = g_edicts + i;
			if (ent->client->pers.connected && !ent->client->pers.spectator)
			{
				if (!ent->inuse || !ent->client->resp.ready)
					return;
				ready++;
			}
		}
		if (ready)
			level.pregameframes = level.framenum + 41;
	}

	if (level.pregameframes - level.framenum == 41)
	{
		gi.WriteByte(svc_stufftext);
		gi.WriteString("play killapin/gstart\n");
		gi.multicast(vec3_origin, MULTICAST_ALL);
	}
}

void getTeamTags();

// check if time,frag,cash limits have been reached in a match
void CheckEndMatch ()
{
	int		i;
	int		count = 0;
	edict_t	*doot;

    // snap - team tags
	if (!manual_tagset && (level.framenum % 100) == 0)
		getTeamTags();

	for_each_player (doot, i)
		count++;
	if (count == 0)
	{
		ResetServer (false);
		return;
	}

	if ((int)fraglimit->value)
	{
		if (team_cash[1] >= (int)fraglimit->value || team_cash[2] >= (int)fraglimit->value || team_cash[3] >= (int)fraglimit->value)
		{
			gi.bprintf (PRINT_HIGH, "Score limit hit.\n");
			MatchEnd ();
			return;
		}
	}

	if ((int)timelimit->value)
	{
		if (level.framenum > (level.startframe + ((int)timelimit->value) * 600 - 1))
		{
			gi.bprintf (PRINT_HIGH, "Timelimit hit.\n");
			MatchEnd();
			return;
		}
		if (((level.framenum - level.startframe) % 10 == 0) && (level.framenum > (level.startframe + (((int)timelimit->value  * 600) - 155))))  
		{
			gi.bprintf(PRINT_HIGH, "The match will end in %d seconds\n", (((int)timelimit->value * 600) + level.startframe - level.framenum) / 10);
			return;
		}
		if (((level.framenum - level.startframe) % 600 == 0) && (level.framenum > (level.startframe + (((int)timelimit->value * 600) - 3000))))  
		{
			gi.bprintf(PRINT_HIGH, "The match will end in %d minutes\n", (((int)timelimit->value * 600) + level.startframe - level.framenum) / 600);
			return;
		}
		if ((((int)timelimit->value * 600) - (level.framenum - level.startframe)) % 3000 == 0)
			gi.bprintf(PRINT_HIGH, "The match will end in %d minutes\n", (((int)timelimit->value * 600) + level.startframe - level.framenum) / 600);
	}
}

// check the timelimit for voting next level/start next map
void CheckEndVoteTime ()
{
	int		i, count = 0, votes[9];

	if (vote_winner)
	{
		if (level.framenum > (level.startframe + 310))
			level.exitintermission = true;
		return;
	}

	memset (&votes, 0, sizeof(votes));

	for (i=1; i<=maxclients->value; i++)
	{
		edict_t	*ent = g_edicts + i;
		if (ent->client->pers.connected)
		{
			count++;
			if (ent->inuse)
				votes[ent->client->mapvote]++;
		}
	}
	if (!count && level.framenum > (level.lastactive + 30))
	{
		if (ResetServer(true))
			return;
		if (wait_for_players)
		{
			level.intermissiontime = -1; // prevent endgame sounds
			level.startframe = level.framenum;
			level.player_num = 0;
			if (team_cash[1] || team_cash[2])
			{
				team_cash[2] = team_cash[1] = 0;
				UpdateScore();
			}
			level.lastactive = -1;
			gi.dprintf("Waiting for players\n");
			UpdateTime();
			if (kpded2) // enable kpded2's idle mode for reduced CPU usage while waiting for players (automatically disabled when players join)
				gi.cvar_forceset("g_idle", "1");
		}
	}

	if (level.framenum == (level.startframe + 300))
	{
		vote_winner = 1;
		for (i = 2; i <= num_vote_set; i++)
		{
			if (votes[i] > votes[vote_winner])
				vote_winner = i;
		}
		level.changemap = maplist[vote_set[vote_winner]];
	}
}

void CheckEndTime()
{
	if (level.framenum > (level.startframe + 200))
		level.exitintermission = true;
}

// check the timelimit for an admin or map vote
void CheckVote()
{
	if (level.framenum == (level.voteframe + 600))
		gi.bprintf(PRINT_HIGH, "30 seconds left for voting\n");
	if (level.framenum >= (level.voteframe + 900))
	{
		switch (level.voteset)
		{
			case VOTE_ON_ADMIN:
				gi.bprintf(PRINT_HIGH, "The request for admin has failed\n");
				break;
			case VOTE_ON_MAP:
				gi.bprintf(PRINT_HIGH, "The request for a map change has failed\n");
				break;
		}
		level.voteset = NO_VOTES;
	}
}

int	CheckNameBan (char *name)
{
	char n[64];
	int i;

	strncpy(n, name, sizeof(n)-1);
	kp_strlwr(n);
	for (i=0; i<num_ban_names; i++)
	{
		if (strstr(n, ban_name[i].value))
			return true;
	}
	return false;
}

int	CheckPlayerBan (char *userinfo)
{
	char	*value;
	int		i;

	if (num_ban_names)
	{
		value = Info_ValueForKey (userinfo, "name");
		if (CheckNameBan(value))
			return true;
	}

	if (num_ban_ips)
	{
		value = Info_ValueForKey (userinfo, "ip");
		for (i=0; i<num_ban_ips; i++)
			if (!strncmp(value, ban_ip[i].value, strlen(ban_ip[i].value)))
				return true;
	}

	return false;
}

void UpdateScore()
{
	char buf[24];
	sprintf(buf, teams->value == 2 ? "%d : %d" : "%d : %d : %d", team_cash[1], team_cash[2], team_cash[3]);
	gi.cvar_set(SCORENAME, buf);
}

void UpdateTime()
{
	char buf[32] = " ";
	if (level.lastactive < 0)
		strcpy(buf, "waiting");
	else if (level.modeset == MATCHCOUNT)
	{
		int t =	((150 - (level.framenum - level.startframe)) / 10);
		sprintf(buf, "start in %d", t);
	}
	else if (level.modeset == PREGAME)
	{
		int t = ((350 -  level.framenum) / 10);
		sprintf(buf, "start in %d", t);
	}
	else if ((level.modeset == MATCH || level.modeset == PUBLIC))
	{
		if ((int)timelimit->value)
		{
			int t = ((((int)timelimit->value * 600) + level.startframe - level.framenum) / 10);
			if (t > 0)
				sprintf(buf, "%d:%02d", t / 60, t % 60);
		}
	}
	else if (level.intermissiontime)
		strcpy(buf, "intermission");
	gi.cvar_set(TIMENAME, buf);
}

// snap - team tags
void setTeamName (int team, char *name) // tical's original code :D
{ 
	static int team_alloc[3] = {0, 0, 0};

	if (!name || !*name)
		return;

	if (strcmp(name, team_names[team])) 
	{
		if (team_alloc[team])
			gi.TagFree(team_names[team]);

		team_names[team] = strcpy(gi.TagMalloc(strlen(name) + 1, TAG_GAME), name);
		team_alloc[team] = 1;
	}
}

// snap - new function.
void getTeamTags()
{

	int			i;
	edict_t		*doot;
	char		names[3][64][16];
	int			namesLen[3] = { 0, 0, 0 };
	char		teamTag[3][12];
	int			teamTagsFound[3]= { FALSE, FALSE, FALSE };

	for_each_player (doot, i)
	{
		int team = doot->client->pers.team;
		if (team && namesLen[team-1] < 64)
		{
			strcpy(names[team-1][namesLen[team-1]++], doot->client->pers.netname);
		}
	}


	for(i=0; i<3; i++)
	{
		int	j;
		for (j=0; j<namesLen[i] && teamTagsFound[i] == FALSE; j++)
		{
			int	k;
			for (k=0; k<namesLen[i] && j != k && teamTagsFound[i] == FALSE; k++)
			{
				char theTag[12];
				int	theTagNum = 0;
				int	y = 0;
				char s = names[i][j][y];
					
				while (s != '\0' && theTagNum == 0)
				{
					int	z = 0;
					char t = names[i][k][z];
					while (t != '\0')
					{
						if (s == t && s != ' ')
						{ // we have a matched char
							int	posY = y+1;
							int	posZ = z+1;
							char ss = names[i][j][posY];
							char tt = names[i][k][posZ];

							while (ss != '\0' && tt != '\0' && ss == tt && theTagNum < 11)
							{
								if (theTagNum == 0)
								{ // we have two consecutive matches, this is a tag
									theTag[theTagNum++] = s;
									theTag[theTagNum++] = ss;
								}
								else
								{
									theTag[theTagNum++] = ss;
								}
								ss = names[i][j][++posY];
								tt = names[i][k][++posZ];
							}
						}
						t = names[i][k][++z];
					}
					s = names[i][j][++y];
				}
				if (theTagNum > 0)
				{
					int	e;
					float howmany = 0.0;
					float percentage; 
					theTag[theTagNum] = '\0';
					
					for (e=0; e<namesLen[i]; e++)
					{
						if (strstr(names[i][e], theTag) != NULL)
						{
							howmany += 1.0;
						}
					}
					percentage = howmany/(float)namesLen[i]*100.0;
					if (percentage > 75.0)
					{
						strcpy(teamTag[i], theTag);
						teamTagsFound[i] = TRUE;
					}	
				}
			}
		}
	}

	setTeamName(1, teamTagsFound[0] == TRUE ? teamTag[0] : "Dragons");
	setTeamName(2, teamTagsFound[1] == TRUE ? teamTag[1] : "Nikki's Boyz");
	setTeamName(3, teamTagsFound[2] == TRUE ? teamTag[2] : "Jokers");
}

// sets up the vote options for the next map
void SetupMapVote()
{
	int		i, j, k;
	int		unique;
	int		selection;
	int		old_vote_set[9];

	vote_winner = 0;

	// find current map index
	vote_set[0] = -1;
	for (i = 0; i < num_maps; i++)
	{
		if (Q_stricmp(maplist[i], level.mapname) == 0)
		{
			vote_set[0] = i;
			break;
		}
	}

	if (num_maps < 9) // less than 9 maps found, just display them all
	{
		i = vote_set[0];
		for (j = 1; j <= num_maps; j++)
		{
			i++;
			if (i == num_maps)
				i = 0;
			vote_set[j] = i;
		}
		num_vote_set = num_maps;
	}
	else
	{
		memcpy(old_vote_set, vote_set, sizeof(old_vote_set));

		for (i = 1; i < 9; i++)
		{
			do
			{
				selection = rand() % num_maps;
				// reduce the chances of maps that were in the previous list
				for (k = 1; k <= num_vote_set; k++)
					if (selection == old_vote_set[k])
					{
						selection = rand() % num_maps;
						break;
					}
				unique = true;
				for (k = 0; k < i; k++)
					if (selection == vote_set[k])
					{
						unique = false;
						break;
					}
			}
			while (!unique);
			vote_set[i] = selection;
		}
		num_vote_set = 8;
	}
}
