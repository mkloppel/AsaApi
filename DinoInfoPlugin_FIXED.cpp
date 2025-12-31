#include <API/ARK/Ark.h>

// ------------------------------------------------------------------------------------------------
// CORRECTED VERSION - Shows all the fixes
// ------------------------------------------------------------------------------------------------

// Get the dino the player is aiming at
APrimalDinoCharacter* GetTargetedDino(AShooterPlayerController* controller)
{
    if (!controller) return nullptr;

    // Get player's view point
    UE::Math::TVector<double> location;
    UE::Math::TRotator<double> rotation;
    controller->GetPlayerViewPoint(&location, &rotation);

    // Get cheat manager (required for GetAimedTargetFromLocation)
    UShooterCheatManager* cheat_mgr = AsaApi::GetApiUtils().GetCheatManagerByPC(controller);
    if (!cheat_mgr) return nullptr;

    // THIS IS THE CORRECT TARGETING METHOD
    AActor* actor = cheat_mgr->GetAimedTargetFromLocation(&location, &rotation, nullptr);

    // Check if it's a dino - FIXED: Use GetPrivateStaticClass()
    if (actor && actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
    {
        APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);

        // FIXED: Correct field accessor syntax (single parentheses)
        if (dino && !dino->bIsDeadField())
        {
            return dino;
        }
    }

    return nullptr;
}

// Chat command handler: /dinoinfo
void DinoInfoCommand(AShooterPlayerController* controller, TArray<FString>* parsed, int mode)
{
    if (!controller) return;

    APrimalDinoCharacter* dino = GetTargetedDino(controller);
    if (!dino)
    {
        AsaApi::GetApiUtils().SendServerMessage(controller, FLinearColor(1, 0, 0),
            "No valid dino targeted. Aim at a creature and try again.");
        return;
    }

    // Get dino name
    FString name = dino->DescriptiveNameField();

    // Get status component
    UPrimalCharacterStatusComponent* status = dino->MyCharacterStatusComponentField();
    if (!status)
    {
        AsaApi::GetApiUtils().SendServerMessage(controller, FLinearColor(1, 0, 0),
            "Could not access dino stats.");
        return;
    }

    // FIXED: Correct field accessor syntax (no double parentheses)
    TArray<float>& current = status->CurrentStatusValuesField();
    TArray<float>& max = status->MaxStatusValuesField();

    // FIXED: Correct field accessor syntax (single parentheses)
    int base = status->BaseCharacterLevelField();
    int extra = status->ExtraCharacterLevelField();

    // Get stats
    float hp = current[(int)EPrimalCharacterStatusValue::Health];
    float max_hp = max[(int)EPrimalCharacterStatusValue::Health];
    float stamina = current[(int)EPrimalCharacterStatusValue::Stamina];
    float max_stamina = max[(int)EPrimalCharacterStatusValue::Stamina];

    int level = base + extra;

    // Build info string
    FString info = FString::Format(
        "Targeted: Lvl {} {} | HP: {:.0f}/{:.0f} | Stam: {:.0f}/{:.0f}",
        level, *name.ToString(), hp, max_hp, stamina, max_stamina
    );

    // FIXED: Use SendServerMessage with color, and pass FString directly (no dereference)
    AsaApi::GetApiUtils().SendServerMessage(controller, FLinearColor(0, 1, 0), info);
}

// ------------------------------------------------------------------------------------------------
// PLUGIN INIT/UNLOAD
// ------------------------------------------------------------------------------------------------

extern "C" __declspec(dllexport) void Plugin_Init()
{
    // FIXED: Use modern Logger API
    Log::GetLog()->info("DinoInfoPlugin loaded!");

    // FIXED: Include slash prefix in command
    AsaApi::GetCommands().AddChatCommand("/dinoinfo", &DinoInfoCommand);
}

extern "C" __declspec(dllexport) void Plugin_Unload()
{
    Log::GetLog()->info("DinoInfoPlugin unloaded!");

    // FIXED: Use same prefix as registration
    AsaApi::GetCommands().RemoveChatCommand("/dinoinfo");
}
