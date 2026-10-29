#pragma once

#include "CoreMinimal.h"
#include "KrerusAttributeTypes.generated.h"

// Atributos primarios del configuracion inicial de GAS, antes y después de aplicar modificadores.
USTRUCT(BlueprintType)
struct FKrerusPrimaryAttributes
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float Strength = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float Agility = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float Intellect = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float Stamina = 10.0f;
};

// Bonificaciones aditivas identificadas por GUID para retirarlas sin alterar otros modificadores.
USTRUCT(BlueprintType)
struct FKrerusAttributeModifier
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    FGuid Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float Strength = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float Agility = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float Intellect = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float Stamina = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Derived")
    float AttackPower = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Derived")
    float SpellPower = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Derived")
    float Armor = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Derived")
    float CriticalChance = 0.0f;
};

// Estadísticas calculadas por GAS; el autoataque consulta AttackPower de GAS.
USTRUCT(BlueprintType)
struct FKrerusDerivedAttributes
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    float AttackPower = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    float SpellPower = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    float Armor = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    float CriticalChance = 0.0f;
};

// Coeficientes editables usados por AttributeComponent al derivar estadísticas y vida máxima.
USTRUCT(BlueprintType)
struct FKrerusAttributeScaling
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
    float StrengthToAttackPower = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
    float AgilityToAttackPower = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
    float IntellectToSpellPower = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
    float AgilityToCriticalChance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
    float StaminaToArmor = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
    float StaminaToHealth = 10.0f;
};