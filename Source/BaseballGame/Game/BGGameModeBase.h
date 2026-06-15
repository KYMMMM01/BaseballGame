// BGGameModeBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/TimerHandle.h"
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
	bool JudgeGame(ABGPlayerController* InChattingPlayerController, int32 InStrikeCount); //게임 결과 (라운드 종료 시 true)
	void ResetGame();

	//턴 제어 (도전 기능)
	void StartTurn(int32 InTurnIndex); //해당 인덱스 플레이어의 턴 시작
	void AdvanceTurn(); //기회가 남은 다음 플레이어로 턴을 넘김
	ABGPlayerController* GetCurrentTurnController() const; //현재 턴 플레이어 컨트롤러

	//턴 타이머 (도전 기능 - 제한 시간)
	void OnTurnTimerTick(); //1초마다 호출 => 남은 시간 감소
	void HandleTurnTimeout(); //시간 초과 => 기회 소모 후 턴 넘김

	void SendSystemMessageToController(ABGPlayerController* InTargetController, const FString& InMessageString); //특정 1명에게
	void BroadcastChatMessage(const FString& InMessageString); //전원에게 채팅 한 줄
	void SetNotificationToAll(const FString& InMessageString); //전원 공지 위젯 갱신

protected:
	//서버만 알고 있는 정답
	FString SecretNumberString;

	//접속한 플레이어 컨트롤러 목록 (접속 순서 = 번호)
	UPROPERTY()
	TArray<TObjectPtr<ABGPlayerController>> AllPlayerControllers;

	//현재 턴인 플레이어 인덱스 (AllPlayerControllers 기준)
	int32 CurrentTurnIndex = 0;

	//2명 이상 모여 턴제가 활성화됐는지
	bool bIsTurnActive = false;

	//턴당 제한 시간(초). BP 디테일 패널에서 조절 가능
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Baseball")
	int32 TurnTimeLimit = 30;

	//턴 타이머 핸들 (1초 간격 반복 호출)
	FTimerHandle TurnTimerHandle;
};
