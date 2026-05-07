#pragma once

#include "CoreMinimal.h"
#include "MovieScene.h"
#include "LevelSequence.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Tracks/MovieSceneSpawnTrack.h"
#include "Sections/MovieSceneSpawnSection.h"
#include "Channels/MovieSceneDoubleChannel.h"
#include "Channels/MovieSceneFloatChannel.h"
#else
#include "MovieScene3DTransformTrack.h"
#include "MovieScene3DTransformSection.h"
#include "MovieSceneFloatTrack.h"
#include "MovieSceneFloatSection.h"
#include "MovieSceneSpawnTrack.h"
#include "MovieSceneSpawnSection.h"
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
#include "UObject/SavePackage.h"
#endif

namespace UEMotionCompat
{
	inline FGuid FindObjectBinding(UMovieScene* MovieScene, AActor* Actor)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		FMovieScenePossessable* Found = MovieScene->FindPossessable(
			[Actor](FMovieScenePossessable& Possessable)
			{
				return Possessable.GetName() == Actor->GetFName();
			});
		return Found ? Found->GetGuid() : FGuid();
#else
		const FMovieSceneBinding* ExistingBinding = MovieScene->FindBinding(Actor);
		return ExistingBinding ? ExistingBinding->GetObjectGuid() : FGuid();
#endif
	}

	inline UMovieScene3DTransformTrack* FindTransformTrack(UMovieScene* MovieScene, const FGuid& ObjectBinding)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		return MovieScene->FindTrack<UMovieScene3DTransformTrack>(ObjectBinding);
#else
		for (UMovieSceneTrack* Track : MovieScene->GetAllTracks())
		{
			UMovieScene3DTransformTrack* TransformTrack = Cast<UMovieScene3DTransformTrack>(Track);
			if (TransformTrack && TransformTrack->GetObjectId() == ObjectBinding)
			{
				return TransformTrack;
			}
		}
		return nullptr;
#endif
	}

	inline FGuid AddPossessable(UMovieScene* MovieScene, AActor* Actor)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		return MovieScene->AddPossessable(Actor->GetName(), Actor->GetClass());
#else
		return MovieScene->AddPossessable(Actor->GetFName(), Actor->GetClass());
#endif
	}

	inline FGuid FindOrAddPossessable(UMovieScene* MovieScene, AActor* Actor, ULevelSequence* LevelSequence, UWorld* World)
	{
		FGuid ObjectBinding = FindObjectBinding(MovieScene, Actor);
		if (!ObjectBinding.IsValid())
		{
			ObjectBinding = AddPossessable(MovieScene, Actor);
		}
		if (ObjectBinding.IsValid() && LevelSequence && World)
		{
			LevelSequence->BindPossessableObject(ObjectBinding, *Actor, World);
		}
		return ObjectBinding;
	}

	inline UMovieSceneSection* CreateAndAddSection(UMovieSceneTrack* Track)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		UMovieSceneSection* NewSection = Track->CreateNewSection();
		Track->AddSection(*NewSection);
		NewSection->SetRange(TRange<FFrameNumber>::All());
		return NewSection;
#else
		Track->AddSectionToAll();
		return Track->GetAllSections().Num() > 0 ? Track->GetAllSections()[0] : nullptr;
