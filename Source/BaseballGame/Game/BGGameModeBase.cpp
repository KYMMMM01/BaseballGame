//BGGameModeBase.cpp

#include "Game/BGGameModeBase.h"

#include "Game/BGGameStateBase.h"
#include "Player/BGPlayerController.h"
#include "Player/BGPlayerState.h"
#include "TimerManager.h"

ABGGameModeBase::ABGGameModeBase()
{
	//이 GameMode가 사용할 기본 클래스들을 C++에서 지정 (BP에서 덮어쓸 수 있음)
	PlayerControllerClass = ABGPlayerController::StaticClass();
	PlayerStateClass = ABGPlayerState::StaticClass();
	GameStateClass = ABGGameStateBase::StaticClass();

	//채팅/UI 기반 게임이라 별도의 폰은 사용하지 않는다.
	DefaultPawnClass = nullptr;
}

void ABGGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	SecretNumberString = GenerateSecretNumber();
	UE_LOG(LogTemp, Warning, TEXT("[BaseballGame] Secret Number = %s"), *SecretNumberString);
}

void ABGGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	ABGPlayerController* BGPlayerController = Cast<ABGPlayerController>(NewPlayer);
	if (IsValid(BGPlayerController) == true)
	{
		AllPlayerControllers.Add(BGPlayerController);

		//접속 순서대로 Player1, Player2 ... 이름 부여
		ABGPlayerState* BGPlayerState = BGPlayerController->GetPlayerState<ABGPlayerState>();
		if (IsValid(BGPlayerState) == true)
		{
			BGPlayerState->PlayerNameString = FString::Printf(TEXT("Player%d"), AllPlayerControllers.Num());

			//접속 사실을 전원에게 알림 (NetMulticast)
			ABGGameStateBase* BGGameState = GetGameState<ABGGameStateBase>();
			if (IsValid(BGGameState) == true)
			{
				BGGameState->MulticastRPCBroadcastSystemMessage(BGPlayerState->PlayerNameString + TEXT(" 님이 접속했습니다."));
			}
		}

		//2명이 모이면 턴제 시작 (Player1부터)
		if (bIsTurnActive == false && AllPlayerControllers.Num() >= 2)
		{
			StartTurn(0);
		}
	}
}

void ABGGameModeBase::ProcessGuess(ABGPlayerController* InChattingPlayerController, const FString& InRawMessageString)
{
	if (IsValid(InChattingPlayerController) == false)
	{
		return;
	}

	ABGPlayerState* BGPlayerState = InChattingPlayerController->GetPlayerState<ABGPlayerState>();
	if (IsValid(BGPlayerState) == false)
	{
		return;
	}

	//턴제가 켜져 있으면, 자기 턴이 아닐 때 입력 거부
	if (bIsTurnActive == true && InChattingPlayerController != GetCurrentTurnController())
	{
		SendSystemMessageToController(InChattingPlayerController, TEXT("아직 당신의 턴이 아닙니다."));
		return;
	}

	const FString GuessString = InRawMessageString.TrimStartAndEnd();

	//기회 소진 검사 (방어 코드)
	if (BGPlayerState->CurrentGuessCount >= BGPlayerState->MaxGuessCount)
	{
		SendSystemMessageToController(InChattingPlayerController, TEXT("기회를 모두 소진했습니다. 다음 라운드를 기다려주세요."));
		return;
	}

	//입력 검증
	const EGuessValidity Validity = ValidateGuessString(GuessString);
	if (Validity != EGuessValidity::Valid)
	{
		SendSystemMessageToController(InChattingPlayerController, GetValidationMessage(Validity));
		return;
	}

	//유효한 추측 => 기회 1 소모 후 판정
	IncreaseGuessCount(InChattingPlayerController);

	const FString JudgeResultString = JudgeResult(SecretNumberString, GuessString);

	//결과를 전원에게 방송. 카운트는 방금 증가된 서버 값을 사용하므로 즉시 반영된다.
	const FString LineString = FString::Printf(TEXT("%s: %s -> %s"),
		*BGPlayerState->GetPlayerInfoString(), *GuessString, *JudgeResultString);
	BroadcastChatMessage(LineString);

	//승리 / 무승부 판정 (라운드 종료 시 true). 종료면 ResetGame에서 턴이 다시 세팅된다.
	const int32 StrikeCount = FCString::Atoi(*JudgeResultString.Left(1));
	const bool bRoundEnded = JudgeGame(InChattingPlayerController, StrikeCount);

	//라운드가 계속되면 다음 플레이어로 턴을 넘긴다
	if (bIsTurnActive == true && bRoundEnded == false)
	{
		AdvanceTurn();
	}
}

