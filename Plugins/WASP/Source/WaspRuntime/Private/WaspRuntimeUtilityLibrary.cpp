// Copyright "The Wallenberg AI, Autonomous Systems and Software Program". All Rights Reserved.


#include "WaspRuntime/Public/WaspRuntimeUtilityLibrary.h"

#include "LevelSequence.h"
#include "MovieScene.h"
#include "Animation/SkeletalMeshActor.h"
#include "Misc/FrameRate.h"
#include "Misc/FrameTime.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"

bool UWaspRuntimeUtilityLibrary::AddAnimationsToLevelSequence(ULevelSequence* InLevelSequence, UDataTable* InAnimationParams)
{
	if (!InLevelSequence)
	{
		return false;
	}

	if (!InAnimationParams)
	{
		return false;
	}
	static const FString ContextString(TEXT("UWaspRuntimeUtilityLibrary::AddAnimationToLevelSequence"));
	TArray<FAnimationTrackAddParams*> ParamsArray;
	InAnimationParams->GetAllRows<FAnimationTrackAddParams>(ContextString, ParamsArray);

	UMovieScene* MovieScene = InLevelSequence->GetMovieScene();
	if (!MovieScene)
	{
		return false;
	}

	for (const FAnimationTrackAddParams* Params : ParamsArray)
	{
		if (FMovieSceneSpawnable* Spawnable = FindCompatibleSpawnableForAnimation(MovieScene, Params->Animation))
		{
			UMovieSceneSkeletalAnimationTrack* TargetTrack = FindOrCreateAnimationTrackForSpawnable(MovieScene, *Spawnable);
			AddAnimationToAnimationTrack(MovieScene, TargetTrack, *Params);
		}
	}

	return true;
}

void UWaspRuntimeUtilityLibrary::GetAllTracksOfType(const FTrackSearchParams& Params, UMovieScene* InMovieScene, TArray<UMovieSceneTrack*>& OutArray)
{
	// Non-spawnable tracks
	if (Params.bSearchNonSpawnable)
	{
		for (UMovieSceneTrack* Track : InMovieScene->GetTracks())
		{
			if (Track->IsA(Params.TrackType))
			{
				OutArray.Add(Track);
			}
		}
	}

	// Spawnable tracks
	if (Params.bSearchSpawnable)
	{
		for (const FMovieSceneBinding Binding : InMovieScene->GetBindings())
		{
			for (UMovieSceneTrack* Track : Binding.GetTracks())
			{
				if (Track->IsA(Params.TrackType))
				{
					OutArray.Add(Track);
				}
			}
		}
	}
}

double UWaspRuntimeUtilityLibrary::GetLastSectionEndTime(const TArray<UMovieSceneTrack*>& InTracks)
{
	double LastSectionEndTime = 0;
	FFrameRate FrameRate = FFrameRate(-1, -1); // Set invalid framerate

	// Iterate tracks
	for (const UMovieSceneTrack* Track : InTracks)
	{
		// Ensure framerate is consistent
		FFrameRate SectionFrameRate = Track->GetTypedOuter<UMovieScene>()->GetTickResolution();		
		if (FrameRate.IsValid())
		{
			ensure(SectionFrameRate.IsValid());
			ensure(FrameRate == SectionFrameRate);
		}
		else
		{
			FrameRate = SectionFrameRate;
		}

		// We should have a valid framerate
		if (!ensure(FrameRate.IsValid()))
		{
			UE_LOG(LogTemp, Error, TEXT("The track framerate is invalid!"));
			return 0;
		}

		// Iterate sections
		for (const UMovieSceneSection* Section : Track->GetAllSections())
		{
			if (Section->HasEndFrame())
			{
				const double SectionEndTime = Section->SectionRange.Value.GetUpperBoundValue() / FrameRate;
				LastSectionEndTime = FMath::Max(LastSectionEndTime, SectionEndTime);
			}
		}
	}
	
	return LastSectionEndTime;
}

UMovieSceneSkeletalAnimationTrack* UWaspRuntimeUtilityLibrary::FindOrCreateAnimationTrackForSpawnable(UMovieScene* InMovieScene, const FMovieSceneSpawnable& TargetSpawnable)
{
	UMovieSceneSkeletalAnimationTrack* SkeletalTrack = nullptr;

	// Search for any skeletal track
	if (FMovieSceneBinding* Binding = InMovieScene->FindBinding(TargetSpawnable.GetGuid()))
	{
		for (UMovieSceneTrack* Track : Binding->GetTracks())
		{
			SkeletalTrack = Cast<UMovieSceneSkeletalAnimationTrack>(Track);
			if (SkeletalTrack)
			{
				return SkeletalTrack;
			}
		}
	}

	// No track found; create one
	// TODO: create track

	ensure(SkeletalTrack); // We should have a track by now
	return SkeletalTrack;
}

