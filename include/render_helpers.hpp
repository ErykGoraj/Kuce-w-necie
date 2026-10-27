#pragma once

//  NARZEDZIA
// ============================================================================
bool loadTexture(const std::string& path, const std::string& name)
{
    if (!textures[name].loadFromFile(path))
    {
        std::cout << "FAILED TO LOAD ASSET --> FILE_PATH:" << path << std::endl;
        return false;
    }
    return true;
}

bool loadPonyAnimation()
{
    std::map<int, std::filesystem::path> frames;
    std::error_code error;
    const std::string prefix = "Sprite-0001_";
    std::filesystem::directory_iterator it("resources/ponies", error), end;
    while (!error && it != end)
    {
        const auto path = it->path();
        const auto stem = path.stem().string();
        if (path.extension() == ".png" && stem.rfind(prefix, 0) == 0)
        {
            const auto number = stem.substr(prefix.size());
            if (!number.empty() && number.size() <= 9 &&
                number.find_first_not_of("0123456789") == std::string::npos)
                frames.emplace(std::stoi(number), path);
        }
        it.increment(error);
    }
    if (error || frames.empty())
    {
        std::cerr << "Brak klatek animacji w resources/ponies.\n";
        return false;
    }
    ponyFrameCount = 0;
    for (const auto& [number, path] : frames)
    {
        if (!loadTexture(path.string(), "pony_frame_" + std::to_string(ponyFrameCount)))
            return false;
        ++ponyFrameCount;
    }
    ponyAnimationTime = 0.f;
    return true;
}

void drawLabel(sf::RenderWindow& window, const sf::String& str, unsigned int size,
               sf::Vector2f pos, sf::Color color = sf::Color::White, bool centered = true)
{
    sf::Text text(arial_font, str, size);
    text.setFillColor(color);

    auto bounds = text.getLocalBounds();
    text.setOrigin({
        centered ? bounds.position.x + bounds.size.x / 2.f : bounds.position.x,
        bounds.position.y + bounds.size.y / 2.f
    });
    text.setPosition(pos);
    window.draw(text);
}

std::vector<sf::String> wrapText(const sf::String& str, unsigned int size, float maxWidth)
{
    std::vector<sf::String> lines;
    std::u32string line;
    float width = 0.f;
    char32_t previous = 0;
    for (char32_t c : str.toUtf32())
    {
        if (c == U'\r') continue;
        if (c == U'\n') { lines.emplace_back(line); line.clear(); width = 0.f; previous = 0; continue; }
        line += c;
        width += arial_font.getKerning(previous, c, size) + arial_font.getGlyph(c, size, false).advance;
        previous = c;
        if (line.size() > 1 && width > maxWidth)
        {
            const auto space = line.find_last_of(U' ');
            if (space != std::u32string::npos && space > 0)
            {
                lines.emplace_back(line.substr(0, space));
                line.erase(0, space + 1);
            }
            else
            {
                const char32_t last = line.back(); line.pop_back();
                lines.emplace_back(line); line.assign(1, last);
            }
            width = 0.f;
            previous = 0;
            for (char32_t remaining : line)
            {
                width += arial_font.getKerning(previous, remaining, size) +
                         arial_font.getGlyph(remaining, size, false).advance;
                previous = remaining;
            }
        }
    }
    if (!line.empty()) lines.emplace_back(line);
    return lines;
}

std::optional<sf::Vector2f> leftClick(const sf::Event& event, const sf::RenderWindow& window)
{
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (click->button == sf::Mouse::Button::Left)
            return window.mapPixelToCoords(click->position);
    }
    return std::nullopt;
}

// ----------------------------------------------------------------------------
//  Przyciski
// ----------------------------------------------------------------------------
void layoutButtonText(Button& button)
{
    auto bounds = button.text.getLocalBounds();
    button.text.setOrigin({
        bounds.position.x + bounds.size.x / 2.f,
        bounds.position.y + bounds.size.y / 2.f
    });
    button.text.setPosition(button.shape.getPosition() + button.shape.getSize() / 2.f);
}

void createButton(const std::string& name, const sf::String& label,
                  sf::Vector2f position, sf::Vector2f size, unsigned int charSize = 28)
{
    buttons.try_emplace(name, arial_font);
    Button& button = buttons.at(name);

    button.shape.setSize(size);
    button.shape.setPosition(position);
    button.text.setString(label);
    button.text.setCharacterSize(charSize);
    layoutButtonText(button);
}

void setButtonLabel(const std::string& name, const sf::String& label)
{
    Button& button = buttons.at(name);
    button.text.setString(label);
    layoutButtonText(button);
}

bool buttonHit(const std::string& name, sf::Vector2f pos)
{
    return buttons.at(name).shape.getGlobalBounds().contains(pos);
}

bool activateButtonAt(const std::string& name, sf::Vector2f pos)
{
    if (!buttonHit(name, pos))
        return false;
    playEffect(name == "volume_minus" || name == "volume_plus" ||
                       name == "mute_sound" || name == "mute_music" ? "ui_change" : "ui_click");
    return true;
}

