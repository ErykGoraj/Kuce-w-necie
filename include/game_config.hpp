#pragma once

// ============================================================================
//  STALE
// ============================================================================
constexpr int FRAME_RATE = 60;
const sf::Vector2u WINDOWS_SIZE{1920, 1080};

constexpr int MAX_NICK_LENGTH = 12;
constexpr float ROLL_DURATION = 0.9f; // czas animacji rzutu kostka (s)
constexpr float STEP_TIME = 0.28f;    // czas przejscia o jedno pole (s)

// --- zasady gry ---
constexpr int LEVEL_COUNT = 3;                              // 3 plansze
constexpr int LEVEL_THRESHOLDS[LEVEL_COUNT] = {20, 35, 45}; // sumy skumulowane: plansza 1 = 20 pkt, plansza 2 = 15 pkt, plansza 3 = 10 pkt
constexpr int BONUS_ROLL_EVERY = 15;                        // co tyle punktow gracz dostaje dodatkowy rzut
constexpr int MAX_SKULLS_LAST_LEVEL = 2;                    // maks. liczba czaszek na ostatniej planszy
const char* const QUESTIONS_PATH = "resources/questions.json";

const sf::Color ACCENT(151, 185, 112);
const sf::Color PANEL_BG(39, 49, 34);
const sf::Color GRAY_TEXT(221, 211, 186);
const sf::Color GOLD(230, 194, 118);

enum Scene {
    MAIN_MENU,
    SETTINGS,
    CREDITS,
    GAME,
    PLAYER_SELECT,
    GAME_OVER,
    QUIT
};

Scene scene = MAIN_MENU;
bool gamePaused = false;
bool rulesOpen = false;
bool confirmLeave = false;
int menuFocus = 0;
float landingTimer = 0.f;
std::array<float, 5> questionScroll{};

// ============================================================================
//  SYSTEM CZĄSTECZEK CYBER (EFEKTY SPECJALNE)
// ============================================================================
struct CyberParticle
{
    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::Color color;
    float life;
    float maxLife;
    float size;
};

std::vector<CyberParticle> cyberParticles;

void spawnCyberParticles(sf::Vector2f center, int count, bool explosion = false)
{
    for (int i = 0; i < count; ++i)
    {
        CyberParticle p;
        p.pos = center;
        float angle = static_cast<float>(std::rand()) / RAND_MAX * 2.f * 3.14159265f;
        float speed = explosion ? (60.f + static_cast<float>(std::rand()) / RAND_MAX * 280.f)
                                : (15.f + static_cast<float>(std::rand()) / RAND_MAX * 45.f);
        p.vel = {std::cos(angle) * speed, std::sin(angle) * speed - (explosion ? 60.f : 25.f)};
        p.maxLife = 0.8f + static_cast<float>(std::rand()) / RAND_MAX * 1.2f;
        p.life = p.maxLife;
        p.size = 2.5f + static_cast<float>(std::rand()) / RAND_MAX * 4.5f;

        int colType = std::rand() % 3;
        if (colType == 0) p.color = ACCENT;
        else if (colType == 1) p.color = GOLD;
        else p.color = sf::Color(183, 199, 125);

        cyberParticles.push_back(p);
    }
}

void updateCyberParticles(float dt)
{
    for (auto it = cyberParticles.begin(); it != cyberParticles.end();)
    {
        it->life -= dt;
        if (it->life <= 0.f)
        {
            it = cyberParticles.erase(it);
        }
        else
        {
            it->pos += it->vel * dt;
            it->vel *= (1.0f - 2.0f * dt);
            ++it;
        }
    }
}

void drawCyberParticles(sf::RenderWindow& window)
{
    for (const auto& p : cyberParticles)
    {
        float ratio = p.life / p.maxLife;
        sf::CircleShape shape(p.size * ratio);
        shape.setPosition(p.pos);
        sf::Color c = p.color;
        c.a = static_cast<std::uint8_t>(255.f * ratio);
        shape.setFillColor(c);
        window.draw(shape);
    }
}

