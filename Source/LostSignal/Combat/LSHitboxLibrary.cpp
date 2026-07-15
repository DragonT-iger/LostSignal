#include "Combat/LSHitboxLibrary.h"

#include "Data/LSCharacterSkillRow.h"

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

bool ULSHitboxLibrary::IsTargetInsideSkillRange(
	const FVector& SourceLocation,
	const FVector& AimDirection2D,
	const FVector& TargetLocation,
	ELSCharacterSkillRangeShape Shape,
	float RangeX,
	float RangeY)
{
	ELSHitboxShape HitboxShape = ELSHitboxShape::Circle;
	switch (Shape)
	{
	case ELSCharacterSkillRangeShape::Cone:
		HitboxShape = ELSHitboxShape::Cone;
		break;
	case ELSCharacterSkillRangeShape::Box:
		HitboxShape = ELSHitboxShape::Box;
		break;
	case ELSCharacterSkillRangeShape::Circle:
	case ELSCharacterSkillRangeShape::None:
	default:
		HitboxShape = ELSHitboxShape::Circle;
		break;
	}

	return IsTargetInsideHitbox(
		SourceLocation,
		AimDirection2D,
		TargetLocation,
		HitboxShape,
		RangeX,
		RangeX,
		RangeY,
		RangeY);
}

float ULSHitboxLibrary::GetSkillRangeQueryRadius(ELSCharacterSkillRangeShape Shape, float RangeX, float RangeY)
{
	return Shape == ELSCharacterSkillRangeShape::Box
		? FMath::Sqrt(FMath::Square(RangeX) + FMath::Square(RangeY * 0.5f))
		: RangeX;
}
