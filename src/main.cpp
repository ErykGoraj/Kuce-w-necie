#include <SFML/Graphics.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "game_config.hpp"
#include "json_parser.hpp"
#include "questions.hpp"
#include "game_state.hpp"
#include "render_helpers.hpp"
#include "menu_screens.hpp"
#include "player_selection.hpp"
#include "gameplay.hpp"
#include "game_rendering.hpp"
#include "game_over.hpp"

//  MAIN
// ============================================================================
int main(int argc, char* argv[])
{
    if (argc > 0)
    {
        std::error_code error;
        const auto executable = std::filesystem::absolute(argv[0], error);
        if (!error && std::filesystem::is_directory(executable.parent_path() / "resources", error))
            std::filesystem::current_path(executable.parent_path(), error);
    }
    sf::RenderWindow window(sf::VideoMode(WINDOWS_SIZE), "Cyber Kucyki", sf::State::Fullscreen);
    window.setFramerateLimit(FRAME_RATE);
    window.setView(sf::View(sf::FloatRect({0.f, 0.f}, {1920.f, 1080.f})));

    if (!arial_font.openFromFile("resources/fonts/ARIAL.TTF"))
    {
        std::cout << "FAILED TO LOAD FONT: resources/fonts/ARIAL.TTF" << std::endl;
        return 1;
    }

    if (!loadTexture("resources/menu_background.png", "menu_background"))
        return 1;
    sprites.emplace("menu_background", sf::Sprite(textures.at("menu_background")));

    if (!loadQuestions(QUESTIONS_PATH))
    {
        std::cout << "NO QUESTIONS LOADED - umiesc plik w: " << QUESTIONS_PATH << std::endl;
        return 1;
    }

    if (!loadSurprises("resources/niespodzianki.json"))
    {
        std::cerr << "Niepoprawny lub brakujacy plik resources/niespodzianki.json\n";
        return 1;
    }

    for (const std::string type : {"bot", "emotions", "alghoritms", "influencer", "safe", "skull"})
    {
        const std::string name = type + "_big";
        if (loadTexture("resources/boards/" + name + ".png", name))
            sprites.emplace(name, sf::Sprite(textures.at(name)));
    }

    if (!loadPonyAnimation())
        return 1;

    for (int value = 1; value <= 6; ++value)
        loadTexture("resources/dice/dice" + std::to_string(value) + ".png", "dice" + std::to_string(value));

    rectangles.emplace("settings_background_outline", sf::RectangleShape({810.f, 810.f}));
    rectangles.emplace("settings_background", sf::RectangleShape({800.f, 800.f}));

    rectangles.at("settings_background_outline").setFillColor(ACCENT);
    rectangles.at("settings_background").setFillColor(PANEL_BG);

    for (const char* name : {"settings_background_outline", "settings_background"})
    {
        auto& rect = rectangles.at(name);
        rect.setPosition({
            WINDOWS_SIZE.x / 2.f - rect.getSize().x / 2.f,
            WINDOWS_SIZE.y / 2.f - rect.getSize().y / 2.f
        });
    }

    const float x = WINDOWS_SIZE.x / 2.f;
    const float y = rectangles.at("settings_background").getPosition().y;

    createButton("volume_minus", "-", {x - 150.f, y + 255.f}, {60.f, 50.f});
    createButton("volume_plus", "+", {x + 90.f, y + 255.f}, {60.f, 50.f});
    createButton("mute_sound", L"NIE", {x - 90.f, y + 385.f}, {180.f, 50.f});
    createButton("mute_music", L"NIE", {x - 90.f, y + 515.f}, {180.f, 50.f});
    createButton("back", L"POWRÓT", {x - 100.f, y + 680.f}, {200.f, 50.f});

    createButton("count2", "2 graczy", {540.f, 450.f}, {240.f, 120.f}, 36);
    createButton("count3", "3 graczy", {840.f, 450.f}, {240.f, 120.f}, 36);
    createButton("count4", "4 graczy", {1140.f, 450.f}, {240.f, 120.f}, 36);
    createButton("select_back", L"POWRÓT", {60.f, 950.f}, {200.f, 60.f});
    createButton("select_confirm", L"DODAJ GRACZA", {1110.f, 940.f}, {400.f, 70.f}, 28);

    createButton("new_game", L"NOWA GRA", {735.f, 425.f}, {410.f, 195.f}, 28);
    createButton("rules", L"ZASADY", {755.f, 980.f}, {190.f, 58.f}, 23);
    createButton("options", L"USTAWIENIA", {330.f, 570.f}, {390.f, 230.f}, 28);
    createButton("credits", L"AUTORZY", {1180.f, 565.f}, {380.f, 225.f}, 28);
    createButton("quit", L"WYJDŹ", {975.f, 980.f}, {190.f, 58.f}, 23);
    createButton("resume", L"WRÓĆ DO GRY", {720.f, 490.f}, {480.f, 65.f});
    createButton("pause_rules", L"ZASADY I STEROWANIE", {720.f, 590.f}, {480.f, 65.f});
    createButton("leave", L"ZAKOŃCZ PARTIĘ", {720.f, 690.f}, {480.f, 65.f});
    createButton("leave_yes", L"TAK, WRÓĆ DO MENU", {720.f, 590.f}, {480.f, 65.f});
    createButton("overlay_back", L"POWRÓT", {760.f, 820.f}, {400.f, 65.f});

    createButton("roll", L"RZUĆ KOSTKĄ", {56.f, 540.f}, {356.f, 62.f}, 25);
    createButton("continue", "DALEJ  /  ENTER", {1184.f, 938.f}, {310.f, 54.f}, 22);
    createButton("menu_end", L"MENU GŁÓWNE", {760.f, 930.f}, {400.f, 70.f});

    buildBoard();
    loadEffects();

    sf::Music backgroundMusic;
    const bool musicLoaded = backgroundMusic.openFromFile("resources/menu_loop.wav");
    float appliedMusicVolume = musicMuted ? 0.f : volume;
    if (musicLoaded)
    {
        backgroundMusic.setLooping(true);
        backgroundMusic.setVolume(appliedMusicVolume);
        backgroundMusic.play();
    }
    else
        std::cerr << "Nie mozna odtworzyc resources/menu_loop.wav; gra bez muzyki.\n";

    sf::Clock frameClock;

    while (window.isOpen())
    {
        const float dt = std::min(frameClock.restart().asSeconds(), 0.1f);
        updateCyberParticles(dt);

        if (!gamePaused)
            ponyAnimationTime = std::fmod(ponyAnimationTime + dt, ponyFrameCount * PONY_FRAME_TIME);

        mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
                break;
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                if (resized->size.x > 0 && resized->size.y > 0)
                {
                    sf::View ui(sf::FloatRect({0.f, 0.f}, {1920.f, 1080.f}));
                    const float aspect = float(resized->size.x) / resized->size.y;
                    const float target = 1920.f / 1080.f;
                    if (aspect > target)
                    {
                        const float width = target / aspect;
                        ui.setViewport(sf::FloatRect({(1.f - width) / 2.f, 0.f}, {width, 1.f}));
                    }
                    else
                    {
                        const float height = aspect / target;
                        ui.setViewport(sf::FloatRect({0.f, (1.f - height) / 2.f}, {1.f, height}));
                    }
                    window.setView(ui);
                }
            }

            switch (scene)
            {
            case MAIN_MENU:     handleMenuEvent(*event, window); break;
            case SETTINGS:      handleSettingsEvent(*event, window); break;
            case CREDITS:       handleCreditsEvent(*event, window); break;
            case PLAYER_SELECT: handlePlayerSelectEvent(*event, window); break;
            case GAME:          handleGameEvent(*event, window); break;
            case GAME_OVER:     handleGameOverEvent(*event, window); break;
            }
        }

        if (!window.isOpen())
            break;

        const float musicVolume = musicMuted ? 0.f : volume;
        for (auto& [name, sound] : effects)
            sound.setVolume(effectVolume(name));
        if (musicLoaded && musicVolume != appliedMusicVolume)
        {
            backgroundMusic.setVolume(musicVolume);
            appliedMusicVolume = musicVolume;
        }

        window.clear(sf::Color(33, 42, 29));
        hoveredButton.clear();

        switch (scene)
        {
        case MAIN_MENU:
            updateMenu();
            drawMenu(window);
            break;
        case SETTINGS:
            drawSettings(window);
            break;
        case CREDITS:
            drawCredits(window);
            break;
        case PLAYER_SELECT:
            drawPlayerSelect(window);
            break;
        case GAME:
            updateGame(dt);
            drawGame(window);
            break;
        case GAME_OVER:
            drawGameOver(window);
            break;
        case QUIT:
            window.close();
            break;
        }

        if (!hoveredButton.empty() && hoveredButton != previousHoveredButton)
            playEffect("ui_hover");
        previousHoveredButton = hoveredButton;
        window.display();
    }

    return 0;
}