// ============================================================================
//  KUCYKI / GRACZE
// ============================================================================
constexpr int PONY_COUNT = 4;
constexpr int COLOR_COUNT = 12;
int ponyFrameCount = 0;
constexpr float PONY_FRAME_TIME = 0.12f;
float ponyAnimationTime = 0.f;

const std::array<sf::String, COLOR_COUNT> ponyLabels = {{
    sf::String(L"Czerwony"), sf::String(L"Niebieski"),
    sf::String(L"Zielony"), sf::String(L"Żółty"),
    L"Różowy", L"Fioletowy", L"Turkusowy", L"Pomarańczowy",
    L"Biały", L"Lawendowy", L"Limonkowy", L"Brzoskwiniowy"
}};

const std::array<sf::Color, COLOR_COUNT> ponyColors = {{
    sf::Color(220, 60, 60), sf::Color(70, 130, 255),
    sf::Color(70, 200, 90), sf::Color(240, 210, 60),
    sf::Color(255, 120, 190), sf::Color(180, 100, 255),
    sf::Color(70, 225, 215), sf::Color(255, 155, 65),
    sf::Color(245, 245, 255), sf::Color(190, 175, 255),
    sf::Color(185, 240, 90), sf::Color(255, 190, 160)
}};

struct Player
{
    sf::String nick;
    int pony = 0;
    int tile = 0;
    int level = 0;
    int points = 0;
    int levelPoints = 0;
    int bonusMilestones = 0;
};

int levelGoal(int level)
{
    return LEVEL_THRESHOLDS[level] - (level == 0 ? 0 : LEVEL_THRESHOLDS[level - 1]);
}

float boardZoom = 1.f;
std::vector<Player> players;
int playersCount = 0;
int countFocus = 2;
char32_t ignoredCountCharacter = 0;
sf::String enteredNick;
sf::String selectMessage;
int ponyFocus = 0;

// ============================================================================
//  ZASOBY
// ============================================================================
std::map<std::string, sf::Texture> textures;
std::map<std::string, sf::Sprite> sprites;
std::map<std::string, sf::RectangleShape> rectangles;
sf::Font arial_font;

float volume = 5.f;
bool muted = false;
bool musicMuted = false;

std::map<std::string, sf::SoundBuffer> effectBuffers;
std::map<std::string, sf::Sound> effects;
std::string hoveredButton;
std::string previousHoveredButton;

float effectVolume(const std::string& name)
{
    return muted ? 0.f : volume * (name == "ui_hover" ? 0.25f : 0.55f);
}

void playEffect(const std::string& name)
{
    const auto found = effects.find(name);
    if (found == effects.end() || muted || volume <= 0.f)
        return;
    found->second.setVolume(effectVolume(name));
    found->second.play();
}

void loadEffects()
{
    for (const std::string name : {"ui_click", "ui_hover", "ui_change"})
    {
        auto& buffer = effectBuffers[name];
        if (buffer.loadFromFile("resources/sounds/" + name + ".wav"))
            effects.try_emplace(name, buffer);
        else
            std::cerr << "Brak efektu dzwiekowego: " << name << '\n';
    }
}

sf::Vector2f mousePos;

struct Button
{
    sf::RectangleShape shape;
    sf::Text text;

    explicit Button(const sf::Font& font) : text(font) {}
};

std::map<std::string, Button> buttons;

// ============================================================================
//  PLANSZA
// ============================================================================
struct Tile
{
    sf::Vector2f position;
    std::string type;
    float size = 80.f;
};

std::vector<Tile> board;
std::array<std::vector<int>, LEVEL_COUNT> tracks;

std::mt19937& rng()
{
    static std::mt19937 generator{std::random_device{}()};
    return generator;
}

int rollDice()
{
    return std::uniform_int_distribution<int>(1, 6)(rng());
}
