#include "UEMotionScene.h"
#include "UEMotionMobject.h"
#include "Rendering/UEMotionRenderer.h"
#include "Actors/UEMotionSceneActor.h"
#include "UEMotionCamera.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "EditorAssetLibrary.h"
#include "UnrealEdGlobals.h"
#include "FileHelpers.h"
#include "LevelSequence.h"
#include "Utils/UEMotionSequencerCompat.h"

void UUEMotionScene::RenderFrames(const FString& OutputDirectory, float Duration, float FPS)
{
	if (!bInitialized || !LevelSequence) return;

	if (!Renderer)
	{
		Renderer = NewObject<UUEMotionRenderer>(this);
		Renderer->Initialize(SceneWorld.Get(), ResolutionWidth, ResolutionHeight);
		Renderer->SetUseUnlit(bUseUnlitMode);
		Renderer->OnRenderFinishedDelegate.AddDynamic(this, &UUEMotionScene::OnRendererFinished);
	}

	Renderer->BindCamera(SceneActor.Get());

	SaveAssets();

	UE_LOG(LogTemp, Log, TEXT("UEMotionScene::RenderFrames: Starting MRQ render to '%s', Duration=%.2fs, FPS=%.0f"),
		*OutputDirectory, Duration, FPS);

	Renderer->RenderSequence(LevelSequence, OutputDirectory, Duration, FPS);
}

void UUEMotionScene::RenderSingleFrame(const FString& FilePath)
{
	if (!bInitialized || !LevelSequence) return;

	FString Directory = FPaths::GetPath(FilePath);
	FString Extension = FPaths::GetExtension(FilePath);
	if (Directory.IsEmpty())
	{
		Directory = FPaths::ProjectSavedDir() / TEXT("UEMotion") / TEXT("Renders");
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	RenderFrames(Directory, 1.0f / 30.0f, 30.0f);
}

void UUEMotionScene::ClearScene()
{
	for (UUEMotionMobject* Obj : Mobjects)
	{
		if (Obj)
		{
			Obj->Destroy();
		}
	}
	Mobjects.Empty();
}

void UUEMotionScene::Destroy()
{
	ClearScene();

	if (SceneActor.IsValid())
	{
		SceneActor.Get()->Destroy();
		SceneActor.Reset();
	}

	if (Renderer)
	{
		Renderer->OnRenderFinishedDelegate.RemoveAll(this);
		Renderer = nullptr;
	}

	Camera = nullptr;
	LevelSequence = nullptr;
	SceneWorld.Reset();
	bInitialized = false;

	if (bAutoCleanup)
	{
		CleanupAssets();
	}
}

void UUEMotionScene::SaveAssets()
{
	if (LevelSequence)
	{
		LevelSequence->MarkPackageDirty();
		UPackage* SeqPackage = LevelSequence->GetOutermost();
		if (SeqPackage)
		{
			UEMotionCompat::MarkPackageDirty(SeqPackage);
			if (UEMotionCompat::SavePackageToDisk(SeqPackage, TEXT(".uasset")))
			{
				UE_LOG(LogTemp, Log, TEXT("UEMotionScene: LevelSequence saved to '%s'"), *SeqPackage->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: Failed to save LevelSequence '%s'"), *SeqPackage->GetName());
			}
		}
	}

	if (SceneWorld.IsValid())
	{
		FString MapPath = GetMapPath();
		if (UEditorLoadingAndSavingUtils::SaveMap(SceneWorld.Get(), MapPath))
		{
			UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Map saved to '%s'"), *MapPath);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UEMotionScene: Failed to save map '%s'"), *MapPath);
		}
	}
}

void UUEMotionScene::CleanupAssets()
{
	FString MapPath = GetMapPath();
	FString SeqPath = GetSequencePath();

	if (UEditorAssetLibrary::DoesAssetExist(MapPath))
	{
		UEditorAssetLibrary::DeleteAsset(MapPath);
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Cleaned up map asset: %s"), *MapPath);
	}

	if (UEditorAssetLibrary::DoesAssetExist(SeqPath))
	{
		UEditorAssetLibrary::DeleteAsset(SeqPath);
		UE_LOG(LogTemp, Log, TEXT("UEMotionScene: Cleaned up sequence asset: %s"), *SeqPath);
	}
}

void UUEMotionScene::OnRendererFinished(bool bSuccess)
{
	OnRenderFinished.Broadcast(this, bSuccess);
}