FMovieSceneSpawnable* UWaspRuntimeUtilityLibrary::FindCompatibleSpawnableForAnimation(UMovieScene* InMovieScene, const UAnimSequence* InAnimation)
{
	for (FMovieSceneBinding Binding : InMovieScene->GetBindings())
	{
		if (FMovieSceneSpawnable* Spawnable = InMovieScene->FindSpawnable(Binding.GetObjectGuid()))
		{
			if (IsMovieSceneSpawnableCompatibleWithAnimation(*Spawnable, InAnimation))
			{
				return Spawnable;
			}
		}
	}
	return nullptr;
}

bool UWaspRuntimeUtilityLibrary::IsMovieSceneSpawnableCompatibleWithAnimation(FMovieSceneSpawnable& InSpawnable, const UAnimSequence* InAnimation)
{
	if (InAnimation)
	{
		return IsMovieSceneSpawnableCompatibleWithSkeleton(InSpawnable, InAnimation->GetSkeleton());
	}
	return false;
}

bool UWaspRuntimeUtilityLibrary::IsMovieSceneSpawnableCompatibleWithSkeleton(FMovieSceneSpawnable& InSpawnable, const USkeleton* InSkeleton)
{
	if (InSkeleton)
	{
		if (TObjectPtr<ASkeletalMeshActor> SkeletalMeshActor = Cast<ASkeletalMeshActor>(InSpawnable.GetObjectTemplate()))
		{
			if (TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent = SkeletalMeshActor->GetSkeletalMeshComponent())
			{
				if (TObjectPtr<USkeletalMesh> SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset())
				{
					if (TObjectPtr<USkeleton> Skeleton = SkeletalMesh->GetSkeleton())
					{
						FString SkeletonAssetName = FAssetData(Skeleton).GetExportTextName();
						FString TargetSkeletonAssetName = FAssetData(InSkeleton).GetExportTextName();
						if (SkeletonAssetName == TargetSkeletonAssetName)
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

void UWaspRuntimeUtilityLibrary::AddAnimationToAnimationTrack(UMovieScene* InMovieScene, UMovieSceneSkeletalAnimationTrack* InTrack, const FAnimationTrackAddParams Params)
{
	if (ensure(Params.Animation) && ensure(InMovieScene) && ensure(InTrack))
	{
		// General Sequencer variables
		const FFrameRate FrameRate = InMovieScene->GetTickResolution();
		
		// Get the last section end frame time (in seconds)
		FTrackSearchParams SearchParams;
		SearchParams.TrackType = UMovieSceneSkeletalAnimationTrack::StaticClass();
		TArray<UMovieSceneTrack*> Tracks;
		GetAllTracksOfType(SearchParams, InMovieScene, Tracks);
		const double LastSectionEndTime = GetLastSectionEndTime(Tracks);

		// Compute start time (in seconds) of inserted clip, accounting for start trim
		double StartTime = 0;
		const double StartOffsetTime = FMath::Abs(Params.StartOffset); // Ensure only positive numbers
		const double EndTrimTime = FMath::Abs(Params.EndTrim); // Ensure only positive numbers
		switch (Params.TimeMode) {
			case EWaspAnimationAddTimeMode::Precise:
				StartTime = Params.Time - StartOffsetTime; // Negative time allowed in precise mode
				break;
			case EWaspAnimationAddTimeMode::LastAnimationOffset:
				StartTime = LastSectionEndTime + FMath::Abs(Params.Time) - StartOffsetTime;
				break;
			case EWaspAnimationAddTimeMode::Blend:
				StartTime = LastSectionEndTime - FMath::Abs(Params.Time) - StartOffsetTime;
				break;
		}

		// Insert the clip at start time
		const FFrameNumber StartFrame = (StartTime * FrameRate).RoundToFrame();
		InTrack->Modify();
		UMovieSceneSection* NewSection = InTrack->AddNewAnimation(StartFrame, Params.Animation);

		// Trim the clip
		const float AnimationDuration = Params.Animation->GetPlayLength();
		const FFrameTime StartOffsetFrameTime = FrameRate.AsFrameTime(StartOffsetTime);
		const FFrameTime EndTrimFrameTime = FrameRate.AsFrameTime(AnimationDuration - EndTrimTime);
		const FQualifiedFrameTime StartOffsetQFrameTime = FQualifiedFrameTime(StartOffsetFrameTime + StartFrame, FrameRate);
		const FQualifiedFrameTime EndTrimQFrameTime = FQualifiedFrameTime(EndTrimFrameTime + StartFrame, FrameRate);
		NewSection->Modify();
		NewSection->TrimSection(StartOffsetQFrameTime, true, false);
		NewSection->TrimSection(EndTrimQFrameTime, false, false);

		// Move section to different row (e.g. to blend animations)
		UMovieSceneSkeletalAnimationSection* NewSectionCasted = Cast<UMovieSceneSkeletalAnimationSection>(NewSection);
		if (Params.bBlend)
		{
			const int32 RowIndex = 0;
			NewSectionCasted->SetRowIndex(RowIndex);
			InTrack->UpdateEasing();
		}

		UE_LOG(LogTemp, Log, TEXT("Start %.4f | LastSectionEnd %.4f | StartTrim %.4f | EndTrim %.4f"), StartTime, LastSectionEndTime, StartOffsetTime, EndTrimTime)
	}
}
