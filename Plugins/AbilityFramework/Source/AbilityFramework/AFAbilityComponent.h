// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameplayTags.h"
#include "AFAbilityTypes.h"

#include "Attributes/GAAttributeBase.h"
#include "Attributes/GAAttributesBase.h"
#include "Effects/GAEffectCueGlobals.h"

#include "Effects/GAGameEffect.h"
#include "GAGlobalTypes.h"

#include "GameplayTagAssetInterface.h"

#include "AssetRegistryModule.h"
#include "Engine/AssetManager.h"

#include "AFAbilityComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("AttributeComponent"), STATGROUP_AttributeComponent, STATCAT_Advanced);
DECLARE_CYCLE_STAT_EXTERN(TEXT("AttributeComponentApplyEffect"), STAT_ApplyEffect, STATGROUP_AttributeComponent, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("AttributeComponentModifyAttribute"), STAT_ModifyAttribute, STATGROUP_AttributeComponent, );

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGAOnAttributeChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGAOnAttributeModifed, const FAFAttributeChangedData&, Mod);

DECLARE_MULTICAST_DELEGATE_OneParam(FGAGenericEffectDelegate, FGAEffectHandle);


DECLARE_MULTICAST_DELEGATE_OneParam(FAFAttributeChangedDelegate, FAFAttributeChangedData);

DECLARE_DELEGATE(FAFOnAbilityReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAFOnAbilityAdded, const FGameplayTag&, AbilityTag);
DECLARE_DELEGATE(FAFGenericAttributeDelegate);


//UAFAssetManager* GetAssetManager()
//{
//	return Cast<UAFASsetManager>(UAssetManager::GetIfValid());
//};

USTRUCT()
struct FGAModifiedAttributeData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
		TArray<FGAModifiedAttribute> Mods;
	UPROPERTY()
		int32 ForceUpdate;

	FGAModifiedAttributeData()
		: ForceUpdate(0)
	{}
};

USTRUCT(BlueprintType)
struct FGAEffectUIData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "UI")
		float RemainingTime;

	FGAEffectUIData()
		: RemainingTime(0)
	{};
};
struct FGAContextSetup
{
public:
	UGAAttributesBase* IntigatorAttributes;
	UGAAttributesBase* TargetAttributes;
	UAFAbilityComponent* InstigatorComp;
	UAFAbilityComponent* TargetComp;

	FGAContextSetup()
	{};
	FGAContextSetup(UGAAttributesBase* IntigatorAttributesIn, UGAAttributesBase* TargetAttributesIn,
		UAFAbilityComponent* InstigatorCompIn, UAFAbilityComponent* TargetCompIn)
		: IntigatorAttributes(IntigatorAttributesIn),
		TargetAttributes(TargetAttributesIn),
		InstigatorComp(InstigatorCompIn),
		TargetComp(TargetCompIn)
	{};
};
DECLARE_MULTICAST_DELEGATE_TwoParams(FGASOnActiveAbilityAdded, int32, int32);
DECLARE_DELEGATE_TwoParams(FAFMontageGenericDelegate, const FGameplayTag&, const FName&);
/* TODO:: Implement fast serialization for structs. */
/* TODO:: REmove all those structs for customization and replace it with something sane like tmap. */
/**/

USTRUCT(BlueprintType)
struct ABILITYFRAMEWORK_API FGASMontageRepData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
		UAnimMontage* CurrentMontage;

	UPROPERTY()
		FName SectionName;

	UPROPERTY()
		uint8 ForceRep;
};

USTRUCT()
struct FAFReplicatedAttributeItem : public FFastArraySerializerItem
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FGameplayTag AttributeTag;
	UPROPERTY()
		UGAAttributesBase* Attributes;

	void PreReplicatedRemove(const struct FAFReplicatedAttributeContainer& InArraySerializer);
	void PostReplicatedAdd(const struct FAFReplicatedAttributeContainer& InArraySerializer);
	void PostReplicatedChange(const struct FAFReplicatedAttributeContainer& InArraySerializer);
};

