#pragma once

// ============================================================================
//  WYBOR GRACZY
// ============================================================================
sf::FloatRect ponyCardRect(int i)
{
    return sf::FloatRect({900.f + (i % 6) * 100.f, 580.f + (i / 6) * 100.f},
                         {72.f, 72.f});
}

bool isBlank(const sf::String& str)
{
    for (char32_t c : str.toUtf32())
    {
        if (c != U' ')
            return false;
    }
    return true;
}

void moveFocus(int dir)
{
    ponyFocus = (ponyFocus + dir + COLOR_COUNT) % COLOR_COUNT;
    playEffect("ui_change");
}

void setPlayersCount(int count)
{
    if (count < 2 || count > 4) return;
    playersCount = count;
    ponyFocus = 0;
    enteredNick.clear();
    selectMessage.clear();
}

void confirmPony(int index)
{
    if (index < 0 || index >= COLOR_COUNT ||
        playersCount < 2 || static_cast<int>(players.size()) >= playersCount)
        return;

    if (isBlank(enteredNick))
    {
        selectMessage = L"Najpierw wpisz nick!";
        return;
    }

    for (const Player& p : players)
    {
        if (p.nick == enteredNick)
        {
            selectMessage = L"Ten nick jest już zajęty!";
            return;
        }
    }

    players.push_back({enteredNick, index, 0});
    enteredNick.clear();
    selectMessage.clear();

    if (static_cast<int>(players.size()) == playersCount)
    {
        startGame();
        return;
    }

    ponyFocus = static_cast<int>(players.size());
}

void handlePlayerSelectEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (event.is<sf::Event::KeyReleased>())
        ignoredCountCharacter = 0;
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        ignoredCountCharacter = 0;
        if (key->code == sf::Keyboard::Key::Escape)
        {
            resetToMenu();
            return;
        }

        if (playersCount == 0)
        {
            const auto code = key->code;
            if (code == sf::Keyboard::Key::Left || code == sf::Keyboard::Key::Up)
                countFocus = countFocus == 2 ? 4 : countFocus - 1;
            else if (code == sf::Keyboard::Key::Right || code == sf::Keyboard::Key::Down ||
                     code == sf::Keyboard::Key::Tab)
                countFocus = countFocus == 4 ? 2 : countFocus + 1;
            else if (code == sf::Keyboard::Key::Enter)
            {
                playEffect("ui_click");
                setPlayersCount(countFocus);
            }
            else
            {
                int count = 0;
                if (code == sf::Keyboard::Key::Num2 || code == sf::Keyboard::Key::Numpad2) count = 2;
                if (code == sf::Keyboard::Key::Num3 || code == sf::Keyboard::Key::Numpad3) count = 3;
                if (code == sf::Keyboard::Key::Num4 || code == sf::Keyboard::Key::Numpad4) count = 4;
                if (count != 0)
                {
                    playEffect("ui_click");
                    setPlayersCount(count);
                    ignoredCountCharacter = U'0' + count;
                }
            }
            return;
        }
        else
        {
            if (key->code == sf::Keyboard::Key::Left)
                moveFocus(-1);
            else if (key->code == sf::Keyboard::Key::Right)
                moveFocus(1);
            else if (key->code == sf::Keyboard::Key::Up ||
                     key->code == sf::Keyboard::Key::Down)
                moveFocus(6);
            else if (key->code == sf::Keyboard::Key::Enter)
                confirmPony(ponyFocus);
        }
    }

    if (const auto click = leftClick(event, window))
    {
        if (activateButtonAt("select_back", *click))
        {
            resetToMenu();
            return;
        }

        if (playersCount == 0)
        {
            for (int n = 2; n <= 4; n++)
            {
                if (activateButtonAt("count" + std::to_string(n), *click))
                    setPlayersCount(n);
            }
        }
        else
        {
            for (int i = 0; i < COLOR_COUNT; i++)
            {
                if (ponyCardRect(i).contains(*click))
                {
                    if (ponyFocus != i) playEffect("ui_change");
                    ponyFocus = i;
                    break;
                }
            }
            if (activateButtonAt("select_confirm", *click))
                confirmPony(ponyFocus);
        }
    }

    if (const auto* input = event.getIf<sf::Event::TextEntered>())
    {
        if (ignoredCountCharacter != 0)
        {
            const bool ignore = input->unicode == ignoredCountCharacter;
            ignoredCountCharacter = 0;
            if (ignore) return;
        }
        if (playersCount == 0)
        {
            if (input->unicode >= U'2' && input->unicode <= U'4')
                setPlayersCount(static_cast<int>(input->unicode - U'0'));
        }
        else
        {
            if (input->unicode == 8)
            {
                if (!enteredNick.isEmpty())
                    enteredNick.erase(enteredNick.getSize() - 1, 1);
                selectMessage.clear();
            }
            else if (input->unicode >= 32 && input->unicode != 127 &&
                     enteredNick.getSize() < MAX_NICK_LENGTH)
            {
                enteredNick += sf::String(input->unicode);
                selectMessage.clear();
            }
        }
    }
}

