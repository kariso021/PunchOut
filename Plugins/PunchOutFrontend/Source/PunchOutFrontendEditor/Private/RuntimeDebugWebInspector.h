#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "HttpServerResponse.h"

class UWidget;
class UWorld;
class UGameViewportClient;
class SWidget;
struct FHttpServerRequest;
struct FSlateDebuggingInputEventArgs;

class FRuntimeDebugWebInspector
{
public:
	FRuntimeDebugWebInspector();
	~FRuntimeDebugWebInspector();
	TUniquePtr<FHttpServerResponse> Handle(const FHttpServerRequest& Request, UWorld* World);
	void Reset();
	void Record(const FString& Message);

private:
	void Refresh(UWorld* World);
	TSharedPtr<FJsonValue> BuildWidget(UWidget* Widget);
	TSharedRef<FJsonObject> Snapshot() const;
	void OnInput(const FSlateDebuggingInputEventArgs& Event);
	void SetHighlight(bool bEnabled);

	TWeakObjectPtr<UWorld> InspectedWorld;
	TMap<FString, TWeakObjectPtr<UWidget>> Widgets;
	TMap<TWeakObjectPtr<UWidget>, FString> WidgetIds;
	TArray<TSharedPtr<FJsonValue>> Roots;
	TWeakObjectPtr<UWidget> Selected;
	TWeakObjectPtr<UGameViewportClient> HighlightViewport;
	TSharedPtr<SWidget> HighlightOverlay;
	TArray<FString> Events;
	int32 EventCursor = 0;
	bool bRecordInput = false;
	double LastRefresh = -1;
};
