// BGGameModeBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BGGameModeBase.generated.h"

class ABGPlayerController;

//입력 검증 결과 (사유별 안내 메시지를 위해 구분)
enum class EGuessValidity : uint8
{
	Valid, //유효한 추측
	WrongLength, //3자리가 아님
	NotDigit, //1~9 외의 문자 또는 0 포함
	Duplicated //중복되는 숫자 존재
};

/**
 * 게임의 모든 규칙을 서버에서 제어한다.
 * - 정답 생성, 입력 검증, S/B/OUT 판정, 승리/무승부/리셋
 * GameMode는 서버에만 존재하므로 정답은 클라이언트가 볼 수 없다.
 */
UCLASS()
class BASEBALLGAME_API ABGGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABGGameModeBase();

	virtual void OnPostLogin(AController* NewPlayer) override;
	virtual void BeginPlay() override;

	//채팅(추측) 처리 진입점 => 서버에서 호출됨
	void ProcessGuess(ABGPlayerController* InChattingPlayerController, const FString& InRawMessageString);

protected:
	FString GenerateSecretNumber() const; //1~9 중복 없는 3자리 생성
	EGuessValidity ValidateGuessString(const FString& InGuessString) const; //입력 검증
	FString JudgeResult(const FString& InSecret, const FString& InGuess) const; //판정 결과
	FString GetValidationMessage(EGuessValidity InValidity) const; //검증 실패 안내문

	void IncreaseGuessCount(ABGPlayerController* InChattingPlayerController);
	void JudgeGame(ABGPlayerController* InChattingPlayerController, int32 InStrikeCount); //게임 결과 
	void ResetGame();                                                         

	
	void SendSystemMessageToController(ABGPlayerController* InTargetController, const FString& InMessageString); //특정 1명에게
	void BroadcastChatMessage(const FString& InMessageString); //전원에게 채팅 한 줄
	void SetNotificationToAll(const FString& InMessageString); //전원 공지 위젯 갱신

protected:
	//서버만 알고 있는 정답
	FString SecretNumberString;

	//접속한 플레이어 컨트롤러 목록 (접속 순서 = 번호)
	UPROPERTY()
	TArray<TObjectPtr<ABGPlayerController>> AllPlayerControllers;
};
