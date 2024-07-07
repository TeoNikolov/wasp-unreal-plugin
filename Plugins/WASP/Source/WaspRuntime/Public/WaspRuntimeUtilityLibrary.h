// Copyright "The Wallenberg AI, Autonomous Systems and Software Program". All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WaspRuntimeUtilityLibrary.generated.h"

struct FMovieSceneSpawnable;
struct FFrameNumber;
class UMovieSceneSkeletalAnimationTrack;
class ULevelSequence;
class UAnimSequence;
class UMovieSceneTrack;
class UDataTable;

UENUM()
enum class EWaspAnimationAddTimeMode : uint8
{
	Precise,
	LastAnimationOffset,
	Blend
};

USTRUCT(BlueprintType)
struct WASPRUNTIME_API FAnimationTrackAddParams : public FTableRowBase
{
	GENERATED_BODY()
	
	/** Animation clip to inject. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	UAnimSequence* Animation { nullptr };

	/** Audio clip to inject. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	USoundBase* Audio { nullptr };
	
	/** The time mode for which the Time value applies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	EWaspAnimationAddTimeMode TimeMode { EWaspAnimationAddTimeMode::Precise };
	
	/** The time at which to inject the animation clip in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	double Time { 0.0 };

	/** An offset from the animation start. Animation before the offset is trimmed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	double StartOffset { 0.0 };
	
	/** An offset from the animation end to trim. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	double EndTrim { 0.0 };

	/** Whether the animation should be blended. TODO: Define how it will be blended. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	bool bBlend { false };
	
	FAnimationTrackAddParams() {}
	FAnimationTrackAddParams(UAnimSequence* InAnimation, const EWaspAnimationAddTimeMode InTimeMode, const double InTime)
		: Animation(InAnimation), TimeMode(InTimeMode), Time(InTime), StartOffset(0.0), EndTrim(0.0), bBlend(false) {}
};

USTRUCT(BlueprintType)
struct FSectionInjectParams
{
	GENERATED_BODY()

	UPROPERTY()
	UObject* Data { nullptr };
	
	UPROPERTY()
	UMovieSceneTrack* Track { nullptr };
	
	UPROPERTY()
	int32 RowIndex { 0 };

	UPROPERTY()
	double Time { 0.0 };
	
	UPROPERTY()
	double StartTrim { 0.0 };
	
	UPROPERTY()
	double EndTrim { 0.0 };
	
	FFrameRate GetFrameRate() const
	{
		FFrameRate FrameRate = FFrameRate(-1, -1);
		if (Track)
		{
			if (const UMovieScene* MovieScene = Track->GetTypedOuter<UMovieScene>())
			{
				FrameRate = MovieScene->GetTickResolution();
			}
		}
		return FrameRate;
	}

	FFrameNumber GetTimeAsFrameNumber() const
	{
		return (Time * GetFrameRate()).RoundToFrame();
	}

	FQualifiedFrameTime GetStartTrimFrameTime() const
	{
		const FFrameTime StartOffsetFrameTime = GetFrameRate().AsFrameTime(StartTrim);
		return FQualifiedFrameTime(StartOffsetFrameTime + GetTimeAsFrameNumber(), GetFrameRate());
	}

	FQualifiedFrameTime GetEndTrimFrameTime(const double SectionDuration) const
	{
		const FFrameTime EndTrimFrameTime = GetFrameRate().AsFrameTime(SectionDuration - EndTrim);
		return FQualifiedFrameTime(EndTrimFrameTime + GetTimeAsFrameNumber(), GetFrameRate());
	}
	
};

USTRUCT(BlueprintType)
struct FTrackSearchParams
{

	GENERATED_BODY()

	/** Whether to search through tracks bound to non-spawnable / non-posessable objects. */
	UPROPERTY()
	bool bSearchNonSpawnable { true };

	/** Whether to search through tracks bound to spawnable / posessable objects. */
	UPROPERTY()
	bool bSearchSpawnable { true };

	/** A Guid to filter by when spawnables are searched through. */
	UPROPERTY()
	FGuid SpawnableGuid { FGuid() };

	/** Movie scene in which to search for tracks. */
	UPROPERTY()
	UMovieScene* MovieScene { nullptr };
	
	/** A type of track to filter by. */
	UPROPERTY()
	TSubclassOf<UMovieSceneTrack> TrackType { UObject::StaticClass() };
	
};

/**
 * Globally callable helper functions.
 */
UCLASS()
class WASPRUNTIME_API UWaspRuntimeUtilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	/** Adds a list of animations to a skeleton track in a LevelSequence, given a data table of params. */
	UFUNCTION(BlueprintCallable, Category = "WASP")
	static bool AddDataToLevelSequence(ULevelSequence* InLevelSequence, UDataTable* InDataParams);

	UFUNCTION(BlueprintCallable, Category = "WASP")
	static bool AddAudioToTrack(const FSectionInjectParams Params);

	UFUNCTION(BlueprintCallable, Category = "WASP")
	static bool AddAnimationToTrack(const FSectionInjectParams Params);
	
	/** Populates an array with tracks of a given type found in a movie scene. */
	UFUNCTION(BlueprintCallable, Category = "WASP")
	static void GetAllTracksOfType(const FTrackSearchParams& Params, TArray<UMovieSceneTrack*>& OutArray);
	
	/** Returns the time (in seconds) of the last section end in a list of tracks. */
	static double GetLastSectionEndTime(TSubclassOf<UMovieSceneTrack> TrackType, UMovieScene* InMovieScene);
	
	static UMovieSceneSkeletalAnimationTrack* FindOrCreateAnimationTrackForSpawnable(UMovieScene* InMovieScene, const FMovieSceneSpawnable& TargetSpawnable);
	static FMovieSceneSpawnable* FindCompatibleSpawnableForAnimation(UMovieScene* InMovieScene, const UAnimSequence* InAnimation);
	static bool IsMovieSceneSpawnableCompatibleWithAnimation(FMovieSceneSpawnable& InSpawnable, const UAnimSequence* InAnimation);
	static bool IsMovieSceneSpawnableCompatibleWithSkeleton(FMovieSceneSpawnable& InSpawnable, const USkeleton* InSkeleton);
	
};
