// BGPlayerState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BGPlayerState.generated.h"

/**
 * 플레이어 개별 데이터(이름, 시도 횟수)를 관리/동기화.
 * PlayerState는 서버 + 모든 클라이언트에 존재하므로 공개 데이터를 두기 좋다.
 */
UCLASS()
class BASEBALLGAME_API ABGPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ABGPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	//Player1 형태의 표시용 문자열 반환 (Getter)
	FString GetPlayerInfoString() const;

public:
	//접속 순서대로 부여되는 이름 (Player1, Player2 ...)
	UPROPERTY(Replicated)
	FString PlayerNameString;

	//사용한 시도 횟수
	UPROPERTY(Replicated)
	int32 CurrentGuessCount;

	//최대 시도 횟수
	UPROPERTY(Replicated)
	int32 MaxGuessCount;
};