USTRUCT()
struct FAFReplicatedAttributeContainer : public FFastArraySerializer
{
	GENERATED_BODY()
public:
	UPROPERTY()
		TArray<FAFReplicatedAttributeItem> Attributes;

	UPROPERTY()
		TMap<FGameplayTag, UGAAttributesBase*> AttributeMap;


	TMap<FGameplayTag, FSimpleDelegate> AttributeReplicatedEvent;

	void RegisterAttributeRepEvent(const FGameplayTag& InTag, const FSimpleDelegate& InDelegate)
	{
		if (!AttributeReplicatedEvent.Contains(InTag))
			return;

		AttributeReplicatedEvent.Add(InTag, InDelegate);
	}

	void OnAttributeReplicated(const FGameplayTag& InTag) const
	{
		if (const FSimpleDelegate* Delegate = AttributeReplicatedEvent.Find(InTag))
		{
			Delegate->Execute();
			//AttributeReplicatedEvent.Remove(InTag);
		}
	}

	UGAAttributesBase* Add(const FGameplayTag InTag, UGAAttributesBase* InAttributes, class UAFAbilityComponent* InOuter);
	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FAFReplicatedAttributeItem, FAFReplicatedAttributeContainer>(Attributes, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits< FAFReplicatedAttributeContainer > : public TStructOpsTypeTraitsBase2<FAFReplicatedAttributeContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};


USTRUCT(BlueprintType)
struct ABILITYFRAMEWORK_API FAFAbilityActionSet
{
	GENERATED_BODY()
public:
	UPROPERTY()
		TSoftClassPtr<UGAAbilityBase> AbilityTag;
	UPROPERTY()
		TArray<FGameplayTag> AbilityInputs;
};

UCLASS(hidecategories = (Object, LOD, Lighting, Transform, Sockets, TextureStreaming), editinlinenew, meta = (BlueprintSpawnableComponent))
class ABILITYFRAMEWORK_API UAFAbilityComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()
		/* Attributes handling */
public:
	//Only for base testing and prototyping cue application.
	//will be removed when Cue Manager will be more functional.
	
	//FAFEffectTimerManager EffectTimerManager;

	UPROPERTY(EditAnywhere, Category = "Test")
		FGameplayTag TagTest;
	/*
	Set attribute which will be considered for indicating whether or not actor is dead.
	*/
	UPROPERTY(EditAnywhere, Category = "Config")
		FGAAttribute DeathAttribute;

	UPROPERTY(EditAnywhere, Category = "Input Config")
		TArray<FGameplayTag> AbilityInputs;

	UPROPERTY(EditAnywhere, Category = "Tags")
		FGameplayTagContainer DefaultTags;

	UPROPERTY()
		FGACountedTagContainer AppliedTags;

	UPROPERTY()
		FGACountedTagContainer ImmunityTags;

	UFUNCTION()
		void OnRep_ActiveEffects();