FString ABGGameModeBase::GenerateSecretNumber() const
{
	TArray<int32> Numbers;
	for (int32 i = 1; i <= 9; ++i)
	{
		Numbers.Add(i);
	}

	FMath::RandInit(FDateTime::Now().GetTicks());

	FString Result;
	for (int32 i = 0; i < 3; ++i)
	{
		const int32 Index = FMath::RandRange(0, Numbers.Num() - 1);
		Result.Append(FString::FromInt(Numbers[Index]));
		Numbers.RemoveAt(Index);
	}
	return Result;
}

EGuessValidity ABGGameModeBase::ValidateGuessString(const FString& InGuessString) const
{
	if (InGuessString.Len() != 3)
	{
		return EGuessValidity::WrongLength;
	}

	TSet<TCHAR> SeenDigits;
	for (TCHAR C : InGuessString)
	{
		//1~9 숫자인가? (문자/0 불가)
		if (FChar::IsDigit(C) == false || C == TCHAR('0'))
		{
			return EGuessValidity::NotDigit;
		}

		//중복 검사
		if (SeenDigits.Contains(C) == true)
		{
			return EGuessValidity::Duplicated;
		}
		SeenDigits.Add(C);
	}

	return EGuessValidity::Valid;
}

FString ABGGameModeBase::GetValidationMessage(EGuessValidity InValidity) const
{
	switch (InValidity)
	{
	case EGuessValidity::WrongLength: return TEXT("세 자리 숫자를 입력해주세요.");
	case EGuessValidity::NotDigit:    return TEXT("1~9 사이의 숫자만 입력할 수 있어요.");
	case EGuessValidity::Duplicated:  return TEXT("중복되지 않은 숫자를 입력해주세요.");
	default:                          return FString();
	}
}

FString ABGGameModeBase::JudgeResult(const FString& InSecret, const FString& InGuess) const
{
	int32 StrikeCount = 0;
	int32 BallCount = 0;

	for (int32 i = 0; i < 3; ++i)
	{
		if (InSecret[i] == InGuess[i])
		{
			++StrikeCount;
		}
		else
		{
			const FString GuessChar = FString::Printf(TEXT("%c"), InGuess[i]);
			if (InSecret.Contains(GuessChar) == true)
			{
				++BallCount;
			}
		}
	}

	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}
	return FString::Printf(TEXT("%dS %dB"), StrikeCount, BallCount);
}

void ABGGameModeBase::IncreaseGuessCount(ABGPlayerController* InChattingPlayerController)
{
	ABGPlayerState* BGPlayerState = InChattingPlayerController->GetPlayerState<ABGPlayerState>();
	if (IsValid(BGPlayerState) == true)
	{
		++BGPlayerState->CurrentGuessCount;
	}
}

