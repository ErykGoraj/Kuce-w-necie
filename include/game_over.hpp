#pragma once

// ============================================================================
//  KONIEC GRY
// ============================================================================
void handleGameOverEvent(const sf::Event& event, const sf::RenderWindow& window)
{
if (const auto* key = event.getIf<sf::Event::KeyPressed>())
{
if (key->code == sf::Keyboard::Key::Escape || key->code == sf::Keyboard::Key::Enter)
{
resetToMenu();
return;
}
}

if (const auto click = leftClick(event, window))
{
    if (activateButtonAt("menu_end", *click))
        resetToMenu();
}


}

void drawGameOver(sf::RenderWindow& window)
{
const float cx = WINDOWS_SIZE.x / 2.f;

drawLabel(window, "KONIEC GRY", 96, {cx, 170.f}, ACCENT);

if (winner >= 0 && winner < static_cast<int>(players.size()))
{
    const Player& w = players[winner];
    drawLabel(window, L"Wygrywa:", 40, {cx, 290.f}, GRAY_TEXT);
    drawLabel(window, w.nick, 72, {cx, 370.f}, ponyColors[w.pony]);
    drawPony(window, w.pony, {cx - 60.f, 420.f}, 120.f);
}

std::vector<int> order(players.size());
std::iota(order.begin(), order.end(), 0);
std::stable_sort(order.begin(), order.end(), [](int a, int b)
                 {
                     if ((a == winner) != (b == winner)) return a == winner;
                     if (players[a].level != players[b].level) return players[a].level > players[b].level;
                     if (players[a].levelPoints != players[b].levelPoints) return players[a].levelPoints > players[b].levelPoints;
                     return players[a].points > players[b].points;
                 });

const float rowH = 56.f;
const float tableW = 1000.f;
const float tx = cx - tableW / 2.f;
const float ty = 600.f;
const float colPlace = tx + 50.f, colNick = tx + 130.f, colLevel = tx + 600.f, colPoints = tx + 800.f;

drawLabel(window, L"TABELA WYNIKÓW", 30, {cx, ty - 40.f}, GOLD);
uiBox(window, {tx, ty}, {tableW, rowH}, UI_CARD, ACCENT);
drawLabel(window, L"#", 24, {colPlace, ty + rowH / 2.f}, UI_MUTED);
drawLabel(window, L"GRACZ", 24, {colNick, ty + rowH / 2.f}, UI_MUTED, false);
drawLabel(window, L"PLANSZA", 24, {colLevel, ty + rowH / 2.f}, UI_MUTED, false);
drawLabel(window, L"PUNKTY", 24, {colPoints, ty + rowH / 2.f}, UI_MUTED, false);

for (size_t i = 0; i < order.size(); i++)
{
    const Player& p = players[order[i]];
    const float ry = ty + rowH * static_cast<float>(i + 1);
    const bool isWinner = order[i] == winner;
    uiBox(window, {tx, ry}, {tableW, rowH}, isWinner ? sf::Color(55, 50, 25) : UI_SURFACE,
          isWinner ? GOLD : UI_BORDER);
    const float cy = ry + rowH / 2.f;
    drawLabel(window, std::to_wstring(i + 1), 28, {colPlace, cy}, isWinner ? GOLD : sf::Color::White);
    drawPony(window, p.pony, {colNick, ry + 6.f}, 44.f);
    drawLabel(window, p.nick, 28, {colNick + 60.f, cy}, ponyColors[p.pony], false);
    drawLabel(window, std::to_wstring(p.level + 1) + L" / 3", 28, {colLevel, cy}, sf::Color::White, false);
    drawLabel(window, std::to_wstring(p.points) + L" pkt", 28, {colPoints, cy}, sf::Color::White, false);
}

drawButton(window, "menu_end");


}