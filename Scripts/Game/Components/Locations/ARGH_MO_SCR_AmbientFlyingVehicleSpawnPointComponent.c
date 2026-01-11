// =============================================================
// ARGH_MO_SCR_AmbientFlyingVehicleSpawnPointComponent.c
//
// Ambient flying vehicle variant of the ground vehicle spawnpoint logic.
// Scans helicopter/aircraft prefabs and uses a dedicated catalog/config.
// =============================================================

// ------------------------------------------------------------
// CUSTOM EDITABLE ENTITY LABEL (shows in .et dropdown)
// ------------------------------------------------------------
modded enum EEditableEntityLabel
{
	ARGH_AMBIENT_FLYING_VEHICLES
};

// -----------------------------
// Variant rule (leaf)
// -----------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sDisplayName")]
class ARGH_AmbientFlyingVehicleSpawnRule : Managed
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
// Group node (Aircraft Model)
// -----------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sName")]
class ARGH_AmbientFlyingVehicleGroup : Managed
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Group name (e.g. ALL IN-GAME AIR, AH6M, UH1, Mi-8...)")]
	string m_sName;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Enable/disable this entire group (affects children too)")]
	bool m_bEnabled;

	// Leaf list used on MODEL groups
	[Attribute(defvalue: "{}", desc: "Vehicle variants in this group")]
	ref array<ref ARGH_AmbientFlyingVehicleSpawnRule> m_aVehicles;

	// Child groups used for nesting (Root -> Models)
	[Attribute(defvalue: "{}", desc: "Child groups")]
	ref array<ref ARGH_AmbientFlyingVehicleGroup> m_aChildren;
}

// -----------------------------
// Catalog config (root)
// -----------------------------
[BaseContainerProps(configRoot: true)]
class ARGH_AmbientFlyingVehicleCatalogConfig : Managed
{
	// Grouped tree (preferred)
	[Attribute(defvalue: "{}", desc: "Grouped aircraft catalog (Root -> Models -> Variants)")]
	ref array<ref ARGH_AmbientFlyingVehicleGroup> m_aGroups;

	// Legacy fallback (so old files still load)
	[Attribute(defvalue: "{}", desc: "(Legacy) Flat aircraft catalog entries")]
	ref array<ref ARGH_AmbientFlyingVehicleSpawnRule> m_aVehicles;
}

// -----------------------------
// Spawn config (root)
// -----------------------------
[BaseContainerProps(configRoot: true)]
class ARGH_AmbientFlyingVehicleSpawnConfig : Managed
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "conf", desc: "Optional aircraft catalog resource to use for ambient spawns.")]
	ResourceName m_rVehicleCatalog;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Ignore spawnpoint labels and always use the catalog list.")]
	bool m_bRulesIgnoreSpawnpointLabels;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Override label filtering and auto-allow helicopter/plane prefabs.")]
	bool m_bOverrideLabelsWithFlyingCriteria;

	[Attribute(defvalue: "0.369", uiwidget: UIWidgets.EditBox, desc: "Chance (0-1) to spawn when players are nearby.")]
	float m_fSpawnChance;

	[Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "Respawn cooldown multiplier when a spawn roll fails.")]
	int m_iLongCooldownMultiplier;

	[Attribute(defvalue: "2500", uiwidget: UIWidgets.EditBox, desc: "Area cap radius in meters (<=0 disables cap).")]
	float m_fAreaRadius;

	[Attribute(defvalue: "5", uiwidget: UIWidgets.EditBox, desc: "Max vehicles within area radius (<=0 disables cap).")]
	int m_iMaxVehiclesInArea;

	[Attribute(defvalue: "{}", desc: "Additional root paths to scan for aircraft prefabs.")]
	ref array<string> m_aCatalogRoots;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Debug: log enabled/disabled groups/variants and selection decisions")]
	bool m_bDebugLogCatalog;
}

class ARGH_SCR_AmbientFlyingVehicleSpawnPointComponentClass : SCR_AmbientVehicleSpawnPointComponentClass
{
}

class ARGH_SCR_AmbientFlyingVehicleSpawnPointComponent : SCR_AmbientVehicleSpawnPointComponent
{
	#ifdef WORKBENCH
	private static int s_iFlyingSpawnpointInitCount = 0;
	#endif

	// Config resource path (Workbench-editable .conf)
	static const ResourceName ARGH_AMBIENT_FLYING_VEHICLE_CONFIG = "{A98C7C1F0F9A2B34}Configs/Systems/ARGH_AmbientFlyingVehicleSpawn.conf";
	private static ref ARGH_AmbientFlyingVehicleSpawnConfig s_FlyingVehicleSpawnConfig;
	private static bool s_bFlyingConfigLoaded = false;

