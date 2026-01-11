modded class SCR_AmbientVehicleSystem : GameSystem
{
	private bool m_bDebugSpawnpointLogged = false;
	private bool m_bDebugInitLogged = false;
	#ifdef WORKBENCH
	private int m_iRegisterCount = 0;
	#endif

	private static void ARGH_FillVehicleFuel(Vehicle vehicle)
	{
		if (!vehicle)
			return;

		SCR_FuelManagerComponent fuelMgr = SCR_FuelManagerComponent.Cast(vehicle.FindComponent(SCR_FuelManagerComponent));
		if (!fuelMgr)
			return;

		array<BaseFuelNode> nodes = {};
		fuelMgr.GetFuelNodesList(nodes);
		foreach (BaseFuelNode node : nodes)
		{
			if (!node)
				continue;

			float maxFuel = node.GetMaxFuel();
			if (maxFuel > 0.0)
				node.SetFuel(maxFuel);
		}
	}
	
	override void OnInit()
	{
		super.OnInit();

#ifdef WORKBENCH
		if (!m_bDebugInitLogged)
		{
			Print("[ARGH_AMBIENT] SCR_AmbientVehicleSystem OnInit");
			m_bDebugInitLogged = true;
		}
#endif
	}

	override void RegisterSpawnpoint(notnull SCR_AmbientVehicleSpawnPointComponent spawnpoint)
	{
		super.RegisterSpawnpoint(spawnpoint);

	#ifdef WORKBENCH
		m_iRegisterCount++;
		if (m_iRegisterCount <= 5)
			Print("[ARGH_AMBIENT] RegisterSpawnpoint: " + spawnpoint);
		else if (m_iRegisterCount == 6)
			Print("[ARGH_AMBIENT] RegisterSpawnpoint: more...");
	#endif
	}

	override void UnregisterSpawnpoint(notnull SCR_AmbientVehicleSpawnPointComponent spawnpoint)
	{
		super.UnregisterSpawnpoint(spawnpoint);
	}

	private static void ARGH_FullHealVehicle(Vehicle vehicle)
	{
		if (!vehicle)
			return;

		SCR_VehicleDamageManagerComponent dmgMgr = SCR_VehicleDamageManagerComponent.Cast(vehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		if (!dmgMgr)
			return;

		dmgMgr.FullHeal(true);
	}

	private static void ARGH_FullHealVehicleDelayed(Vehicle vehicle)
	{
		if (!vehicle || !GetGame())
			return;

		GetGame().GetCallqueue().CallLater(ARGH_FullHealVehicle, 500, false, vehicle);
	}
	
	private static void ARGH_FillVehicleFuelDelayed(Vehicle vehicle)
	{
		if (!vehicle || !GetGame())
			return;

		GetGame().GetCallqueue().CallLater(ARGH_FillVehicleFuel, 2000, false, vehicle);
	}

	override protected void ProcessSpawnpoint(int spawnpointIndex)
	{
		// Guard: skip invalid lists/indices.
		if (!m_aSpawnpoints || !m_aSpawnpoints.IsIndexValid(spawnpointIndex))
			return;

		if (!m_bDebugSpawnpointLogged)
		{
			Print("[ARGH_AMBIENT] ProcessSpawnpoint called. Spawnpoints=" + m_aSpawnpoints.Count());
			m_bDebugSpawnpointLogged = true;
		}

		// Resolve the spawnpoint and ignore depleted ones.
		SCR_AmbientVehicleSpawnPointComponent spawnpoint = m_aSpawnpoints[spawnpointIndex];
		if (!spawnpoint || spawnpoint.GetIsDepleted())
			return;

		// Read current vehicle + timing data.
		Vehicle spawnedVeh = spawnpoint.GetSpawnedVehicle();
		ChimeraWorld world = GetWorld();
		WorldTimestamp currentTime = world.GetServerTimestamp();
		int respawnSeconds = spawnpoint.GetRespawnPeriod();
		if (respawnSeconds == 0)
			respawnSeconds = 3600; // Unique override: treat disabled respawns as 1h cooldown.

		// If a non-respawning vehicle vanished, retire the spawnpoint.
		if (!spawnedVeh && spawnpoint.GetIsSpawnProcessed() && respawnSeconds <= 0)
		{
			spawnpoint.SetIsDepleted(true);
			return;
		}

		// When no vehicle is present, validate respawn timing.
		if (!spawnedVeh)
		{
			WorldTimestamp respawnTimestamp = spawnpoint.GetRespawnTimestamp();

			// Active respawn timer: do nothing.
			if (respawnTimestamp.Greater(currentTime))
				return;

			if (spawnpoint.GetIsFirstSpawnDone())
			{
				// If the vehicle was removed, start the respawn timer.
				if (respawnTimestamp == 0 && respawnSeconds > 0)
				{
					spawnpoint.SetRespawnTimestamp(currentTime.PlusSeconds(respawnSeconds));
					return;
				}
			}
		}

		// Cache spawnpoint position for distance checks.
		vector location = spawnpoint.GetOwner().GetOrigin();

		// If the vehicle drove away, detach and optionally deplete.
		if (spawnedVeh && vector.DistanceXZ(spawnedVeh.GetOrigin(), location) > PARKED_THRESHOLD)
		{
			spawnpoint.RemoveInteractionHandlers(spawnedVeh);

			// Non-respawning spawnpoints are consumed once the vehicle leaves.
			if (spawnpoint.GetRespawnPeriod() <= 0)
				spawnpoint.SetIsDepleted(true);

			return;
		}

		// Scan player proximity to decide spawn/despawn.
		bool playersNear = false;
		bool playersFar = true;
		int distance;

		// Determine if any player is close enough to spawn and if all are far enough to despawn.
		foreach (IEntity player : m_aPlayers)
		{
			if (!player)
				continue;

			distance = vector.DistanceSq(player.GetOrigin(), location);

			if (distance < m_iDespawnDistanceSq)
			{
				playersFar = false;

				if (distance < m_iSpawnDistanceSq)
				{
					playersNear = true;
					break;
				}
			}
		}

		// Spawn logic: only when empty and players are close.
		if (!spawnedVeh && playersNear)
		{
			ARGH_SCR_AmbientFlyingVehicleSpawnPointComponent flyingSpawnpoint = ARGH_SCR_AmbientFlyingVehicleSpawnPointComponent.Cast(spawnpoint);
			bool isFlying = (flyingSpawnpoint != null);

			ARGH_AmbientVehicleSpawnConfig cfg = null;
			ARGH_AmbientFlyingVehicleSpawnConfig cfgFlying = null;

			if (isFlying)
				cfgFlying = ARGH_SCR_AmbientFlyingVehicleSpawnPointComponent.GetAmbientFlyingVehicleSpawnConfig();
			else
				cfg = SCR_AmbientVehicleSpawnPointComponent.GetAmbientVehicleSpawnConfig();

			float areaRadius = 2500.0;
			int maxVehiclesInArea = 5;
			float spawnChance = 0.369;
			int longCooldownMultiplier = 10;
			if (isFlying && cfgFlying)
			{
				areaRadius = cfgFlying.m_fAreaRadius;
				maxVehiclesInArea = cfgFlying.m_iMaxVehiclesInArea;
				spawnChance = cfgFlying.m_fSpawnChance;
				longCooldownMultiplier = cfgFlying.m_iLongCooldownMultiplier;
			}
			else if (!isFlying && cfg)
			{
				areaRadius = cfg.m_fAreaRadius;
				maxVehiclesInArea = cfg.m_iMaxVehiclesInArea;
				spawnChance = cfg.m_fSpawnChance;
				longCooldownMultiplier = cfg.m_iLongCooldownMultiplier;
			}

			if (areaRadius > 0.0 && maxVehiclesInArea > 0)
			{
				const float areaRadiusSq = areaRadius * areaRadius;
				int nearbyVehicleCount = 0;

				foreach (SCR_AmbientVehicleSpawnPointComponent otherSpawnpoint : m_aSpawnpoints)
				{
					if (!otherSpawnpoint)
						continue;

					if (isFlying)
					{
						if (!ARGH_SCR_AmbientFlyingVehicleSpawnPointComponent.Cast(otherSpawnpoint))
							continue;
					}
					else
					{
						if (ARGH_SCR_AmbientFlyingVehicleSpawnPointComponent.Cast(otherSpawnpoint))
							continue;
					}

					Vehicle otherVehicle = otherSpawnpoint.GetSpawnedVehicle();
					if (!otherVehicle)
						continue;

					if (vector.DistanceSq(otherVehicle.GetOrigin(), location) <= areaRadiusSq)
					{
						nearbyVehicleCount++;
						if (nearbyVehicleCount >= maxVehiclesInArea)
							break;
					}
				}

				if (nearbyVehicleCount >= maxVehiclesInArea)
				{
					spawnpoint.SetRespawnTimestamp(currentTime.PlusSeconds(respawnSeconds));
					return;
				}
			}

			// Weighted roll that can skip and push a long cooldown.
			if (spawnChance < 1.0)
			{
				float clampedChance = Math.Clamp(spawnChance, 0.0, 1.0);
				int safeMultiplier = Math.Max(1, longCooldownMultiplier);
				if (Math.RandomFloat(0, 1) >= clampedChance)
				{
					spawnpoint.SetRespawnTimestamp(currentTime.PlusSeconds(respawnSeconds * safeMultiplier));
					return;
				}
			}
			
			Vehicle vehicle = spawnpoint.SpawnVehicle();

			if (vehicle && isFlying)
			{
				ARGH_FillVehicleFuel(vehicle);
				ARGH_FullHealVehicle(vehicle);
				ARGH_FullHealVehicleDelayed(vehicle);
				ARGH_FillVehicleFuelDelayed(vehicle);
			}

			if (vehicle && m_OnVehicleSpawned)
				m_OnVehicleSpawned.Invoke(spawnpoint, vehicle);

			return;
		}

		// Despawn logic: delay so players don't see vehicles pop out.
		if (spawnedVeh && playersFar && spawnpoint.IsDespawnAllowed())
		{
			const WorldTimestamp despawnT = spawnpoint.GetDespawnTimestamp();
			if (despawnT == 0)
			{
				spawnpoint.SetDespawnTimestamp(currentTime.PlusMilliseconds(DESPAWN_TIMEOUT + respawnSeconds * 10));
			}
			else if (currentTime.Greater(despawnT))
			{
				spawnpoint.DespawnVehicle();
			}
		}
		else
		{
			// Clear any pending despawn when players return or spawnpoint is empty.
			spawnpoint.SetDespawnTimestamp(null);
		}
	}
}
