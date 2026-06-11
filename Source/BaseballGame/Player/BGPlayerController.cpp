//BGPlayerController.cpp

#include "Player/BGPlayerController.h"

#include "UI/BGChatInput.h"
#include "Game/BGGameModeBase.h"
#include "BGFunctionLibrary.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ABGPlayerController::ABGPlayerController()
{
	bReplicates = true;
}

void ABGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	//UI는 내 화면(로컬 컨트롤러)에서만 생성한다.
	if (IsLocalController() == false)
	{
		return;
	}

	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	//채팅 입력 위젯
	if (IsValid(ChatInputWidgetClass) == true)
	{
		ChatInputWidgetInstance = CreateWidget<UBGChatInput>(this, ChatInputWidgetClass);
		if (IsValid(ChatInputWidgetInstance) == true)
		{
			ChatInputWidgetInstance->AddToViewport();
		}
	}

	//공지 위젯
	if (IsValid(NotificationTextWidgetClass) == true)
	{
		NotificationTextWidgetInstance = CreateWidget<UUserWidget>(this, NotificationTextWidgetClass);
		if (IsValid(NotificationTextWidgetInstance) == true)
		{
			NotificationTextWidgetInstance->AddToViewport();
		}
	}
}

void ABGPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, NotificationText);
}

void ABGPlayerController::SetChatMessageString(const FString& InChatMessageString)
{
	//로컬에서 입력한 원문만 서버로 보낸다.
	//이름/시도횟수 표기는 서버가 판정 후 직접 붙인다(카운트 표시 지연 방지).
	if (IsLocalController() == true)
	{
		ServerRPCPrintChatMessageString(InChatMessageString);
	}
}

void ABGPlayerController::PrintChatMessageString(const FString& InChatMessageString)
{
	BGFunctionLibrary::MyPrintString(this, InChatMessageString, 10.f);
}

void ABGPlayerController::ServerRPCPrintChatMessageString_Implementation(const FString& InChatMessageString)
{
	//서버에서 실행. 게임 규칙/판정은 GameMode가 담당한다.
	ABGGameModeBase* BGGameMode = Cast<ABGGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (IsValid(BGGameMode) == true)
	{
		BGGameMode->ProcessGuess(this, InChatMessageString);
	}
}

void ABGPlayerController::ClientRPCPrintChatMessageString_Implementation(const FString& InChatMessageString)
{
	PrintChatMessageString(InChatMessageString);
}
