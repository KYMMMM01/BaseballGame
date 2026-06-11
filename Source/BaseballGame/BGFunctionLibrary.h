//BGFunctionLibrary.h
//숫자 야구 게임 공용 헬퍼 (C++ 라이브러리)

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

/**
 * 네트워크 디버깅/출력용 정적 헬퍼 모음.
 * 클라이언트/리슨서버 : 화면에 메시지 출력
 * 데디케이티드 서버 : 로그로 출력
 */
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
