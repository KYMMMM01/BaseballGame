//BGGameStateBase.cpp

#include "Game/BGGameStateBase.h"

#include "Player/BGPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

void ABGGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//남은 시간과 현재 턴 플레이어 이름을 모든 클라에 복제
	DOREPLIFETIME(ABGGameStateBase, RemainingTime);
	DOREPLIFETIME(ABGGameStateBase, CurrentTurnPlayerNameString);
}

void ABGGameStateBase::MulticastRPCBroadcastSystemMessage_Implementation(const FString& InMessageString)
{
	//이 함수는 서버와 모든 클라이언트의 머신에서 각각 실행된다.
	//각 머신에서 자신의 로컬 플레이어 컨트롤러를 찾아 화면에 메시지를 출력한다.
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ABGPlayerController* BGPlayerController = Cast<ABGPlayerController>(PlayerController);
	if (IsValid(BGPlayerController) == true)
	{
		BGPlayerController->PrintChatMessageString(InMessageString);
	}
}
