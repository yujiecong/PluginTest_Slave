#include "PyManimRenderer.h"
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
#include "MoviePipelineInProcessExecutor.h"
#include "MoviePipelineBlueprintLibrary.h"
#include "CineCameraComponent.h"
#include "ImageWriteQueue.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"

void UPyManimRenderer::Initialize(UWorld* World, int32 Width, int32 Height)
{
	TargetWorld = World;
	ResolutionWidth = Width;
	ResolutionHeight = Height;
}

void UPyManimRenderer::BindCamera(AActor* CameraActor)
{
	BoundCamera = CameraActor;
}

void UPyManimRenderer::SetAntiAliasing(int32 AAMode)
{
	AntiAliasingMode = AAMode;
}

void UPyManimRenderer::SetOutputFormat(const FString& Format)
{
	OutputFileFormat = Format;
}

void UPyManimRenderer::RenderSequence(const FString& OutputDirectory, float Duration, float FPS)
{
	if (!TargetWorld || bIsRendering) return;

	bIsRendering = true;

	ULevelSequence* Sequence = CreateTempSequence(Duration, FPS);
	if (!Sequence)
	{
		bIsRendering = false;
		return;
	}

	UMoviePipelineQueue* PipelineQueue = NewObject<UMoviePipelineQueue>(this);
	UMoviePipelineExecutorJob* Job = PipelineQueue->AllocateNewJob(UMoviePipelineExecutorJob::StaticClass());
	if (!Job)
	{
		bIsRendering = false;
		return;
	}

	Job->JobName = TEXT("PyManimRender");
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

	UMoviePipelineInProcessExecutor* Executor = NewObject<UMoviePipelineInProcessExecutor>(this);
	Executor->bUseCurrentLevel = true;

	Executor->OnExecutorFinished().AddUObject(this, &UPyManimRenderer::OnRenderFinished);

	ActiveExecutor = Executor;
	Executor->Execute(PipelineQueue);
}

void UPyManimRenderer::RenderSingleFrame(const FString& FilePath)
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

	RenderSequence(Directory, 1.0f / 30.0f, 30.0f);
}

bool UPyManimRenderer::IsRendering() const
{
	return bIsRendering;
}

float UPyManimRenderer::GetProgress() const
{
	if (!ActiveExecutor) return 0.0f;
	return 0.5f;
}

ULevelSequence* UPyManimRenderer::CreateTempSequence(float Duration, float FPS)
{
	if (!TargetWorld) return nullptr;

	ULevelSequence* Sequence = NewObject<ULevelSequence>(GetTransientPackage(), NAME_None, RF_Transient);
	if (!Sequence) return nullptr;

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene) return nullptr;

	int32 TotalFrames = FMath::CeilToInt(Duration * FPS);
	FFrameRate FrameRate(FMath::RoundToInt(FPS), 1);
	MovieScene->SetDisplayRate(FrameRate);
	MovieScene->SetPlaybackRange(TRange<FFrameNumber>(0, TotalFrames));

	return Sequence;
}

void UPyManimRenderer::OnRenderFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess)
{
	bIsRendering = false;
	ActiveExecutor = nullptr;

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("PyManimRenderer: Render completed successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PyManimRenderer: Render failed"));
	}
}
