// Copyright "The Wallenberg AI, Autonomous Systems and Software Program". All Rights Reserved.


#include "WaspRuntime/Public/WaspRuntimeUtilityLibrary.h"

#include "LevelSequence.h"
#include "MovieScene.h"
#include "Animation/SkeletalMeshActor.h"
#include "Misc/FrameRate.h"
#include "Misc/FrameTime.h"
#include "Tracks/MovieSceneAudioTrack.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"

bool UWaspRuntimeUtilityLibrary::AddDataToLevelSequence(ULevelSequence* InLevelSequence, UDataTable* InDataParams)
{
	if (!InLevelSequence)
	{
		return false;
	}

	if (!InDataParams)
	{
		return false;
	}
	static const FString ContextString(TEXT("UWaspRuntimeUtilityLibrary::AddDataToLevelSequence"));
	TArray<FAnimationTrackAddParams*> ParamsArray;
	InDataParams->GetAllRows<FAnimationTrackAddParams>(ContextString, ParamsArray);

	UMovieScene* MovieScene = InLevelSequence->GetMovieScene();
	if (!MovieScene)
	{
		return false;
	}

	for (const FAnimationTrackAddParams* ParamsPtr : ParamsArray)
	{
		const FAnimationTrackAddParams& Params = *ParamsPtr;
		if (FMovieSceneSpawnable* Spawnable = FindCompatibleSpawnableForAnimation(MovieScene, Params.Animation))
		{			
			// Get the last section end frame time (in seconds)
			const double LastSectionEndTime = GetLastSectionEndTime(UMovieSceneSkeletalAnimationTrack::StaticClass(), MovieScene);

			// Calculate start time
			const double StartTrimTime = FMath::Abs(Params.StartOffset);
			const double EndTrimTime = FMath::Abs(Params.EndTrim);
			double Time = 0;
			switch (Params.TimeMode) {
				case EWaspAnimationAddTimeMode::Precise:
					Time = Params.Time - StartTrimTime; // Negative time allowed in precise mode
					break;
				case EWaspAnimationAddTimeMode::LastAnimationOffset:
					Time = LastSectionEndTime + FMath::Abs(Params.Time) - StartTrimTime;
					break;
				case EWaspAnimationAddTimeMode::Blend:
					Time = LastSectionEndTime - FMath::Abs(Params.Time) - StartTrimTime;
					break;
			}

			// Audio
			if (Params.Audio)
			{
				// Track
				TArray<UMovieSceneTrack*> AudioTracks;
				FTrackSearchParams SearchAudioParams;
				SearchAudioParams.TrackType = UMovieSceneAudioTrack::StaticClass();
				SearchAudioParams.SpawnableGuid = Spawnable->GetGuid();
				SearchAudioParams.MovieScene = MovieScene;;
				GetAllTracksOfType(SearchAudioParams, AudioTracks);
				UMovieSceneAudioTrack* AudioTrack = nullptr;
				if (AudioTracks.Num())
				{
					AudioTrack = Cast<UMovieSceneAudioTrack>(AudioTracks[0]);
					check(AudioTrack);
				}

				// Params
				FSectionInjectParams InjectParamsAudio;
				InjectParamsAudio.Data = Params.Audio;
				InjectParamsAudio.Track = AudioTrack;
				InjectParamsAudio.RowIndex = 0;
				InjectParamsAudio.Time = Time;
				InjectParamsAudio.StartTrim = StartTrimTime;
				InjectParamsAudio.EndTrim = EndTrimTime;
				AddAudioToTrack(InjectParamsAudio);
				UE_LOG(LogTemp, Log, TEXT("Injected audio: Start %.4f | LastSectionEnd %.4f | StartTrim %.4f | EndTrim %.4f"), InjectParamsAudio.Time, LastSectionEndTime, InjectParamsAudio.StartTrim, InjectParamsAudio.EndTrim);
			}

			// Animation
			if (Params.Animation)
			{
				// Track
				TArray<UMovieSceneTrack*> AnimationTracks;
				FTrackSearchParams SearchAnimationParams;
				SearchAnimationParams.TrackType = UMovieSceneSkeletalAnimationTrack::StaticClass();
				SearchAnimationParams.SpawnableGuid = Spawnable->GetGuid();
				SearchAnimationParams.MovieScene = MovieScene;;
				GetAllTracksOfType(SearchAnimationParams, AnimationTracks);
				UMovieSceneSkeletalAnimationTrack* AnimationTrack = nullptr;
				if (AnimationTracks.Num())
				{
					AnimationTrack = Cast<UMovieSceneSkeletalAnimationTrack>(AnimationTracks[0]);
					check(AnimationTrack);
				}

				// Params
				FSectionInjectParams InjectParamsAnimation;
				InjectParamsAnimation.Data = Params.Animation;
				InjectParamsAnimation.Track = AnimationTrack;
				InjectParamsAnimation.RowIndex = 0;
				InjectParamsAnimation.Time = Time;
				InjectParamsAnimation.StartTrim = StartTrimTime;
				InjectParamsAnimation.EndTrim = EndTrimTime;
				AddAnimationToTrack(InjectParamsAnimation);
				UE_LOG(LogTemp, Log, TEXT("Injected animation: Start %.4f | LastSectionEnd %.4f | StartTrim %.4f | EndTrim %.4f"), InjectParamsAnimation.Time, LastSectionEndTime, InjectParamsAnimation.StartTrim, InjectParamsAnimation.EndTrim);
			}
		}
	}

	return true;
}

