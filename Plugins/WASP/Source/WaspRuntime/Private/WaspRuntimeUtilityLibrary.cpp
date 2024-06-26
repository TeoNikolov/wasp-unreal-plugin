// Copyright "The Wallenberg AI, Autonomous Systems and Software Program". All Rights Reserved.


#include "WaspRuntime/Public/WaspRuntimeUtilityLibrary.h"

#include "LevelSequence.h"
#include "MovieScene.h"
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
	
	UMovieSceneSkeletalAnimationTrack* TargetTrack = FindFirstSkeletonTrackInLevelSequence(InLevelSequence);
	if (!TargetTrack)
	{
		return false;
	}

	for (const FAnimationTrackAddParams* Params : ParamsArray)
	{
		AddAnimationToSkeletonTrack(MovieScene, TargetTrack, *Params);
	}

	return true;
}

UMovieSceneSkeletalAnimationTrack* UWaspRuntimeUtilityLibrary::FindFirstSkeletonTrackInLevelSequence(const ULevelSequence* InLevelSequence)
{
	if (const UMovieScene* MovieScene = InLevelSequence->GetMovieScene())
	{
		// Find first compatible skeleton animation track in non-possessables
		UMovieSceneSkeletalAnimationTrack* TargetTrack = FindFirstSkeletonTrackInTrackList(MovieScene->GetTracks());
		if (TargetTrack)
		{
			return TargetTrack;
		}

		// Find first compatible skeleton animation track in possessables
		for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
		{
			TargetTrack = FindFirstSkeletonTrackInTrackList(Binding.GetTracks());
			if (TargetTrack)
			{
				return TargetTrack;
			}
		}
	}

	return nullptr;
}

UMovieSceneSkeletalAnimationTrack* UWaspRuntimeUtilityLibrary::FindFirstSkeletonTrackInTrackList(const TArray<UMovieSceneTrack*>& InTracks)
{
	for (UMovieSceneTrack* Track : InTracks)
	{
		if (UMovieSceneSkeletalAnimationTrack* CastedTrack = Cast<UMovieSceneSkeletalAnimationTrack>(Track))
		{
			return CastedTrack;
		}
	}

	return nullptr;
}

void UWaspRuntimeUtilityLibrary::AddAnimationToSkeletonTrack(UMovieScene* InMovieScene, UMovieSceneSkeletalAnimationTrack* InTrack, const FAnimationTrackAddParams Params)
{
	if (ensure(Params.Animation) && ensure(InMovieScene) && ensure(InTrack))
	{
		// General Sequencer variables
		const FFrameRate FrameRate = InMovieScene->GetTickResolution();
		
		// Get the last section end frame time (in seconds)
		double LastSectionEndTime = 0;
		for (UMovieSceneSection* Section : InTrack->GetAllSections())
		{
			if (Section->HasEndFrame())
			{
				const double SectionEndTime = Section->SectionRange.Value.GetUpperBoundValue() / FrameRate;
				LastSectionEndTime = FMath::Max(LastSectionEndTime, SectionEndTime);
			}
		}

		// Compute start time (in seconds) of inserted clip, accounting for start trim
		double StartTime = 0;
		const double StartTrimTime = FMath::Abs(Params.StartTrim); // Ensure only positive numbers
		const double EndTrimTime = FMath::Abs(Params.EndTrim); // Ensure only positive numbers
		switch (Params.TimeMode) {
			case EWaspAnimationAddTimeMode::Precise:
				StartTime = Params.Time - StartTrimTime; // Negative time allowed in precise mode
				break;
			case EWaspAnimationAddTimeMode::LastAnimationOffset:
				StartTime = LastSectionEndTime + FMath::Abs(Params.Time) - StartTrimTime;
				break;
			case EWaspAnimationAddTimeMode::Blend:
				StartTime = LastSectionEndTime - FMath::Abs(Params.Time) - StartTrimTime;
				break;
		}

		// Insert the clip at start time
		const FFrameNumber StartFrame = (StartTime * FrameRate).RoundToFrame();
		InTrack->Modify();
		UMovieSceneSection* NewSection = InTrack->AddNewAnimation(StartFrame, Params.Animation);

		// Trim the clip
		const float AnimationDuration = Params.Animation->GetPlayLength();
		const FFrameTime StartTrimFrameTime = FrameRate.AsFrameTime(StartTrimTime);
		const FFrameTime EndTrimFrameTime = FrameRate.AsFrameTime(AnimationDuration - EndTrimTime);
		const FQualifiedFrameTime StartTrimQFrameTime = FQualifiedFrameTime(StartTrimFrameTime + StartFrame, FrameRate);
		const FQualifiedFrameTime EndTrimQFrameTime = FQualifiedFrameTime(EndTrimFrameTime + StartFrame, FrameRate);
		NewSection->Modify();
		NewSection->TrimSection(StartTrimQFrameTime, true, false);
		NewSection->TrimSection(EndTrimQFrameTime, false, false);

		// Move section to different row (e.g. to blend animations)
		UMovieSceneSkeletalAnimationSection* NewSectionCasted = Cast<UMovieSceneSkeletalAnimationSection>(NewSection);
		if (Params.bBlend)
		{
			const int32 RowIndex = 0;
			NewSectionCasted->SetRowIndex(RowIndex);
			InTrack->UpdateEasing();
		}

		UE_LOG(LogTemp, Log, TEXT("Start %.4f | LastSectionEnd %.4f | StartTrim %.4f | EndTrim %.4f"), StartTime, LastSectionEndTime, StartTrimTime, EndTrimTime)
	}
}

void UWaspRuntimeUtilityLibrary::AddAnimationsToSkeletonTrack(UMovieScene* InMovieScene, UMovieSceneSkeletalAnimationTrack* InTrack, TArray<FAnimationTrackAddParams> ParamsArray)
{
	for (const FAnimationTrackAddParams Params : ParamsArray)
	{
		AddAnimationToSkeletonTrack(InMovieScene, InTrack, Params);
	}
}
