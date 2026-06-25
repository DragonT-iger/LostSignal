#include "Combat/LSHitboxLibrary.h"

bool ULSHitboxLibrary::IsTargetInsideHitbox(
	const FVector& SourceLocation,
	const FVector& AimDirection2D,
	const FVector& TargetLocation,
	ELSHitboxShape Shape,
	float Radius,
	float Length,
	float Width,
	float ConeAngleDegrees)
{
	FVector ToTarget = TargetLocation - SourceLocation;
	ToTarget.Z = 0.0f;
	const float Distance = ToTarget.Size2D();
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		return true;
	}

	switch (Shape)
	{
	case ELSHitboxShape::Cone:
		{
			if (Distance > Radius)
			{
				return false;
			}

			const float Dot = FVector::DotProduct(AimDirection2D, ToTarget.GetSafeNormal2D());
			const float HalfAngle = FMath::Clamp(ConeAngleDegrees * 0.5f, 0.0f, 180.0f);
			return Dot >= FMath::Cos(FMath::DegreesToRadians(HalfAngle));
		}

	case ELSHitboxShape::Box:
		{
			const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, AimDirection2D).GetSafeNormal();
			const float ForwardDistance = FVector::DotProduct(ToTarget, AimDirection2D);
			const float RightDistance = FVector::DotProduct(ToTarget, RightDirection);
			return ForwardDistance >= 0.0f &&
				ForwardDistance <= Length &&
				FMath::Abs(RightDistance) <= Width * 0.5f;
		}

	case ELSHitboxShape::Circle:
	default:
		return Distance <= Radius;
	}
}
