#include "UEMotionRenderer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "MovieScene.h"
#include "MovieSceneSequencePlayer.h"
#include "MoviePipelineQueue.h"
#include "MoviePipelineExecutor.h"
#include "MoviePipelinePrimaryConfig.h"
#include "MoviePipelineOutputSetting.h"
#include "MoviePipelineAntiAliasingSetting.h"
#include "MoviePipelinePIEExecutor.h"
#include "MoviePipelineBlueprintLibrary.h"
#include "MoviePipelineQueueSubsystem.h"
#include "CineCameraComponent.h"
#include "ImageWriteQueue.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"

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

void UUEMotionRenderer::SetAntiAliasing(int32 AAMode)
{
	AntiAliasingMode = AAMode;
}

void UUEMotionRenderer::SetOutputFormat(const FString& Format)
{
	OutputFileFormat = Format;
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
		bIsRendering = false;
		return;
	}

	UMoviePipelineQueue* PipelineQueue = QueueSubsystem->GetQueue();
	PipelineQueue->DeleteAllJobs();

	UMoviePipelineExecutorJob* Job = PipelineQueue->AllocateNewJob(UMoviePipelineExecutorJob::StaticClass());
	if (!Job)
	{
		bIsRendering = false;
		return;
	}

	Job->JobName = TEXT("UEMotionRender");
	Job->Sequence = Sequence;

	UMoviePipelinePrimaryConfig* Config = NewObject<UMoviePipelinePrimaryConfig>(this);
	Job->SetConfiguration(Config);

	UMoviePipelineOutputSetting* OutputSetting = Cast<UMoviePipelineOutputSetting>(
		Config->FindOrAddSettingByClass(UMoviePipelineOutputSetting::StaticClass()));
	if (OutputSetting)
	{
		OutputSetting->OutputDirectory.Path = OutputDirectory;
		OutputSetting->FileNameFormat = TEXT("{shot_name}_{frame_number}");
		OutputSetting->OutputResolution = FIntPoint(ResolutionWidth, ResolutionHeight);
		OutputSetting->bUseCustomFrameRate = true;
		OutputSetting->OutputFrameRate = FFrameRate(FMath::RoundToInt(FPS), 1);
		OutputSetting->bOverrideExistingOutput = true;
	}

	UMoviePipelineAntiAliasingSetting* AASetting = Cast<UMoviePipelineAntiAliasingSetting>(
		Config->FindOrAddSettingByClass(UMoviePipelineAntiAliasingSetting::StaticClass()));
	if (AASetting)
	{
		AASetting->bOverrideAntiAliasing = true;
		AASetting->AntiAliasingMethod = EAntiAliasingMethod::AAM_TemporalAA;
		AASetting->TemporalSampleCount = 8;
		AASetting->SpatialSampleCount = 8;
		AASetting->RenderWarmUpCount = 32;
	}

	UMoviePipelinePIEExecutor* Executor = NewObject<UMoviePipelinePIEExecutor>(QueueSubsystem);

	Executor->OnExecutorFinished().AddUObject(this, &UUEMotionRenderer::OnRenderFinished);

	ActiveExecutor = Executor;
	QueueSubsystem->RenderQueueWithExecutorInstance(Executor);
}

void UUEMotionRenderer::RenderSingleFrame(const FString& FilePath)
{
	if (!TargetWorld) return;

	FString Directory = FPaths::GetPath(FilePath);
	FString Extension = FPaths::GetExtension(FilePath);

	if (Extension.IsEmpty())
	{
		Extension = TEXT("png");
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	if (ActiveSequence)
	{
		RenderSequence(ActiveSequence, Directory, 1.0f / 30.0f, 30.0f);
	}
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
