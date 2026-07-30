#pragma once

#include "CoreMinimal.h"
#include "Vision/LSVisionTypes.h"

class UBoxComponent;
class UPrimitiveComponent;

namespace LSVisionCollisionGeometry
{
	LOSTSIGNAL_API void AppendBoxComponentSegments(
		const UBoxComponent* BoxComponent,
		TArray<FLSVisionSegment2D>& OutSegments);

	LOSTSIGNAL_API void AppendCollisionSegments(
		const UPrimitiveComponent* PrimitiveComponent,
		float SliceZ,
		TArray<FLSVisionSegment2D>& OutSegments);

	LOSTSIGNAL_API void AppendMeshBoundsSegments(
		const UPrimitiveComponent* PrimitiveComponent,
		float SliceZ,
		TArray<FLSVisionSegment2D>& OutSegments);

	LOSTSIGNAL_API void AppendPrimitiveBoundsSegments(
		const UPrimitiveComponent* PrimitiveComponent,
		float SliceZ,
		TArray<FLSVisionSegment2D>& OutSegments);
}
