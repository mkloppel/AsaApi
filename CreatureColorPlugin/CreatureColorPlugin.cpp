#include <API/ARK/Ark.h>

// Command: /colorregion <region> <colorid>
// Colors the creature you're looking at if you own it

void ColorAimedCreature(AShooterPlayerController* player_controller, TArray<FString>* parsed, int mode)
{
    if (!player_controller || !parsed || parsed->Num() < 2)
    {
        AsaApi::GetApiUtils().SendServerMessage(player_controller, FLinearColor(1, 0, 0),
            "Usage: /colorregion <region 0-5> <colorid 0-226>");
        return;
    }

    // Parse arguments
    int region = std::stoi(*(*parsed)[0].ToString());
    int color_id = std::stoi(*(*parsed)[1].ToString());

    // Validate inputs
    if (region < 0 || region >= 6)
    {
        AsaApi::GetApiUtils().SendServerMessage(player_controller, FLinearColor(1, 0, 0),
            "Region must be 0-5");
        return;
    }

    if (color_id < 0 || color_id > 226)
    {
        AsaApi::GetApiUtils().SendServerMessage(player_controller, FLinearColor(1, 0, 0),
            "Color ID must be 0-226");
        return;
    }

    // Get player's view point
    UE::Math::TVector<double> view_location;
    UE::Math::TRotator<double> view_rotation;
    player_controller->GetPlayerViewPoint(&view_location, &view_rotation);

    // Get cheat manager (needed for GetAimedTargetFromLocation)
    UShooterCheatManager* cheat_mgr = AsaApi::GetApiUtils().GetCheatManagerByPC(player_controller);
    if (!cheat_mgr)
    {
        AsaApi::GetApiUtils().SendServerMessage(player_controller, FLinearColor(1, 0, 0),
            "Error: Could not get cheat manager");
        return;
    }

    // Get the actor the player is aiming at
    AActor* aimed_actor = cheat_mgr->GetAimedTargetFromLocation(&view_location, &view_rotation, nullptr);

    if (!aimed_actor)
    {
        AsaApi::GetApiUtils().SendServerMessage(player_controller, FLinearColor(1, 0, 0),
            "No creature in crosshair");
        return;
    }

    // Check if it's a dinosaur/creature
    if (!aimed_actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
    {
        AsaApi::GetApiUtils().SendServerMessage(player_controller, FLinearColor(1, 0, 0),
            "Target is not a creature");
        return;
    }

    auto* dino = static_cast<APrimalDinoCharacter*>(aimed_actor);

    // Check if the creature is tamed
    if (!dino->IsTamedField())
    {
        AsaApi::GetApiUtils().SendServerMessage(player_controller, FLinearColor(1, 0, 0),
            "Creature is not tamed");
        return;
    }

    // Get player's tribe ID
    int player_tribe_id = AsaApi::GetApiUtils().GetTribeID(player_controller);

    // Get creature's tribe ID
    int dino_tribe_id = dino->TamingTeamIDField();

    // Check ownership (must be same tribe, and tribe must not be 0)
    if (player_tribe_id == 0 || dino_tribe_id == 0 || player_tribe_id != dino_tribe_id)
    {
        // Additional check: is the player the direct owner?
        unsigned __int64 player_id = AsaApi::GetApiUtils().GetPlayerID(player_controller);
        unsigned int dino_owner_id = dino->OwningPlayerIDField();

        if (player_id != dino_owner_id)
        {
            AsaApi::GetApiUtils().SendServerMessage(player_controller, FLinearColor(1, 0, 0),
                "You do not own this creature!");
            return;
        }
    }

    // Check if this region can be colored
    if (dino->PreventColorizationRegionsField()[region] == 1)
    {
        AsaApi::GetApiUtils().SendServerMessage(player_controller, FLinearColor(1, 0.5, 0),
            "This region cannot be colored on this creature");
        return;
    }

    // Set the color!
    dino->ColorSetIndicesField()[region] = static_cast<unsigned __int8>(color_id);

    // Force network update so the change is visible immediately
    // This is the key to making it update without cryo/render
    dino->ForceNetUpdate(false, true, false);  // bAbsoluteForceNetUpdate = true

    // Also mark it dirty for replication
    dino->MarkComponentsRenderStateDirty();

    // Success message
    FString creature_name = dino->DescriptiveNameField();
    FString msg = FString::Format("Colored {} - Region: {}, Color: {}",
        *creature_name.ToString(), region, color_id);

    AsaApi::GetApiUtils().SendServerMessage(player_controller, FLinearColor(0, 1, 0), msg);
}

// RCON version of the command
void RconColorCreature(RCONClientConnection* rcon_client, RCONPacket* rcon_packet, UWorld* world)
{
    FString response = "RCON color command executed. Use in-game command: /colorregion <region> <colorid>";
    rcon_client->SendMessageW(rcon_packet->Id,
        static_cast<int>(SERVERDATA_sent::SERVERDATA_RESPONSE_VALUE),
        &response);
}

extern "C" __declspec(dllexport) void Plugin_Init()
{
    Log::GetLog()->info("CreatureColorPlugin loaded!");

    // Add chat command
    AsaApi::GetCommands().AddChatCommand("/colorregion", &ColorAimedCreature);

    // Add console command (same handler)
    AsaApi::GetCommands().AddConsoleCommand("colorregion", &ColorAimedCreature);

    // Add RCON command
    AsaApi::GetCommands().AddRconCommand("colorregion", &RconColorCreature);
}

extern "C" __declspec(dllexport) void Plugin_Unload()
{
    Log::GetLog()->info("CreatureColorPlugin unloaded!");

    // Remove commands
    AsaApi::GetCommands().RemoveChatCommand("/colorregion");
    AsaApi::GetCommands().RemoveConsoleCommand("colorregion");
    AsaApi::GetCommands().RemoveRconCommand("colorregion");
}