	private static bool s_bFlyingDumpedCatalog = false;
	private static ResourceName s_FlyingCatalogConfigPath;
	private static ref ARGH_AmbientFlyingVehicleCatalogConfig s_FlyingVehicleCatalogConfig;

#ifdef WORKBENCH
	static bool s_bFlyingDumpScheduled = false;
	static bool s_bFlyingRebuildRequested = false;

	static void ResetFlyingCatalogState()
	{
		s_bFlyingDumpScheduled = false;
		s_bFlyingRebuildRequested = false;
		s_bFlyingDumpedCatalog = false;
		s_FlyingVehicleCatalogConfig = null;
		s_FlyingCatalogConfigPath = ResourceName.Empty;
		s_FlyingVehicleSpawnConfig = null;
		s_bFlyingConfigLoaded = false;
	}
#endif

	// ------------------------------------------------------------
	// Spawn config load
	// ------------------------------------------------------------
	static ARGH_AmbientFlyingVehicleSpawnConfig GetAmbientFlyingVehicleSpawnConfig()
	{
		if (s_bFlyingConfigLoaded && s_FlyingVehicleSpawnConfig)
			return s_FlyingVehicleSpawnConfig;

		Print("[ARGH_FLYING_CATALOG] Loading config from: " + ARGH_AMBIENT_FLYING_VEHICLE_CONFIG);

		Resource res = Resource.Load(ARGH_AMBIENT_FLYING_VEHICLE_CONFIG);
		if (res && res.IsValid())
		{
			BaseResourceObject resource = res.GetResource();
			if (resource)
			{
				BaseContainer bc = resource.ToBaseContainer();
				if (bc)
				{
					ARGH_AmbientFlyingVehicleSpawnConfig cfg = new ARGH_AmbientFlyingVehicleSpawnConfig();

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
					if (bc.Get("m_bOverrideLabelsWithFlyingCriteria", tmpB))
						cfg.m_bOverrideLabelsWithFlyingCriteria = tmpB;
					if (bc.Get("m_bDebugLogCatalog", tmpB))
						cfg.m_bDebugLogCatalog = tmpB;

					ResourceName tmpR;
					if (bc.Get("m_rVehicleCatalog", tmpR))
						cfg.m_rVehicleCatalog = tmpR;

					array<string> roots = {};
					if (bc.Get("m_aCatalogRoots", roots))
						cfg.m_aCatalogRoots = roots;

					s_FlyingVehicleSpawnConfig = cfg;
					s_bFlyingConfigLoaded = true;

					Print("[ARGH_FLYING_CATALOG] Config loaded successfully");
					Print(string.Format("[ARGH_FLYING_CATALOG] Catalog path: %1", cfg.m_rVehicleCatalog));
					Print(string.Format("[ARGH_FLYING_CATALOG] Debug log: %1", cfg.m_bDebugLogCatalog));
				}
				else
				{
					Print("[ARGH_FLYING_CATALOG] Failed to get BaseContainer from resource", LogLevel.WARNING);
				}
			}
			else
			{
				Print("[ARGH_FLYING_CATALOG] Failed to get BaseResourceObject", LogLevel.WARNING);
			}
		}
		else
		{
			Print("[ARGH_FLYING_CATALOG] Failed to load config resource - using defaults", LogLevel.WARNING);
		}

		if (!s_FlyingVehicleSpawnConfig)
		{
			Print("[ARGH_FLYING_CATALOG] Creating default config");
			s_FlyingVehicleSpawnConfig = new ARGH_AmbientFlyingVehicleSpawnConfig();
			ApplyFlyingSpawnConfigDefaults(s_FlyingVehicleSpawnConfig);
			s_bFlyingConfigLoaded = true;
		}

		return s_FlyingVehicleSpawnConfig;
	}

	private static void ApplyFlyingSpawnConfigDefaults(ARGH_AmbientFlyingVehicleSpawnConfig cfg)
	{
		if (!cfg)
			return;

		cfg.m_bRulesIgnoreSpawnpointLabels = true;
		cfg.m_bOverrideLabelsWithFlyingCriteria = true;
		cfg.m_fSpawnChance = 0.369;
		cfg.m_iLongCooldownMultiplier = 10;
		cfg.m_fAreaRadius = 2500.0;
		cfg.m_iMaxVehiclesInArea = 5;
		cfg.m_bDebugLogCatalog = false;
		if (!cfg.m_aCatalogRoots)
			cfg.m_aCatalogRoots = {};

		Print("[ARGH_FLYING_CATALOG] Applied default config values");
	}

