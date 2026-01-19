// -----------------------------------------------------------------------------
// ARGH_PersistenceSerializers.c
// 
// All persistence serializers for ARGHSURVIVORS addon.
// Location: ARGH/Shared/Serializers/
// 
// REAL SERIALIZERS:
//   - InventoryStorageManagerComponentSerializer: Saves/restores inventory contents
//   - SCR_DamageManagerComponentSerializer: Saves/restores health state
//   - ARGH_CharacterControllerMetabolismSerializer: Saves/restores metabolism state
//
// STUB SERIALIZERS: (No state to persist, just satisfy config references)
//   - Player/AI components, editor components, etc.
// -----------------------------------------------------------------------------

class StubPersistenceRuleBase {}

class CharacterPersistenceConfigRule : StubPersistenceRuleBase {}
class VehiclePersistenceConfigRule : StubPersistenceRuleBase {}
class EntityPrefabPersistenceConfigRule : StubPersistenceRuleBase {}

class SCR_CharacterSerializer : GenericEntitySerializer {}
class SCR_VehicleSerializer : GenericEntitySerializer {}

// -----------------------------------------------------------------------------
// STUB SERIALIZERS
// -----------------------------------------------------------------------------

class ControlledByComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_PlayerControllerComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_PlayerStateComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_PlayerExperienceComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_AIInformationComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_AudioBlacklistComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class FuelManagerComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_PrefabResourceLoadComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_ArmorManagerComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_AuthoritySwitcherComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_GameModePointComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_LightEditorManagerComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_MannedEditorComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_MarketVillageComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_PowerConsumerEditorComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_PowerSourceEditorComponentSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

class SCR_TriggerEntitySerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return GenericComponent; }
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context) { return ESerializeResult.DEFAULT; }
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context) { return true; }
}

// -----------------------------------------------------------------------------
// REAL SERIALIZER: InventoryStorageManagerComponentSerializer
// 
// Persists inventory contents by saving prefab paths and recreating items.
// Works for player inventories, storage containers, and vehicles.
// -----------------------------------------------------------------------------

//! Data structure for serializing a single inventory item
class ARGH_SerializedInventoryItem
{
	string m_sPrefabPath;
	int m_iQuantity;
	float m_fHealthScaled;
	bool m_bHasHealth;
	
	void ARGH_SerializedInventoryItem()
	{
		m_sPrefabPath = "";
		m_iQuantity = 1;
		m_fHealthScaled = 1.0;
		m_bHasHealth = false;
	}
}

class InventoryStorageManagerComponentSerializer : ScriptedComponentSerializer
{
	// Serializer version for forward compatibility
	protected static const int SERIALIZER_VERSION = 1;
	
	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType() 
	{ 
		return SCR_InventoryStorageManagerComponent; 
	}
	
	//------------------------------------------------------------------------------------------------
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context)
	{
		SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(component);
		if (!invManager)
			return ESerializeResult.DEFAULT;
		
		// Get all items in inventory using non-deprecated API
		array<IEntity> allItems();
		int itemCount = invManager.GetItems(allItems, EStoragePurpose.PURPOSE_ANY);
		
		if (itemCount <= 0)
			return ESerializeResult.DEFAULT;
		
		// Convert to serializable array (store per-item data)
		array<ref ARGH_SerializedInventoryItem> items();
		foreach (IEntity item : allItems)
		{
			if (!item)
				continue;
			
			EntityPrefabData prefabData = item.GetPrefabData();
			if (!prefabData)
				continue;
			
			string prefabPath = prefabData.GetPrefabName();
			if (prefabPath.IsEmpty())
				continue;
			
			ARGH_SerializedInventoryItem entry();
			entry.m_sPrefabPath = prefabPath;
			entry.m_iQuantity = 1;
			
			SCR_DamageManagerComponent dmgManager = SCR_DamageManagerComponent.Cast(item.FindComponent(SCR_DamageManagerComponent));
			if (dmgManager)
			{
				entry.m_bHasHealth = true;
				entry.m_fHealthScaled = dmgManager.GetHealthScaled();
			}
			
			items.Insert(entry);
		}
		
		if (items.IsEmpty())
			return ESerializeResult.DEFAULT;
		
		// Write to context
		context.WriteValue("inv_version", SERIALIZER_VERSION);
		context.WriteValue("inv_count", items.Count());
		
		bool prevTypeDiscrim = context.EnableTypeDiscriminator(false);
		context.WriteValue("inv_items", items);
		context.EnableTypeDiscriminator(prevTypeDiscrim);
		
		return ESerializeResult.OK;
	}
	
	//------------------------------------------------------------------------------------------------
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context)
	{
		SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(component);
		if (!invManager)
			return true;
		
		int version = 0;
		if (!context.ReadValue("inv_version", version))
			return true; // No saved data
		
		int itemCount = 0;
		context.ReadValue("inv_count", itemCount);
		
		if (itemCount <= 0)
			return true;
		
		array<ref ARGH_SerializedInventoryItem> items();
		bool prevTypeDiscrim = context.EnableTypeDiscriminator(false);
		context.ReadValue("inv_items", items);
		context.EnableTypeDiscriminator(prevTypeDiscrim);
		
		if (!items || items.IsEmpty())
			return true;
		
		// Spawn items and insert into inventory
		EntitySpawnParams params();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = owner.GetOrigin();
		
		foreach (ARGH_SerializedInventoryItem entry : items)
		{
			if (!entry || entry.m_sPrefabPath.IsEmpty())
				continue;
			
			Resource resource = Resource.Load(entry.m_sPrefabPath);
			if (!resource || !resource.IsValid())
			{
				Print(string.Format("[ARGH] InventorySerializer: Failed to load prefab: %1", entry.m_sPrefabPath), LogLevel.WARNING);
				continue;
			}
			
			for (int i = 0; i < entry.m_iQuantity; i++)
			{
				IEntity spawnedItem = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
				if (!spawnedItem)
					continue;
			
				if (entry.m_bHasHealth)
				{
					SCR_DamageManagerComponent dmgManager = SCR_DamageManagerComponent.Cast(spawnedItem.FindComponent(SCR_DamageManagerComponent));
					if (dmgManager)
					{
						float clampedHealth = Math.Clamp(entry.m_fHealthScaled, 0.0, 1.0);
						dmgManager.SetHealthScaled(clampedHealth);
					}
				}
				
				bool inserted = invManager.TryInsertItem(spawnedItem);
				if (!inserted)
				{
					// Failed to insert - delete the spawned item
					SCR_EntityHelper.DeleteEntityAndChildren(spawnedItem);
					Print(string.Format("[ARGH] InventorySerializer: Failed to insert item: %1", entry.m_sPrefabPath), LogLevel.WARNING);
				}
			}
		}
		
		return true;
	}
}

