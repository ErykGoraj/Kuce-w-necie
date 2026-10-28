#pragma once

// ============================================================================
//  MENU GŁÓWNE
// ============================================================================
const std::array<std::string, 5> menuButtons = {"new_game", "rules", "options", "credits", "quit"};

void drawMenuPlaqueHighlight(sf::RenderWindow& window, int index)
{
const std::array<std::array<sf::Vector2f, 4>, 3> plaques = {{
{{{738.f, 432.f}, {1142.f, 429.f}, {1137.f, 617.f}, {750.f, 618.f}}},
{{{350.f, 637.f}, {687.f, 569.f}, {716.f, 718.f}, {397.f, 795.f}}},
{{{1207.f, 571.f}, {1545.f, 630.f}, {1506.f, 784.f}, {1182.f, 724.f}}}
}};
const int plaqueIndex = index == 0 ? 0 : (index == 2 ? 1 : 2);

sf::ConvexShape highlight;
highlight.setPointCount(4);
for (size_t i = 0; i < plaques[plaqueIndex].size(); ++i)
    highlight.setPoint(i, plaques[plaqueIndex][i]);
highlight.setFillColor(sf::Color(255, 255, 255, 45));
highlight.setOutlineColor(sf::Color(255, 255, 255, 235));
highlight.setOutlineThickness(4.f);
window.draw(highlight);


}

void drawMenuSecondaryButton(sf::RenderWindow& window, const std::string& name, bool focused)
{
Button& button = buttons.at(name);
const bool hovered = button.shape.getGlobalBounds().contains(mousePos);
if (hovered)
hoveredButton = name;

button.shape.setFillColor(hovered || focused ? sf::Color(133, 87, 67, 235)
                                             : sf::Color(75, 51, 43, 220));
button.shape.setOutlineThickness(2.f);
button.shape.setOutlineColor(hovered || focused ? sf::Color::White
                                                : sf::Color(224, 201, 174));
button.text.setFillColor(sf::Color::White);
window.draw(button.shape);
window.draw(button.text);


}

void drawOverlay(sf::RenderWindow& window)
{
hoveredButton.clear();
sf::RectangleShape shade({1920.f, 1080.f});
shade.setFillColor(sf::Color(0, 0, 0, 195));
window.draw(shade);
sf::RectangleShape panel({1120.f, 780.f});
panel.setPosition({400.f, 150.f});
panel.setFillColor(PANEL_BG);
panel.setOutlineColor(ACCENT);
panel.setOutlineThickness(3.f);
window.draw(panel);
drawLabel(window, rulesOpen ? L"JAK GRAĆ?" : (confirmLeave ? L"ZAKOŃCZYĆ PARTIĘ?" : L"PAUZA"),
48, {960.f, 230.f}, ACCENT);
if (rulesOpen)
{
const std::array<sf::String, 9> lines = {{
L"Rzuć kostką i przesuń pionek o wylosowaną liczbę pól.",
L"Na polu z pytaniem wybierz odpowiedź: poprawna daje 1–3 pkt, błędna 0 pkt.",
L"Pole „safe” (bezpieczne) losuje kartę niespodzianki: punkty i/lub ruch.",
L"Awans: 20 pkt na planszy 1, 15 pkt na planszy 2, 10 pkt na planszy 3.",
L"Kto pierwszy zdobędzie 10 pkt na ostatniej planszy, wygrywa grę.",
L"Czaszka cofa o planszę (bez pytania) i zeruje postęp na planszy.",
L"Łączny wynik zostaje. Każde kolejne 15 pkt daje dodatkowy rzut.",
L"SPACJA: rzut  |  A–D / 1–4: odpowiedź  |  ENTER: dalej",
L"Kółko nad planszą: zoom  |  R: pełny widok  |  ESC: pauza"
}};
for (size_t i = 0; i < lines.size(); ++i)
drawLabel(window, lines[i], 25, {960.f, 325.f + i * 52.f}, sf::Color::White);
drawButton(window, "overlay_back");
}
else if (confirmLeave)
{
drawLabel(window, L"Bieżąca rozgrywka zostanie utracona.", 30, {960.f, 410.f}, GRAY_TEXT);
drawButton(window, "leave_yes");
drawButton(window, "resume");
}
else
{
drawLabel(window, L"Gra jest zatrzymana. Możesz wrócić w dowolnym momencie.", 26,
{960.f, 350.f}, GRAY_TEXT);
drawButton(window, "resume");
drawButton(window, "pause_rules");
drawButton(window, "leave");
}
}

void updateMenu() {}

void activateMenu(int index)
{
playEffect("ui_click");
if (index == 0) scene = PLAYER_SELECT;
else if (index == 1) rulesOpen = true;
else if (index == 2) scene = SETTINGS;
else if (index == 3) scene = CREDITS;
else scene = QUIT;
}