#endif
	}

	inline void RecordTransformKeys(UMovieScene3DTransformSection* Section, FFrameNumber Frame,
		const FVector& Location, const FRotator& Rotation, const FVector& Scale)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		using namespace UE::MovieScene;
		FRotator RotDegrees = Rotation;

		TArrayView<FMovieSceneDoubleChannel*> DoubleChannels = Section->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
		if (DoubleChannels.Num() >= 9)
		{
			DoubleChannels[0]->SetDefault(Location.X);
			DoubleChannels[1]->SetDefault(Location.Y);
			DoubleChannels[2]->SetDefault(Location.Z);
			DoubleChannels[3]->SetDefault(RotDegrees.Roll);
			DoubleChannels[4]->SetDefault(RotDegrees.Pitch);
			DoubleChannels[5]->SetDefault(RotDegrees.Yaw);
			DoubleChannels[6]->SetDefault(Scale.X);
			DoubleChannels[7]->SetDefault(Scale.Y);
			DoubleChannels[8]->SetDefault(Scale.Z);

			AddKeyToChannel(DoubleChannels[0], Frame, Location.X, EMovieSceneKeyInterpolation::Linear);
			AddKeyToChannel(DoubleChannels[1], Frame, Location.Y, EMovieSceneKeyInterpolation::Linear);
			AddKeyToChannel(DoubleChannels[2], Frame, Location.Z, EMovieSceneKeyInterpolation::Linear);
			AddKeyToChannel(DoubleChannels[3], Frame, (double)RotDegrees.Roll, EMovieSceneKeyInterpolation::Linear);
			AddKeyToChannel(DoubleChannels[4], Frame, (double)RotDegrees.Pitch, EMovieSceneKeyInterpolation::Linear);
			AddKeyToChannel(DoubleChannels[5], Frame, (double)RotDegrees.Yaw, EMovieSceneKeyInterpolation::Linear);
			AddKeyToChannel(DoubleChannels[6], Frame, Scale.X, EMovieSceneKeyInterpolation::Linear);
			AddKeyToChannel(DoubleChannels[7], Frame, Scale.Y, EMovieSceneKeyInterpolation::Linear);
			AddKeyToChannel(DoubleChannels[8], Frame, Scale.Z, EMovieSceneKeyInterpolation::Linear);
		}
#else
		FFrameTime FrameTime(Frame);
		auto SetChannelKey = [&](FMovieSceneDoubleChannel& Channel, double Value)
		{
			int32 KeyIndex = INDEX_NONE;
			Channel.Evaluate(FrameTime, Value);
			FMovieSceneDoubleValue NewValue;
			NewValue.Value = Value;
			Channel.SetKeyInChannel(FrameTime, &NewValue, KeyIndex);
		};

		FRotator RotDegrees = Rotation;
		SetChannelKey(Section->GetChannel(0), Location.X);
		SetChannelKey(Section->GetChannel(1), Location.Y);
		SetChannelKey(Section->GetChannel(2), Location.Z);
		SetChannelKey(Section->GetChannel(3), RotDegrees.Roll);
		SetChannelKey(Section->GetChannel(4), RotDegrees.Pitch);
		SetChannelKey(Section->GetChannel(5), RotDegrees.Yaw);
		SetChannelKey(Section->GetChannel(6), Scale.X);
		SetChannelKey(Section->GetChannel(7), Scale.Y);
		SetChannelKey(Section->GetChannel(8), Scale.Z);
#endif
	}

	inline void RecordFloatKey(UMovieSceneFloatSection* Section, FFrameNumber Frame, float Value)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		using namespace UE::MovieScene;
		Section->GetChannel().SetDefault(Value);
		AddKeyToChannel(&Section->GetChannel(), Frame, Value, EMovieSceneKeyInterpolation::Linear);
#else
		FFrameTime FrameTime(Frame);
		FMovieSceneFloatValue NewValue;
		NewValue.Value = Value;
		int32 KeyIndex = INDEX_NONE;
		Section->GetChannel().SetKeyInChannel(FrameTime, &NewValue, KeyIndex);
#endif
	}

	inline bool SavePackageToDisk(UPackage* Package, const FString& Extension)
	{
		FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), *Extension);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Package, nullptr, *Filename, SaveArgs);
#else
		return UPackage::SavePackage(Package, nullptr, RF_Public | RF_Standalone, *Filename);
#endif
	}

	inline void MarkPackageDirty(UPackage* Package)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		Package->SetDirtyFlag(true);
#else
		Package->MarkDirty();
#endif
	}
}