	UPROPERTY(ReplicatedUsing = OnRep_ActiveCues)
		FGameCueContainer ActiveCues;
	UFUNCTION()
		void OnRep_ActiveCues();
	/*
	Could make it array. But realistically. How many times do you really need more, than one
	attribute set for actor ?

	Of course array of attributes would allow to compose attributes from small discreete
	attribute sets. On the other hand similiar funcionality can be achieved by using
	inheritance.

	And I think that using inheritance in this case will be easier.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Replicated)
		class UGAAttributesBase* DefaultAttributes;

	//probabaly replace FGameplayTag with FObjectKey
	TMap<FGameplayTag, class UGAAttributesBase*> AdditionalAttributes;

	UPROPERTY(Replicated)
		FAFReplicatedAttributeContainer RepAttributes;

	UPROPERTY(BlueprintAssignable, Category = "Game Attributes")
		FGAOnAttributeModifed OnAttributeModifed;
	UPROPERTY(BlueprintAssignable, Category = "Game Attributes")
		FGAOnAttributeModifed OnTargetAttributeModifed;

	
	TMap<FGAAttribute, FAFAttributeChangedDelegate> AttributeChanged;


	void BroadcastAttributeChange(const FGAAttribute& InAttribute, 
		const FAFAttributeChangedData& InData);
	
	UFUNCTION()
		void OnRep_GameEffectContainer();

	template<typename T>
	T* GetAttributes()
	{
		return CastChecked<T>(DefaultAttributes);
	}

	template<class T>
	T* GetAttributeSet(FGameplayTag InOwner)
	{
		UGAAttributesBase* AttributeSet = AdditionalAttributes.FindRef(InOwner);
		return Cast<T>(AttributeSet);
	}
	UGAAttributesBase* AddAddtionalAttributes(FGameplayTag InOwner, UGAAttributesBase* InAttributes)
	{
		if (!InAttributes)
			return nullptr;
		UGAAttributesBase* retVal = RepAttributes.Add(InOwner, InAttributes, this);
		RepAttributes.MarkArrayDirty();

		return retVal;
	}
	UFUNCTION(BlueprintCallable, Category = "Test")
		void GetAttributeStructTest(FGAAttribute Name);

	/** UActorComponent Interface - Begin */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DestroyComponent(bool bPromoteChildren = false) override;
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	/* UActorComponent Interface - End **/
public:
	/////////////////////////////////////////////////
	//////////// ATTRIBUTES HANDLING

	/*
	Two functions, They will allow to apply any static numerical mods from player who
	initiated attribute change, and from player who will be affected by change.

	Mods will be appiled by small objects, and changed against tags.
	For example there might be physical armor mod, which will apply changes only
	to attributes tagged as Damage.Physical and only if you are reciving change, not causing it.
	*/

	inline float GetFinalAttributeValue(const FGAAttribute& Name)
	{
		return DefaultAttributes->GetFinalAttributeValue(Name);
	}
	inline float GetCurrentAttributeValue(const FGAAttribute& Name)
	{
		return DefaultAttributes->GetCurrentAttributeValue(Name);
	}
	//////////// ATTRIBUTES HANDLING
	/////////////////////////////////////////////////
	/*
	Attribute replication.
	*/
	void OnAttributeModified(const FGAEffectMod& InMod, const FGAEffectHandle& InHandle, UGAAttributesBase* InAttributeSet);
	//Helper functions:
public:
	/*
	IGameplayTagAssetInterface Start
	*/
	/**
	* Get any owned gameplay tags on the asset
	*
	* @param OutTags	[OUT] Set of tags on the asset
	*/
	UFUNCTION(BlueprintCallable, Category = GameplayTags)
		virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	/*
	IGameplayTagAssetInterface End
	*/

	UFUNCTION(BlueprintCallable, Category = "AbilityFramework|Attributes")
		class UGAAttributesBase* GetAttributes() { return DefaultAttributes; };

	UFUNCTION(BlueprintCallable, Category = "AbilityFramework|Attributes")
		float GetAttributeValue(FGAAttribute AttributeIn) const { return DefaultAttributes->GetCurrentAttributeValue(AttributeIn); };

	void ModifyAttribute(FGAEffectMod& ModIn, const FGAEffectHandle& HandleIn, FGAEffectProperty& InProperty);// { DefaultAttributes->ModifyAttribute(ModIn, HandleIn); };
	void NotifyInstigatorTargetAttributeChanged(const FAFAttributeChangedData& InData, 
		const FGAEffectContext& InContext);
	FAFAttributeBase* GetAttribute(FGAAttribute AttributeIn) { return DefaultAttributes->GetAttribute(AttributeIn); };
	void RemoveBonus(FGAAttribute AttributeIn, const FGAEffectHandle& HandleIn, EGAAttributeMod InMod) { DefaultAttributes->RemoveBonus(AttributeIn, HandleIn, InMod);  };
	float NativeGetAttributeValue(const FGAAttribute AttributeIn) const { return 0; };

private:
	class IAFAbilityInterface* AttributeInterface;

public:
	UAFAbilityComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION()
		void OnRep_InstancedAbilities();
	UPROPERTY(Replicated)
		FAFAbilityContainer AbilityContainer;
	UPROPERTY()
		TArray<UGAAbilityBase*> AbilitiesRefs;