void handleMenuEvent(const sf::Event& event, const sf::RenderWindow& window)
{
if (rulesOpen)
{
if (const auto* key = event.getIf<sf::Event::KeyPressed>())
if (key->code == sf::Keyboard::Key::Escape || key->code == sf::Keyboard::Key::Enter)
rulesOpen = false;
if (const auto click = leftClick(event, window))
if (activateButtonAt("overlay_back", *click)) rulesOpen = false;
return;
}
if (const auto key = event.getIf<sf::Event::KeyPressed>())
{
const int count = static_cast<int>(menuButtons.size());
if (key->code == sf::Keyboard::Key::Down) menuFocus = (menuFocus + 1) % count;
if (key->code == sf::Keyboard::Key::Up) menuFocus = (menuFocus + count - 1) % count;
if (key->code == sf::Keyboard::Key::Enter) activateMenu(menuFocus);
}
if (const auto* move = event.getIf<sf::Event::MouseMoved>())
{
const auto pos = window.mapPixelToCoords(move->position);
for (int i = 0; i < static_cast<int>(menuButtons.size()); ++i)
if (buttonHit(menuButtons[i], pos)) { menuFocus = i; break; }
}
if (const auto click = leftClick(event, window))
for (int i = 0; i < static_cast<int>(menuButtons.size()); ++i)
if (buttonHit(menuButtons[i], *click)) { menuFocus = i; activateMenu(i); break; }
}

void drawMenu(sf::RenderWindow& window)
{
window.draw(sprites.at("menu_background"));

for (int i = 0; i < static_cast<int>(menuButtons.size()); ++i)
{
    const std::string& name = menuButtons[i];
    if (i == 1 || i == 4)
    {
        drawMenuSecondaryButton(window, name, i == menuFocus);
        continue;
    }

    Button& button = buttons.at(name);
    const sf::FloatRect bounds = button.shape.getGlobalBounds();
    if (bounds.contains(mousePos))
        hoveredButton = name;

    if (bounds.contains(mousePos) || i == menuFocus)
        drawMenuPlaqueHighlight(window, i);
}

if (rulesOpen) drawOverlay(window);


}

// ============================================================================
//  USTAWIENIA
// ============================================================================
void handleSettingsEvent(const sf::Event& event, const sf::RenderWindow& window)
{
if (const auto* key = event.getIf<sf::Event::KeyPressed>())
{
if (key->code == sf::Keyboard::Key::Escape)
{
scene = MAIN_MENU;
return;
}
}

const auto click = leftClick(event, window);
if (!click)
    return;

if (activateButtonAt("volume_minus", *click))
    volume = std::max(0.f, volume - 5.f);

if (activateButtonAt("volume_plus", *click))
    volume = std::min(100.f, volume + 5.f);

if (buttonHit("mute_sound", *click))
{
    muted = !muted;
    playEffect("ui_change");
    setButtonLabel("mute_sound", muted ? L"TAK" : L"NIE");
}

if (activateButtonAt("mute_music", *click))
{
    musicMuted = !musicMuted;
    setButtonLabel("mute_music", musicMuted ? L"TAK" : L"NIE");
}

if (activateButtonAt("back", *click))
    scene = MAIN_MENU;


}

void drawPanel(sf::RenderWindow& window)
{
window.draw(rectangles.at("settings_background_outline"));
window.draw(rectangles.at("settings_background"));
}

void drawSettings(sf::RenderWindow& window)
{
drawPanel(window);

const float cx = WINDOWS_SIZE.x / 2.f;
const float panelTop = rectangles.at("settings_background").getPosition().y;

drawLabel(window, "USTAWIENIA", 48, {cx, panelTop + 80.f}, ACCENT);
drawLabel(window,
          L"Poziom dźwięku: " + std::to_wstring(static_cast<int>(volume)) + L"%",
          28, {cx, panelTop + 220.f});
drawLabel(window, L"Wycisz dźwięk", 28, {cx, panelTop + 350.f});
drawLabel(window, L"Wycisz muzykę", 28, {cx, panelTop + 480.f});

for (const char* name : {"volume_minus", "volume_plus", "mute_sound", "mute_music", "back"})
    drawButton(window, name);


}

// ============================================================================
//  AUTORZY (KOMPATYBILNE Z SFML 3.0 I BEZPIECZNE DLA UTF-8)
// ============================================================================
float creditsScrollOffset = 0.f;

void uiLabel(sf::RenderWindow& window, const sf::String& text, unsigned int size,
sf::Vector2f pos, float width, sf::Color color);

