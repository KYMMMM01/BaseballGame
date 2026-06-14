//BGFunctionLibrary.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"


class BGFunctionLibrary
{
public:
	static void MyPrintString(const AActor* InWorldContextActor, const FString& InString, float InTimeToDisplay = 2.f, FColor InColor = FColor::Cyan)
	{
		if (IsValid(GEngine) == true && IsValid(InWorldContextActor) == true)
		{
			const ENetMode NetMode = InWorldContextActor->GetNetMode();
			if (NetMode == NM_Client || NetMode == NM_ListenServer || NetMode == NM_Standalone)
			{
				GEngine->AddOnScreenDebugMessage(-1, InTimeToDisplay, InColor, InString);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("%s"), *InString);
			}
		}
	}

	static FString GetNetModeString(const AActor* InWorldContextActor)
	{
		FString NetModeString = TEXT("None");
		if (IsValid(InWorldContextActor) == true)
		{
			const ENetMode NetMode = InWorldContextActor->GetNetMode();
			if (NetMode == NM_Client)
			{
				NetModeString = TEXT("Client");
			}
			else if (NetMode == NM_Standalone)
			{
				NetModeString = TEXT("Standalone");
			}
			else
			{
				NetModeString = TEXT("Server");
			}
		}
		return NetModeString;
	}

	static FString GetRoleString(const AActor* InActor)
	{
		FString RoleString = TEXT("None");
		if (IsValid(InActor) == true)
		{
			const FString LocalRoleString = UEnum::GetValueAsString(TEXT("Engine.ENetRole"), InActor->GetLocalRole());
			const FString RemoteRoleString = UEnum::GetValueAsString(TEXT("Engine.ENetRole"), InActor->GetRemoteRole());
			RoleString = FString::Printf(TEXT("%s / %s"), *LocalRoleString, *RemoteRoleString);
		}
		return RoleString;
	}
};
