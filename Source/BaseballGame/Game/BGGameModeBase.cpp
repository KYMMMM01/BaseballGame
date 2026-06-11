//BGGameModeBase.cpp

#include "Game/BGGameModeBase.h"

#include "Game/BGGameStateBase.h"
#include "Player/BGPlayerController.h"
#include "Player/BGPlayerState.h"

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

	//승리 / 무승부 판정 (1회만 호출)
	const int32 StrikeCount = FCString::Atoi(*JudgeResultString.Left(1));
	JudgeGame(InChattingPlayerController, StrikeCount);
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

void ABGGameModeBase::JudgeGame(ABGPlayerController* InChattingPlayerController, int32 InStrikeCount)
{
	//승리
	if (InStrikeCount == 3)
	{
		ABGPlayerState* WinnerState = InChattingPlayerController->GetPlayerState<ABGPlayerState>();
		const FString WinnerName = IsValid(WinnerState) ? WinnerState->PlayerNameString : TEXT("Someone");

		SetNotificationToAll(FString::Printf(TEXT("%s 님이 승리했습니다!"), *WinnerName));
		BroadcastChatMessage(FString::Printf(TEXT("=== %s 님 승리! (정답: %s) ==="), *WinnerName, *SecretNumberString));
		ResetGame();
		return;
	}

	//무승부
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
	}
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
