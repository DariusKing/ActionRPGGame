// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ARUMGWidgetBase.h"
#include "GameplayTags.h"
#include "AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "AMTypes.h"
#include "../Abilities/ARAbilityManagerComponent.h"
#include "ARAbilityWidget.generated.h"

/**
 * 
 */
UCLASS()
class ACTIONRPGGAME_API UARAbilityWidget : public UARUMGWidgetBase
{
	GENERATED_BODY()
public:
	TWeakObjectPtr<class UARAbilityManagerComponent> AbilityManager;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
};
