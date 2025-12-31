#include <API/ARK/Ark.h>
#include <fstream>
#include <filesystem>

// Configuration structure
struct RconAckConfig
{
    bool enabled = true;
    bool send_ack = true;              // Send immediate acknowledgement
    bool send_processing = true;        // Send "processing" message
    bool send_completion = true;        // Send "completed" message
    std::string ack_message = "ACK: Command received";
    std::string processing_message = "PROCESSING: {command}";
    std::string completion_message = "COMPLETED: {command}";
    std::string error_message = "ERROR: {error}";
    bool include_timestamp = true;
    bool log_commands = true;
};

RconAckConfig g_config;

// Load configuration from JSON
void LoadConfig()
{
    try
    {
        std::filesystem::path config_path = AsaApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/RconAckPlugin/config.json";

        if (!std::filesystem::exists(config_path))
        {
            Log::GetLog()->warn("RconAckPlugin: config.json not found, using defaults");
            return;
        }

        std::ifstream file(config_path);
        if (!file.is_open())
        {
            Log::GetLog()->error("RconAckPlugin: Could not open config.json");
            return;
        }

        nlohmann::json json;
        file >> json;
        file.close();

        // Parse configuration
        if (json.contains("enabled"))
            g_config.enabled = json["enabled"].get<bool>();

        if (json.contains("send_ack"))
            g_config.send_ack = json["send_ack"].get<bool>();

        if (json.contains("send_processing"))
            g_config.send_processing = json["send_processing"].get<bool>();

        if (json.contains("send_completion"))
            g_config.send_completion = json["send_completion"].get<bool>();

        if (json.contains("ack_message"))
            g_config.ack_message = json["ack_message"].get<std::string>();

        if (json.contains("processing_message"))
            g_config.processing_message = json["processing_message"].get<std::string>();

        if (json.contains("completion_message"))
            g_config.completion_message = json["completion_message"].get<std::string>();

        if (json.contains("error_message"))
            g_config.error_message = json["error_message"].get<std::string>();

        if (json.contains("include_timestamp"))
            g_config.include_timestamp = json["include_timestamp"].get<bool>();

        if (json.contains("log_commands"))
            g_config.log_commands = json["log_commands"].get<bool>();

        Log::GetLog()->info("RconAckPlugin: Configuration loaded successfully");
    }
    catch (const std::exception& ex)
    {
        Log::GetLog()->error("RconAckPlugin: Error loading config: {}", ex.what());
    }
}

// Helper function to replace {command} placeholder
std::string FormatMessage(const std::string& template_msg, const std::string& command)
{
    std::string result = template_msg;
    size_t pos = result.find("{command}");
    if (pos != std::string::npos)
    {
        result.replace(pos, 9, command);
    }
    return result;
}

// Helper function to add timestamp
std::string AddTimestamp(const std::string& message)
{
    if (!g_config.include_timestamp)
        return message;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&time), "%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
    ss << message;

    return ss.str();
}

// Send RCON response packet
void SendRconResponse(RCONClientConnection* rcon_client, int packet_id, const std::string& message)
{
    if (!rcon_client || rcon_client->IsClosedField())
        return;

    std::string formatted_msg = AddTimestamp(message);
    FString response(formatted_msg);

    rcon_client->SendMessageW(
        packet_id,
        static_cast<int>(SERVERDATA_sent::SERVERDATA_RESPONSE_VALUE),
        &response
    );
}

// Hook declaration
DECLARE_HOOK(RCONClientConnection_ProcessRCONPacket, void, RCONClientConnection*, RCONPacket*, UWorld*);

