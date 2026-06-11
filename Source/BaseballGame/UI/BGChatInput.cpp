//BGChatInput.cpp

#include "UI/BGChatInput.h"

#include "Components/EditableTextBox.h"
#include "Player/BGPlayerController.h"

void UBGChatInput::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(EditableTextBox_ChatInput) == true)
	{
		if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == false)
		{
			EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &ThisClass::OnChatInputTextCommitted);
		}
	}
}

void UBGChatInput::NativeDestruct()
{
	Super::NativeDestruct();

	if (IsValid(EditableTextBox_ChatInput) == true)
	{
		if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == true)
		{
			EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &ThisClass::OnChatInputTextCommitted);
		}
	}
}

void UBGChatInput::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	//엔터로 확정했을 때만 처리
	if (CommitMethod == ETextCommit::OnEnter)
	{
		ABGPlayerController* OwningBGPlayerController = Cast<ABGPlayerController>(GetOwningPlayer());
		if (IsValid(OwningBGPlayerController) == true)
		{
			const FString MessageString = Text.ToString();
			if (MessageString.IsEmpty() == false)
			{
				OwningBGPlayerController->SetChatMessageString(MessageString);
			}

			//입력창 비우기
			if (IsValid(EditableTextBox_ChatInput) == true)
			{
				EditableTextBox_ChatInput->SetText(FText());
			}
		}
	}
}
