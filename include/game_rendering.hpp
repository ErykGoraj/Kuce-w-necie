#pragma once

// ============================================================================
//  GRA - RYSOWANIE
// ============================================================================
void drawDice(sf::RenderWindow& window, int value, sf::Vector2f center, float size)
{
const auto texture = textures.find("dice" + std::to_string(value));
if (value >= 1 && value <= 6 && texture != textures.end())
{
sf::Sprite die(texture->second);
const sf::Vector2u textureSize = texture->second.getSize();
die.setOrigin({textureSize.x / 2.f, textureSize.y / 2.f});
die.setPosition(center);
die.setScale({size / textureSize.x, size / textureSize.y});
window.draw(die);
return;
}

sf::RectangleShape body({size, size});
body.setOrigin({size / 2.f, size / 2.f});
body.setPosition(center);
body.setFillColor(sf::Color(240, 240, 245));
body.setOutlineThickness(4.f);
body.setOutlineColor(ACCENT);
window.draw(body);

if (value < 1 || value > 6)
{
    drawLabel(window, "?", static_cast<unsigned int>(size * 0.6f), center, sf::Color(54, 45, 34));
    return;
}

static const std::vector<std::vector<int>> patterns = {
    {}, {4}, {0, 8}, {0, 4, 8}, {0, 2, 6, 8}, {0, 2, 4, 6, 8}, {0, 2, 3, 5, 6, 8}
};

const float r = size * 0.09f;
for (int idx : patterns[value])
{
    const float dx = (idx % 3 - 1) * size * 0.27f;
    const float dy = (idx / 3 - 1) * size * 0.27f;

    sf::CircleShape pip(r);
    pip.setOrigin({r, r});
    pip.setPosition(center + sf::Vector2f(dx, dy));
    pip.setFillColor(sf::Color(54, 45, 34));
    window.draw(pip);
}


}

float tokenSize(int level, int playerCount)
{
const float side = level == LEVEL_COUNT - 1 ? 60.f : 80.f;
const std::array<float, 4> sizes = {0.66f, 0.35f, 0.32f, 0.29f};
return side * sizes[std::clamp(playerCount, 1, 4) - 1];
}

sf::Vector2f tokenOffset(int level, int playerCount, int slot, float size)
{
const float side = level == LEVEL_COUNT - 1 ? 60.f : 80.f;
const int rows = playerCount > 2 ? 2 : 1;
const int row = slot / 2;
const int columns = std::min(2, playerCount - row * 2);
const float pitch = size + 9.f;
return {(side - (columns * pitch - 9.f)) / 2.f + (slot % 2) * pitch,
(side - (rows * pitch - 9.f)) / 2.f + row * pitch};
}

struct TokenPose { sf::Vector2f position; float size; };

TokenPose tokenPose(int index, int movingTile)
{
const auto& p = players[index];
const int tile = index == currentPlayer ? movingTile : p.tile;
int count = 0, slot = 0;
for (int i = 0; i < static_cast<int>(players.size()); ++i)
{
const int otherTile = i == currentPlayer ? movingTile : players[i].tile;
if (players[i].level == p.level && otherTile == tile)
{
if (i < index) ++slot;
++count;
}
}
const float size = tokenSize(p.level, count);
return {board[tracks[p.level][tile]].position + tokenOffset(p.level, count, slot, size), size};
}

void drawToken(sf::RenderWindow& window, int playerIndex)
{
const auto& p = players[playerIndex];
const auto& mover = players[currentPlayer];
TokenPose pose = tokenPose(playerIndex, mover.tile);
if (turnState == TurnState::MOVING)
{
const int nextTile = nextMovementTile(mover);
const TokenPose end = tokenPose(playerIndex, nextTile);
const float t = std::clamp(stepProgress, 0.f, 1.f);
const float ease = t * t * (3.f - 2.f * t);
const auto delta = end.position - pose.position;
if (playerIndex != currentPlayer || std::hypot(delta.x, delta.y) < 160.f)
pose.position += delta * ease;
else if (t >= 0.5f) pose.position = end.position;
pose.size += (end.size - pose.size) * ease;
}
sf::CircleShape ring(pose.size / 2.f);
ring.setPosition(pose.position);
ring.setFillColor(PANEL_BG);
ring.setOutlineThickness(3.f);
ring.setOutlineColor(ponyColors[p.pony]);
window.draw(ring);
drawPony(window, p.pony, pose.position, pose.size);
}

