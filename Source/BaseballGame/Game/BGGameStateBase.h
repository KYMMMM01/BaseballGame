//BGGameStateBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BGGameStateBase.generated.h"

/**
 * 모든 클라이언트에게 동시에 알릴 시스템 메시지(접속 알림 등)를 담당.
 */
UCLASS()
class BASEBALLGAME_API ABGGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	//서버 + 모든 클라에서 실행되는 멀티캐스트 RPC
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPCBroadcastSystemMessage(const FString& InMessageString);
};