	UPROPERTY()
		APlayerController* PCOwner;

	/* Ability which is currently being executed. */
	UPROPERTY(BlueprintReadOnly, Category = "Game Abilities")
		class UGAAbilityBase* ExecutingAbility;



	/*UFUNCTION(NetMulticast, Reliable)
		MulticastExecuteEffect()*/
	

	

	/*
		True if player is currently casting/channeling/activating(?)
		any ability.
	*/
	bool bIsAnyAbilityActive;

	//AbilityTag, Delegate
	TMap<TSoftClassPtr<UGAAbilityBase>, FAFOnAbilityReady> OnAbilityReadyMap;

	void AddOnAbilityReadyDelegate(const TSoftClassPtr<UGAAbilityBase>& InAbilityPtr, FAFOnAbilityReady& InDelegate)
	{
		if(InDelegate.IsBound())
			OnAbilityReadyMap.Add(InAbilityPtr, InDelegate);
	}

	void NotifyOnAbilityReady(const TSoftClassPtr<UGAAbilityBase>& InAbilityPtr)
	{
		if (FAFOnAbilityReady* Ready = OnAbilityReadyMap.Find(InAbilityPtr))
		{
			Ready->ExecuteIfBound();
			OnAbilityReadyMap.Remove(InAbilityPtr);
		}
	}

	TMap<TSoftClassPtr<UGAAbilityBase>, FAFOnAbilityReady> OnAbilityInputReadyMap;

	void AddOnAbilityInputReadyDelegate(const TSoftClassPtr<UGAAbilityBase>& InAbilityPtr, const FAFOnAbilityReady& InDelegate)
	{
		if(InDelegate.IsBound())
			OnAbilityInputReadyMap.Add(InAbilityPtr, InDelegate);
	}

	void NotifyOnAbilityInputReady(const TSoftClassPtr<UGAAbilityBase>& InAbilityPtr)
	{
		if (FAFOnAbilityReady* Ready = OnAbilityInputReadyMap.Find(InAbilityPtr))
		{
			Ready->ExecuteIfBound();
			OnAbilityInputReadyMap.Remove(InAbilityPtr);
		}
	}

	TMap<FGAAttribute, TArray<FAFGenericAttributeDelegate>> OnPreAttributeModifiedMap;
	void AddOnPreAttributeModifiedDelegate(const FGAAttribute& InAttribute, const FAFGenericAttributeDelegate& InDelegate)
	{
		TArray<FAFGenericAttributeDelegate>& delegates = OnPreAttributeModifiedMap.FindOrAdd(InAttribute);
		delegates.Add(InDelegate);
	}

	void NotifyOnPreAttributeModified(const FGAAttribute& InAttribute)
	{
		if (TArray<FAFGenericAttributeDelegate>* Ready = OnPreAttributeModifiedMap.Find(InAttribute))
		{
			for(const FAFGenericAttributeDelegate& delegate : *Ready)
				delegate.ExecuteIfBound();
		}
	}

	void RemoveOnPreAttributeModified(const FGAAttribute& InAttribute, const FAFGenericAttributeDelegate& Delegate)
	{
		if (TArray<FAFGenericAttributeDelegate>* Ready = OnPreAttributeModifiedMap.Find(InAttribute))
		{
			//Ready->RemoveSingle(Delegate);
			if (Ready->Num() <= 0)
			{
				OnPreAttributeModifiedMap.Remove(InAttribute);
			}
		}
	}

