#include "Vision/LSVisionMaskRenderer.h"

#include "CoreGlobals.h"
#include "Engine/TextureRenderTarget2D.h"
#include "LSVisionGlobalShader.h"
#include "RenderGraphUtils.h"

ALSVisionMaskRenderer::ALSVisionMaskRenderer()
{
	PrimaryActorTick.bCanEverTick = false;
}

// Uploads polygon data to the render thread and refreshes the visibility mask texture.
void ALSVisionMaskRenderer::RequestMaskUpdate(const FLSVisionPolygonData& PolygonData)
{
	LastRenderedPolygon = PolygonData;

	if (VisibilityMaskRenderTarget == nullptr || IsEngineExitRequested())
	{
		return;
	}

	FTextureRenderTargetResource* RenderTargetResource = VisibilityMaskRenderTarget->GameThread_GetRenderTargetResource();
	if (RenderTargetResource == nullptr)
	{
		return;
	}

	TArray<FVector2f> PolygonUploadData;
	PolygonUploadData.Reserve(PolygonData.Points.Num());

	for (const FVector2D& Point : PolygonData.Points)
	{
		const FVector2D LocalPoint = Point - PolygonData.Origin;
		PolygonUploadData.Add(FVector2f(LocalPoint));
	}

	const float VisionRadius = PolygonData.VisionRadius;
	const float Extent = PolygonData.Extent;

	ENQUEUE_RENDER_COMMAND(RenderLostSignalVisionMask)(
		[RenderTargetResource, VisionRadius, Extent, PolygonUploadData = MoveTemp(PolygonUploadData)](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			FRHITexture* OutputTextureRHI = RenderTargetResource->GetRenderTargetTexture();
			if (OutputTextureRHI == nullptr)
			{
				return;
			}

			FRDGTextureRef OutputTexture = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(OutputTextureRHI, TEXT("LostSignalVisionMaskRT")));

			LSVision::FMaskDispatchInputs Inputs;
			Inputs.OutputTexture = OutputTexture;
			Inputs.VisionOrigin = FVector2f::ZeroVector;
			Inputs.VisionRadius = VisionRadius;
			Inputs.FeatherWidth = 30.0f;
			Inputs.WorldMin = FVector2f(-Extent, -Extent);
			Inputs.WorldMax = FVector2f(Extent, Extent);
			Inputs.VisibleColor = FLinearColor::White;
			Inputs.HiddenColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

			Inputs.PolygonPointCount = PolygonUploadData.Num();
			if (PolygonUploadData.Num() > 0)
			{
				FRDGBufferRef PolygonBuffer = CreateStructuredBuffer(
					GraphBuilder,
					TEXT("LostSignalVision.PolygonPoints"),
					PolygonUploadData);

				Inputs.PolygonPointsSRV = GraphBuilder.CreateSRV(PolygonBuffer);
			}

			LSVision::AddVisionMaskPass(GraphBuilder, Inputs);
			GraphBuilder.Execute();
		});
}
