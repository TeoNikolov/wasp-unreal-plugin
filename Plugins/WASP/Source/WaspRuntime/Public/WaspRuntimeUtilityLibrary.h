// Copyright "The Wallenberg AI, Autonomous Systems and Software Program". All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WaspRuntimeUtilityLibrary.generated.h"

class UMovieScene;
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
	
	/** Animation clip to inject into the animation track. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	UAnimSequence* Animation {nullptr};

	/** The time mode for which the Time value applies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	EWaspAnimationAddTimeMode TimeMode {EWaspAnimationAddTimeMode::Precise};
	
	/** The time at which to inject the animation clip in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	double Time {0.0};

	/** An offset from the animation start. Animation before the offset is trimmed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	double StartOffset {0.0};
	
	/** An offset from the animation end to trim. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	double EndTrim {0.0};

	/** Whether the animation should be blended. TODO: Define how it will be blended. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WASP")
	bool bBlend {false};
	
	FAnimationTrackAddParams() {}
	FAnimationTrackAddParams(UAnimSequence* InAnimation, const EWaspAnimationAddTimeMode InTimeMode, const double InTime)
		: Animation(InAnimation), TimeMode(InTimeMode), Time(InTime), StartOffset(0.0), EndTrim(0.0), bBlend(false) {}
};

/**
 * Globally callable helper functions.
 */
UCLASS()
class WASPRUNTIME_API UWaspRuntimeUtilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	/** Adds a list of animations to a skeleton track in a LevelSequence, given a data table of params. */
	UFUNCTION(BlueprintCallable, Category="WASP")
	static bool AddAnimationsToLevelSequence(ULevelSequence* InLevelSequence, UDataTable* InAnimationParams);
	
	/** Finds a skeleton track in a level sequence to add animations to. Does not check for compatible skeleton! */
	UFUNCTION(BlueprintCallable, Category="WASP")
	static UMovieSceneSkeletalAnimationTrack* FindFirstSkeletonTrackInLevelSequence(const ULevelSequence* InLevelSequence);

	/** Finds a skeleton track in a list of tracks to add animations to. Does not check for compatible skeleton! */
	UFUNCTION(BlueprintCallable, Category="WASP")
	static UMovieSceneSkeletalAnimationTrack* FindFirstSkeletonTrackInTrackList(const TArray<UMovieSceneTrack*>& InTracks);

	/**
	 * Adds an animation clip to a given skeletal track.
	 * @param InMovieScene Movie scene in which the track is located.
	 * @param InTrack Track into which to inject an animation clip.
	 * @param Params Parameters that define how to add the animation to the track.
	 * @param bBlend Whether to place and blend animation onto an existing track.
	 */
	UFUNCTION(BlueprintCallable, Category="WASP")
	static void AddAnimationToSkeletonTrack(UMovieScene* InMovieScene, UMovieSceneSkeletalAnimationTrack* InTrack, const FAnimationTrackAddParams Params);

	/**
	 * Adds an array of animation clips to a given skeletal track.
	 * @param InMovieScene Movie scene in which the track is located.
	 * @param InTrack Track into which to inject an animation clip.
	 * @param ParamsArray Array of parameters that define how to add the animations to the track.
	 * @param bBlend Whether to place and blend animations onto a single track.
	 */
	UFUNCTION(BlueprintCallable, Category="WASP")
	static void AddAnimationsToSkeletonTrack(UMovieScene* InMovieScene, UMovieSceneSkeletalAnimationTrack* InTrack, TArray<FAnimationTrackAddParams> ParamsArray);
	
};