void drawButton(sf::RenderWindow& window, const std::string& name, bool enabled = true)
{
    Button& button = buttons.at(name);

    sf::Color fill(74, 59, 43);
    if (!enabled)
        fill = sf::Color(61, 58, 48);
    else if (button.shape.getGlobalBounds().contains(mousePos))
    {
        fill = sf::Color(85, 111, 58);
        hoveredButton = name;
    }

    button.shape.setFillColor(fill);
    button.shape.setOutlineThickness(2.f);
    button.shape.setOutlineColor(enabled ? ACCENT : sf::Color(90, 90, 100));
    button.text.setFillColor(enabled ? sf::Color::White : sf::Color(140, 140, 150));

    window.draw(button.shape);
    window.draw(button.text);
}

// ----------------------------------------------------------------------------
//  Kucyki i pola
// ----------------------------------------------------------------------------
void drawPony(sf::RenderWindow& window, int index, sf::Vector2f topLeft, float size)
{
    const int frame = ponyFrameCount > 0
                          ? static_cast<int>(ponyAnimationTime / PONY_FRAME_TIME) % ponyFrameCount : 0;
    auto it = textures.find("pony_frame_" + std::to_string(frame));

    if (it != textures.end() && it->second.getSize().x > 0)
    {
        sf::Sprite sprite(it->second);
        auto texSize = it->second.getSize();
        const float scale = size / std::max(texSize.x, texSize.y);
        sprite.setScale({scale, scale});
        sprite.setPosition(topLeft + sf::Vector2f{
                               (size - texSize.x * scale) / 2.f, (size - texSize.y * scale) / 2.f});
        sprite.setColor(ponyColors[index]);
        window.draw(sprite);
    }
    else
    {
        sf::CircleShape circle(size / 2.f);
        circle.setPosition(topLeft);
        circle.setFillColor(ponyColors[index]);
        window.draw(circle);
    }
}

sf::Color tileColor(const std::string& type)
{
    if (type.rfind("emotions", 0) == 0)    return sf::Color(230, 120, 170);
    if (type.rfind("alghoritms", 0) == 0)  return sf::Color(132, 166, 91);
    if (type.rfind("influencer", 0) == 0)  return sf::Color(166, 112, 67);
    if (type.rfind("safe", 0) == 0)        return sf::Color(103, 148, 76);
    return sf::Color(98, 91, 67);
}

void drawTile(sf::RenderWindow& window, const Tile& tile)
{
    auto it = sprites.find(tile.type);

    if (it != sprites.end())
    {
        const auto texSize = it->second.getTexture().getSize();
        it->second.setScale({tile.size / static_cast<float>(texSize.x),
                             tile.size / static_cast<float>(texSize.y)});
        it->second.setPosition(tile.position);
        window.draw(it->second);
        return;
    }

    const float size = tile.size;
    sf::RectangleShape rect({size - 4.f, size - 4.f});
    rect.setPosition(tile.position + sf::Vector2f(2.f, 2.f));
    rect.setFillColor(tileColor(tile.type));
    window.draw(rect);
}

void addTile(float x, float y, const std::string& type)
{
    board.push_back({{x, y}, type, 80.f});
}

void randomizeTileTypes()
{
    static const std::array<std::string, 5> types = {
        "bot", "emotions", "alghoritms", "influencer", "safe"
    };
    const int typeCount = static_cast<int>(types.size());

    for (int level = 0; level < LEVEL_COUNT; level++)
    {
        const std::vector<int>& track = tracks[level];
        if (track.empty())
            continue;

        const std::string suffix = "_big";

        std::array<int, 5> used{};
        board[track[0]].type = types[0] + suffix;
        used[0] = 1;
        int previous = 0;

        for (size_t i = 1; i < track.size(); i++)
        {
            const bool isLast = (i == track.size() - 1);

            auto allowed = [&](int t)
            {
                return t != previous && !(isLast && t == 0);
            };

            int minUsed = static_cast<int>(track.size());
            for (int t = 0; t < typeCount; t++)
            {
                if (allowed(t))
                    minUsed = std::min(minUsed, used[t]);
            }

            std::vector<int> candidates;
            for (int t = 0; t < typeCount; t++)
            {
                if (allowed(t) && used[t] <= minUsed + 1)
                    candidates.push_back(t);
            }

            const int chosen = candidates[std::uniform_int_distribution<size_t>(0, candidates.size() - 1)(rng())];
            used[chosen]++;
            previous = chosen;
            board[track[i]].type = types[chosen] + suffix;
        }
    }

    auto placeSkulls = [](int level, int count, const std::string& suffix)
    {
        const std::vector<int>& track = tracks[level];
        std::vector<int> candidates;
        for (int i = 1; i < static_cast<int>(track.size()); i++)
            candidates.push_back(i);
        std::shuffle(candidates.begin(), candidates.end(), rng());

        for (int i = 0; i < count; i++)
            board[track[candidates[i]]].type = "skull" + suffix;
    };

    placeSkulls(1, 2, "_big");
    placeSkulls(2, MAX_SKULLS_LAST_LEVEL, "_big");
}

