// Copyright "The Wallenberg AI, Autonomous Systems and Software Program". All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WaspRuntimeUtilityLibrary.generated.h"

struct FMovieSceneSpawnable;
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
		
	/**
	 * Adds an animation clip to a given skeletal track.
	 * @param InMovieScene Movie scene in which the track is located.
	 * @param InTrack Track into which to inject an animation clip.
	 * @param Params Parameters that define how to add the animation to the track.
	 * @param bBlend Whether to place and blend animation onto an existing track.
	 */
	UFUNCTION(BlueprintCallable, Category = "WASP")
	static void AddAnimationToAnimationTrack(UMovieScene* InMovieScene, UMovieSceneSkeletalAnimationTrack* InTrack, const FAnimationTrackAddParams Params);
	
	/** Adds a list of animations to a skeleton track in a LevelSequence, given a data table of params. */
	UFUNCTION(BlueprintCallable, Category = "WASP")
	static bool AddAnimationsToLevelSequence(ULevelSequence* InLevelSequence, UDataTable* InAnimationParams);

	/** Populates an array with tracks of a given type found in a movie scene. */
	UFUNCTION(BlueprintCallable, Category = "WASP")
	static void GetAllTracksOfType(const FTrackSearchParams& Params, UMovieScene* InMovieScene, TArray<UMovieSceneTrack*>& OutArray);

	/** Returns the time (in seconds) of the last section end in a list of tracks. */
	static double GetLastSectionEndTime(const TArray<UMovieSceneTrack*>& InTracks);
	
	static UMovieSceneSkeletalAnimationTrack* FindOrCreateAnimationTrackForSpawnable(UMovieScene* InMovieScene, const FMovieSceneSpawnable& TargetSpawnable);
	static FMovieSceneSpawnable* FindCompatibleSpawnableForAnimation(UMovieScene* InMovieScene, const UAnimSequence* InAnimation);
	static bool IsMovieSceneSpawnableCompatibleWithAnimation(FMovieSceneSpawnable& InSpawnable, const UAnimSequence* InAnimation);
	static bool IsMovieSceneSpawnableCompatibleWithSkeleton(FMovieSceneSpawnable& InSpawnable, const USkeleton* InSkeleton);
	
};