bool ABGGameModeBase::JudgeGame(ABGPlayerController* InChattingPlayerController, int32 InStrikeCount)
{
	//승리
	if (InStrikeCount == 3)
	{
		ABGPlayerState* WinnerState = InChattingPlayerController->GetPlayerState<ABGPlayerState>();
		const FString WinnerName = IsValid(WinnerState) ? WinnerState->PlayerNameString : TEXT("Someone");

		SetNotificationToAll(FString::Printf(TEXT("%s 님이 승리했습니다!"), *WinnerName));
		BroadcastChatMessage(FString::Printf(TEXT("=== %s 님 승리! (정답: %s) ==="), *WinnerName, *SecretNumberString));
		ResetGame();
		return true;
	}

	//무승부 (모든 플레이어가 기회를 소진)
	bool bIsDraw = true;
	for (const TObjectPtr<ABGPlayerController>& PlayerController : AllPlayerControllers)
	{
		if (IsValid(PlayerController) == false)
		{
			continue;
		}

		ABGPlayerState* BGPlayerState = PlayerController->GetPlayerState<ABGPlayerState>();
		if (IsValid(BGPlayerState) == true && BGPlayerState->CurrentGuessCount < BGPlayerState->MaxGuessCount)
		{
			bIsDraw = false;
			break;
		}
	}
	if (bIsDraw == true)
	{
		SetNotificationToAll(TEXT("무승부! 아무도 정답을 맞히지 못했습니다."));
		BroadcastChatMessage(FString::Printf(TEXT("=== 무승부! (정답: %s) ==="), *SecretNumberString));
		ResetGame();
		return true;
	}

	return false;
}

void ABGGameModeBase::ResetGame()
{
	//새 정답 생성
	SecretNumberString = GenerateSecretNumber();
	UE_LOG(LogTemp, Warning, TEXT("[BaseballGame] (Reset) Secret Number = %s"), *SecretNumberString);

	//모든 플레이어 시도 횟수 초기화 (Replicated => 클라 UI도 0으로 복귀)
	for (const TObjectPtr<ABGPlayerController>& PlayerController : AllPlayerControllers)
	{
		if (IsValid(PlayerController) == false)
		{
			continue;
		}

		ABGPlayerState* BGPlayerState = PlayerController->GetPlayerState<ABGPlayerState>();
		if (IsValid(BGPlayerState) == true)
		{
			BGPlayerState->CurrentGuessCount = 0;
		}
	}

	BroadcastChatMessage(TEXT("새 라운드가 시작되었습니다! 새로운 정답을 맞혀보세요."));

	//새 라운드는 Player1(인덱스 0)부터 시작
	if (bIsTurnActive == true)
	{
		StartTurn(0);
	}
}

void ABGGameModeBase::SendSystemMessageToController(ABGPlayerController* InTargetController, const FString& InMessageString)
{
	if (IsValid(InTargetController) == true)
	{
		InTargetController->ClientRPCPrintChatMessageString(InMessageString);
	}
}

void ABGGameModeBase::BroadcastChatMessage(const FString& InMessageString)
{
	for (const TObjectPtr<ABGPlayerController>& PlayerController : AllPlayerControllers)
	{
		if (IsValid(PlayerController) == true)
		{
			PlayerController->ClientRPCPrintChatMessageString(InMessageString);
		}
	}
}

void ABGGameModeBase::SetNotificationToAll(const FString& InMessageString)
{
	const FText MessageText = FText::FromString(InMessageString);
	for (const TObjectPtr<ABGPlayerController>& PlayerController : AllPlayerControllers)
	{
		if (IsValid(PlayerController) == true)
		{
			//NotificationText는 Replicated => 각 클라의 공지 위젯 바인딩이 자동 갱신
			PlayerController->NotificationText = MessageText;
		}
	}
}

ABGPlayerController* ABGGameModeBase::GetCurrentTurnController() const
{
	if (AllPlayerControllers.IsValidIndex(CurrentTurnIndex) == true)
	{
		return AllPlayerControllers[CurrentTurnIndex];
	}
	return nullptr;
}

