	// =============================================================
	// ARGH_MO_SCR_AmbientFlyingSpawnPointComponent.c
	//
	// Reorganized for core-grouping like vehicles, but simplified:
	// - Uses a prebuilt catalog (no rebuild pipeline here).
	// - Catalog is now grouped: Root -> Models -> Variants.
	// - Still supports legacy flat list if present.
	// =============================================================

	// -----------------------------
	// Variant rule (leaf)
	// -----------------------------
	[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sDisplayName")]
	class ARGH_AmbientFlyingSpawnRule : Managed
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
	// Group node (model)
	// -----------------------------
	[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sName")]
	class ARGH_AmbientFlyingGroup : Managed
	{
		[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Group name (e.g. MI8, UH60, ...) ")]
		string m_sName;

		[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Enable/disable this entire group (affects children too)")]
		bool m_bEnabled;

		[Attribute(defvalue: "{}", desc: "Flying variants in this group")]
		ref array<ref ARGH_AmbientFlyingSpawnRule> m_aVehicles;

		[Attribute(defvalue: "{}", desc: "Child groups")]
		ref array<ref ARGH_AmbientFlyingGroup> m_aChildren;
	}

	// -----------------------------
	// Catalog config (root)
	// -----------------------------
	[BaseContainerProps(configRoot: true)]
	class ARGH_AmbientFlyingCatalogConfig : Managed
	{
		[Attribute(defvalue: "{}", desc: "Grouped flying catalog (Root -> Models -> Variants)")]
		ref array<ref ARGH_AmbientFlyingGroup> m_aGroups;

		[Attribute(defvalue: "{}", desc: "(Legacy) Flat flying catalog entries")]
		ref array<ref ARGH_AmbientFlyingSpawnRule> m_aVehicles;
	}

	modded class SCR_AmbientFlyingSpawnPointComponent : ScriptComponent
	{
		static const ResourceName ARGH_AMBIENT_FLYING_CATALOG = "{23830C67D5982CC5}Configs/EntityCatalog/ARGH_AmbientFlying.conf";

		static ResourceName s_CatalogConfigPath;
		static ref ARGH_AmbientFlyingCatalogConfig s_AmbientFlyingCatalogConfig;

		private static void FlattenEnabledGroups(array<ref ARGH_AmbientFlyingGroup> groups, array<ref ARGH_AmbientFlyingSpawnRule> outRules, bool parentEnabled)
		{
			if (!groups || !outRules)
				return;

			foreach (ARGH_AmbientFlyingGroup g : groups)
			{
				if (!g)
					continue;

				bool enabled = parentEnabled && g.m_bEnabled;
				if (!enabled)
					continue;

				if (g.m_aVehicles)
				{
					foreach (ARGH_AmbientFlyingSpawnRule r : g.m_aVehicles)
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

		private ARGH_AmbientFlyingCatalogConfig LoadAmbientFlyingCatalog(ResourceName path)
		{
			if (path.IsEmpty())
				return null;

			if (s_AmbientFlyingCatalogConfig && s_CatalogConfigPath == path)
				return s_AmbientFlyingCatalogConfig;

			s_CatalogConfigPath = path;
			s_AmbientFlyingCatalogConfig = null;

			Resource res = Resource.Load(path);
			if (!res)
				return null;

			BaseContainer bc = res.GetResource().ToBaseContainer();
			if (!bc)
				return null;

			ARGH_AmbientFlyingCatalogConfig cfg = new ARGH_AmbientFlyingCatalogConfig();

			cfg.m_aGroups = {};
			array<ref ARGH_AmbientFlyingGroup> groups = {};
			if (bc.Get("m_aGroups", groups))
				cfg.m_aGroups = groups;

			cfg.m_aVehicles = {};
			array<ref ARGH_AmbientFlyingSpawnRule> rules = {};
			if (bc.Get("m_aVehicles", rules))
				cfg.m_aVehicles = rules;

			s_AmbientFlyingCatalogConfig = cfg;
			return s_AmbientFlyingCatalogConfig;
		}

		private array<ref ARGH_AmbientFlyingSpawnRule> ResolvePrefabRules()
		{
			ARGH_AmbientFlyingCatalogConfig catalog = LoadAmbientFlyingCatalog(ARGH_AMBIENT_FLYING_CATALOG);
			if (!catalog)
				return null;

			if (catalog.m_aGroups && catalog.m_aGroups.Count() > 0)
			{
				array<ref ARGH_AmbientFlyingSpawnRule> flat = {};
				FlattenEnabledGroups(catalog.m_aGroups, flat, true);
				return flat;
			}

			if (catalog.m_aVehicles && catalog.m_aVehicles.Count() > 0)
				return catalog.m_aVehicles;

			return {};
		}

		private string PickPrefabByRules(array<SCR_EntityCatalogEntry> data, array<ref ARGH_AmbientFlyingSpawnRule> rules, bool forceIgnoreLabels = false)
		{
			if (!rules || rules.IsEmpty())
				return string.Empty;

			array<float> weights = {};
			float totalWeight = 0.0;
			array<ref ARGH_AmbientFlyingSpawnRule> candidates = {};
			bool ignoreLabels = forceIgnoreLabels;

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

			foreach (ARGH_AmbientFlyingSpawnRule rule : rules)
			{
				if (!rule)
					continue;

				if (!rule.m_bEnabled)
					continue;

				float weight = rule.m_fSpawnWeight;
				if (weight <= 0.0)
					continue;

				if (!ignoreLabels)
				{
					string normalized = NormalizePrefabPath(rule.m_rPrefab);
					bool isAllowed;
					if (normalized.IsEmpty() || !allowed.Find(normalized, isAllowed) || !isAllowed)
						continue;
				}

				candidates.Insert(rule);
				weights.Insert(weight);
				totalWeight += weight;
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

		private string NormalizePrefabPath(string prefabPath)
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

		override void ForceUpdate(SCR_Faction faction)
		{
			return Update(faction);
		}

		override protected void Update(SCR_Faction faction)
		{
			m_SavedFaction = faction;

			SCR_EntityCatalog entityCatalog = SCR_EntityCatalog.GetEditorInstance("FlyingList");
			array<SCR_EntityCatalogEntry> data = {};
			entityCatalog.GetFullFilteredEntityListWithLabels(
				data,
				m_aIncludedEditableEntityLabels,
				m_aExcludedEditableEntityLabels,
				m_bRequireAllIncludedLabels
			);

			if (data.IsEmpty())
				return;

			array<ref ARGH_AmbientFlyingSpawnRule> rules = ResolvePrefabRules();
			if (rules)
			{
				string configuredPrefab = PickPrefabByRules(data, rules);
				if (!configuredPrefab.IsEmpty())
				{
					m_sPrefab = configuredPrefab;
					return;
				}
			}

			m_sPrefab = data.GetRandomElement().GetPrefab();
		}
	}
