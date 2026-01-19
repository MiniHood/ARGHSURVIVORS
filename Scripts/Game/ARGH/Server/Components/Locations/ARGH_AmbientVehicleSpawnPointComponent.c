	// =============================================================
	// ARGH_MO_SCR_AmbientVehicleSpawnPointComponent.c
	//
	// UPDATED: scans both
	// - Prefabs/Vehicles/Wheeled/<Model>/<Variant>.et
	// - Prefabs/Vehicles/Core/<Variant>.et
	//
	// Core grouping: model name is prefix before first "_" (or whole filename if none)
	//   unpreza_Black -> model "unpreza"
	//   bug -> model "bug"
	//   Lunia_Gamma_Red -> model "Lunia"   (change to LastIndexOf("_") if you want "Lunia_Gamma")
	// =============================================================


	// ------------------------------------------------------------
	// CUSTOM EDITABLE ENTITY LABEL (shows in .et dropdown)
	// ------------------------------------------------------------
	modded enum EEditableEntityLabel
	{
		ARGH_AMBIENT_VEHICLES
	};

	// -----------------------------
	// Variant rule (leaf)
	// -----------------------------
	[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sDisplayName")]
	class ARGH_AmbientVehicleSpawnRule : Managed
	{
		[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Friendly name shown in the editor tree")]
		string m_sDisplayName;

		[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", desc: "Prefab resource to spawn.")]
		ResourceName m_rPrefab;

		[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Enable this prefab for ambient spawning.")]
		bool m_bEnabled;

		[Attribute(defvalue: "1.0", uiwidget: UIWidgets.EditBox, desc: "Spawn weight for this prefab (0 disables).")]
		float m_fSpawnWeight;
	}

	// -----------------------------
	// Group node (Vehicle Model)
	// -----------------------------
	[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sName")]
	class ARGH_AmbientVehicleGroup : Managed
	{
		[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Group name (e.g. ALL IN-GAME VEHICLES, BTR70, UAZ, M151A2...)")]
		string m_sName;

		[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Enable/disable this entire group (affects children too)")]
		bool m_bEnabled;

		// Leaf list used on MODEL groups
		[Attribute(defvalue: "{}", desc: "Vehicle variants in this group")]
		ref array<ref ARGH_AmbientVehicleSpawnRule> m_aVehicles;

		// Child groups used for nesting (Root -> Models)
		[Attribute(defvalue: "{}", desc: "Child groups")]
		ref array<ref ARGH_AmbientVehicleGroup> m_aChildren;
	}

	// -----------------------------
	// Catalog config (root)
	// -----------------------------
	[BaseContainerProps(configRoot: true)]
	class ARGH_AmbientVehicleCatalogConfig : Managed
	{
		// Grouped tree (preferred)
		[Attribute(defvalue: "{}", desc: "Grouped vehicle catalog (Root -> Models -> Variants)")]
		ref array<ref ARGH_AmbientVehicleGroup> m_aGroups;

		// Legacy fallback (so old files still load)
		[Attribute(defvalue: "{}", desc: "(Legacy) Flat vehicle catalog entries")]
		ref array<ref ARGH_AmbientVehicleSpawnRule> m_aVehicles;
	}

	// -----------------------------
	// Spawn config (root)
	// -----------------------------
	[BaseContainerProps(configRoot: true)]
	class ARGH_AmbientVehicleSpawnConfig : Managed
	{
		[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "conf", desc: "Optional vehicle catalog resource to use for ambient spawns.")]
		ResourceName m_rVehicleCatalog;

		[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Ignore spawnpoint labels and always use the catalog list.")]
		bool m_bRulesIgnoreSpawnpointLabels;

		[Attribute(defvalue: "0.369", uiwidget: UIWidgets.EditBox, desc: "Chance (0-1) to spawn when players are nearby.")]
		float m_fSpawnChance;

		[Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "Respawn cooldown multiplier when a spawn roll fails.")]
		int m_iLongCooldownMultiplier;

		[Attribute(defvalue: "2500", uiwidget: UIWidgets.EditBox, desc: "Area cap radius in meters (<=0 disables cap).")]
		float m_fAreaRadius;

		[Attribute(defvalue: "5", uiwidget: UIWidgets.EditBox, desc: "Max vehicles within area radius (<=0 disables cap).")]
		int m_iMaxVehiclesInArea;

		[Attribute(defvalue: "{}", desc: "Additional root paths to scan for vehicle prefabs.")]
		ref array<string> m_aCatalogRoots;

		[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Debug: log enabled/disabled groups/variants and selection decisions")]
		bool m_bDebugLogCatalog;
	}

	modded class SCR_AmbientVehicleSpawnPointComponent : ScriptComponent
	{
	#ifdef WORKBENCH
		private static int s_iSpawnpointInitCount = 0;
	#endif
		private bool m_bRegistered = false;
		private int m_iRegisterAttempts = 0;

		// Config resource path (Workbench-editable .conf)
		static const ResourceName ARGH_AMBIENT_VEHICLE_CONFIG = "{9825E69BE1F90D44}Configs/Systems/ARGH_AmbientVehicleSpawn.conf";
		private static ref ARGH_AmbientVehicleSpawnConfig s_AmbientVehicleSpawnConfig;
		private static bool s_bConfigLoaded = false;

		private static bool s_bDumpedCatalog = false;
		private static ResourceName s_CatalogConfigPath;
		private static ref ARGH_AmbientVehicleCatalogConfig s_AmbientVehicleCatalogConfig;

	#ifdef WORKBENCH
		static bool s_bDumpScheduled = false;
		static bool s_bRebuildRequested = false;

		static void ResetCatalogState()
		{
			s_bDumpScheduled = false;
			s_bRebuildRequested = false;
			s_bDumpedCatalog = false;
			s_AmbientVehicleCatalogConfig = null;
			s_CatalogConfigPath = ResourceName.Empty;
			s_AmbientVehicleSpawnConfig = null;
			s_bConfigLoaded = false;
		}
	#endif

		protected void TryRegisterAmbientSystem()
		{
			if (m_bRegistered)
				return;

			if (!Replication.IsServer() || !GetGame())
				return;

			ChimeraWorld world = GetGame().GetWorld();
			if (!world)
				return;

			SCR_AmbientVehicleSystem ambientSystem = SCR_AmbientVehicleSystem.Cast(world.FindSystem(SCR_AmbientVehicleSystem));
			if (ambientSystem)
			{
				ambientSystem.RegisterSpawnpoint(this);
				m_bRegistered = true;
				return;
			}

			m_iRegisterAttempts++;
			if (m_iRegisterAttempts <= 20)
				GetGame().GetCallqueue().CallLater(TryRegisterAmbientSystem, 250, false);
		#ifdef WORKBENCH
			else
				Print("[ARGH_AMBIENT] RegisterSpawnpoint timed out: " + GetOwner());
		#endif
		}

		override void OnPostInit(IEntity owner)
		{
			super.OnPostInit(owner);

		#ifdef WORKBENCH
			s_iSpawnpointInitCount++;
			if (s_iSpawnpointInitCount <= 5)
				Print("[ARGH_AMBIENT] Spawnpoint init: " + owner);
			else if (s_iSpawnpointInitCount == 6)
				Print("[ARGH_AMBIENT] Spawnpoint init: more...");
		#endif

			TryRegisterAmbientSystem();
		}

		protected override void OnDelete(IEntity owner)
		{
			super.OnDelete(owner);

			if (Replication.IsServer())
			{
				ChimeraWorld world = GetGame().GetWorld();
				if (world)
				{
					SCR_AmbientVehicleSystem ambientSystem = SCR_AmbientVehicleSystem.Cast(world.FindSystem(SCR_AmbientVehicleSystem));
					if (ambientSystem)
						ambientSystem.UnregisterSpawnpoint(this);
				}
			}
		}

		// ------------------------------------------------------------
		// Spawn config load
		// ------------------------------------------------------------
		static ARGH_AmbientVehicleSpawnConfig GetAmbientVehicleSpawnConfig()
		{
			if (s_bConfigLoaded && s_AmbientVehicleSpawnConfig)
				return s_AmbientVehicleSpawnConfig;

			Print("[ARGH_VEHICLE_CATALOG] Loading config from: " + ARGH_AMBIENT_VEHICLE_CONFIG);

			Resource res = Resource.Load(ARGH_AMBIENT_VEHICLE_CONFIG);
			if (res && res.IsValid())
			{
				BaseResourceObject resource = res.GetResource();
				if (resource)
				{
					BaseContainer bc = resource.ToBaseContainer();
					if (bc)
					{
						ARGH_AmbientVehicleSpawnConfig cfg = new ARGH_AmbientVehicleSpawnConfig();

						float tmpF;
						if (bc.Get("m_fSpawnChance", tmpF))
							cfg.m_fSpawnChance = tmpF;
						if (bc.Get("m_fAreaRadius", tmpF))
							cfg.m_fAreaRadius = tmpF;

						int tmpI;
						if (bc.Get("m_iLongCooldownMultiplier", tmpI))
							cfg.m_iLongCooldownMultiplier = tmpI;
						if (bc.Get("m_iMaxVehiclesInArea", tmpI))
							cfg.m_iMaxVehiclesInArea = tmpI;

						bool tmpB;
						if (bc.Get("m_bRulesIgnoreSpawnpointLabels", tmpB))
							cfg.m_bRulesIgnoreSpawnpointLabels = tmpB;
						if (bc.Get("m_bDebugLogCatalog", tmpB))
							cfg.m_bDebugLogCatalog = tmpB;

						ResourceName tmpR;
						if (bc.Get("m_rVehicleCatalog", tmpR))
							cfg.m_rVehicleCatalog = tmpR;

						array<string> roots = {};
						if (bc.Get("m_aCatalogRoots", roots))
							cfg.m_aCatalogRoots = roots;

						s_AmbientVehicleSpawnConfig = cfg;
						s_bConfigLoaded = true;

						Print("[ARGH_VEHICLE_CATALOG] Config loaded successfully");
						Print(string.Format("[ARGH_VEHICLE_CATALOG] Catalog path: %1", cfg.m_rVehicleCatalog));
						Print(string.Format("[ARGH_VEHICLE_CATALOG] Debug log: %1", cfg.m_bDebugLogCatalog));
					}
					else
					{
						Print("[ARGH_VEHICLE_CATALOG] Failed to get BaseContainer from resource", LogLevel.WARNING);
					}
				}
				else
				{
					Print("[ARGH_VEHICLE_CATALOG] Failed to get BaseResourceObject", LogLevel.WARNING);
				}
			}
			else
			{
				Print("[ARGH_VEHICLE_CATALOG] Failed to load config resource - using defaults", LogLevel.WARNING);
			}

			if (!s_AmbientVehicleSpawnConfig)
			{
				Print("[ARGH_VEHICLE_CATALOG] Creating default config");
				s_AmbientVehicleSpawnConfig = new ARGH_AmbientVehicleSpawnConfig();
				ApplySpawnConfigDefaults(s_AmbientVehicleSpawnConfig);
				s_bConfigLoaded = true;
			}

			return s_AmbientVehicleSpawnConfig;
		}

		private static void ApplySpawnConfigDefaults(ARGH_AmbientVehicleSpawnConfig cfg)
		{
			if (!cfg)
				return;

			cfg.m_bRulesIgnoreSpawnpointLabels = true;
			cfg.m_fSpawnChance = 0.369;
			cfg.m_iLongCooldownMultiplier = 10;
			cfg.m_fAreaRadius = 2500.0;
			cfg.m_iMaxVehiclesInArea = 5;
			cfg.m_bDebugLogCatalog = false;
			if (!cfg.m_aCatalogRoots)
				cfg.m_aCatalogRoots = {};

			Print("[ARGH_VEHICLE_CATALOG] Applied default config values");
		}

		// ------------------------------------------------------------
		// Normalize for matching
		// ------------------------------------------------------------
		private static string NormalizePrefabPath(string prefabPath)
		{
			if (prefabPath.IsEmpty())
				return prefabPath;

			int braceIndex = prefabPath.IndexOf("}");
			if (braceIndex >= 0)
				prefabPath = prefabPath.Substring(braceIndex + 1, prefabPath.Length() - braceIndex - 1);

			int prefabsIndex = prefabPath.IndexOf("Prefabs/");
			if (prefabsIndex < 0)
				prefabsIndex = prefabPath.IndexOf("Prefabs\\");
			if (prefabsIndex >= 0)
				prefabPath = prefabPath.Substring(prefabsIndex, prefabPath.Length() - prefabsIndex);

			return prefabPath;
		}

		// ------------------------------------------------------------
		// Model/variant helpers (UPDATED for Core)
		// ------------------------------------------------------------
		private static string ExtractVehicleModelFromNormalized(string normalizedPath)
		{
			string p = normalizedPath;
			p.Replace("\\", "/");

			string lower = p;
			lower.ToLower();

			// Wheeled: folder is model
			int wheeledIndex = lower.IndexOf("prefabs/vehicles/wheeled/");
			if (wheeledIndex >= 0)
			{
				int pathStart = wheeledIndex + "prefabs/vehicles/wheeled/".Length();
				string relativePath = p.Substring(pathStart, p.Length() - pathStart);

				array<string> parts = {};
				relativePath.Split("/", parts, false);
				if (parts.Count() < 2)
					return "UNKNOWN_MODEL";

				return parts[0];
			}

			// Core: filename prefix is model (before first "_"), or full name if none
			int coreIndex = lower.IndexOf("prefabs/vehicles/core/");
			if (coreIndex >= 0)
			{
				string fileNoExt = ExtractVariantNameFromNormalized(p);
				int us = fileNoExt.IndexOf("_");
				if (us > 0)
					return fileNoExt.Substring(0, us);

				return fileNoExt;
			}

			return "UNKNOWN_MODEL";
		}

		private static string ExtractVariantNameFromNormalized(string normalizedPath)
		{
			string p = normalizedPath;
			p.Replace("\\", "/");

			int slash = p.LastIndexOf("/");
			string file;

			if (slash >= 0)
				file = p.Substring(slash + 1, p.Length() - (slash + 1));
			else
				file = p;

			int dot = file.LastIndexOf(".");
			if (dot > 0)
				file = file.Substring(0, dot);

			return file;
		}

		// ------------------------------------------------------------
		// Debug helpers
		// ------------------------------------------------------------
		private static void CountGroupTree(array<ref ARGH_AmbientVehicleGroup> groups, bool parentEnabled,
			out int totalGroups, out int enabledGroups, out int totalVariants, out int enabledVariants)
		{
			if (!groups)
				return;

			foreach (ARGH_AmbientVehicleGroup g : groups)
			{
				if (!g)
					continue;

				totalGroups++;
				bool groupEnabled = parentEnabled && g.m_bEnabled;
				if (groupEnabled)
					enabledGroups++;

				if (g.m_aVehicles)
				{
					foreach (ARGH_AmbientVehicleSpawnRule r : g.m_aVehicles)
					{
						if (!r)
							continue;
						totalVariants++;
						if (groupEnabled && r.m_bEnabled && (r.m_fSpawnWeight > 0.0))
							enabledVariants++;
					}
				}

				if (g.m_aChildren)
					CountGroupTree(g.m_aChildren, groupEnabled, totalGroups, enabledGroups, totalVariants, enabledVariants);
			}
		}

		private static void DebugDumpGroupTree(array<ref ARGH_AmbientVehicleGroup> groups, bool parentEnabled, string indent, bool logVariants)
		{
			if (!groups)
				return;

			foreach (ARGH_AmbientVehicleGroup g : groups)
			{
				if (!g)
					continue;

				bool groupEnabled = parentEnabled && g.m_bEnabled;
				Print(string.Format("[ARGH_VEHICLE_CATALOG] %1Group: %2 | enabled=%3 (self=%4)", indent, g.m_sName, groupEnabled, g.m_bEnabled));

				if (logVariants && g.m_aVehicles)
				{
					foreach (ARGH_AmbientVehicleSpawnRule r : g.m_aVehicles)
					{
						if (!r)
							continue;
						Print(string.Format("[ARGH_VEHICLE_CATALOG] %1  Variant: %2 | enabled=%3 | weight=%4 | prefab=%5",
							indent,
							r.m_sDisplayName,
							(groupEnabled && r.m_bEnabled && r.m_fSpawnWeight > 0.0),
							r.m_fSpawnWeight,
							r.m_rPrefab));
					}
				}

				if (g.m_aChildren)
					DebugDumpGroupTree(g.m_aChildren, groupEnabled, indent + "  ", logVariants);
			}
		}

		private static void FlattenEnabledGroups(array<ref ARGH_AmbientVehicleGroup> groups, array<ref ARGH_AmbientVehicleSpawnRule> outRules, bool parentEnabled)
		{
			if (!groups || !outRules)
				return;

			foreach (ARGH_AmbientVehicleGroup g : groups)
			{
				if (!g)
					continue;

				bool enabled = parentEnabled && g.m_bEnabled;
				if (!enabled)
					continue;

				if (g.m_aVehicles)
				{
					foreach (ARGH_AmbientVehicleSpawnRule r : g.m_aVehicles)
					{
						if (!r)
							continue;
						outRules.Insert(r);
					}
				}

				if (g.m_aChildren)
					FlattenEnabledGroups(g.m_aChildren, outRules, enabled);
			}
		}

		// ------------------------------------------------------------
		// Catalog load
		// ------------------------------------------------------------
		private ARGH_AmbientVehicleCatalogConfig LoadAmbientVehicleCatalog(ResourceName path)
		{
			if (path.IsEmpty())
				return null;

			if (s_AmbientVehicleCatalogConfig && s_CatalogConfigPath == path)
				return s_AmbientVehicleCatalogConfig;

			s_CatalogConfigPath = path;
			s_AmbientVehicleCatalogConfig = null;

			Resource res = Resource.Load(path);
			if (!res)
				return null;

			BaseContainer bc = res.GetResource().ToBaseContainer();
			if (!bc)
				return null;

			ARGH_AmbientVehicleCatalogConfig cfg = new ARGH_AmbientVehicleCatalogConfig();

			cfg.m_aGroups = {};
			array<ref ARGH_AmbientVehicleGroup> groups = {};
			if (bc.Get("m_aGroups", groups))
				cfg.m_aGroups = groups;

			cfg.m_aVehicles = {};
			array<ref ARGH_AmbientVehicleSpawnRule> rules = {};
			if (bc.Get("m_aVehicles", rules))
				cfg.m_aVehicles = rules;

			s_AmbientVehicleCatalogConfig = cfg;
			return s_AmbientVehicleCatalogConfig;
		}

		private array<ref ARGH_AmbientVehicleSpawnRule> ResolvePrefabRules(ARGH_AmbientVehicleSpawnConfig cfg)
		{
			if (!cfg || cfg.m_rVehicleCatalog.IsEmpty())
				return null;

			ARGH_AmbientVehicleCatalogConfig catalog = LoadAmbientVehicleCatalog(cfg.m_rVehicleCatalog);
			if (!catalog)
				return null;

			if (catalog.m_aGroups && catalog.m_aGroups.Count() > 0)
			{
				if (cfg.m_bDebugLogCatalog)
				{
					int totalG, enabledG, totalV, enabledV;
					CountGroupTree(catalog.m_aGroups, true, totalG, enabledG, totalV, enabledV);
					Print(string.Format("[ARGH_VEHICLE_CATALOG] Catalog summary: groups=%1 enabledGroups=%2 variants=%3 enabledVariants=%4",
						totalG, enabledG, totalV, enabledV));
					DebugDumpGroupTree(catalog.m_aGroups, true, "", false);
				}

				array<ref ARGH_AmbientVehicleSpawnRule> flat = {};
				FlattenEnabledGroups(catalog.m_aGroups, flat, true);
				return flat;
			}

			if (catalog.m_aVehicles && catalog.m_aVehicles.Count() > 0)
				return catalog.m_aVehicles;

			return {};
		}

	#ifdef WORKBENCH
		// ------------------------------------------------------------
		// Rebuild triggers
		// ------------------------------------------------------------
		static void TriggerCatalogRebuild()
		{
			if (s_bRebuildRequested)
			{
				Print("[ARGH_VEHICLE_CATALOG] Rebuild already requested, skipping duplicate");
				return;
			}

			ResetCatalogState();
			s_bRebuildRequested = true;
			ScheduleCatalogDump(5);
		}

		static void ForceRebuildCatalog()
		{
			ResetCatalogState();
			s_bRebuildRequested = true;
			DumpAmbientVehicleCatalogOnce();
		}

		static void ScheduleCatalogDump(int retries = 5)
		{
			if (s_bDumpScheduled)
			{
				Print("[ARGH_VEHICLE_CATALOG] Dump already scheduled");
				return;
			}

			if (!GetGame())
			{
				Print("[ARGH_VEHICLE_CATALOG] GetGame() returned null, cannot schedule dump", LogLevel.WARNING);
				return;
			}

			s_bDumpScheduled = true;
			Print(string.Format("[ARGH_VEHICLE_CATALOG] Scheduling catalog dump with %1 retries", retries));
			GetGame().GetCallqueue().CallLater(AttemptCatalogDump, 1000, false, retries);
		}

		static void AttemptCatalogDump(int retries)
		{
			s_bDumpScheduled = false;
			Print(string.Format("[ARGH_VEHICLE_CATALOG] Attempting catalog dump (retries remaining: %1)", retries));
			DumpAmbientVehicleCatalogOnce();

			if (!s_bDumpedCatalog && retries > 0)
			{
				Print(string.Format("[ARGH_VEHICLE_CATALOG] Dump not completed, scheduling retry in 1s"));
				if (GetGame())
					GetGame().GetCallqueue().CallLater(AttemptCatalogDump, 1000, false, retries - 1);
			}
			else if (s_bDumpedCatalog)
			{
				Print("[ARGH_VEHICLE_CATALOG] Catalog dump completed successfully");
			}
			else
			{
				Print("[ARGH_VEHICLE_CATALOG] Catalog dump failed after all retries", LogLevel.ERROR);
			}
		}

		private static bool IsVehicleRegistryPath(ResourceName confName)
		{
			string pathLower = confName.GetPath();
			pathLower.Replace("\\", "/");
			pathLower.ToLower();

			if (pathLower.IndexOf("placeableentities") < 0)
				return false;

			if (pathLower.IndexOf("vehicle") < 0)
				return false;

			return true;
		}

		// ------------------------------------------------------------
		// UPDATED: accepts Wheeled and Core
		// ------------------------------------------------------------
		private static bool IsVehiclePrefabPath(string normalizedPathLower)
		{
			int vehiclesIndex = normalizedPathLower.IndexOf("prefabs/vehicles/");
			if (vehiclesIndex < 0)
				return false;

			if (normalizedPathLower.IndexOf("/vehparts/") >= 0) return false;
			if (normalizedPathLower.IndexOf("/lights/") >= 0) return false;
			if (normalizedPathLower.IndexOf("/dst/") >= 0) return false;
			if (normalizedPathLower.IndexOf("/doors/") >= 0) return false;
			if (normalizedPathLower.IndexOf("/anims/") >= 0) return false;
			if (normalizedPathLower.IndexOf("/probes/") >= 0) return false;

			// Wheeled: Prefabs/Vehicles/Wheeled/<VehicleFolder>/<VehiclePrefab>.et
			int wheeledIndex = normalizedPathLower.IndexOf("prefabs/vehicles/wheeled/");
			if (wheeledIndex >= 0)
			{
				int pathStart = wheeledIndex + "prefabs/vehicles/wheeled/".Length();
				string relativePath = normalizedPathLower.Substring(pathStart, normalizedPathLower.Length() - pathStart);
				if (relativePath.IsEmpty())
					return false;

				array<string> parts = {};
				relativePath.Split("/", parts, false);
				return (parts.Count() == 2);
			}

			// Core: Prefabs/Vehicles/Core/<VehiclePrefab>.et (direct child)
			int coreIndex = normalizedPathLower.IndexOf("prefabs/vehicles/core/");
			if (coreIndex >= 0)
			{
				int pathStart2 = coreIndex + "prefabs/vehicles/core/".Length();
				string relativePath2 = normalizedPathLower.Substring(pathStart2, normalizedPathLower.Length() - pathStart2);
				if (relativePath2.IsEmpty())
					return false;

				array<string> parts2 = {};
				relativePath2.Split("/", parts2, false);
				return (parts2.Count() == 1);
			}

			return false;
		}

		private static bool IsVehiclePrefabResource(ResourceName prefabName)
		{
			Resource prefabResource = Resource.Load(prefabName);
			if (!prefabResource || !prefabResource.IsValid())
				return false;

			IEntitySource prefabSource = prefabResource.GetResource().ToEntitySource();
			if (!prefabSource)
				return false;

			string className = prefabSource.GetClassName();
			if (className.IsEmpty())
				return false;

			typename prefabType = className.ToType();
			if (!prefabType)
				return false;

			return prefabType.IsInherited(Vehicle);
		}

		private static bool MatchesRootFilters(string normalizedPathLower, array<string> rootFilters)
		{
			if (!rootFilters || rootFilters.IsEmpty())
				return true;

			foreach (string root : rootFilters)
			{
				if (root.IsEmpty())
					continue;

				string normalizedRoot = root;
				normalizedRoot.Replace("\\", "/");
				normalizedRoot.ToLower();

				if (normalizedPathLower.IndexOf(normalizedRoot) >= 0)
					return true;
			}

			return false;
		}

		private static bool AddVehiclePrefab(map<string, ResourceName> uniqueResources, ResourceName prefabName, array<string> rootFilters)
		{
			if (!uniqueResources)
				return false;

			string normalized = NormalizePrefabPath(prefabName);
			if (normalized.IsEmpty())
				return false;

			string normalizedPath = normalized;
			normalizedPath.Replace("\\", "/");

			string normalizedPathLower = normalizedPath;
			normalizedPathLower.ToLower();

			if (!MatchesRootFilters(normalizedPathLower, rootFilters))
				return false;
			if (!IsVehiclePrefabPath(normalizedPathLower))
				return false;

			if (!IsVehiclePrefabResource(prefabName))
				return false;

			ResourceName existingName;
			if (uniqueResources.Find(normalizedPath, existingName))
				return false;

			uniqueResources.Insert(normalizedPath, prefabName);
			return true;
		}

		private static int CollectVehiclePrefabsFromPlaceableRegistries(ARGH_AmbientVehicleSpawnConfig cfg, map<string, ResourceName> uniqueResources)
		{
			if (!uniqueResources)
				return 0;

			Print("[ARGH_VEHICLE_CATALOG] Collecting vehicles from PlaceableEntities registries");

			array<ResourceName> registryConfs = {};
			SearchResourcesFilter filter = new SearchResourcesFilter();
			filter.fileExtensions = { "conf" };
			filter.rootPath = "Configs/Editor/PlaceableEntities";
			ResourceDatabase.SearchResources(filter, registryConfs.Insert);

			Print(string.Format("[ARGH_VEHICLE_CATALOG] Found %1 registry configs", registryConfs.Count()));

			int added = 0;
			foreach (ResourceName confName : registryConfs)
			{
				if (!IsVehicleRegistryPath(confName))
					continue;

				Resource res = Resource.Load(confName);
				if (!res || !res.IsValid())
					continue;

				BaseContainer bc = res.GetResource().ToBaseContainer();
				if (!bc)
					continue;

				// UPDATED: allow wheeled OR core
				ResourceName sourceDir;
				if (bc.Get("m_sSourceDirectory", sourceDir) && !sourceDir.IsEmpty())
				{
					string sourceNormalized = NormalizePrefabPath(sourceDir);
					string sourceLower = sourceNormalized;
					sourceLower.Replace("\\", "/");
					sourceLower.ToLower();

					if (sourceLower.IndexOf("prefabs/vehicles/wheeled") < 0 && sourceLower.IndexOf("prefabs/vehicles/core") < 0)
						continue;
				}

				array<ResourceName> prefabs = {};
				if (!bc.Get("m_Prefabs", prefabs))
					continue;

				foreach (ResourceName prefab : prefabs)
				{
					if (AddVehiclePrefab(uniqueResources, prefab, cfg.m_aCatalogRoots))
						added++;
				}
			}

			Print(string.Format("[ARGH_VEHICLE_CATALOG] Added %1 vehicles from registries", added));
			return added;
		}

		private static int CollectVehiclePrefabsFromAllEt(ARGH_AmbientVehicleSpawnConfig cfg, map<string, ResourceName> uniqueResources)
		{
			if (!uniqueResources)
				return 0;

			Print("[ARGH_VEHICLE_CATALOG] Collecting vehicles from .et files");

			array<ResourceName> resources = {};
			SearchResourcesFilter filter = new SearchResourcesFilter();
			filter.fileExtensions = { "et" };

			array<string> roots = cfg.m_aCatalogRoots;
			if (!roots || roots.IsEmpty())
			{
				// UPDATED: scan Wheeled + Core by default
				roots = { "Prefabs/Vehicles/Wheeled", "Prefabs/Vehicles/Core" };
				Print("[ARGH_VEHICLE_CATALOG] Using default root filters: Prefabs/Vehicles/Wheeled + Prefabs/Vehicles/Core");
			}
			else
			{
				Print(string.Format("[ARGH_VEHICLE_CATALOG] Using %1 custom root filters", roots.Count()));
			}

			filter.rootPath = string.Empty;
			ResourceDatabase.SearchResources(filter, resources.Insert);
			Print(string.Format("[ARGH_VEHICLE_CATALOG] Found %1 .et files to process", resources.Count()));

			int added = 0;
			foreach (ResourceName resName : resources)
			{
				if (AddVehiclePrefab(uniqueResources, resName, roots))
					added++;
			}

			Print(string.Format("[ARGH_VEHICLE_CATALOG] Added %1 vehicles from .et scan", added));
			return added;
		}

		// ------------------------------------------------------------
		// REBUILD: saves MODEL-GROUPED catalog
		// ------------------------------------------------------------
		private static void DumpAmbientVehicleCatalog(ARGH_AmbientVehicleSpawnConfig cfg)
		{
			if (!cfg)
			{
				Print("[ARGH_VEHICLE_CATALOG] No config provided to DumpAmbientVehicleCatalog", LogLevel.ERROR);
				return;
			}

			if (s_bDumpedCatalog)
			{
				Print("[ARGH_VEHICLE_CATALOG] Catalog already dumped this session");
				return;
			}

			Print("[ARGH_VEHICLE_CATALOG] ===== Starting catalog rebuild (MODEL-GROUPED) =====");

			ResourceName target = cfg.m_rVehicleCatalog;
			if (target.IsEmpty())
			{
				target = "{B8EF144730EA7CB3}Configs/EntityCatalog/ARGH_AmbientVehicles.conf";
				Print(string.Format("[ARGH_VEHICLE_CATALOG] No catalog path in config, using default: %1", target));
			}
			else
			{
				Print(string.Format("[ARGH_VEHICLE_CATALOG] Using catalog path from config: %1", target));
			}

			string targetPath = target.GetPath();
			Resource res = Resource.Load(target);
			if (!res || !res.IsValid())
			{
				Print(string.Format("[ARGH_VEHICLE_CATALOG] ERROR: Target resource missing or invalid (%1)", targetPath), LogLevel.ERROR);
				return;
			}

			BaseContainer bc = res.GetResource().ToBaseContainer();
			if (!bc)
			{
				Print(string.Format("[ARGH_VEHICLE_CATALOG] ERROR: Failed to read container: %1", targetPath), LogLevel.ERROR);
				return;
			}

			map<string, ResourceName> uniqueResources = new map<string, ResourceName>();
			int fromRegistries = CollectVehiclePrefabsFromPlaceableRegistries(cfg, uniqueResources);
			int fromEt = CollectVehiclePrefabsFromAllEt(cfg, uniqueResources);

			Print(string.Format("[ARGH_VEHICLE_CATALOG] Total unique vehicles collected: %1 (registries: %2, .et scan: %3)",
				uniqueResources.Count(), fromRegistries, fromEt));

			if (uniqueResources.Count() == 0)
			{
				Print("[ARGH_VEHICLE_CATALOG] ERROR: No vehicle prefabs found during rebuild", LogLevel.ERROR);
				return;
			}

			ARGH_AmbientVehicleGroup root = new ARGH_AmbientVehicleGroup();
			root.m_sName = "ALL IN-GAME VEHICLES";
			root.m_bEnabled = true;
			root.m_aChildren = {};
			root.m_aVehicles = {};

			ref map<string, ref ARGH_AmbientVehicleGroup> modelGroups = new map<string, ref ARGH_AmbientVehicleGroup>();

			foreach (string normalizedKey, ResourceName resName : uniqueResources)
			{
				string normalized = normalizedKey;
				normalized.Replace("\\", "/");

				string modelName = ExtractVehicleModelFromNormalized(normalized);
				string variantName = ExtractVariantNameFromNormalized(normalized);
				string display = string.Format("%1  [%2]", variantName, normalized);

				ref ARGH_AmbientVehicleGroup modelGroup;
				if (!modelGroups.Find(modelName, modelGroup))
				{
					modelGroup = new ARGH_AmbientVehicleGroup();
					modelGroup.m_sName = modelName;
					modelGroup.m_bEnabled = true;
					modelGroup.m_aChildren = {};
					modelGroup.m_aVehicles = {};
					modelGroups.Insert(modelName, modelGroup);
					root.m_aChildren.Insert(modelGroup);
				}

				ARGH_AmbientVehicleSpawnRule rule = new ARGH_AmbientVehicleSpawnRule();
				rule.m_sDisplayName = display;
				rule.m_rPrefab = resName;
				rule.m_bEnabled = true;
				rule.m_fSpawnWeight = 1.0;
				modelGroup.m_aVehicles.Insert(rule);
			}

			array<ref ARGH_AmbientVehicleGroup> topGroups = { root };
			bc.Set("m_aGroups", topGroups);

			array<ref ARGH_AmbientVehicleSpawnRule> emptyLegacy = {};
			bc.Set("m_aVehicles", emptyLegacy);

			if (BaseContainerTools.SaveContainer(bc, ResourceName.Empty, targetPath))
			{
				Print(string.Format("[ARGH_VEHICLE_CATALOG] ===== SUCCESS: Saved model-grouped catalog to %1 =====", targetPath));
				s_bDumpedCatalog = true;
			}
			else
			{
				Print(string.Format("[ARGH_VEHICLE_CATALOG] ERROR: Save failed for %1", targetPath), LogLevel.ERROR);
			}
		}

		static void DumpAmbientVehicleCatalogOnce()
		{
			Print("[ARGH_VEHICLE_CATALOG] DumpAmbientVehicleCatalogOnce called");
			ARGH_AmbientVehicleSpawnConfig cfg = GetAmbientVehicleSpawnConfig();
			if (!cfg)
			{
				Print("[ARGH_VEHICLE_CATALOG] Failed to get config", LogLevel.ERROR);
				return;
			}

			DumpAmbientVehicleCatalog(cfg);
		}
	#endif

		// ------------------------------------------------------------
		// Picker logic
		// ------------------------------------------------------------
		private string PickPrefabByRules(array<SCR_EntityCatalogEntry> data, ARGH_AmbientVehicleSpawnConfig cfg, array<ref ARGH_AmbientVehicleSpawnRule> rules, bool forceIgnoreLabels = false)
		{
			if (!rules || rules.IsEmpty())
				return string.Empty;

			array<float> weights = {};
			float totalWeight = 0.0;
			array<ref ARGH_AmbientVehicleSpawnRule> candidates = {};
			bool ignoreLabels = forceIgnoreLabels || (cfg && cfg.m_bRulesIgnoreSpawnpointLabels);

			map<string, bool> allowed = new map<string, bool>();
			if (!ignoreLabels && data)
			{
				foreach (SCR_EntityCatalogEntry entry : data)
				{
					string normalized = NormalizePrefabPath(entry.GetPrefab());
					if (!normalized.IsEmpty())
						allowed.Insert(normalized, true);
				}
			}

			int skippedDisabled = 0;
			int skippedWeight = 0;
			int skippedLabel = 0;

			foreach (ARGH_AmbientVehicleSpawnRule rule : rules)
			{
				if (!rule)
					continue;

				if (!rule.m_bEnabled)
				{
					skippedDisabled++;
					continue;
				}

				float weight = rule.m_fSpawnWeight;
				if (weight <= 0.0)
				{
					skippedWeight++;
					continue;
				}

				if (!ignoreLabels)
				{
					string normalized = NormalizePrefabPath(rule.m_rPrefab);
					bool isAllowed;
					if (normalized.IsEmpty() || !allowed.Find(normalized, isAllowed) || !isAllowed)
					{
						skippedLabel++;
						continue;
					}
				}

				candidates.Insert(rule);
				weights.Insert(weight);
				totalWeight += weight;
			}

			if (cfg && cfg.m_bDebugLogCatalog)
			{
				Print(string.Format("[ARGH_VEHICLE_CATALOG] Rule filter: candidates=%1 totalWeight=%2 skippedDisabled=%3 skippedWeight=%4 skippedLabel=%5",
					candidates.Count(), totalWeight, skippedDisabled, skippedWeight, skippedLabel));
			}

			if (candidates.IsEmpty() || totalWeight <= 0.0)
				return string.Empty;

			float roll = Math.RandomFloat(0.0, totalWeight);
			float running = 0.0;
			for (int i = 0; i < candidates.Count(); i++)
			{
				running += weights[i];
				if (roll <= running)
					return candidates[i].m_rPrefab;
			}

			return candidates[0].m_rPrefab;
		}

		override void ForceUpdate(SCR_Faction faction)
		{
			return Update(faction);
		}

		// Public helper for prefab bootstrap to refresh selection after loot system is ready.
		void ARGH_RefreshNow()
		{
			SCR_Faction savedFaction = SCR_Faction.Cast(m_SavedFaction);
			ForceUpdate(savedFaction);
		}

		// ------------------------------------------------------------
		// UPDATE
		// ------------------------------------------------------------
		override protected void Update(SCR_Faction faction)
		{
			m_SavedFaction = faction;

			ARGH_AmbientVehicleSpawnConfig cfg = GetAmbientVehicleSpawnConfig();
			bool dbg = (cfg && cfg.m_bDebugLogCatalog);

			if (dbg)
			{
				string catPath = "<EMPTY>";
				if (cfg && !cfg.m_rVehicleCatalog.IsEmpty())
					catPath = cfg.m_rVehicleCatalog.GetPath();

				Print(string.Format("[ARGH_VEHICLE_CATALOG] Update() ignoreLabels=%1 catalog=%2",
					cfg.m_bRulesIgnoreSpawnpointLabels,
					catPath));
			}

			array<ref ARGH_AmbientVehicleSpawnRule> rules = ResolvePrefabRules(cfg);
			bool hasCatalog = (cfg && !cfg.m_rVehicleCatalog.IsEmpty() && rules);

			DL_LootSystem loot = DL_LootSystem.GetInstance();
			bool lootReady = (loot && loot.vehicleDataReady && loot.vehicleCatalog);

			if (hasCatalog && (cfg.m_bRulesIgnoreSpawnpointLabels || !lootReady))
			{
				if (!lootReady && dbg)
					Print("[ARGH_VEHICLE_CATALOG] DL_LootSystem not ready; falling back to catalog-only selection");

				string configuredPrefab = PickPrefabByRules(null, cfg, rules, true);
				if (!configuredPrefab.IsEmpty())
				{
					m_sPrefab = configuredPrefab;
					if (dbg)
						Print(string.Format("[ARGH_VEHICLE_CATALOG] Selected prefab from catalog (no DL): %1", m_sPrefab));
					return;
				}

				m_sPrefab = string.Empty;
				if (dbg)
					Print("[ARGH_VEHICLE_CATALOG] Catalog configured but nothing eligible -> no spawn");
				return;
			}

			if (!lootReady)
			{
				if (dbg) Print("[ARGH_VEHICLE_CATALOG] EARLY RETURN: DL_LootSystem is not ready");
				return;
			}

			SCR_EntityCatalog entityCatalog = loot.vehicleCatalog;

			array<SCR_EntityCatalogEntry> data = {};

			if (cfg && !cfg.m_bRulesIgnoreSpawnpointLabels)
			{
				entityCatalog.GetFullFilteredEntityListWithLabels(
					data,
					m_aIncludedEditableEntityLabels,
					m_aExcludedEditableEntityLabels,
					m_bRequireAllIncludedLabels
				);

				if (dbg)
					Print(string.Format("[ARGH_VEHICLE_CATALOG] EntityCatalog label-filtered: data=%1 (inc=%2 exc=%3 requireAll=%4)",
						data.Count(), m_aIncludedEditableEntityLabels.Count(), m_aExcludedEditableEntityLabels.Count(), m_bRequireAllIncludedLabels));
			}
			else
			{
				array<EEditableEntityLabel> emptyInc = {};
				array<EEditableEntityLabel> emptyExc = {};
				entityCatalog.GetFullFilteredEntityListWithLabels(data, emptyInc, emptyExc, false);

				if (dbg)
					Print(string.Format("[ARGH_VEHICLE_CATALOG] EntityCatalog UNFILTERED (ignoreLabels): data=%1", data.Count()));
			}

			if (data.IsEmpty())
			{
				if (dbg) Print("[ARGH_VEHICLE_CATALOG] EARLY RETURN: data is EMPTY after catalog query");
				return;
			}

			if (cfg && !cfg.m_rVehicleCatalog.IsEmpty() && rules)
			{
				string configuredPrefab = PickPrefabByRules(data, cfg, rules);
				if (!configuredPrefab.IsEmpty())
				{
					m_sPrefab = configuredPrefab;
					if (dbg)
						Print(string.Format("[ARGH_VEHICLE_CATALOG] Selected prefab from catalog: %1", m_sPrefab));
					return;
				}

				m_sPrefab = string.Empty;
				if (dbg)
					Print("[ARGH_VEHICLE_CATALOG] Catalog configured but nothing eligible -> no spawn");
				return;
			}

			m_sPrefab = data.GetRandomElement().GetPrefab();
			if (dbg)
				Print(string.Format("[ARGH_VEHICLE_CATALOG] No catalog loaded -> random fallback: %1", m_sPrefab));
		}
	}