void ABGGameModeBase::StartTurn(int32 InTurnIndex)
{
	if (AllPlayerControllers.IsValidIndex(InTurnIndex) == false)
	{
		return;
	}

	CurrentTurnIndex = InTurnIndex;
	bIsTurnActive = true;

	//현재 턴 플레이어 이름을 전원에게 안내
	ABGPlayerController* TurnController = GetCurrentTurnController();
	FString TurnName = TEXT("Someone");
	if (IsValid(TurnController) == true)
	{
		ABGPlayerState* TurnState = TurnController->GetPlayerState<ABGPlayerState>();
		if (IsValid(TurnState) == true)
		{
			TurnName = TurnState->PlayerNameString;
		}
		BroadcastChatMessage(FString::Printf(TEXT("== %s 님의 턴 =="), *TurnName));
	}

	//GameState에 현재 턴 정보 + 제한 시간 세팅 (전 클라 복제 => 타이머 위젯이 읽는다)
	ABGGameStateBase* BGGameState = GetGameState<ABGGameStateBase>();
	if (IsValid(BGGameState) == true)
	{
		BGGameState->CurrentTurnPlayerNameString = TurnName;
		BGGameState->RemainingTime = TurnTimeLimit;
	}

	//1초마다 OnTurnTimerTick 호출(반복). 매 턴 새로 세팅해 시간을 초기화한다.
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &ABGGameModeBase::OnTurnTimerTick, 1.0f, true);
}

void ABGGameModeBase::AdvanceTurn()
{
	const int32 PlayerCount = AllPlayerControllers.Num();
	if (PlayerCount == 0)
	{
		return;
	}

	//기회가 남은 다음 플레이어를 찾아 턴을 넘긴다 (소진한 플레이어는 건너뜀)
	for (int32 Step = 1; Step <= PlayerCount; ++Step)
	{
		const int32 NextIndex = (CurrentTurnIndex + Step) % PlayerCount;
		ABGPlayerController* NextController = AllPlayerControllers[NextIndex];
		if (IsValid(NextController) == false)
		{
			continue;
		}

		ABGPlayerState* NextState = NextController->GetPlayerState<ABGPlayerState>();
		if (IsValid(NextState) == true && NextState->CurrentGuessCount < NextState->MaxGuessCount)
		{
			StartTurn(NextIndex);
			return;
		}
	}
}

void ABGGameModeBase::OnTurnTimerTick()
{
	ABGGameStateBase* BGGameState = GetGameState<ABGGameStateBase>();
	if (IsValid(BGGameState) == false)
	{
		return;
	}

	//남은 시간 1초 감소 (Replicated => 모든 클라의 타이머 위젯이 갱신됨)
	BGGameState->RemainingTime -= 1;

	//0이 되면 시간 초과 처리
	if (BGGameState->RemainingTime <= 0)
	{
		HandleTurnTimeout();
	}
}

void ABGGameModeBase::HandleTurnTimeout()
{
	ABGPlayerController* TimedOutController = GetCurrentTurnController();
	if (IsValid(TimedOutController) == false)
	{
		return;
	}

	ABGPlayerState* TimedOutState = TimedOutController->GetPlayerState<ABGPlayerState>();
	const FString TimedOutName = IsValid(TimedOutState) ? TimedOutState->PlayerNameString : TEXT("Someone");

	//시간 초과 => 기회 1 소모 (아무도 안 치고 버티는 무한 루프 방지)
	IncreaseGuessCount(TimedOutController);
	BroadcastChatMessage(FString::Printf(TEXT("%s 님이 시간 초과로 기회를 잃었습니다."), *TimedOutName));

	//0스트라이크이므로 승리는 불가, 무승부(전원 소진)만 검사된다
	const bool bRoundEnded = JudgeGame(TimedOutController, 0);

	//라운드가 계속되면 다음 플레이어로 턴을 넘긴다 (StartTurn에서 타이머가 재설정됨)
	if (bIsTurnActive == true && bRoundEnded == false)
	{
		AdvanceTurn();
	}
}
