#include "PyManimScene.h"
#include "PyManimCamera.h"
#include "PyManimMobject.h"
#include "Anim/PyManimAnimation.h"
#include "Rendering/PyManimRenderer.h"
#include "Actors/PyManimSceneActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"

void UPyManimScene::Initialize(int32 Width, int32 Height)
{
	if (bInitialized) return;

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return;

	ResolutionWidth = Width;
	ResolutionHeight = Height;

	FVector Location(-500, -500, 300);
	FRotator Rotation = FRotator::ZeroRotator;
	SceneActor = World->SpawnActor<APyManimSceneActor>(APyManimSceneActor::StaticClass(), Location, Rotation);

	if (SceneActor)
	{
		Camera = NewObject<UPyManimCamera>(this);
		Camera->Init(SceneActor);
		Camera->LookAt(FVector(0, 0, 0));
		bInitialized = true;
	}
}

bool UPyManimScene::IsInitialized() const
{
	return bInitialized;
}

UPyManimMobject* UPyManimScene::CreateSphere(float Radius)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Radius <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("PyManimScene::CreateSphere: Invalid radius %.2f, clamping to 1.0"), Radius);
		Radius = 1.0f;
	}

	UPyManimMobject* Obj = NewObject<UPyManimMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsSphere(SceneActor, Radius);
		Mobjects.Add(Obj);
	}
	return Obj;
}

UPyManimMobject* UPyManimScene::CreateCube(float Size)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Size <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("PyManimScene::CreateCube: Invalid size %.2f, clamping to 1.0"), Size);
		Size = 1.0f;
	}

	UPyManimMobject* Obj = NewObject<UPyManimMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Cube.Cube"), Size);
		Obj->SetMobjectName(TEXT("Cube"));
		Mobjects.Add(Obj);
	}
	return Obj;
}

UPyManimMobject* UPyManimScene::CreateCylinder(float Radius, float Height)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Radius <= 0.0f) Radius = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;

	UPyManimMobject* Obj = NewObject<UPyManimMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), Radius);
		Obj->SetMobjectName(TEXT("Cylinder"));
		Mobjects.Add(Obj);
	}
	return Obj;
}

UPyManimMobject* UPyManimScene::CreateCone(float Radius, float Height)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Radius <= 0.0f) Radius = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;

	UPyManimMobject* Obj = NewObject<UPyManimMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Cone.Cone"), Radius);
		Obj->SetMobjectName(TEXT("Cone"));
		Mobjects.Add(Obj);
	}
	return Obj;
}

UPyManimMobject* UPyManimScene::CreatePlane(float Width, float Height)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (Width <= 0.0f) Width = 1.0f;
	if (Height <= 0.0f) Height = 1.0f;

	UPyManimMobject* Obj = NewObject<UPyManimMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Plane.Plane"), Width);
		Obj->SetMobjectName(TEXT("Plane"));
		Mobjects.Add(Obj);
	}
	return Obj;
}

UPyManimMobject* UPyManimScene::CreateTorus(float OuterRadius, float InnerRadius)
{
	if (!bInitialized || !SceneActor) return nullptr;
	if (OuterRadius <= 0.0f) OuterRadius = 1.0f;
	if (InnerRadius <= 0.0f) InnerRadius = 1.0f;

	UPyManimMobject* Obj = NewObject<UPyManimMobject>(this);
	if (Obj)
	{
		Obj->InitializeAsMesh(SceneActor, TEXT("/Engine/BasicShapes/Torus.Torus"), OuterRadius);
		Obj->SetMobjectName(TEXT("Torus"));
		Mobjects.Add(Obj);
	}
	return Obj;
}

UPyManimCamera* UPyManimScene::GetCamera()
{
	return Camera;
}

void UPyManimScene::AddDirectionalLight(const FVector& Direction, const FLinearColor& Color, float Intensity)
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

void UPyManimScene::AddPointLight(const FVector& Location, const FLinearColor& Color, float Intensity)
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

TArray<UPyManimMobject*> UPyManimScene::GetAllMobjects() const
{
	return Mobjects;
}

void UPyManimScene::Play(UPyManimAnimation* Animation)
{
	if (!Animation || !bInitialized) return;
	if (Animation->GetDuration() <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("PyManimScene::Play: Animation duration <= 0, skipping"));
		return;
	}
	ActiveAnimations.Add(Animation);
	Animation->Reset();
}

void UPyManimScene::Tick(float DeltaTime)
{
	if (!bInitialized) return;
	if (DeltaTime <= 0.0f) return;

	for (int32 i = ActiveAnimations.Num() - 1; i >= 0; --i)
	{
		UPyManimAnimation* Anim = ActiveAnimations[i];
		if (!Anim) continue;
		Anim->Advance(DeltaTime);
		if (Anim->IsFinished())
		{
			ActiveAnimations.RemoveAt(i);
		}
	}
}

void UPyManimScene::StopAll()
{
	ActiveAnimations.Empty();
}

bool UPyManimScene::HasActiveAnimations() const
{
	return ActiveAnimations.Num() > 0;
}

void UPyManimScene::RenderFrames(const FString& OutputDirectory, float Duration, float FPS)
{
	if (!bInitialized || !SceneActor) return;
	if (OutputDirectory.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PyManimScene::RenderFrames: Output directory is empty"));
		return;
	}
	if (Duration <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("PyManimScene::RenderFrames: Invalid duration %.2f"), Duration);
		return;
	}
	if (FPS <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("PyManimScene::RenderFrames: Invalid FPS %.2f"), FPS);
		return;
	}

	if (!Renderer)
	{
		Renderer = NewObject<UPyManimRenderer>(this);
	}

	UWorld* World = SceneActor->GetWorld();
	Renderer->Initialize(World, ResolutionWidth, ResolutionHeight);
	Renderer->BindCamera(SceneActor);
	Renderer->SetAntiAliasing(1);
	Renderer->SetOutputFormat(TEXT("png"));
	Renderer->RenderSequence(OutputDirectory, Duration, FPS);
}

void UPyManimScene::RenderSingleFrame(const FString& FilePath)
{
	if (!bInitialized || !SceneActor) return;
	if (FilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PyManimScene::RenderSingleFrame: File path is empty"));
		return;
	}

	if (!Renderer)
	{
		Renderer = NewObject<UPyManimRenderer>(this);
	}

	UWorld* World = SceneActor->GetWorld();
	Renderer->Initialize(World, ResolutionWidth, ResolutionHeight);
	Renderer->BindCamera(SceneActor);
	Renderer->RenderSingleFrame(FilePath);
}

void UPyManimScene::ClearScene()
{
	for (UPyManimMobject* Obj : Mobjects)
	{
		if (Obj)
		{
			Obj->Destroy();
		}
	}
	Mobjects.Empty();
}

void UPyManimScene::Destroy()
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