sf::View boardView(const sf::View& uiView)
{
float minX = 1440.f, minY = 1020.f, maxX = 480.f, maxY = 60.f;
auto includeTile = [&](int level, int tile) {
const auto pos = board[tracks[level][tile]].position;
const float side = level == LEVEL_COUNT - 1 ? 60.f : 80.f;
minX = std::min(minX, pos.x - 20.f);
minY = std::min(minY, pos.y - 20.f);
maxX = std::max(maxX, pos.x + side + 20.f);
maxY = std::max(maxY, pos.y + side + 20.f);
};
for (int i = 0; i < static_cast<int>(players.size()); ++i)
{
const auto& p = players[i];
includeTile(p.level, p.tile);
if (i == currentPlayer && turnState == TurnState::MOVING)
includeTile(p.level, nextMovementTile(p));
}
const float side = std::clamp(std::max({1000.f / boardZoom, maxX - minX, maxY - minY}),
1000.f / 3.f, 1000.f);
const float half = side / 2.f;
sf::View view;
view.setSize({side, side});
view.setCenter({std::clamp((minX + maxX) / 2.f, 460.f + half, 1460.f - half),
std::clamp((minY + maxY) / 2.f, 40.f + half, 1040.f - half)});
const auto vp = uiView.getViewport();
view.setViewport(sf::FloatRect(
{vp.position.x + vp.size.x * 460.f / WINDOWS_SIZE.x,
vp.position.y + vp.size.y * 40.f / WINDOWS_SIZE.y},
{vp.size.x * 1000.f / WINDOWS_SIZE.x, vp.size.y * 1000.f / WINDOWS_SIZE.y}));
return view;
}

const sf::Color UI_SURFACE(36, 45, 32);
const sf::Color UI_CARD(56, 55, 39);
const sf::Color UI_BORDER(102, 110, 75);
const sf::Color UI_TEXT(244, 237, 218);
const sf::Color UI_MUTED(185, 179, 148);
const sf::Color UI_SUCCESS(151, 196, 111);
const sf::Color UI_ERROR(255, 142, 153);

void uiBox(sf::RenderWindow& window, sf::Vector2f pos, sf::Vector2f size,
sf::Color fill, sf::Color border = UI_BORDER)
{
sf::RectangleShape box(size);
box.setPosition(pos); box.setFillColor(fill);
box.setOutlineThickness(-1.f); box.setOutlineColor(border);
window.draw(box);
}

void uiLabel(sf::RenderWindow& window, const sf::String& text, unsigned int size,
sf::Vector2f pos, float width, sf::Color color)
{
sf::Text label(arial_font, text, size);
while (size > 12 && label.getLocalBounds().size.x > width)
label.setCharacterSize(--size);
drawLabel(window, text, size, pos, color, false);
}

void uiProgress(sf::RenderWindow& window, sf::Vector2f pos, float width, float ratio, sf::Color color)
{
uiBox(window, pos, {width, 6.f}, UI_BORDER, UI_BORDER);
const float fill = width * std::clamp(ratio, 0.f, 1.f);
if (fill > 0.f) uiBox(window, pos, {fill, 6.f}, color, color);
}