	// ------------------------------------------------------------
	// Normalize for matching
	// ------------------------------------------------------------
	private static string NormalizeFlyingPrefabPath(string prefabPath)
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
	// Model/variant helpers (HELICOPTERS + AIR)
	// ------------------------------------------------------------
	private static string ExtractFlyingVehicleModelFromNormalized(string normalizedPath)
	{
		string p = normalizedPath;
		p.Replace("\\", "/");

		string lower = p;
		lower.ToLower();

		int heliIndex = lower.IndexOf("prefabs/vehicles/helicopters/");
		if (heliIndex >= 0)
		{
			int pathStart = heliIndex + "prefabs/vehicles/helicopters/".Length();
			string relativePath = p.Substring(pathStart, p.Length() - pathStart);

			array<string> parts = {};
			relativePath.Split("/", parts, false);
			if (parts.Count() < 2)
				return "UNKNOWN_MODEL";

			return parts[0];
		}

		int planeIndex = lower.IndexOf("prefabs/vehicles/planes/");
		if (planeIndex >= 0)
		{
			int pathStart2 = planeIndex + "prefabs/vehicles/planes/".Length();
			string relativePath2 = p.Substring(pathStart2, p.Length() - pathStart2);

			array<string> parts2 = {};
			relativePath2.Split("/", parts2, false);
			if (parts2.Count() < 2)
				return "UNKNOWN_MODEL";

			return parts2[0];
		}

		return "UNKNOWN_MODEL";
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

	#ifdef WORKBENCH
		s_iFlyingSpawnpointInitCount++;
		if (s_iFlyingSpawnpointInitCount <= 5)
			Print("[ARGH_AMBIENT] Flying spawnpoint init: " + owner);
		else if (s_iFlyingSpawnpointInitCount == 6)
			Print("[ARGH_AMBIENT] Flying spawnpoint init: more...");
	#endif
	}

	private static string ExtractFlyingCategoryFromNormalized(string normalizedPath)
	{
		string p = normalizedPath;
		p.Replace("\\", "/");

		string lower = p;
		lower.ToLower();

		if (lower.IndexOf("prefabs/vehicles/helicopters/") >= 0)
			return "Helicopters";
		if (lower.IndexOf("prefabs/vehicles/planes/") >= 0)
			return "Planes";

		return "Other";
	}

	private static string ExtractFlyingVariantNameFromNormalized(string normalizedPath)
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
	private static void CountFlyingGroupTree(array<ref ARGH_AmbientFlyingVehicleGroup> groups, bool parentEnabled,
		out int totalGroups, out int enabledGroups, out int totalVariants, out int enabledVariants)
	{
		if (!groups)
			return;

		foreach (ARGH_AmbientFlyingVehicleGroup g : groups)
		{
			if (!g)
				continue;

			totalGroups++;
			bool groupEnabled = parentEnabled && g.m_bEnabled;
			if (groupEnabled)
				enabledGroups++;

			if (g.m_aVehicles)
			{
				foreach (ARGH_AmbientFlyingVehicleSpawnRule r : g.m_aVehicles)
				{
					if (!r)
						continue;
					totalVariants++;
					if (groupEnabled && r.m_bEnabled && (r.m_fSpawnWeight > 0.0))
						enabledVariants++;
				}
			}

			if (g.m_aChildren)
					CountFlyingGroupTree(g.m_aChildren, groupEnabled, totalGroups, enabledGroups, totalVariants, enabledVariants);
		}
	}

	private static void DebugDumpFlyingGroupTree(array<ref ARGH_AmbientFlyingVehicleGroup> groups, bool parentEnabled, string indent, bool logVariants)
	{
		if (!groups)
			return;

		foreach (ARGH_AmbientFlyingVehicleGroup g : groups)
		{
			if (!g)
				continue;

			bool groupEnabled = parentEnabled && g.m_bEnabled;
			Print(string.Format("[ARGH_FLYING_CATALOG] %1Group: %2 | enabled=%3 (self=%4)", indent, g.m_sName, groupEnabled, g.m_bEnabled));

			if (logVariants && g.m_aVehicles)
			{
				foreach (ARGH_AmbientFlyingVehicleSpawnRule r : g.m_aVehicles)
				{
					if (!r)
						continue;
					Print(string.Format("[ARGH_FLYING_CATALOG] %1  Variant: %2 | enabled=%3 | weight=%4 | prefab=%5",
						indent,
						r.m_sDisplayName,
						(groupEnabled && r.m_bEnabled && r.m_fSpawnWeight > 0.0),
						r.m_fSpawnWeight,
						r.m_rPrefab));
				}
			}

			if (g.m_aChildren)
					DebugDumpFlyingGroupTree(g.m_aChildren, groupEnabled, indent + "  ", logVariants);
		}
	}

