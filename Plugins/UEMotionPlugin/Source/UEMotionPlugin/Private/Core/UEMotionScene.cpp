#include "UEMotionScene.h"
#include "UEMotionCamera.h"
#include "UEMotionMobject.h"
#include "Anim/UEMotionAnimation.h"
#include "Anim/UEMotionGroupAnimation.h"
#include "Rendering/UEMotionRenderer.h"
#include "Actors/UEMotionSceneActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"

void UUEMotionScene::Initialize(int32 Width, int32 Height)
{
	if (bInitialized) return;

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return;

	ResolutionWidth = Width;
	ResolutionHeight = Height;

	FVector Location(-500, -500, 300);
	FRotator Rotation = FRotator::ZeroRotator;
	SceneActor = World->SpawnActor<AUEMotionSceneActor>(AUEMotionSceneActor::StaticClass(), Location, Rotation);

	if (SceneActor)
	{
		SceneActor->SetOwnerScene(this);
		Camera = NewObject<UUEMotionCamera>(this);
		Camera->Init(SceneActor);
		Camera->LookAt(FVector(0, 0, 0));
		bInitialized = true;
	}
}

bool UUEMotionScene::IsInitialized() const
{
	return bInitialized;
}

UUEMotionMobject* UUEMotionScene::CreateSphere(float Radius)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Radius <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionScene::CreateSphere: Invalid radius %.2f, clamping to 1.0"), Radius);
		Radius = 1.0f;
	}

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsSphere(SceneActor, Radius);
		Mobjects.Add(Obj);
	}
	return Obj;
}

UUEMotionMobject* UUEMotionScene::CreateCube(float Size)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Size <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionScene::CreateCube: Invalid size %.2f, clamping to 1.0"), Size);
		Size = 1.0f;
	}

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Cube.Cube"), Size);
		Obj->SetMobjectName(TEXT("Cube"));
		Mobjects.Add(Obj);
	}
	return Obj;
}

UUEMotionMobject* UUEMotionScene::CreateCylinder(float Radius, float Height)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Radius <= 0.0f) Radius = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), Radius);
		Obj->SetMobjectName(TEXT("Cylinder"));
		Mobjects.Add(Obj);
	}
	return Obj;
}

UUEMotionMobject* UUEMotionScene::CreateCone(float Radius, float Height)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Radius <= 0.0f) Radius = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Cone.Cone"), Radius);
		Obj->SetMobjectName(TEXT("Cone"));
		Mobjects.Add(Obj);
	}
	return Obj;
}

UUEMotionMobject* UUEMotionScene::CreatePlane(float Width, float Height)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Width <= 0.0f) Width = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Plane.Plane"), Width);
		Obj->SetMobjectName(TEXT("Plane"));
		Mobjects.Add(Obj);
	}
	return Obj;
}

UUEMotionMobject* UUEMotionScene::CreateTorus(float OuterRadius, float InnerRadius)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (OuterRadius <= 0.0f) OuterRadius = 1.0f;
	if (InnerRadius <= 0.0f) InnerRadius = 1.0f;

	UUEMotionMobject* Obj = NewObject<UUEMotionMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Torus.Torus"), OuterRadius);
		Obj->SetMobjectName(TEXT("Torus"));
		Mobjects.Add(Obj);
	}
	return Obj;
}

UUEMotionCamera* UUEMotionScene::GetCamera()
{
	return Camera;
}

void UUEMotionScene::AddDirectionalLight(const FVector& Direction, const FLinearColor& Color, float Intensity)
{
	if (!bInitialized || !SceneActor) return;

	UWorld* World = SceneActor->GetWorld();
	if (!World) return;

	ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>();
	if (Light)
	{
		Light->SetActorRotation(Direction.Rotation());
		UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(Light->GetLightComponent());
		if (LightComp)
		{
			LightComp->SetIntensity(Intensity);
			LightComp->SetLightColor(Color);
		}
	}
}

void UUEMotionScene::AddPointLight(const FVector& Location, const FLinearColor& Color, float Intensity)
{
	if (!bInitialized || !SceneActor) return;

	UWorld* World = SceneActor->GetWorld();
	if (!World) return;

	APointLight* Light = World->SpawnActor<APointLight>();
	if (Light)
	{
		Light->SetActorLocation(Location);
		UPointLightComponent* LightComp = Cast<UPointLightComponent>(Light->GetLightComponent());
		if (LightComp)
		{
			LightComp->SetIntensity(Intensity);
			LightComp->SetLightColor(Color);
		}
	}
}

TArray<UUEMotionMobject*> UUEMotionScene::GetAllMobjects() const
{
	return Mobjects;
}

void UUEMotionScene::Play(UUEMotionAnimation* Animation)
{
	if (!Animation || !bInitialized) return;
	if (Animation->GetDuration() <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionScene::Play: Animation duration <= 0, skipping"));
		return;
	}
	UUEMotionGroupAnimation* Group = Cast<UUEMotionGroupAnimation>(Animation);
	if (Group && Group->GetAnimationCount() == 0)
	{
		return;
	}
	ActiveAnimations.Add(Animation);
	Animation->Reset();
}

void UUEMotionScene::Tick(float DeltaTime)
{
	if (!bInitialized) return;
	if (DeltaTime <= 0.0f) return;

	for (int32 i = ActiveAnimations.Num() - 1; i >= 0; --i)
	{
		UUEMotionAnimation* Anim = ActiveAnimations[i];
		if (!Anim) continue;
		Anim->Advance(DeltaTime);
		if (Anim->IsFinished())
		{
			ActiveAnimations.RemoveAt(i);
		}
	}
}

void UUEMotionScene::StopAll()
{
	ActiveAnimations.Empty();
}

bool UUEMotionScene::HasActiveAnimations() const
{
	return ActiveAnimations.Num() > 0;
}

void UUEMotionScene::RenderFrames(const FString& OutputDirectory, float Duration, float FPS)
{
	if (!bInitialized || !SceneActor) return;
	if (OutputDirectory.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionScene::RenderFrames: Output directory is empty"));
		return;
	}
	if (Duration <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionScene::RenderFrames: Invalid duration %.2f"), Duration);
		return;
	}
	if (FPS <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionScene::RenderFrames: Invalid FPS %.2f"), FPS);
		return;
	}

	if (!Renderer)
	{
		Renderer = NewObject<UUEMotionRenderer>(this);
	}

	UWorld* World = SceneActor->GetWorld();
	Renderer->Initialize(World, ResolutionWidth, ResolutionHeight);
	Renderer->BindCamera(SceneActor);
	Renderer->SetAntiAliasing(1);
	Renderer->SetOutputFormat(TEXT("png"));
	Renderer->RenderSequence(OutputDirectory, Duration, FPS);
}

void UUEMotionScene::RenderSingleFrame(const FString& FilePath)
{
	if (!bInitialized || !SceneActor) return;
	if (FilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionScene::RenderSingleFrame: File path is empty"));
		return;
	}

	if (!Renderer)
	{
		Renderer = NewObject<UUEMotionRenderer>(this);
	}

	UWorld* World = SceneActor->GetWorld();
	Renderer->Initialize(World, ResolutionWidth, ResolutionHeight);
	Renderer->BindCamera(SceneActor);
	Renderer->RenderSingleFrame(FilePath);
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

	if (SceneActor)
	{
		SceneActor->Destroy();
		SceneActor = nullptr;
	}

	Camera = nullptr;
	bInitialized = false;
}