void uiTextBlock(sf::RenderWindow& window, const sf::String& text, sf::FloatRect rect,
unsigned int size, float& scroll, sf::Color color)
{
struct CachedBlock
{
sf::String source;
unsigned int size = 0;
float width = 0.f;
std::vector<sf::Text> lines;
};
static std::map<float*, CachedBlock> cache;
auto& block = cache[&scroll];
if (block.source != text || block.size != size || block.width != rect.size.x)
{
block.source = text;
block.size = size;
block.width = rect.size.x;
block.lines.clear();
for (const auto& line : wrapText(text, size, rect.size.x - 16.f))
{
block.lines.emplace_back(arial_font, line, size);
const auto bounds = block.lines.back().getLocalBounds();
block.lines.back().setOrigin({bounds.position.x, bounds.position.y + bounds.size.y / 2.f});
}
}
auto& lines = block.lines;
const float lineHeight = size * 1.35f;
const float contentHeight = lines.size() * lineHeight;
const float maxScroll = std::max(0.f, contentHeight - rect.size.y);
scroll = std::clamp(scroll, 0.f, maxScroll);
const sf::View original = window.getView();
const auto vp = original.getViewport();
const auto viewSize = original.getSize();
const auto viewStart = original.getCenter() - viewSize / 2.f;
sf::View clip(rect);
clip.setViewport(sf::FloatRect(
{vp.position.x + (rect.position.x - viewStart.x) / viewSize.x * vp.size.x,
vp.position.y + (rect.position.y - viewStart.y) / viewSize.y * vp.size.y},
{rect.size.x / viewSize.x * vp.size.x, rect.size.y / viewSize.y * vp.size.y}));
window.setView(clip);
const float top = rect.position.y + (maxScroll == 0.f ? (rect.size.y - contentHeight) / 2.f : 0.f);
for (size_t i = 0; i < lines.size(); ++i)
{
lines[i].setPosition({rect.position.x, top + (i + 0.5f) * lineHeight - scroll});
lines[i].setFillColor(color);
window.draw(lines[i]);
}
window.setView(original);
if (maxScroll > 0.f)
{
const float thumb = std::max(18.f, rect.size.y * rect.size.y / contentHeight);
uiBox(window, {rect.position.x + rect.size.x - 5.f, rect.position.y}, {3.f, rect.size.y}, UI_BORDER, UI_BORDER);
uiBox(window, {rect.position.x + rect.size.x - 5.f,
rect.position.y + (rect.size.y - thumb) * scroll / maxScroll}, {3.f, thumb}, ACCENT, ACCENT);
}
}