	private static void FlattenFlyingEnabledGroups(array<ref ARGH_AmbientFlyingVehicleGroup> groups, array<ref ARGH_AmbientFlyingVehicleSpawnRule> outRules, bool parentEnabled)
	{
		if (!groups || !outRules)
			return;

		foreach (ARGH_AmbientFlyingVehicleGroup g : groups)
		{
			if (!g)
				continue;

			bool enabled = parentEnabled && g.m_bEnabled;
			if (!enabled)
				continue;

			if (g.m_aVehicles)
			{
				foreach (ARGH_AmbientFlyingVehicleSpawnRule r : g.m_aVehicles)
				{
					if (!r)
						continue;
					outRules.Insert(r);
				}
			}

			if (g.m_aChildren)
					FlattenFlyingEnabledGroups(g.m_aChildren, outRules, enabled);
		}
	}

	// ------------------------------------------------------------
	// Catalog load
	// ------------------------------------------------------------
	private ARGH_AmbientFlyingVehicleCatalogConfig LoadFlyingVehicleCatalog(ResourceName path)
	{
		if (path.IsEmpty())
			return null;

		if (s_FlyingVehicleCatalogConfig && s_FlyingCatalogConfigPath == path)
			return s_FlyingVehicleCatalogConfig;

		s_FlyingCatalogConfigPath = path;
		s_FlyingVehicleCatalogConfig = null;

		Resource res = Resource.Load(path);
		if (!res)
			return null;

		BaseContainer bc = res.GetResource().ToBaseContainer();
		if (!bc)
			return null;

		ARGH_AmbientFlyingVehicleCatalogConfig cfg = new ARGH_AmbientFlyingVehicleCatalogConfig();

		cfg.m_aGroups = {};
		array<ref ARGH_AmbientFlyingVehicleGroup> groups = {};
		if (bc.Get("m_aGroups", groups))
			cfg.m_aGroups = groups;

		cfg.m_aVehicles = {};
		array<ref ARGH_AmbientFlyingVehicleSpawnRule> rules = {};
		if (bc.Get("m_aVehicles", rules))
			cfg.m_aVehicles = rules;

		s_FlyingVehicleCatalogConfig = cfg;
		return s_FlyingVehicleCatalogConfig;
	}

