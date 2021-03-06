#include <iostream>
#include <Files/Files.hpp>
#include <Helpers/FileSystem.hpp>
#include <Inputs/Mouse.hpp>
#include <Renderer/Renderer.hpp>
#include <Scenes/Scenes.hpp>
#include "Behaviours/HeightDespawn.hpp"
#include "Behaviours/NameTag.hpp"
#include "Configs/ConfigManager.hpp"
#include "MainRenderer.hpp"
#include "Scenes/PlayerFps.hpp"
#include "Scenes/Scene1.hpp"
#include "Skybox/CelestialBody.hpp"
#include "Skybox/SkyboxCycle.hpp"
#include "Terrain/MaterialTerrain.hpp"
#include "Terrain/Terrain.hpp"
#include "World/World.hpp"

using namespace test;
using namespace acid;

int main(int argc, char **argv)
{
	// Registers file search paths.
	Files::SetBaseDirectory(argv[0]);

	for (auto &file : FileSystem::FilesInPath(FileSystem::GetWorkingDirectory(), false))
	{
		if (String::Contains(file, "data-"))
		{
			Files::AddSearchPath(String::ReplaceFirst(file, FileSystem::GetWorkingDirectory() + FileSystem::SEPARATOR, ""));
		}
	}

	Files::AddSearchPath("Resources/Engine");

	// Creates the engine.
	auto engine = Engine();

	auto configManager = ConfigManager();
	Log::Out("Working Directory: %s\n", FileSystem::GetWorkingDirectory().c_str());

	// Registers modules.
	auto &moduleManager = Engine::Get()->GetModuleManager();
	moduleManager.Add<World>(MODULE_UPDATE_NORMAL);
//	moduleManager.Remove<Shadows>();

	// Registers components.
	auto &componentRegister = Scenes::Get()->GetComponentRegister();
	componentRegister.Add<HeightDespawn>("HeightDespawn");
	componentRegister.Add<NameTag>("NameTag");
	componentRegister.Add<CelestialBody>("CelestialBody");
	componentRegister.Add<PlayerFps>("PlayerFps");
	componentRegister.Add<CelestialBody>("CelestialBody");
	componentRegister.Add<SkyboxCycle>("SkyboxCycle");
	componentRegister.Add<MaterialTerrain>("MaterialTerrain");
	componentRegister.Add<Terrain>("Terrain");

	// Initializes modules.
	Display::Get()->SetTitle("Test Physics");
	Display::Get()->SetIcon("Logos/Flask.png");
	Mouse::Get()->SetCustomMouse("Guis/Cursor.png");
	Renderer::Get()->SetManager(new MainRenderer());
	Scenes::Get()->SetScene(new Scene1());

	// Runs the game loop.
	int32_t exitCode = engine.Run();
	configManager.Save();

	// Pauses the console.
	std::cout << "Press enter to continue...";
	std::cin.get();
	return exitCode;
}