	TMap<FGAAttribute, TArray<FAFGenericAttributeDelegate>> OnPostAttributeModifiedMap;
	void AddOnPostAttributeModifiedDelegate(const FGAAttribute& InAttribute, const FAFGenericAttributeDelegate& InDelegate)
	{
		TArray<FAFGenericAttributeDelegate>& delegates = OnPostAttributeModifiedMap.FindOrAdd(InAttribute);
		delegates.Add(InDelegate);
	}

	void NotifyOnPostAttributeModified(const FGAAttribute& InAttribute)
	{
		if (TArray<FAFGenericAttributeDelegate>* Ready = OnPostAttributeModifiedMap.Find(InAttribute))
		{
			for (const FAFGenericAttributeDelegate& delegate : *Ready)
				delegate.ExecuteIfBound();
		}
	}



	FAFOnAbilityReady OnAbilityReady;
	
	UPROPERTY(BlueprintAssignable, Category = "AbilityFramework")
			FAFOnAbilityAdded OnAbilityAdded;

	FAFMontageGenericDelegate OnAbilityNotifyBegin;
	FAFMontageGenericDelegate OnAbilityNotifyTick;
	FAFMontageGenericDelegate OnAbilityNotifyEnd;

private:
	

	UPROPERTY(ReplicatedUsing=OnRep_PlayMontage)
		FGASMontageRepData RepMontage;
	UFUNCTION()
		void OnRep_PlayMontage();

public:
	UFUNCTION(BlueprintCallable, Category = "AbilityFramework")
		void PlayMontage(UAnimMontage* MontageIn, FName SectionName, float Speed = 1);
	UFUNCTION(NetMulticast, Unreliable)
		void MulticastPlayMontage(UAnimMontage* MontageIn, FName SectionName, float Speed = 1);
public:
	inline class UGAAbilityBase* GetGASAbility(int32 IndexIn)
	{
		return nullptr;
	}

	TMap<FGameplayTag, bool> BlockedInput;

	

	void SetBlockedInput(const FGameplayTag& InActionName, bool bBlock);
	void BindInputs(class UInputComponent* InputComponent);
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Bind Ability To Action"), Category = "AbilityFramework|Abilities")
		void BP_BindAbilityToAction(FGameplayTag ActionName);
	void BindAbilityToAction(UInputComponent* InputComponent, FGameplayTag ActionName);
	
	//need to be called on both client and server.
	//Change InInputTag To Array, clear previous binds on the same tag.
	void SetAbilityToAction(const TSoftClassPtr<UGAAbilityBase>& InAbilityPtr, const TArray<FGameplayTag>& InInputTag, const FAFOnAbilityReady& InputDelegate);

	TSoftClassPtr<UGAAbilityBase> IsAbilityBoundToAction(const TSoftClassPtr<UGAAbilityBase>& InAbilityPtr, const TArray<FGameplayTag>& InInputTag);
	
protected:
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerSetAbilityToAction(const TSoftClassPtr<UGAAbilityBase>& InAbilityPtr, const TArray<FGameplayTag>& InInputTag);
	void ServerSetAbilityToAction_Implementation(const TSoftClassPtr<UGAAbilityBase>& InAbilityPtr, const TArray<FGameplayTag>& InInputTag);
	bool ServerSetAbilityToAction_Validate(const TSoftClassPtr<UGAAbilityBase>& InAbilityPtr, const TArray<FGameplayTag>& InInputTag);
public:
	/* Called when ability action has been binded on server. */
	UFUNCTION(Client, Reliable)
		void ClientNotifyAbilityInputReady(const TSoftClassPtr<UGAAbilityBase>& InAbilityPtr);
	void ClientNotifyAbilityInputReady_Implementation(const TSoftClassPtr<UGAAbilityBase>& InAbilityPtr);


	void SetAbilitiesToActions(const TArray<FAFAbilityActionSet>& InAbilitiesActions, const TArray<FAFOnAbilityReady>& InputDelegate);
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerSetAbilitiesToActions(const TArray<FAFAbilityActionSet>& InAbilitiesActions);
	void ServerSetAbilitiesToActions_Implementation(const TArray<FAFAbilityActionSet>& InAbilitiesActions);
	bool ServerSetAbilitiesToActions_Validate(const TArray<FAFAbilityActionSet>& InAbilitiesActions);


