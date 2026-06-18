#pragma once

#include "AbilitySystemComponent.h"

// 모든 LS AttributeSet 공용 접근자 매크로.
// 어트리뷰트마다 Property Getter / Value Getter / Setter / Initter를 생성한다.
// 셋별로 따로 정의하지 말고 이 헤더를 include 해서 LS_ATTRIBUTE_ACCESSORS만 사용한다.
#define LS_ATTRIBUTE_ACCESSORS(ClassName, PropertyName)        \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
