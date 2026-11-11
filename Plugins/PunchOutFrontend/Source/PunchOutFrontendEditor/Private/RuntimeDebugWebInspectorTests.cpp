#if WITH_DEV_AUTOMATION_TESTS

#include "RuntimeDebugWebInspector.h"

#include "HttpServerRequest.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeDebugWebInspectorTest, "PunchOut.Frontend.WebInspector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRuntimeDebugWebInspectorTest::RunTest(const FString& Parameters)
{
	FRuntimeDebugWebInspector Inspector;
	for (int32 Index = 0; Index < 300; ++Index)
	{
		Inspector.Record(FString::Printf(TEXT("Event %d"), Index));
	}

	FHttpServerRequest Request;
	Request.Verb = EHttpServerRequestVerbs::VERB_GET;
	const auto Response = Inspector.Handle(Request, nullptr);
	const FUTF8ToTCHAR Body(reinterpret_cast<const ANSICHAR*>(Response->Body.GetData()), Response->Body.Num());
	TSharedPtr<FJsonObject> Snapshot;
	if (!TestTrue(TEXT("Snapshot is valid JSON"), FJsonSerializer::Deserialize(
		TJsonReaderFactory<>::Create(FString(Body.Length(), Body.Get())), Snapshot))) return false;
	const auto& Events = Snapshot->GetArrayField(TEXT("events"));
	TestEqual(TEXT("Event storage is bounded"), Events.Num(), 256);
	TestTrue(TEXT("Oldest retained event is first"), Events[0]->AsString().EndsWith(TEXT("Event 44")));
	TestTrue(TEXT("Latest event is last"), Events.Last()->AsString().EndsWith(TEXT("Event 299")));

	auto Post = [&Inspector](const FString& Json)
	{
		FHttpServerRequest Action;
		Action.Verb = EHttpServerRequestVerbs::VERB_POST;
		const FTCHARToUTF8 Utf8(*Json);
		Action.Body.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		return Inspector.Handle(Action, nullptr)->Code;
	};
	TestTrue(TEXT("Malformed input is rejected"), Post(TEXT("{")) == EHttpServerResponseCodes::BadRequest);
	TestTrue(TEXT("Unknown commands are rejected"), Post(TEXT("{\"action\":\"execute\"}")) == EHttpServerResponseCodes::BadRequest);
	TestTrue(TEXT("Removed widget IDs are rejected"),
		Post(TEXT("{\"action\":\"select\",\"id\":\"missing\",\"highlight\":true}")) == EHttpServerResponseCodes::NotFound);
	TestTrue(TEXT("Clear is supported"), Post(TEXT("{\"action\":\"clear\"}")) == EHttpServerResponseCodes::Ok);
	return true;
}

#endif
