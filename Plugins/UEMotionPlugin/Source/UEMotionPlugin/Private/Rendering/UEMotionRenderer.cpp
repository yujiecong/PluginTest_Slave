#include "UEMotionRenderer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "MoviePipelineQueue.h"
#include "MoviePipelineExecutor.h"
#include "MoviePipelinePrimaryConfig.h"
#include "MoviePipelineOutputSetting.h"
#include "MoviePipelineImageSequenceOutput.h"
#include "MoviePipelineAntiAliasingSetting.h"
#include "MoviePipelinePIEExecutor.h"
#include "MoviePipelineQueueSubsystem.h"

void UUEMotionRenderer::Initialize(UWorld* World, int32 Width, int32 Height)
{
	TargetWorld = World;
	ResolutionWidth = Width;
	ResolutionHeight = Height;
}

void UUEMotionRenderer::BindCamera(AActor* CameraActor)
{
	BoundCamera = CameraActor;
}

void UUEMotionRenderer::RenderSequence(ULevelSequence* Sequence, const FString& OutputDirectory, float Duration, float FPS)
{
	if (!GEditor) return;
	if (bIsRendering) return;
	if (!Sequence) return;

	bIsRendering = true;
	ActiveSequence = Sequence;

	UMoviePipelineQueueSubsystem* QueueSubsystem = GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>();
	if (!QueueSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionRenderer: Failed to get MoviePipelineQueueSubsystem"));
		bIsRendering = false;
		return;
	}

	UMoviePipelineQueue* PipelineQueue = QueueSubsystem->GetQueue();
	PipelineQueue->Modify();
	PipelineQueue->DeleteAllJobs();

	UMoviePipelineExecutorJob* Job = PipelineQueue->AllocateNewJob(UMoviePipelineExecutorJob::StaticClass());
	if (!Job)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionRenderer: Failed to allocate new job"));
		bIsRendering = false;
		return;
	}

	Job->Modify();
	Job->JobName = TEXT("UEMotionRender");
	Job->Sequence = Sequence;

	if (TargetWorld)
	{
		Job->Map = FSoftObjectPath(TargetWorld);
	}

	UMoviePipelinePrimaryConfig* Config = NewObject<UMoviePipelinePrimaryConfig>();
	Job->SetConfiguration(Config);

	UMoviePipelineOutputSetting* OutputSetting = Cast<UMoviePipelineOutputSetting>(
		Config->FindOrAddSettingByClass(UMoviePipelineOutputSetting::StaticClass()));
	if (OutputSetting)
	{
		OutputSetting->OutputDirectory.Path = OutputDirectory;
		OutputSetting->FileNameFormat = TEXT("{frame_number}");
		OutputSetting->OutputResolution = FIntPoint(ResolutionWidth, ResolutionHeight);
		OutputSetting->bUseCustomFrameRate = true;
		OutputSetting->OutputFrameRate = FFrameRate(FMath::RoundToInt(FPS), 1);
		OutputSetting->bOverrideExistingOutput = true;
		OutputSetting->ZeroPadFrameNumbers = 4;
	}

	UMoviePipelineImageSequenceOutput_PNG* PNGOutput = Cast<UMoviePipelineImageSequenceOutput_PNG>(
		Config->FindOrAddSettingByClass(UMoviePipelineImageSequenceOutput_PNG::StaticClass()));
	if (PNGOutput)
	{
		PNGOutput->bWriteAlpha = false;
	}

	UMoviePipelineAntiAliasingSetting* AASetting = Cast<UMoviePipelineAntiAliasingSetting>(
		Config->FindOrAddSettingByClass(UMoviePipelineAntiAliasingSetting::StaticClass()));
	if (AASetting)
	{
		AASetting->bOverrideAntiAliasing = true;
		AASetting->AntiAliasingMethod = EAntiAliasingMethod::AAM_TemporalAA;
		AASetting->TemporalSampleCount = 4;
		AASetting->SpatialSampleCount = 4;
		AASetting->RenderWarmUpCount = 8;
	}

	UMoviePipelinePIEExecutor* Executor = NewObject<UMoviePipelinePIEExecutor>(QueueSubsystem);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
	Executor->OnExecutorFinished().AddUObject(this, &UUEMotionRenderer::OnRenderFinished);
#else
	Executor->OnExecutorFinished().AddUObject(this, &UUEMotionRenderer::OnRenderFinished);
#endif

	ActiveExecutor = Executor;

	UE_LOG(LogTemp, Log, TEXT("UEMotionRenderer: Starting render - OutputDir='%s', Resolution=%dx%d, FPS=%.0f"),
		*OutputDirectory, ResolutionWidth, ResolutionHeight, FPS);

	QueueSubsystem->RenderQueueWithExecutorInstance(Executor);
}

bool UUEMotionRenderer::IsRendering() const
{
	return bIsRendering;
}

float UUEMotionRenderer::GetProgress() const
{
	if (!ActiveExecutor) return 0.0f;
	return 0.5f;
}

void UUEMotionRenderer::OnRenderFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess)
{
	bIsRendering = false;
	ActiveExecutor = nullptr;

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("UEMotionRenderer: Render completed successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionRenderer: Render failed"));
	}

	OnRenderFinishedDelegate.Broadcast(bSuccess);
}