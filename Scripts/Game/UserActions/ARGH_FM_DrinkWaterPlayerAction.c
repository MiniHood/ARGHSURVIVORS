// File: FM_DrinkWaterPlayerAction.c
modded class FM_DrinkWaterPlayerAction
{
	[Attribute(defvalue: "20.0", desc: "Hydration added per use. Can be negative for dehydration.")]
	protected float m_iWaterValue;
	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		ApplyHydrationEffect(pUserEntity);
	}
	
	override void ApplyHydrationEffect(IEntity characterOwner)
	{
		if (characterOwner)
		{
			IEntity resolvedOwner = characterOwner;
			SCR_PlayerController pc = SCR_PlayerController.Cast(characterOwner);
			if (pc)
			{
				IEntity controlled = pc.GetControlledEntity();
				if (!controlled)
					controlled = pc.GetMainEntity();

				if (controlled)
					resolvedOwner = controlled;
			}

			if (resolvedOwner && !SCR_CharacterControllerComponent.Cast(resolvedOwner.FindComponent(SCR_CharacterControllerComponent)))
			{
				IEntity parent = resolvedOwner.GetParent();
				if (parent)
					resolvedOwner = parent;
			}

			SCR_CharacterControllerComponent metabolismComponent = SCR_CharacterControllerComponent.Cast(resolvedOwner.FindComponent(SCR_CharacterControllerComponent));
			if (metabolismComponent)
			{
				Print("SCR_CharacterControllerComponent found.");
				
				// Apply hydration changes on server (RPC if needed)
				metabolismComponent.RequestHydrationChange(m_iWaterValue);
				
				float currentHydration = metabolismComponent.GetHydration();
				Print("Increased hydration by " + m_iWaterValue);
				Print("Current Hydration: " + currentHydration);
			}
			else
			{
				Print("SCR_CharacterControllerComponent not found on character.");
			}
		}
		else
		{
			Print("Character not found.");
		}
	}
}