// -----------------------------------------------------------------------------
// REAL SERIALIZER: SCR_DamageManagerComponentSerializer
// 
// Persists entity health state. Works for buildings, vehicles, and characters.
// Saves the scaled health value (0.0-1.0) and restores it on load.
// -----------------------------------------------------------------------------

class SCR_DamageManagerComponentSerializer : ScriptedComponentSerializer
{
	// Serializer version for forward compatibility
	protected static const int SERIALIZER_VERSION = 1;
	
	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType() 
	{ 
		return SCR_DamageManagerComponent; 
	}
	
	//------------------------------------------------------------------------------------------------
	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context)
	{
		SCR_DamageManagerComponent dmgManager = SCR_DamageManagerComponent.Cast(component);
		if (!dmgManager)
			return ESerializeResult.DEFAULT;
		
		// Get current health state
		float healthScaled = dmgManager.GetHealthScaled();
		float maxHealth = dmgManager.GetMaxHealth();
		
		// Only save if not at full health (optimization)
		if (healthScaled >= 0.999)
			return ESerializeResult.DEFAULT;
		
		// Write health data
		context.WriteValue("dmg_version", SERIALIZER_VERSION);
		context.WriteValue("dmg_health_scaled", healthScaled);
		context.WriteValue("dmg_max_health", maxHealth);
		
		return ESerializeResult.OK;
	}
	
	//------------------------------------------------------------------------------------------------
	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context)
	{
		SCR_DamageManagerComponent dmgManager = SCR_DamageManagerComponent.Cast(component);
		if (!dmgManager)
			return true;
		
		int version = 0;
		if (!context.ReadValue("dmg_version", version))
			return true; // No saved data
		
		float healthScaled = 1.0;
		float maxHealth = 100.0;
		
		context.ReadValue("dmg_health_scaled", healthScaled);
		context.ReadValue("dmg_max_health", maxHealth);
		
		// Clamp health to valid range
		healthScaled = Math.Clamp(healthScaled, 0.0, 1.0);
		
		// Restore health state
		if (healthScaled < 1.0)
		{
			dmgManager.SetHealthScaled(healthScaled);
		}
		
		return true;
	}
}

// -----------------------------------------------------------------------------
// REAL SERIALIZER: ARGH_CharacterControllerMetabolismSerializer
// 
// Persists character metabolism state (hydration, energy).
// Restores survival meter values on character load.
// -----------------------------------------------------------------------------

class ARGH_CharacterControllerMetabolismSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return SCR_CharacterControllerComponent; }

	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context)
	{
		SCR_CharacterControllerComponent ctrl = SCR_CharacterControllerComponent.Cast(component);
		if (!ctrl)
			return ESerializeResult.DEFAULT;

		context.WriteValue("metab_version", 1);
		context.WriteValue("metab_hydration", ctrl.GetHydration());
		context.WriteValue("metab_energy", ctrl.GetEnergy());
		return ESerializeResult.OK;
	}

	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context)
	{
		SCR_CharacterControllerComponent ctrl = SCR_CharacterControllerComponent.Cast(component);
		if (!ctrl)
			return true;

		int version;
		float hydration = 1.0;
		float energy = 1.0;

		if (context.ReadValue("metab_version", version))
		{
			context.ReadValue("metab_hydration", hydration);
			context.ReadValue("metab_energy", energy);
			ctrl.SetMetabolismLoaded(true);
		}

		ctrl.SetHydration(hydration);
		ctrl.SetEnergy(energy);
		return true;
	}
}
