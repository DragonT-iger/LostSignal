#include "Vision/LSVisionMaskRenderer.h"

#include "Engine/TextureRenderTarget2D.h"
#include "RenderGraphUtils.h"
#include "Vision/LSVisionGlobalShader.h"

ALSVisionMaskRenderer::ALSVisionMaskRenderer()
{
	PrimaryActorTick.bCanEverTick = false;
}

// Uploads polygon data to the render thread and refreshes the visibility mask texture.
void ALSVisionMaskRenderer::RequestMaskUpdate(const FLSVisionPolygonData& PolygonData)
{
	LastRenderedPolygon = PolygonData;

	if (VisibilityMaskRenderTarget == nullptr)
	{
		return;
	}

	FTextureRenderTargetResource* RenderTargetResource = VisibilityMaskRenderTarget->GameThread_GetRenderTargetResource();
	if (RenderTargetResource == nullptr)
	{
		return;
	}

	ENQUEUE_RENDER_COMMAND(RenderLostSignalVisionMask)(
		[RenderTargetResource, PolygonData](FRHICommandListImmediate& RHICmdList)
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
			Inputs.VisionRadius = PolygonData.VisionRadius;
			Inputs.FeatherWidth = 30.0f;
			Inputs.WorldMin = FVector2f(-PolygonData.Extent, -PolygonData.Extent);
			Inputs.WorldMax = FVector2f(PolygonData.Extent, PolygonData.Extent);
			Inputs.VisibleColor = FLinearColor::White;
			Inputs.HiddenColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

			TArray<FVector2f> PolygonUploadData;
			PolygonUploadData.Reserve(PolygonData.Points.Num());

			for (const FVector2D& Point : PolygonData.Points)
			{
				const FVector2D LocalPoint = Point - PolygonData.Origin;
				PolygonUploadData.Add(FVector2f(LocalPoint));
			}

			FRDGBufferRef PolygonBuffer = CreateStructuredBuffer(
				GraphBuilder,
				TEXT("LostSignalVision.PolygonPoints"),
				PolygonUploadData);

			Inputs.PolygonPointCount = PolygonUploadData.Num();
			Inputs.PolygonPointsSRV = GraphBuilder.CreateSRV(PolygonBuffer);

			LSVision::AddVisionMaskPass(GraphBuilder, Inputs);
			GraphBuilder.Execute();
		});
}