void buildBoard()
{
    board.clear();

    const float T = 80.f;
    const int N = 12;
    const float boardLen = T * N;

    const float bx = (WINDOWS_SIZE.x - boardLen) / 2.f;
    const float by = (WINDOWS_SIZE.y - boardLen) / 2.f + boardLen - T;

    const char* left[12] = {
        "bot_big", "alghoritms_big", "influencer_big", "safe_big",
        "alghoritms_big", "influencer_big", "bot_big", "emotions_big",
        "bot_big", "alghoritms_big", "safe_big", "emotions_big"
    };
    const char* top[10] = {
        "alghoritms_big", "influencer_big", "safe_big", "bot_big", "bot_big",
        "bot_big", "bot_big", "bot_big", "bot_big", "bot_big"
    };

    for (int i = 0; i < 12; i++)
        addTile(bx, by - T * i, left[i]);

    for (int i = 1; i <= 10; i++)
        addTile(bx + T * i, by - T * 11, top[i - 1]);

    for (int i = 11; i >= 0; i--)
        addTile(bx + T * 11, by - T * i, "bot_big");

    for (int i = 10; i >= 1; i--)
        addTile(bx + T * i, by, "bot_big");

    const int firstEnd = static_cast<int>(board.size());

    const float sx = bx + 160.f;
    const float sy = by - 160.f;

    for (int i = 0; i < 8; i++)
        addTile(sx, sy - T * i, "bot_big");
    for (int i = 1; i <= 6; i++)
        addTile(sx + T * i, sy - T * 7, "bot_big");
    for (int i = 7; i >= 0; i--)
        addTile(sx + T * 7, sy - T * i, "bot_big");
    for (int i = 6; i >= 1; i--)
        addTile(sx + T * i, sy, "bot_big");

    const int secondEnd = static_cast<int>(board.size());

    const float S = 60.f;
    const float tx = sx + 140.f;
    const float ty = sy - 120.f;

    addTile(tx + S,     ty - S * 5, "alghoritms_big");
    addTile(tx + S * 2, ty - S * 5, "influencer_big");
    addTile(tx + S * 3, ty - S * 5, "safe_big");
    addTile(tx + S * 4, ty - S * 5, "emotions_big");
    addTile(tx + S * 5, ty - S * 5, "bot_big");

    addTile(tx, ty,         "bot_big");
    addTile(tx, ty - S,     "alghoritms_big");
    addTile(tx, ty - S * 2, "influencer_big");
    addTile(tx, ty - S * 3, "safe_big");
    addTile(tx, ty - S * 4, "emotions_big");
    addTile(tx, ty - S * 5, "bot_big");

    addTile(tx + S,     ty, "alghoritms_big");
    addTile(tx + S * 2, ty, "influencer_big");
    addTile(tx + S * 3, ty, "safe_big");
    addTile(tx + S * 4, ty, "emotions_big");

    addTile(tx + S * 5, ty,         "bot_big");
    addTile(tx + S * 5, ty - S,     "alghoritms_big");
    addTile(tx + S * 5, ty - S * 2, "influencer_big");
    addTile(tx + S * 5, ty - S * 3, "safe_big");

    addTile(tx + S * 4, ty - S * 3, "emotions_big");
    addTile(tx + S * 3, ty - S * 3, "alghoritms_big");
    addTile(tx + S * 2, ty - S * 3, "influencer_big");

    addTile(tx + S * 2, ty - S * 2, "safe_big");

    for (auto& track : tracks)
        track.clear();

    for (int i = 0; i < firstEnd; i++)
        tracks[0].push_back(i);
    for (int i = firstEnd; i < secondEnd; i++)
        tracks[1].push_back(i);

    const int spiral[23] = {4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5,
                            11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};
    for (int k : spiral)
        tracks[2].push_back(secondEnd + k);

    for (int index : tracks[2])
        board[index].size = 60.f;

    randomizeTileTypes();
}

void resetGameState()
{
    currentSurprise.reset();
    surpriseMovement = false;
    moveDirection = 1;
    gamePaused = rulesOpen = confirmLeave = false;
    landingTimer = 0.f;
    boardZoom = 1.f;
    turnState = TurnState::WAIT_ROLL;
    currentPlayer = 0;
    displayedDice = 0;
    stepsLeft = 0;
    stepProgress = 0.f;
    currentQuestion = nullptr;
    chosenAnswer = -1;
    questionScroll.fill(0.f);
    lastCorrect = false;
    lastGained = 0;
    bonusRoll = false;
    levelUpMessage.clear();
    skullLanding = false;
    winner = -1;
    questionUsed.assign(questions.size(), false);
}

void resetToMenu()
{
    players.clear();
    playersCount = 0;
    countFocus = 2;
    ignoredCountCharacter = 0;
    enteredNick.clear();
    selectMessage.clear();
    ponyFocus = 0;

    resetGameState();
    scene = MAIN_MENU;
}

void startGame()
{
    randomizeTileTypes();
    resetGameState();
    scene = GAME;
}
