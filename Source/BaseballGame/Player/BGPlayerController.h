// BGPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BGPlayerController.generated.h"

class UBGChatInput;
class UUserWidget;

/**
 * 플레이어 입력 처리 및 서버/클라 통신(RPC) 담당.
 */
UCLASS()
class BASEBALLGAME_API ABGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABGPlayerController();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	//위젯에서 호출 : 입력한 채팅(추측) 원문을 서버로 전송
	void SetChatMessageString(const FString& InChatMessageString);

	//화면에 한 줄 출력
	void PrintChatMessageString(const FString& InChatMessageString);

	//서버 => 이 클라이언트 : 한 줄 출력 요청
	UFUNCTION(Client, Reliable)
	void ClientRPCPrintChatMessageString(const FString& InChatMessageString);

	//클라이언트 => 서버 : 추측 처리 요청
	UFUNCTION(Server, Reliable)
	void ServerRPCPrintChatMessageString(const FString& InChatMessageString);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "BaseballGame")
	TSubclassOf<UBGChatInput> ChatInputWidgetClass;

	UPROPERTY()
	TObjectPtr<UBGChatInput> ChatInputWidgetInstance;

	UPROPERTY(EditDefaultsOnly, Category = "BaseballGame")
	TSubclassOf<UUserWidget> NotificationTextWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> NotificationTextWidgetInstance;

public:
	//승리/무승부/접속 등 공지. 위젯 Text에 바인딩하여 표시.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "BaseballGame")
	FText NotificationText;
};