void Hook_RCONClientConnection_ProcessRCONPacket(RCONClientConnection* rcon_client, RCONPacket* packet, UWorld* world)
{
    // Check if plugin is enabled
    if (!g_config.enabled)
    {
        RCONClientConnection_ProcessRCONPacket_original(rcon_client, packet, world);
        return;
    }

    // Only process authenticated connections
    if (!rcon_client->IsAuthenticatedField())
    {
        RCONClientConnection_ProcessRCONPacket_original(rcon_client, packet, world);
        return;
    }

    // Get the command from the packet
    std::string command = packet->Body.ToString();
    int packet_id = packet->Id;

    // Log the command if enabled
    if (g_config.log_commands)
    {
        Log::GetLog()->info("RCON Command [ID:{}]: {}", packet_id, command);
    }

    // Send immediate acknowledgement
    if (g_config.send_ack)
    {
        SendRconResponse(rcon_client, packet_id, g_config.ack_message);
    }

    // Send processing message
    if (g_config.send_processing)
    {
        std::string processing_msg = FormatMessage(g_config.processing_message, command);
        SendRconResponse(rcon_client, packet_id, processing_msg);
    }

    // Execute the original command processing
    try
    {
        RCONClientConnection_ProcessRCONPacket_original(rcon_client, packet, world);

        // Send completion message
        if (g_config.send_completion)
        {
            std::string completion_msg = FormatMessage(g_config.completion_message, command);
            SendRconResponse(rcon_client, packet_id, completion_msg);
        }
    }
    catch (const std::exception& ex)
    {
        // Send error message if command failed
        std::string error_msg = FormatMessage(g_config.error_message, ex.what());
        SendRconResponse(rcon_client, packet_id, error_msg);

        Log::GetLog()->error("RCON Command failed [ID:{}]: {}", packet_id, ex.what());
    }
}

// Reload config command
void ReloadConfigCommand(AShooterPlayerController* player_controller, TArray<FString>* parsed, int mode)
{
    LoadConfig();
    AsaApi::GetApiUtils().SendServerMessage(player_controller, FLinearColor(0, 1, 0),
        "RconAckPlugin configuration reloaded");
}

void RconReloadConfigCommand(RCONClientConnection* rcon_client, RCONPacket* rcon_packet, UWorld* world)
{
    LoadConfig();
    FString response = "RconAckPlugin configuration reloaded";
    rcon_client->SendMessageW(rcon_packet->Id,
        static_cast<int>(SERVERDATA_sent::SERVERDATA_RESPONSE_VALUE),
        &response);
}

// Toggle plugin command
void TogglePluginCommand(AShooterPlayerController* player_controller, TArray<FString>* parsed, int mode)
{
    g_config.enabled = !g_config.enabled;

    FString msg = FString::Format("RconAckPlugin: {}", g_config.enabled ? "ENABLED" : "DISABLED");
    AsaApi::GetApiUtils().SendServerMessage(player_controller, FLinearColor(0, 1, 0), msg);
}

void RconTogglePluginCommand(RCONClientConnection* rcon_client, RCONPacket* rcon_packet, UWorld* world)
{
    g_config.enabled = !g_config.enabled;

    FString response = FString::Format("RconAckPlugin: {}", g_config.enabled ? "ENABLED" : "DISABLED");
    rcon_client->SendMessageW(rcon_packet->Id,
        static_cast<int>(SERVERDATA_sent::SERVERDATA_RESPONSE_VALUE),
        &response);
}

extern "C" __declspec(dllexport) void Plugin_Init()
{
    Log::GetLog()->info("RconAckPlugin initializing...");

    // Load configuration
    LoadConfig();

    // Set up the RCON packet processing hook
    AsaApi::GetHooks().SetHook(
        "RCONClientConnection.ProcessRCONPacket(RCONPacket&,UWorld*)",
        &Hook_RCONClientConnection_ProcessRCONPacket,
        &RCONClientConnection_ProcessRCONPacket_original
    );

    // Add admin commands for managing the plugin
    AsaApi::GetCommands().AddConsoleCommand("rconack.reload", &ReloadConfigCommand);
    AsaApi::GetCommands().AddConsoleCommand("rconack.toggle", &TogglePluginCommand);

    AsaApi::GetCommands().AddRconCommand("rconack.reload", &RconReloadConfigCommand);
    AsaApi::GetCommands().AddRconCommand("rconack.toggle", &RconTogglePluginCommand);

    Log::GetLog()->info("RconAckPlugin loaded successfully! Version 1.0");
}

extern "C" __declspec(dllexport) void Plugin_Unload()
{
    Log::GetLog()->info("RconAckPlugin unloading...");

    // Disable the hook
    AsaApi::GetHooks().DisableHook(
        "RCONClientConnection.ProcessRCONPacket(RCONPacket&,UWorld*)",
        &Hook_RCONClientConnection_ProcessRCONPacket
    );

    // Remove commands
    AsaApi::GetCommands().RemoveConsoleCommand("rconack.reload");
    AsaApi::GetCommands().RemoveConsoleCommand("rconack.toggle");
    AsaApi::GetCommands().RemoveRconCommand("rconack.reload");
    AsaApi::GetCommands().RemoveRconCommand("rconack.toggle");

    Log::GetLog()->info("RconAckPlugin unloaded");
}