bool UWaspRuntimeUtilityLibrary::AddAudioToTrack(const FSectionInjectParams Params)
{
	UMovieSceneAudioTrack* CastTrack = Cast<UMovieSceneAudioTrack>(Params.Track);
	if (!CastTrack)
	{
		return false;
	}

	USoundBase* Sound = Cast<USoundBase>(Params.Data);
	if (!Sound)
	{
		return false;
	}

	// Insert section
	CastTrack->Modify();
	UMovieSceneSection* NewSection = CastTrack->AddNewSound(Sound, Params.GetTimeAsFrameNumber());

	// Trim section
	NewSection->Modify();
	NewSection->TrimSection(Params.GetStartTrimFrameTime(), true, false);
	NewSection->TrimSection(Params.GetEndTrimFrameTime(Sound->GetDuration()), false, false);

	// Move section
	NewSection->SetRowIndex(Params.RowIndex);
	CastTrack->UpdateEasing();
	
	return true;
}

bool UWaspRuntimeUtilityLibrary::AddAnimationToTrack(const FSectionInjectParams Params)
{
	UMovieSceneSkeletalAnimationTrack* CastTrack = Cast<UMovieSceneSkeletalAnimationTrack>(Params.Track);
	if (!CastTrack)
	{
		return false;
	}

	UAnimSequence* Animation = Cast<UAnimSequence>(Params.Data);
	if (!Animation)
	{
		return false;
	}
	
	// Insert section
	CastTrack->Modify();
	UMovieSceneSection* NewSection = CastTrack->AddNewAnimation(Params.GetTimeAsFrameNumber(), Animation);

	// Trim section
	NewSection->Modify();
	NewSection->TrimSection(Params.GetStartTrimFrameTime(), true, false);
	NewSection->TrimSection(Params.GetEndTrimFrameTime(Animation->GetPlayLength()), false, false);

	// Move section
	NewSection->SetRowIndex(Params.RowIndex);
	CastTrack->UpdateEasing();
	
	return true;
}

void UWaspRuntimeUtilityLibrary::GetAllTracksOfType(const FTrackSearchParams& Params, TArray<UMovieSceneTrack*>& OutArray)
{
	if (!ensure(Params.MovieScene))
	{
		return;
	}

	// Non-spawnable tracks
	if (Params.bSearchNonSpawnable)
	{
		for (UMovieSceneTrack* Track : Params.MovieScene->GetTracks())
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
		for (const FMovieSceneBinding Binding : Params.MovieScene->GetBindings())
		{
			if (Params.SpawnableGuid.IsValid())
			{
				if (Binding.GetObjectGuid() != Params.SpawnableGuid)
				{
					continue;
				}
			}

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

double UWaspRuntimeUtilityLibrary::GetLastSectionEndTime(TSubclassOf<UMovieSceneTrack> TrackType, UMovieScene* InMovieScene)
{
	double LastSectionEndTime = 0;
	FFrameRate FrameRate = FFrameRate(-1, -1); // Set invalid framerate

	if (!InMovieScene)
	{
		return LastSectionEndTime;
	}

	// Find tracks to compare
	FTrackSearchParams SearchParams;
	SearchParams.TrackType = TrackType;
	SearchParams.MovieScene = InMovieScene;
	TArray<UMovieSceneTrack*> Tracks;
	GetAllTracksOfType(SearchParams, Tracks);
	
	// Iterate tracks
	for (const UMovieSceneTrack* Track : Tracks)
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