void drawGamePanels(sf::RenderWindow& window)
{
const Player& cur = players[currentPlayer];
const sf::Color playerColor = ponyColors[cur.pony];
uiBox(window, {28.f, 40.f}, {412.f, 1000.f}, UI_SURFACE);
uiBox(window, {1480.f, 40.f}, {412.f, 1000.f}, UI_SURFACE);
uiLabel(window, L"CYBER KUCYKI", 26, {56.f, 82.f}, 356.f, ACCENT);
uiLabel(window, L"TWOJA TURA", 16, {56.f, 147.f}, 350.f, UI_MUTED);
uiBox(window, {56.f, 179.f}, {356.f, 116.f}, UI_CARD);
drawPony(window, cur.pony, {74.f, 200.f}, 70.f);
uiLabel(window, cur.nick, 28, {164.f, 220.f}, 226.f, playerColor);
uiLabel(window, L"Plansza " + std::to_wstring(cur.level + 1) + L" z 3", 20,
{164.f, 260.f}, 226.f, UI_MUTED);
drawDice(window, displayedDice, {234.f, 398.f}, 118.f);
sf::String status;
switch (turnState)
{
case TurnState::WAIT_ROLL: status = bonusRoll ? L"Masz dodatkowy rzut" : L"Rzuć kostką, aby rozpocząć"; break;
case TurnState::ROLLING: status = L"Losowanie wyniku…"; break;
case TurnState::MOVING: status = L"Pozostałe kroki: " + std::to_wstring(stepsLeft); break;
case TurnState::LANDING: status = L"Sprawdzamy pole…"; break;
case TurnState::SURPRISE: status = L"Niespodzianka! Przeczytaj komunikat"; break;
case TurnState::QUESTION: status = L"Wybierz jedną odpowiedź"; break;
case TurnState::ANSWERED: status = lastCorrect ? L"Dobra odpowiedź!" : L"Spróbuj w kolejnej turze"; break;
}
uiLabel(window, status, 23, {56.f, 500.f}, 356.f, UI_TEXT);
drawButton(window, "roll", turnState == TurnState::WAIT_ROLL);
uiLabel(window, L"POSTĘP NA PLANSZY", 16, {56.f, 638.f}, 350.f, UI_MUTED);
uiLabel(window, std::to_wstring(cur.levelPoints) + L" / " + std::to_wstring(levelGoal(cur.level)) + L" pkt",
32, {56.f, 679.f}, 350.f, UI_TEXT);
uiProgress(window, {56.f, 713.f}, 356.f, float(cur.levelPoints) / levelGoal(cur.level), playerColor);
uiBox(window, {56.f, 747.f}, {356.f, 85.f}, UI_CARD);
uiLabel(window, L"ŁĄCZNY WYNIK", 15, {74.f, 772.f}, 220.f, UI_MUTED);
uiLabel(window, std::to_wstring(cur.points) + L" pkt", 28, {74.f, 807.f}, 310.f, GOLD);
uiLabel(window, L"Do bonusowego rzutu: " + std::to_wstring(BONUS_ROLL_EVERY - cur.points % BONUS_ROLL_EVERY) + L" pkt",
19, {56.f, 868.f}, 350.f, UI_MUTED);
uiLabel(window, L"SPACJA   Rzut kostką", 18, {56.f, 928.f}, 350.f, UI_MUTED);
uiLabel(window, L"KÓŁKO   Zoom     R   Cała plansza", 17, {56.f, 965.f}, 350.f, UI_MUTED);
uiLabel(window, L"ESC   Pauza i zasady", 18, {56.f, 1002.f}, 350.f, UI_MUTED);

uiLabel(window, L"UCZESTNICY", 24, {1508.f, 82.f}, 356.f, UI_TEXT);
uiLabel(window, L"Kolejność tur • " + std::to_wstring(players.size()) + L" graczy", 18,
        {1508.f, 119.f}, 356.f, UI_MUTED);
for (size_t i = 0; i < players.size(); ++i)
{
    const auto& p = players[i];
    const bool active = static_cast<int>(i) == currentPlayer;
    const float y = 156.f + i * 166.f;
    uiBox(window, {1508.f, y}, {356.f, 148.f}, active ? sf::Color(48, 65, 39) : UI_CARD,
          active ? ponyColors[p.pony] : UI_BORDER);
    drawPony(window, p.pony, {1526.f, y + 20.f}, 46.f);
    uiLabel(window, p.nick, 23, {1588.f, y + 28.f}, 255.f, ponyColors[p.pony]);
    uiLabel(window, active ? L"TERAZ GRA" : L"WYNIK: " + std::to_wstring(p.points) + L" PKT", 14,
            {1588.f, y + 58.f}, 255.f, active ? UI_TEXT : UI_MUTED);
    uiLabel(window, L"Plansza " + std::to_wstring(p.level + 1) + L" / 3     •     " +
                        std::to_wstring(p.levelPoints) + L" / " + std::to_wstring(levelGoal(p.level)) + L" pkt",
            18, {1526.f, y + 102.f}, 320.f, UI_MUTED);
    uiProgress(window, {1526.f, y + 125.f}, 320.f, float(p.levelPoints) / levelGoal(p.level), ponyColors[p.pony]);
}
uiLabel(window, L"DROGA DO ZWYCIĘSTWA", 16, {1508.f, 865.f}, 356.f, UI_MUTED);
const std::array<sf::String, 3> stages = {{L"12×12", L"8×8", L"FINAŁ"}};
for (int i = 0; i < 3; ++i)
{
    const float x = 1508.f + i * 122.f;
    uiBox(window, {x, 892.f}, {112.f, 60.f}, i == cur.level ? sf::Color(65, 82, 45) : UI_CARD,
          i == cur.level ? ACCENT : UI_BORDER);
    drawLabel(window, stages[i], 20, {x + 56.f, 922.f}, i == cur.level ? ACCENT : UI_MUTED);
}
uiLabel(window, L"Cel: 20 / 15 / 10 pkt na kolejnych planszach", 18, {1508.f, 991.f}, 356.f, UI_MUTED);


}

