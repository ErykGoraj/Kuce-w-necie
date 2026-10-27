#pragma once

// ============================================================================
//  PYTANIA
// ============================================================================
struct Category
{
    std::string id;
    sf::String name;
    std::string tile;
};

struct Question
{
    int id = 0;
    std::string category;
    sf::String text;
    std::array<sf::String, 4> answers;
    int correct = 0;
    int points = 1;
};

std::vector<Category> categories;
std::vector<Question> questions;
std::vector<bool> questionUsed;

sf::String fromUtf8(const std::string& s)
{
    return sf::String::fromUtf8(s.begin(), s.end());
}

int pointsFromDifficulty(const std::string& difficulty)
{
    if (difficulty == "hard")   return 3;
    if (difficulty == "medium") return 2;
    return 1;
}

struct Surprise
{
    sf::String title;
    sf::String description;
    int points = 0;
    int steps = 0;
};

std::vector<Surprise> surprises;
std::optional<Surprise> currentSurprise;
bool surpriseMovement = false;
int moveDirection = 1;

bool loadSurprises(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    if (content.compare(0, 3, "\xEF\xBB\xBF") == 0) content.erase(0, 3);
    Json root;
    if (!JsonParser(content).parse(root)) return false;
    const Json* cards = root.get("karty_zdarzen");
    if (!cards || cards->type != Json::Type::Array) return false;
    std::vector<Surprise> loaded;
    for (const auto& card : cards->array)
    {
        const auto* points = card.get("punkty");
        const auto* steps = card.get("tury");
        if (!points || !steps || points->type != Json::Type::Number ||
            steps->type != Json::Type::Number ||
            std::floor(points->number) != points->number ||
            std::floor(steps->number) != steps->number ||
            std::abs(points->number) > 1000 || std::abs(steps->number) > 1000 ||
            card.getString("tytul").empty() || card.getString("opis").empty())
            return false;
        loaded.push_back({fromUtf8(card.getString("tytul")), fromUtf8(card.getString("opis")),
                          card.getInt("punkty"), card.getInt("tury")});
    }
    if (loaded.empty()) return false;
    surprises = std::move(loaded);
    return true;
}

bool loadQuestions(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        std::cout << "FAILED TO OPEN QUESTIONS FILE: " << path << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF)
    {
        content.erase(0, 3);
    }

    Json root;
    JsonParser parser(content);
    if (!parser.parse(root) || root.type != Json::Type::Object)
    {
        std::cout << "INVALID JSON IN: " << path << std::endl;
        return false;
    }

    categories.clear();
    questions.clear();

    if (const Json* cats = root.get("categories"))
    {
        for (const Json& c : cats->array)
            categories.push_back({c.getString("id"), fromUtf8(c.getString("name")), c.getString("tile")});
    }

    const Json* cards = root.get("cards");
    if (!cards)
        return false;

    for (const Json& card : cards->array)
    {
        const Json* answers = card.get("answers");
        if (!answers)
            continue;

        Question q;
        q.id = card.getInt("id");
        q.category = card.getString("category");
        q.text = fromUtf8(card.getString("question"));

        bool ok = true;
        for (int i = 0; i < 4; i++)
        {
            const Json* a = answers->get(std::string(1, static_cast<char>('A' + i)));
            if (!a || a->type != Json::Type::String)
            {
                ok = false;
                break;
            }
            q.answers[i] = fromUtf8(a->string);
        }

        const std::string correct = card.getString("correct_answer");
        if (!ok || correct.size() != 1 || correct[0] < 'A' || correct[0] > 'D')
        {
            std::cout << "SKIPPED INVALID CARD id=" << q.id << std::endl;
            continue;
        }
        q.correct = correct[0] - 'A';

        int points = card.getInt("points", 0);
        if (points <= 0)
            points = pointsFromDifficulty(card.getString("difficulty"));
        q.points = std::clamp(points, 1, 3);

        questions.push_back(std::move(q));
    }

    questionUsed.assign(questions.size(), false);
    return !questions.empty();
}

sf::String categoryName(const std::string& id)
{
    for (const Category& c : categories)
    {
        if (c.id == id)
            return c.name;
    }
    return sf::String();
}

const Question& pickQuestion(const std::string& tileType)
{
    std::string base = tileType;
    const std::string suffix = "_big";
    if (base.size() > suffix.size() &&
        base.compare(base.size() - suffix.size(), suffix.size(), suffix) == 0)
    {
        base.erase(base.size() - suffix.size());
    }

    std::vector<size_t> pool;
    for (size_t i = 0; i < questions.size(); i++)
    {
        for (const Category& c : categories)
        {
            if (c.id == questions[i].category && c.tile == base)
            {
                pool.push_back(i);
                break;
            }
        }
    }

    if (pool.empty())
    {
        pool.resize(questions.size());
        std::iota(pool.begin(), pool.end(), size_t{0});
    }

    std::vector<size_t> fresh;
    for (size_t idx : pool)
    {
        if (!questionUsed[idx])
            fresh.push_back(idx);
    }

    if (fresh.empty())
    {
        for (size_t idx : pool)
            questionUsed[idx] = false;
        fresh = pool;
    }

    const size_t chosen = fresh[std::uniform_int_distribution<size_t>(0, fresh.size() - 1)(rng())];
    questionUsed[chosen] = true;
    return questions[chosen];
}