	private array<ref ARGH_AmbientFlyingVehicleSpawnRule> ResolvePrefabRules(ARGH_AmbientFlyingVehicleSpawnConfig cfg)
	{
		if (!cfg || cfg.m_rVehicleCatalog.IsEmpty())
			return null;

		ARGH_AmbientFlyingVehicleCatalogConfig catalog = LoadFlyingVehicleCatalog(cfg.m_rVehicleCatalog);
		if (!catalog)
			return null;

		if (catalog.m_aGroups && catalog.m_aGroups.Count() > 0)
		{
			if (cfg.m_bDebugLogCatalog)
			{
				int totalG, enabledG, totalV, enabledV;
					CountFlyingGroupTree(catalog.m_aGroups, true, totalG, enabledG, totalV, enabledV);
					Print(string.Format("[ARGH_FLYING_CATALOG] Catalog summary: groups=%1 enabledGroups=%2 variants=%3 enabledVariants=%4",
						totalG, enabledG, totalV, enabledV));
					DebugDumpFlyingGroupTree(catalog.m_aGroups, true, "", false);
				}

				array<ref ARGH_AmbientFlyingVehicleSpawnRule> flat = {};
				FlattenFlyingEnabledGroups(catalog.m_aGroups, flat, true);
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
	static void TriggerFlyingCatalogRebuild()
	{
		if (s_bFlyingRebuildRequested)
		{
			Print("[ARGH_FLYING_CATALOG] Rebuild already requested, skipping duplicate");
			return;
		}

		ResetFlyingCatalogState();
		s_bFlyingRebuildRequested = true;
		ScheduleFlyingCatalogDump(5);
	}

	static void ForceRebuildFlyingCatalog()
	{
		ResetFlyingCatalogState();
		s_bFlyingRebuildRequested = true;
		DumpAmbientFlyingVehicleCatalogOnce();
	}

	static void ScheduleFlyingCatalogDump(int retries = 5)
	{
		if (s_bFlyingDumpScheduled)
		{
			Print("[ARGH_FLYING_CATALOG] Dump already scheduled");
			return;
		}

		if (!GetGame())
		{
			Print("[ARGH_FLYING_CATALOG] GetGame() returned null, cannot schedule dump", LogLevel.WARNING);
			return;
		}

		s_bFlyingDumpScheduled = true;
		Print(string.Format("[ARGH_FLYING_CATALOG] Scheduling catalog dump with %1 retries", retries));
		GetGame().GetCallqueue().CallLater(AttemptFlyingCatalogDump, 1000, false, retries);
	}

	static void AttemptFlyingCatalogDump(int retries)
	{
		s_bFlyingDumpScheduled = false;
		Print(string.Format("[ARGH_FLYING_CATALOG] Attempting catalog dump (retries remaining: %1)", retries));
		DumpAmbientFlyingVehicleCatalogOnce();

		if (!s_bFlyingDumpedCatalog && retries > 0)
		{
			Print(string.Format("[ARGH_FLYING_CATALOG] Dump not completed, scheduling retry in 1s"));
			if (GetGame())
				GetGame().GetCallqueue().CallLater(AttemptFlyingCatalogDump, 1000, false, retries - 1);
		}
		else if (s_bFlyingDumpedCatalog)
		{
			Print("[ARGH_FLYING_CATALOG] Catalog dump completed successfully");
		}
		else
		{
			Print("[ARGH_FLYING_CATALOG] Catalog dump failed after all retries", LogLevel.ERROR);
		}
	}
#endif

	private static bool IsFlyingVehiclePrefabPath(string normalizedPathLower)
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

		// Helicopters: Prefabs/Vehicles/Helicopters/<VehicleFolder>/<VehiclePrefab>.et
		int heliIndex = normalizedPathLower.IndexOf("prefabs/vehicles/helicopters/");
		if (heliIndex >= 0)
		{
			int pathStart = heliIndex + "prefabs/vehicles/helicopters/".Length();
			string relativePath = normalizedPathLower.Substring(pathStart, normalizedPathLower.Length() - pathStart);
			if (relativePath.IsEmpty())
				return false;

			array<string> parts = {};
			relativePath.Split("/", parts, false);
			return (parts.Count() == 2);
		}

		// Planes: Prefabs/Vehicles/Planes/<VehicleFolder>/<VehiclePrefab>.et
		int planeIndex = normalizedPathLower.IndexOf("prefabs/vehicles/planes/");
		if (planeIndex >= 0)
		{
			int pathStart2 = planeIndex + "prefabs/vehicles/planes/".Length();
			string relativePath2 = normalizedPathLower.Substring(pathStart2, normalizedPathLower.Length() - pathStart2);
			if (relativePath2.IsEmpty())
				return false;

			array<string> parts2 = {};
			relativePath2.Split("/", parts2, false);
			return (parts2.Count() == 2);
		}

		return false;
	}

	private static bool IsFlyingVehiclePrefabResource(ResourceName prefabName)
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

	private static bool MatchesFlyingRootFilters(string normalizedPathLower, array<string> rootFilters)
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

	private static bool AddFlyingVehiclePrefab(map<string, ResourceName> uniqueResources, ResourceName prefabName, array<string> rootFilters)
	{
		if (!uniqueResources)
			return false;

		string normalized = NormalizeFlyingPrefabPath(prefabName);
		if (normalized.IsEmpty())
			return false;

		string normalizedPath = normalized;
		normalizedPath.Replace("\\", "/");

		string normalizedPathLower = normalizedPath;
		normalizedPathLower.ToLower();

		if (!MatchesFlyingRootFilters(normalizedPathLower, rootFilters))
			return false;
		if (!IsFlyingVehiclePrefabPath(normalizedPathLower))
			return false;

		if (!IsFlyingVehiclePrefabResource(prefabName))
			return false;

		ResourceName existingName;
		if (uniqueResources.Find(normalizedPath, existingName))
			return false;

		uniqueResources.Insert(normalizedPath, prefabName);
		return true;
	}

	private static int CollectFlyingVehiclePrefabsFromAllEt(ARGH_AmbientFlyingVehicleSpawnConfig cfg, map<string, ResourceName> uniqueResources)
	{
		if (!uniqueResources)
			return 0;

		Print("[ARGH_FLYING_CATALOG] Collecting aircraft from .et files");

		array<ResourceName> resources = {};
		SearchResourcesFilter filter = new SearchResourcesFilter();
		filter.fileExtensions = { "et" };

		array<string> roots = cfg.m_aCatalogRoots;
		if (!roots || roots.IsEmpty())
		{
			roots = { "Prefabs/Vehicles/Helicopters", "Prefabs/Vehicles/Planes" };
			Print("[ARGH_FLYING_CATALOG] Using default root filters: Prefabs/Vehicles/Helicopters + Prefabs/Vehicles/Planes");
		}
		else
		{
			Print(string.Format("[ARGH_FLYING_CATALOG] Using %1 custom root filters", roots.Count()));
		}

		filter.rootPath = string.Empty;
		ResourceDatabase.SearchResources(filter, resources.Insert);
		Print(string.Format("[ARGH_FLYING_CATALOG] Found %1 .et files to process", resources.Count()));

		int added = 0;
		foreach (ResourceName resName : resources)
		{
			if (AddFlyingVehiclePrefab(uniqueResources, resName, roots))
				added++;
		}

		Print(string.Format("[ARGH_FLYING_CATALOG] Added %1 aircraft from .et scan", added));
		return added;
	}

	// ------------------------------------------------------------
	// REBUILD: saves MODEL-GROUPED catalog
	// ------------------------------------------------------------
	private static void DumpAmbientFlyingVehicleCatalog(ARGH_AmbientFlyingVehicleSpawnConfig cfg)
	{
		if (!cfg)
		{
			Print("[ARGH_FLYING_CATALOG] No config provided to DumpAmbientFlyingVehicleCatalog", LogLevel.ERROR);
			return;
		}

		if (s_bFlyingDumpedCatalog)
		{
			Print("[ARGH_FLYING_CATALOG] Catalog already dumped this session");
			return;
		}

		Print("[ARGH_FLYING_CATALOG] ===== Starting catalog rebuild (AIRCRAFT) =====");

		ResourceName target = cfg.m_rVehicleCatalog;
		if (target.IsEmpty())
		{
			target = "{B7D4C5E6123F4A90}Configs/EntityCatalog/ARGH_AmbientFlyingVehicles.conf";
			Print(string.Format("[ARGH_FLYING_CATALOG] No catalog path in config, using default: %1", target));
		}
		else
		{
			Print(string.Format("[ARGH_FLYING_CATALOG] Using catalog path from config: %1", target));
		}

		string targetPath = target.GetPath();
		Resource res = Resource.Load(target);
		if (!res || !res.IsValid())
		{
			Print(string.Format("[ARGH_FLYING_CATALOG] ERROR: Target resource missing or invalid (%1)", targetPath), LogLevel.ERROR);
			return;
		}

		BaseContainer bc = res.GetResource().ToBaseContainer();
		if (!bc)
		{
			Print(string.Format("[ARGH_FLYING_CATALOG] ERROR: Failed to read container: %1", targetPath), LogLevel.ERROR);
			return;
		}

		map<string, ResourceName> uniqueResources = new map<string, ResourceName>();
		int fromEt = CollectFlyingVehiclePrefabsFromAllEt(cfg, uniqueResources);

		Print(string.Format("[ARGH_FLYING_CATALOG] Total unique aircraft collected: %1 (.et scan: %2)",
			uniqueResources.Count(), fromEt));

		if (uniqueResources.Count() == 0)
		{
			Print("[ARGH_FLYING_CATALOG] ERROR: No aircraft prefabs found during rebuild", LogLevel.ERROR);
			return;
		}

		ARGH_AmbientFlyingVehicleGroup root = new ARGH_AmbientFlyingVehicleGroup();
		root.m_sName = "ALL IN-GAME AIR";
		root.m_bEnabled = true;
		root.m_aChildren = {};
		root.m_aVehicles = {};

		ref map<string, ref ARGH_AmbientFlyingVehicleGroup> categoryGroups = new map<string, ref ARGH_AmbientFlyingVehicleGroup>();
		ref map<string, ref ARGH_AmbientFlyingVehicleGroup> modelGroups = new map<string, ref ARGH_AmbientFlyingVehicleGroup>();

		foreach (string normalizedKey, ResourceName resName : uniqueResources)
		{
			string normalized = normalizedKey;
			normalized.Replace("\\", "/");

			string categoryName = ExtractFlyingCategoryFromNormalized(normalized);
			string modelName = ExtractFlyingVehicleModelFromNormalized(normalized);
			string variantName = ExtractFlyingVariantNameFromNormalized(normalized);
			string display = string.Format("%1  [%2]", variantName, normalized);

			ref ARGH_AmbientFlyingVehicleGroup categoryGroup;
			if (!categoryGroups.Find(categoryName, categoryGroup))
			{
				categoryGroup = new ARGH_AmbientFlyingVehicleGroup();
				categoryGroup.m_sName = categoryName;
				categoryGroup.m_bEnabled = true;
				categoryGroup.m_aChildren = {};
				categoryGroup.m_aVehicles = {};
				categoryGroups.Insert(categoryName, categoryGroup);
				root.m_aChildren.Insert(categoryGroup);
			}

			ref ARGH_AmbientFlyingVehicleGroup modelGroup;
			string modelKey = categoryName + "::" + modelName;
			if (!modelGroups.Find(modelKey, modelGroup))
			{
				modelGroup = new ARGH_AmbientFlyingVehicleGroup();
				modelGroup.m_sName = modelName;
				modelGroup.m_bEnabled = true;
				modelGroup.m_aChildren = {};
				modelGroup.m_aVehicles = {};
				modelGroups.Insert(modelKey, modelGroup);
				categoryGroup.m_aChildren.Insert(modelGroup);
			}

			ARGH_AmbientFlyingVehicleSpawnRule rule = new ARGH_AmbientFlyingVehicleSpawnRule();
			rule.m_sDisplayName = display;
			rule.m_rPrefab = resName;
			rule.m_bEnabled = true;
			rule.m_fSpawnWeight = 1.0;
			modelGroup.m_aVehicles.Insert(rule);
		}

		array<ref ARGH_AmbientFlyingVehicleGroup> topGroups = { root };
		bc.Set("m_aGroups", topGroups);

		array<ref ARGH_AmbientFlyingVehicleSpawnRule> emptyLegacy = {};
		bc.Set("m_aVehicles", emptyLegacy);

		if (BaseContainerTools.SaveContainer(bc, target, targetPath))
		{
			Print(string.Format("[ARGH_FLYING_CATALOG] ===== SUCCESS: Saved aircraft catalog to %1 =====", targetPath));
			s_bFlyingDumpedCatalog = true;
		}
		else
		{
			Print(string.Format("[ARGH_FLYING_CATALOG] ERROR: Save failed for %1", targetPath), LogLevel.ERROR);
		}
	}

	static void DumpAmbientFlyingVehicleCatalogOnce()
	{
		Print("[ARGH_FLYING_CATALOG] DumpAmbientFlyingVehicleCatalogOnce called");
		ARGH_AmbientFlyingVehicleSpawnConfig cfg = GetAmbientFlyingVehicleSpawnConfig();
		if (!cfg)
		{
			Print("[ARGH_FLYING_CATALOG] Failed to get config", LogLevel.ERROR);
			return;
		}

		DumpAmbientFlyingVehicleCatalog(cfg);
	}

	// ------------------------------------------------------------
	// Picker logic
	// ------------------------------------------------------------
	private string PickPrefabByRules(array<SCR_EntityCatalogEntry> data, ARGH_AmbientFlyingVehicleSpawnConfig cfg, array<ref ARGH_AmbientFlyingVehicleSpawnRule> rules, bool forceIgnoreLabels = false)
	{
		if (!rules || rules.IsEmpty())
			return string.Empty;

		array<float> weights = {};
		float totalWeight = 0.0;
		array<ref ARGH_AmbientFlyingVehicleSpawnRule> candidates = {};
		bool ignoreLabels = forceIgnoreLabels || (cfg && cfg.m_bRulesIgnoreSpawnpointLabels);

		map<string, bool> allowed = new map<string, bool>();
		if (!ignoreLabels && data)
		{
			foreach (SCR_EntityCatalogEntry entry : data)
			{
			string normalized = NormalizeFlyingPrefabPath(entry.GetPrefab());
				if (!normalized.IsEmpty())
					allowed.Insert(normalized, true);
			}
		}

		int skippedDisabled = 0;
		int skippedWeight = 0;
		int skippedLabel = 0;

		foreach (ARGH_AmbientFlyingVehicleSpawnRule rule : rules)
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
				string normalizedRule = NormalizeFlyingPrefabPath(rule.m_rPrefab);
				bool isAllowed;
				if (normalizedRule.IsEmpty() || !allowed.Find(normalizedRule, isAllowed) || !isAllowed)
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
			Print(string.Format("[ARGH_FLYING_CATALOG] Rule filter: candidates=%1 totalWeight=%2 skippedDisabled=%3 skippedWeight=%4 skippedLabel=%5",
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

	// ------------------------------------------------------------
	// UPDATE
	// ------------------------------------------------------------
	override protected void Update(SCR_Faction faction)
	{
		m_SavedFaction = faction;

		ARGH_AmbientFlyingVehicleSpawnConfig cfg = GetAmbientFlyingVehicleSpawnConfig();
		bool dbg = (cfg && cfg.m_bDebugLogCatalog);

		if (dbg)
		{
			string catPath = "<EMPTY>";
			if (cfg && !cfg.m_rVehicleCatalog.IsEmpty())
				catPath = cfg.m_rVehicleCatalog.GetPath();

			Print(string.Format("[ARGH_FLYING_CATALOG] Update() ignoreLabels=%1 catalog=%2",
				cfg.m_bRulesIgnoreSpawnpointLabels,
				catPath));
		}

		array<ref ARGH_AmbientFlyingVehicleSpawnRule> rules = ResolvePrefabRules(cfg);
		bool hasCatalog = (cfg && !cfg.m_rVehicleCatalog.IsEmpty() && rules);

		DL_LootSystem loot = DL_LootSystem.GetInstance();
		bool lootReady = (loot && loot.vehicleDataReady && loot.vehicleCatalog);

		if (hasCatalog && (cfg.m_bRulesIgnoreSpawnpointLabels || !lootReady))
		{
			if (!lootReady && dbg)
				Print("[ARGH_FLYING_CATALOG] DL_LootSystem not ready; falling back to catalog-only selection");

			string configuredPrefab = PickPrefabByRules(null, cfg, rules, true);
			if (!configuredPrefab.IsEmpty())
			{
				m_sPrefab = configuredPrefab;
				if (dbg)
					Print(string.Format("[ARGH_FLYING_CATALOG] Selected prefab from catalog (no DL): %1", m_sPrefab));
				return;
			}

			m_sPrefab = string.Empty;
			if (dbg)
				Print("[ARGH_FLYING_CATALOG] Catalog configured but nothing eligible -> no spawn");
			return;
		}

		if (!lootReady)
		{
			if (dbg) Print("[ARGH_FLYING_CATALOG] EARLY RETURN: DL_LootSystem is not ready");
			return;
		}

		SCR_EntityCatalog entityCatalog = loot.vehicleCatalog;

		array<SCR_EntityCatalogEntry> data = {};

		array<EEditableEntityLabel> emptyInc = {};
		array<EEditableEntityLabel> emptyExc = {};
		if (cfg && !cfg.m_bRulesIgnoreSpawnpointLabels)
		{
			if (cfg.m_bOverrideLabelsWithFlyingCriteria)
			{
				array<SCR_EntityCatalogEntry> all = {};
				entityCatalog.GetFullFilteredEntityListWithLabels(all, emptyInc, emptyExc, false);

				foreach (SCR_EntityCatalogEntry entry : all)
				{
					string normalized = NormalizeFlyingPrefabPath(entry.GetPrefab());
					if (normalized.IsEmpty())
						continue;

					string normalizedLower = normalized;
					normalizedLower.Replace("\\", "/");
					normalizedLower.ToLower();

					if (IsFlyingVehiclePrefabPath(normalizedLower))
						data.Insert(entry);
				}

				if (dbg)
					Print(string.Format("[ARGH_FLYING_CATALOG] EntityCatalog override: data=%1 (from %2)",
						data.Count(), all.Count()));
			}
			else
			{
				entityCatalog.GetFullFilteredEntityListWithLabels(
					data,
					m_aIncludedEditableEntityLabels,
					m_aExcludedEditableEntityLabels,
					m_bRequireAllIncludedLabels
				);

				if (dbg)
					Print(string.Format("[ARGH_FLYING_CATALOG] EntityCatalog label-filtered: data=%1 (inc=%2 exc=%3 requireAll=%4)",
						data.Count(), m_aIncludedEditableEntityLabels.Count(), m_aExcludedEditableEntityLabels.Count(), m_bRequireAllIncludedLabels));
			}
		}
		else
		{
			entityCatalog.GetFullFilteredEntityListWithLabels(data, emptyInc, emptyExc, false);

			if (dbg)
				Print(string.Format("[ARGH_FLYING_CATALOG] EntityCatalog UNFILTERED (ignoreLabels): data=%1", data.Count()));
		}

		if (data.IsEmpty())
		{
			if (dbg) Print("[ARGH_FLYING_CATALOG] EARLY RETURN: data is EMPTY after catalog query");
			return;
		}

		if (cfg && !cfg.m_rVehicleCatalog.IsEmpty() && rules)
		{
			string configuredPrefab = PickPrefabByRules(data, cfg, rules);
			if (!configuredPrefab.IsEmpty())
			{
				m_sPrefab = configuredPrefab;
				if (dbg)
					Print(string.Format("[ARGH_FLYING_CATALOG] Selected prefab from catalog: %1", m_sPrefab));
				return;
			}

			m_sPrefab = string.Empty;
			if (dbg)
				Print("[ARGH_FLYING_CATALOG] Catalog configured but nothing eligible -> no spawn");
			return;
		}

		m_sPrefab = data.GetRandomElement().GetPrefab();
		if (dbg)
			Print(string.Format("[ARGH_FLYING_CATALOG] No catalog loaded -> random fallback: %1", m_sPrefab));
	}
}