void drawQuestionModal(sf::RenderWindow& window)
{
if (!currentQuestion) return;
hoveredButton.clear();
const auto& q = *currentQuestion;
const bool answered = turnState == TurnState::ANSWERED;
uiBox(window, {0.f, 0.f}, {1920.f, 1080.f}, sf::Color(14, 20, 12, 220), sf::Color::Transparent);
uiBox(window, {70.f, 35.f}, {1780.f, 1000.f}, sf::Color(18, 19, 11, 110), sf::Color::Transparent);
uiBox(window, {60.f, 25.f}, {1800.f, 1010.f}, UI_SURFACE);
uiBox(window, {60.f, 25.f}, {1800.f, 4.f}, ACCENT, ACCENT);
sf::String category = categoryName(q.category);
if (category.isEmpty()) category = L"Wiedza ogólna";
uiLabel(window, L"PYTANIE  /  " + category, 18, {100.f, 65.f}, 1450.f, ACCENT);
uiLabel(window, players[currentPlayer].nick + L" • wybierz jedną odpowiedź", 18,
{100.f, 105.f}, 1450.f, UI_MUTED);
uiBox(window, {1646.f, 51.f}, {174.f, 68.f}, UI_CARD);
drawLabel(window, L"+" + std::to_wstring(q.points) + L" PKT", 27, {1733.f, 85.f}, GOLD);
uiTextBlock(window, q.text, sf::FloatRect({100.f, 140.f}, {1720.f, 180.f}), 34, questionScroll[0], UI_TEXT);
for (int i = 0; i < 4; ++i)
{
const auto rect = answerRect(i);
const bool correct = answered && i == q.correct;
const bool wrong = answered && i == chosenAnswer && !correct;
const bool hover = !answered && rect.contains(mousePos);
const sf::Color color = correct ? UI_SUCCESS : (wrong ? UI_ERROR : (hover ? ACCENT : UI_BORDER));
const sf::Color fill = correct ? sf::Color(44, 66, 39) : (wrong ? sf::Color(65, 34, 45) :
(hover ? sf::Color(69, 82, 47) : UI_CARD));
uiBox(window, rect.position, rect.size, fill, color);
uiBox(window, {rect.position.x + 18.f, rect.position.y + 40.f}, {48.f, 48.f}, UI_SURFACE, color);
drawLabel(window, sf::String(std::wstring(1, static_cast<wchar_t>(L'A' + i))), 25,
{rect.position.x + 42.f, rect.position.y + 64.f}, correct || wrong ? color : UI_TEXT);
uiTextBlock(window, q.answers[i], sf::FloatRect({rect.position.x + 86.f, rect.position.y + 8.f},
{1460.f, 112.f}), 26, questionScroll[i + 1], UI_TEXT);
const sf::String tag = correct ? L"POPRAWNA" : (wrong ? L"TWÓJ WYBÓR" : (hover ? L"WYBIERZ" : L""));
drawLabel(window, tag, 14, {rect.position.x + 1636.f, rect.position.y + 64.f}, color);
}

drawCyberParticles(window);

if (!answered)
{
    uiLabel(window, L"A–D / 1–4   Wybór odpowiedzi     •     ESC   Pauza", 18, {100.f, 940.f}, 1720.f, UI_MUTED);
    uiLabel(window, L"Dłuższą treść przewiniesz kółkiem myszy nad tekstem.", 17, {100.f, 990.f}, 1720.f, UI_MUTED);
    return;
}
const sf::String result = lastCorrect ? L"Dobra odpowiedź!  +" + std::to_wstring(lastGained) + L" pkt" :
                              L"Tym razem 0 pkt. Poprawna odpowiedź: " + std::wstring(1, static_cast<wchar_t>(L'A' + q.correct));
uiLabel(window, result, 24, {100.f, 930.f}, 1720.f, lastCorrect ? UI_SUCCESS : UI_ERROR);
sf::String detail = levelUpMessage;
if (winner >= 0) detail = L"Ostatnia plansza ukończona. Zwycięstwo!";
else if (bonusRoll) detail += detail.isEmpty() ? L"Bonus: rzucasz jeszcze raz!" : L"  •  Bonusowy rzut!";
if (detail.isEmpty()) detail = L"Naciśnij ENTER, aby przejść do kolejnej tury.";
uiLabel(window, detail, 18, {100.f, 990.f}, 1020.f, UI_MUTED);
drawButton(window, "continue");


}