void drawPlayerSelect(sf::RenderWindow& window)
{
    const float cx = WINDOWS_SIZE.x / 2.f;

    if (playersCount == 0)
    {
        drawLabel(window, L"WYBIERZ LICZBĘ GRACZY", 52, {cx, 280.f}, ACCENT);

        for (int n = 2; n <= 4; n++)
        {
            if (n == countFocus)
            {
                const auto& shape = buttons.at("count" + std::to_string(n)).shape;
                sf::RectangleShape focus(shape.getSize() + sf::Vector2f{16.f, 16.f});
                focus.setPosition(shape.getPosition() - sf::Vector2f{8.f, 8.f});
                focus.setFillColor(sf::Color::Transparent);
                focus.setOutlineColor(GOLD);
                focus.setOutlineThickness(3.f);
                window.draw(focus);
            }
            drawButton(window, "count" + std::to_string(n));
        }

        drawLabel(window, L"Strzałki / TAB: wybór • ENTER: zatwierdź • skróty: 2, 3, 4", 24,
                  {cx, 640.f}, GRAY_TEXT);
    }
    else
    {
        drawLabel(window,
                  L"Gracz " + std::to_wstring(players.size() + 1) + L" z " +
                      std::to_wstring(playersCount) + L" - wpisz nick i wybierz kucyka",
                  44, {cx, 140.f}, ACCENT);

        for (size_t i = 0; i < players.size(); i++)
        {
            const Player& p = players[i];
            drawLabel(window,
                      sf::String(L"Gracz ") + sf::String(std::to_wstring(i + 1)) + sf::String(L": ") +
                          p.nick + sf::String(L"  -  ") + ponyLabels[p.pony],
                      24, {cx, 215.f + static_cast<float>(i) * 32.f}, ponyColors[p.pony]);
        }

        drawLabel(window, L"Nick (maks. " + std::to_wstring(MAX_NICK_LENGTH) + L" znaków):", 24, {cx, 365.f}, GRAY_TEXT);

        sf::RectangleShape box({600.f, 70.f});
        box.setPosition({cx - 300.f, 395.f});
        box.setFillColor(PANEL_BG);
        box.setOutlineThickness(3.f);
        box.setOutlineColor(selectMessage.isEmpty() ? ACCENT : sf::Color(255, 90, 90));
        window.draw(box);

        static sf::Clock blink;
        const bool showCursor = std::fmod(blink.getElapsedTime().asSeconds(), 1.f) < 0.5f;
        drawLabel(window, enteredNick + sf::String(showCursor ? L"|" : L" "), 36,
                  {cx - 280.f, 430.f}, sf::Color::White, false);

        if (!selectMessage.isEmpty())
            drawLabel(window, selectMessage, 26, {cx, 495.f}, sf::Color(255, 110, 110));

        drawPony(window, ponyFocus, {490.f, 530.f}, 250.f);
        drawLabel(window, ponyLabels[ponyFocus], 28, {615.f, 810.f}, ponyColors[ponyFocus]);
        drawLabel(window, L"Wybierz kolor swojego kucyka", 28, {1186.f, 535.f}, ACCENT);
        for (int i = 0; i < COLOR_COUNT; ++i)
        {
            const auto rect = ponyCardRect(i);
            sf::RectangleShape swatch(rect.size);
            swatch.setPosition(rect.position);
            swatch.setFillColor(ponyColors[i]);
            swatch.setOutlineThickness(i == ponyFocus ? 5.f : 2.f);
            swatch.setOutlineColor(i == ponyFocus ? sf::Color::White : PANEL_BG);
            window.draw(swatch);
            if (i == ponyFocus)
                drawLabel(window, L"+", 40, rect.position + rect.size / 2.f, PANEL_BG);
        }
        drawLabel(window, L"Kliknij kolor lub użyj strzałek. ENTER zatwierdza gracza.",
                  24, {cx, 870.f}, GRAY_TEXT);
        buttons.at("select_confirm").text.setString(
            static_cast<int>(players.size()) + 1 == playersCount ? L"ROZPOCZNIJ GRĘ" : L"DODAJ GRACZA");
        drawButton(window, "select_confirm");
    }
    drawButton(window, "select_back");
}

