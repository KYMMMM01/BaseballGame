//BGChatInput.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BGChatInput.generated.h"

class UEditableTextBox;

/**
 * 채팅(숫자) 입력 위젯.
 * WBP에 'EditableTextBox_ChatInput' 이름의 EditableTextBox가 있어야 BindWidget이 연결된다.
 */
UCLASS()
class BASEBALLGAME_API UBGChatInput : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> EditableTextBox_ChatInput;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	void OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
};