const wchar_t* fieldsWord(int n)
{
n = std::abs(n);
if (n == 1) return L"pole";
if (n % 10 >= 2 && n % 10 <= 4 && !(n % 100 >= 12 && n % 100 <= 14)) return L"pola";
return L"pól";
}

void drawSurpriseModal(sf::RenderWindow& window)
{
if (!currentSurprise) return;
hoveredButton.clear();
const auto& card = *currentSurprise;
uiBox(window, {0.f, 0.f}, {1920.f, 1080.f}, sf::Color(14, 20, 12, 220), sf::Color::Transparent);
uiBox(window, {390.f, 140.f}, {1140.f, 875.f}, UI_SURFACE);
uiLabel(window, L"NIESPODZIANKA", 22, {426.f, 190.f}, 1068.f, GOLD);
uiLabel(window, card.title, 38, {426.f, 265.f}, 1068.f, UI_TEXT);
uiTextBlock(window, card.description, sf::FloatRect({426.f, 335.f}, {1068.f, 260.f}),
32, questionScroll[0], UI_TEXT);
const sf::String points = card.points > 0 ? L"+" + std::to_wstring(card.points)
: std::to_wstring(card.points);
uiLabel(window, L"Punkty: " + points, 30, {426.f, 660.f}, 1068.f,
card.points < 0 ? UI_ERROR : UI_SUCCESS);
const sf::String movement = card.steps == 0 ? L"Bez przesunięcia pionka"
: L"Ruch o " + std::to_wstring(std::abs(card.steps)) + L" " + fieldsWord(card.steps) +
(card.steps > 0 ? L" do przodu" : L" do tyłu");
uiLabel(window, movement, 30, {426.f, 730.f}, 1068.f, ACCENT);
uiLabel(window, L"ENTER lub DALEJ: zastosuj efekt i zakończ turę.", 22,
{426.f, 840.f}, 1068.f, UI_MUTED);
drawButton(window, "continue");
}

void drawGame(sf::RenderWindow& window)
{
const sf::View uiView = window.getView();
window.setView(boardView(uiView));
for (const Tile& tile : board)
drawTile(window, tile);

for (int i = 0; i < static_cast<int>(players.size()); i++)
{
    if (i != currentPlayer)
        drawToken(window, i);
}
drawToken(window, currentPlayer);

window.setView(uiView);
drawGamePanels(window);
if (turnState == TurnState::LANDING && !levelUpMessage.isEmpty())
{
    sf::RectangleShape banner({900.f, 70.f});
    banner.setPosition({510.f, 50.f});
    banner.setFillColor(PANEL_BG);
    window.draw(banner);
    drawLabel(window, levelUpMessage, 28, {960.f, 85.f}, GOLD);
}

if (turnState == TurnState::QUESTION || turnState == TurnState::ANSWERED)
    drawQuestionModal(window);
if (turnState == TurnState::SURPRISE)
    drawSurpriseModal(window);
if (gamePaused) drawOverlay(window);


}