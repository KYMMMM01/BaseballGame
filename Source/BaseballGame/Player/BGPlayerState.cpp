//BGPlayerState.cpp

#include "Player/BGPlayerState.h"

#include "Net/UnrealNetwork.h"

ABGPlayerState::ABGPlayerState()
	: PlayerNameString(TEXT("None"))
	, CurrentGuessCount(0)
	, MaxGuessCount(3)
{
	bReplicates = true;
}

void ABGPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PlayerNameString);
	DOREPLIFETIME(ThisClass, CurrentGuessCount);
	DOREPLIFETIME(ThisClass, MaxGuessCount);
}

FString ABGPlayerState::GetPlayerInfoString() const
{
	return FString::Printf(TEXT("%s(%d/%d)"), *PlayerNameString, CurrentGuessCount, MaxGuessCount);
}