void handleCreditsEvent(const sf::Event& event, const sf::RenderWindow& window)
{
if (const auto* key = event.getIf<sf::Event::KeyPressed>())
{
if (key->code == sf::Keyboard::Key::Escape)
{
scene = MAIN_MENU;
return;
}
}
if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
{
creditsScrollOffset = std::clamp(creditsScrollOffset - wheel->delta * 35.f, 0.f, 320.f);
}
if (const auto click = leftClick(event, window))
{
if (activateButtonAt("back", *click))
scene = MAIN_MENU;
}
}

void drawCredits(sf::RenderWindow& window)
{
drawPanel(window);

const float cx = WINDOWS_SIZE.x / 2.f;
const float panelTop = rectangles.at("settings_background").getPosition().y;

// Bezpieczne stałe łańcuchy znaków
std::string titleText = "TWÓRCE GRY";
std::string subtitleText = "Zespół utalentowanych twórców";
std::string scrollTipText = "Użyj kółka myszy, aby przewijać listę autorów";

std::string n1 = "Eryk Goraj", r1 = "Główny Programista & Architekt";
std::string n2 = "Konrad Witkowski", r2 = "Game Tester & Pomysł";
std::string n3 = "Maja Protasewicz", r3 = "Grafik";
std::string n4 = "Szymon Woźnica", r4 = "Project Manager & Pytania";
std::string n5 = "Filip Czepiela", r5 = "Project Manager & Pytania";
std::string n6 = "Przemysław Żebrak", r6 = "Grafik, Animator & Muzyka";
std::string n7 = "Mikołaj Śliwiński", r7 = "Projekt Planszy";
std::string n8 = "Marcel Kuchta", r8 = "Grafik";
std::string n9 = "Grzegorz Curyło", r9 = "Tester";
std::string n10 = "Kacper Gorajczyk", r10 = "Grafik, Tester & Projekt Planszy 3D";

auto u8 = [](const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
};

drawLabel(window, u8(titleText), 44, {cx, panelTop + 55.f}, ACCENT);
drawLabel(window, u8(subtitleText), 20, {cx, panelTop + 95.f}, GRAY_TEXT);

struct AuthorItem {
    sf::String name;
    sf::String role;
    sf::Color color;
};

const std::vector<AuthorItem> authors = {
    {u8(n1), u8(r1), ACCENT},
    {u8(n2), u8(r2), GOLD},
    {u8(n3), u8(r3), sf::Color(188, 165, 111)},
    {u8(n4), u8(r4), sf::Color(151, 185, 112)},
    {u8(n5), u8(r5), sf::Color(198, 180, 125)},
    {u8(n6), u8(r6), sf::Color(133, 157, 91)},
    {u8(n7), u8(r7), sf::Color(177, 130, 78)},
    {u8(n8), u8(r8), sf::Color(171, 190, 130)},
    {u8(n9), u8(r9), sf::Color(190, 148, 105)},
    {u8(n10), u8(r10), sf::Color(212, 188, 121)}
};

static float animTimer = 0.f;
animTimer += 0.04f;

sf::FloatRect clipRect({cx - 360.f, panelTop + 125.f}, {720.f, 490.f});
const sf::View originalView = window.getView();
const auto vp = originalView.getViewport();
const auto viewSize = originalView.getSize();
const auto viewStart = originalView.getCenter() - viewSize / 2.f;

sf::View clipView(clipRect);
clipView.setViewport(sf::FloatRect(
    {vp.position.x + (clipRect.position.x - viewStart.x) / viewSize.x * vp.size.x,
     vp.position.y + (clipRect.position.y - viewStart.y) / viewSize.y * vp.size.y},
    {clipRect.size.x / viewSize.x * vp.size.x, clipRect.size.y / viewSize.y * vp.size.y}));
window.setView(clipView);

float startY = clipRect.position.y + 15.f - creditsScrollOffset;
for (size_t i = 0; i < authors.size(); ++i)
{
    float itemY = startY + static_cast<float>(i) * 72.f;
    if (itemY > clipRect.position.y - 70.f && itemY < clipRect.position.y + clipRect.size.y + 50.f)
    {
        float floatOffset = std::sin(animTimer + i * 0.5f) * 2.f;

        sf::RectangleShape card({700.f, 60.f});
        card.setPosition({cx - 350.f, itemY + floatOffset});
        card.setFillColor(sf::Color(42, 48, 34, 220));
        card.setOutlineThickness(1.5f);
        card.setOutlineColor(authors[i].color);
        window.draw(card);

        uiLabel(window, authors[i].name, 21, {cx - 330.f, itemY + floatOffset + 18.f}, 270.f, authors[i].color);
        uiLabel(window, authors[i].role, 15, {cx - 40.f, itemY + floatOffset + 18.f}, 370.f, GRAY_TEXT);
    }
}

window.setView(originalView);

drawLabel(window, u8(scrollTipText), 16, {cx, panelTop + 635.f}, GRAY_TEXT);
drawButton(window, "back");


}