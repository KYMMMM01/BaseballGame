//BGGameStateBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BGGameStateBase.generated.h"

/**
 * 모든 클라이언트에게 동시에 알릴 시스템 메시지(접속 알림 등)를 담당.
 * 턴 타이머(남은 시간 / 현재 턴 플레이어)도 여기서 복제한다.
 * GameState는 서버+모든 클라에 존재하고 복제 빈도가 높아 실시간 타이머에 적합하다.
 */
UCLASS()
class BASEBALLGAME_API ABGGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	//서버 + 모든 클라에서 실행되는 멀티캐스트 RPC
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPCBroadcastSystemMessage(const FString& InMessageString);

public:
	//현재 턴 플레이어의 남은 시간(초). 서버가 1초마다 줄여 전 클라에 복제된다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Baseball")
	int32 RemainingTime = 0;

	//현재 턴인 플레이어 이름 (UI 표시용)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Baseball")
	FString CurrentTurnPlayerNameString;
};
