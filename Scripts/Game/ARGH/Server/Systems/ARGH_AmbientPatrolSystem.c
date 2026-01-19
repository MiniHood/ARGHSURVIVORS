modded class SCR_AmbientPatrolSystem {
//	override protected void OnInit()
//	{
//		// No need to run updates unless some patrols are actually registered
//		if (m_aPatrols.IsEmpty())
//			Enable(false);
//
//		RefreshPlayerList();
//
//		// Calculate (de)spawn distance based on view distance, have it squared for faster distance calculation
//		int fractionOfVD = GetGame().GetViewDistance() * 0.3;
//		m_iSpawnDistanceSq = fractionOfVD * fractionOfVD;
//		m_iSpawnDistanceSq = Math.Min(SPAWN_RADIUS_MAX_SQ, m_iSpawnDistanceSq);
//		m_iSpawnDistanceSq = Math.Max(SPAWN_RADIUS_MIN_SQ, m_iSpawnDistanceSq);
//		m_iDespawnDistanceSq = m_iSpawnDistanceSq + DESPAWN_RADIUS_DIFF_SQ;
//
//		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
//		
//		PrintFormat("SCR_AmbientPatrolSystem | m_iSpawnDistanceSq: %1", m_iSpawnDistanceSq);
//		PrintFormat("SCR_AmbientPatrolSystem | m_iDespawnDistanceSq: %1", m_iDespawnDistanceSq);
//
//		if (!gameMode)
//			return;
//
//		gameMode.GetOnPlayerSpawned().Insert(OnPlayerSpawnedOrDeleted);
//		gameMode.GetOnPlayerKilled().Insert(OnPlayerKilled);
//		gameMode.GetOnPlayerDeleted().Insert(OnPlayerSpawnedOrDeleted);
//		gameMode.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
//	}
	
//	protected void Bacon_ProcessSpawnpoint(int spawnpointIndex)
//	{
//		Bacon_622120A5448725E3_AmbientPatrolSpawnPointComponent spawnpoint = Bacon_622120A5448725E3_AmbientPatrolSpawnPointComponent.Cast(m_aPatrols[spawnpointIndex]);
//		if (spawnpoint) {
//			spawnpoint.HealthCheck();
//			
//		}
//
//		ProcessSpawnpoint(spawnpointIndex);
//	}
	
	override protected void ProcessSpawnpoint(int spawnpointIndex)
	{
		Bacon_622120A5448725E3_AmbientPatrolSpawnPointComponent spawnpoint = Bacon_622120A5448725E3_AmbientPatrolSpawnPointComponent.Cast(m_aPatrols[spawnpointIndex]);
		if (!spawnpoint) {
			super.ProcessSpawnpoint(spawnpointIndex);
			return;
		}

		spawnpoint.HealthCheck();
		ChimeraWorld world = GetWorld();
		WorldTimestamp currentTime = world.GetServerTimestamp();
		if (spawnpoint.GetRespawnTimestamp().Greater(currentTime))
			return;

		bool playersNear;
		bool playersVeryNear;
		bool playersFar = true;
		vector location = spawnpoint.GetOwner().GetOrigin();
		int distance;

		// Define if any player is close enough to spawn or if all players are far enough to despawn
		foreach (IEntity player : m_aPlayers)
		{
			if (!player)
				continue;

			distance = vector.DistanceSq(player.GetOrigin(), location);

			if (distance > m_iDespawnDistanceSq)
				continue;

			playersFar = false;

			if (distance > m_iSpawnDistanceSq)
				continue;

			playersNear = true;

			if (distance > SPAWN_RADIUS_BLOCK_SQ)
				continue;

			playersVeryNear = true;
			break;
		}

		bool isAIOverLimit;
		AIWorld aiWorld = GetGame().GetAIWorld();

		if (aiWorld)
		{
			int maxChars = aiWorld.GetLimitOfActiveAIs();

			if (maxChars <= 0)
				isAIOverLimit = true;
			else
				isAIOverLimit = ((float)aiWorld.GetCurrentNumOfActiveAIs() / (float)maxChars) > spawnpoint.GetAILimitThreshold();
		}

		if (!isAIOverLimit && !playersVeryNear)
			spawnpoint.SetIsPaused(false);
		
		if (playersNear && !playersVeryNear && !spawnpoint.GetIsSpawned())
		{
			spawnpoint.SpawnPatrol();
			return;
		}

		if (playersNear && !spawnpoint.GetIsPaused() && !spawnpoint.IsGroupActive())
		{
			// Do not spawn the patrol if the AI threshold setting has been reached
			if (isAIOverLimit)
			{
				spawnpoint.SetIsPaused(true);	// Make sure a patrol is not spawned too close to players when AI limit suddenly allows spawning of this group
				return;
			}
			
			spawnpoint.ActivateGroup();
			return;
		}

		// Delay is used so dying players don't see the despawn happen
		if (spawnpoint.GetIsSpawned() && playersFar && spawnpoint.IsGroupActive())
		{
			WorldTimestamp despawnT = spawnpoint.GetDespawnTimestamp();

			if (despawnT == 0)
				spawnpoint.SetDespawnTimestamp(currentTime.PlusMilliseconds(DESPAWN_TIMEOUT));
			else if (currentTime.Greater(despawnT))
				spawnpoint.DespawnPatrol();
				// spawnpoint.DeactivateGroup();
		}
		else
		{
			spawnpoint.SetDespawnTimestamp(null);
		}
	}
}