	UFUNCTION(BlueprintCallable, meta=(DisplayName="Input Pressed"), Category = "AbilityFramework|Abilities")
		void BP_InputPressed(FGameplayTag ActionName);

	void NativeInputPressed(FGameplayTag ActionName);
protected:
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerNativeInputPressed(FGameplayTag ActionName, FAFPredictionHandle InPredictionHandle);
	virtual void ServerNativeInputPressed_Implementation(FGameplayTag ActionName, FAFPredictionHandle InPredictionHandle);
	virtual bool ServerNativeInputPressed_Validate(FGameplayTag ActionName, FAFPredictionHandle InPredictionHandle);


public:
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Input Released"), Category = "AbilityFramework|Abilities")
		void BP_InputReleased(FGameplayTag ActionName);

	void NativeInputReleased(FGameplayTag ActionName);
protected:
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerNativeInputReleased(FGameplayTag ActionName);
	virtual void ServerNativeInputReleased_Implementation(FGameplayTag ActionName);
	virtual bool ServerNativeInputReleased_Validate(FGameplayTag ActionName);
public:
	/*
		Finds ability using asset registry and then gives it to component.
		Only valid on server.
		Does not check if component can or can't have given ability.
		So it must be checked before this function is called.
	*/

	UFUNCTION(BlueprintCallable, meta=(DisplayName = "Add Ability"), Category = "AbilityFramework|Ability Component")
	void BP_AddAbility(TSoftClassPtr<UGAAbilityBase> InAbility,
		TArray<FGameplayTag> InInputTag);

	void NativeAddAbility(TSoftClassPtr<UGAAbilityBase> InAbility,
		const TArray<FGameplayTag>& InInputTag);

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerNativeAddAbility(const TSoftClassPtr<UGAAbilityBase>& InAbility,
			const TArray<FGameplayTag>& InInputTag);

	void ServerNativeAddAbility_Implementation(const TSoftClassPtr<UGAAbilityBase>& InAbility,
		const TArray<FGameplayTag>& InInputTag);

	bool ServerNativeAddAbility_Validate(const TSoftClassPtr<UGAAbilityBase>& InAbility,
		const TArray<FGameplayTag>& InInputTag);

	void OnFinishedLoad(TSoftClassPtr<UGAAbilityBase> InAbility);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Remove Ability"), Category = "AbilityFramework|Abilities")
		void BP_RemoveAbility(TSoftClassPtr<UGAAbilityBase> TagIn);

	void NativeRemoveAbility(TSoftClassPtr<UGAAbilityBase> InAbilityTag);
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerNativeRemoveAbility(const TSoftClassPtr<UGAAbilityBase>& InAbilityTag);

	void ServerNativeRemoveAbility_Implementation(const TSoftClassPtr<UGAAbilityBase>& InAbilityTag);

	bool ServerNativeRemoveAbility_Validate(const TSoftClassPtr<UGAAbilityBase>& InAbilityTag);

	//TODO: Make it procted


	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Ability By Tag"), Category = "AbilityFramework|Abilities")
		UGAAbilityBase* BP_GetAbilityByTag(TSoftClassPtr<UGAAbilityBase> TagIn);


	bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;
	void GetSubobjectsWithStableNamesForNetworking(TArray<UObject*>& Objs) override;

	void NotifyOnAbilityAdded(const FGameplayTag& InAbilityTag);
protected:
	void InitializeInstancedAbilities();
	UGAAbilityBase* InstanceAbility(TSubclassOf<class UGAAbilityBase> AbilityClass
		, TSoftClassPtr<class UGAAbilityBase> InClassPtr);


public:
	/*
		Latent Tasks Handling
	*/

	UPROPERTY(Replicated)
		TArray<class UAFTaskBase*> ReplicatedTasks;
};



