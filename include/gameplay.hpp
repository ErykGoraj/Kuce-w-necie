#pragma once

// ============================================================================
//  GRA - LOGIKA TURY
// ============================================================================
void startRoll()
{
    if (turnState != TurnState::WAIT_ROLL)
        return;

    levelUpMessage.clear();
    bonusRoll = false;
    currentSurprise.reset();
    surpriseMovement = false;
    moveDirection = 1;
    turnState = TurnState::ROLLING;
    rollTimer = 0.f;
    faceTimer = 0.f;
}

void startQuestion()
{
    const Player& p = players[currentPlayer];
    const Tile& landedOn = board[tracks[p.level][p.tile]];

    if (landedOn.type == "safe_big")
    {
        currentQuestion = nullptr;
        questionScroll.fill(0.f);
        currentSurprise = surprises.empty()
                              ? Surprise{L"Brak niespodzianek", L"Nie udało się wczytać kart. Brak zmian.", 0, 0}
                              : surprises[std::uniform_int_distribution<size_t>(0, surprises.size() - 1)(rng())];
        turnState = TurnState::SURPRISE;
        return;
    }
    currentQuestion = &pickQuestion(landedOn.type);
    chosenAnswer = -1;
    questionScroll.fill(0.f);
    lastCorrect = false;
    lastGained = 0;
    turnState = TurnState::QUESTION;
}

bool teleportFromSkull(Player& player)
{
    const std::string& tileType = board[tracks[player.level][player.tile]].type;
    int destinationLevel = -1;

    if (tileType == "skull_big" && player.level == 1)
        destinationLevel = 0;
    else if (tileType == "skull_big" && player.level == LEVEL_COUNT - 1)
        destinationLevel = 1;

    if (destinationLevel < 0)
        return false;

    const std::vector<int>& destinationTrack = tracks[destinationLevel];
    std::vector<int> destinations;
    for (size_t i = 0; i < destinationTrack.size(); i++)
    {
        const Tile& tile = board[destinationTrack[i]];
        if (tile.type != "skull_big")
            destinations.push_back(static_cast<int>(i));
    }

    if (destinations.empty())
        return false;

    std::uniform_int_distribution<size_t> chooseDestination(0, destinations.size() - 1);
    player.level = destinationLevel;
    player.levelPoints = 0;
    player.tile = destinations[chooseDestination(rng())];
    levelUpMessage = L"Czaszka! Powrót na planszę " + std::to_wstring(destinationLevel + 1) + L".";
    return true;
}

void awardPoints(int delta)
{
    Player& p = players[currentPlayer];
    p.points = std::max(0, p.points + delta);
    p.levelPoints = std::max(0, p.levelPoints + delta);

    if (delta > 0)
    {
        const int milestones = p.points / BONUS_ROLL_EVERY;
        if (milestones > p.bonusMilestones)
        {
            p.bonusMilestones = milestones;
            bonusRoll = true;
        }

        while (p.levelPoints >= levelGoal(p.level))
        {
            if (p.level == LEVEL_COUNT - 1)
            {
                winner = currentPlayer;
                break;
            }

            p.levelPoints -= levelGoal(p.level);
            p.level++;
            p.tile = 0;
            levelUpMessage = L"Awans na planszę " + std::to_wstring(p.level + 1) + L"!";
        }
    }
}

void submitAnswer(int index)
{
    if (turnState != TurnState::QUESTION || !currentQuestion || index < 0 || index > 3)
        return;
    playEffect("ui_click");
    chosenAnswer = index;
    lastCorrect = (index == currentQuestion->correct);
    lastGained = lastCorrect ? currentQuestion->points : 0;
    awardPoints(lastGained);

    if (lastCorrect)
    {
        spawnCyberParticles({960.f, 540.f}, 80, true);
    }

    turnState = TurnState::ANSWERED;
}

void continueAfterAnswer()
{
    if (turnState != TurnState::ANSWERED)
        return;

    currentQuestion = nullptr;

    if (winner >= 0)
    {
        scene = GAME_OVER;
        return;
    }

    if (!bonusRoll)
        currentPlayer = (currentPlayer + 1) % static_cast<int>(players.size());

    turnState = TurnState::WAIT_ROLL;
}

void finishSurprise()
{
    awardPoints(currentSurprise->points);
    currentSurprise.reset();
    surpriseMovement = false;
    moveDirection = 1;
    turnState = TurnState::ANSWERED;
    continueAfterAnswer();
}

void acceptSurprise()
{
    if (turnState != TurnState::SURPRISE || !currentSurprise) return;
    playEffect("ui_click");
    if (currentSurprise->steps == 0)
    {
        finishSurprise();
        return;
    }
    moveDirection = currentSurprise->steps < 0 ? -1 : 1;
    stepsLeft = std::abs(currentSurprise->steps);
    stepProgress = 0.f;
    surpriseMovement = true;
    turnState = TurnState::MOVING;
}

int nextMovementTile(const Player& player)
{
    const int count = static_cast<int>(tracks[player.level].size());
    return (player.tile + moveDirection + count) % count;
}

void updateGame(float dt)
{
    updateCyberParticles(dt);
    if (gamePaused) return;
    switch (turnState)
    {
    case TurnState::WAIT_ROLL:
    case TurnState::QUESTION:
    case TurnState::ANSWERED:
    case TurnState::SURPRISE:
        break;

    case TurnState::LANDING:
        landingTimer -= dt;
        if (landingTimer <= 0.f)
        {
            if (skullLanding)
            {
                skullLanding = false;
                levelUpMessage.clear();
                currentPlayer = (currentPlayer + 1) % static_cast<int>(players.size());
                turnState = TurnState::WAIT_ROLL;
            }
            else
                startQuestion();
        }
        break;

    case TurnState::ROLLING:
        rollTimer += dt;
        faceTimer += dt;

        if (faceTimer >= 0.06f)
        {
            faceTimer = 0.f;
            displayedDice = rollDice();
        }

        if (rollTimer >= ROLL_DURATION)
        {
            displayedDice = rollDice();
            stepsLeft = displayedDice;
            stepProgress = 0.f;
            turnState = TurnState::MOVING;
        }
        break;

    case TurnState::MOVING:
        stepProgress += dt / STEP_TIME;

        if (stepProgress >= 1.f)
        {
            Player& p = players[currentPlayer];
            p.tile = nextMovementTile(p);
            stepProgress -= 1.f;
            stepsLeft--;

            if (stepsLeft <= 0)
            {
                stepProgress = 0.f;
                if (surpriseMovement)
                {
                    finishSurprise();
                    break;
                }
                const std::string& landedType = board[tracks[players[currentPlayer].level][players[currentPlayer].tile]].type;
                if (landedType == "skull_big")
                {
                    teleportFromSkull(players[currentPlayer]);
                    skullLanding = true;
                }
                else
                {
                    skullLanding = false;
                }

                // Dodajemy 2-sekundową pauzę widoczności pola dla wszystkich pól
                landingTimer = 2.0f;
                turnState = TurnState::LANDING;
            }
        }
        break;
    }
}

sf::FloatRect answerRect(int i)
{
    return sf::FloatRect({100.f, 340.f + static_cast<float>(i) * 140.f}, {1720.f, 128.f});
}

void handleGameEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (event.is<sf::Event::FocusLost>()) { gamePaused = true; return; }
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        if (key->code == sf::Keyboard::Key::Escape)
        {
            if (rulesOpen) rulesOpen = false;
            else if (confirmLeave) confirmLeave = false;
            else gamePaused = !gamePaused;
            return;
        }
    }
    if (gamePaused)
    {
        if (const auto click = leftClick(event, window))
        {
            if (rulesOpen) { if (activateButtonAt("overlay_back", *click)) rulesOpen = false; }
            else if (confirmLeave)
            {
                if (activateButtonAt("leave_yes", *click)) resetToMenu();
                else if (activateButtonAt("resume", *click)) { confirmLeave = false; gamePaused = false; }
            }
            else if (activateButtonAt("resume", *click)) gamePaused = false;
            else if (activateButtonAt("pause_rules", *click)) rulesOpen = true;
            else if (activateButtonAt("leave", *click)) confirmLeave = true;
        }
        return;
    }
    if (turnState == TurnState::SURPRISE)
    {
        if (const auto* key = event.getIf<sf::Event::KeyPressed>())
            if (key->code == sf::Keyboard::Key::Enter) acceptSurprise();
        if (const auto click = leftClick(event, window))
            if (buttonHit("continue", *click)) acceptSurprise();
        if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
            questionScroll[0] -= wheel->delta * 40.f;
        return;
    }
    if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        const sf::Vector2f pos = window.mapPixelToCoords(wheel->position);
        if (turnState == TurnState::QUESTION || turnState == TurnState::ANSWERED)
        {
            if (sf::FloatRect({100.f, 140.f}, {1720.f, 180.f}).contains(pos))
                questionScroll[0] -= wheel->delta * 40.f;
            for (int i = 0; i < 4; ++i)
                if (answerRect(i).contains(pos)) questionScroll[i + 1] -= wheel->delta * 35.f;
            return;
        }
        if (turnState != TurnState::QUESTION && turnState != TurnState::ANSWERED &&
            sf::FloatRect({460.f, 40.f}, {1000.f, 1000.f}).contains(pos))
            boardZoom = std::clamp(boardZoom * std::pow(1.15f, wheel->delta), 1.f, 3.f);
    }
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        const auto code = key->code;

        if (code == sf::Keyboard::Key::R)
            boardZoom = 1.f;

        if (turnState == TurnState::WAIT_ROLL)
        {
            if (code == sf::Keyboard::Key::Space)
                startRoll();
        }
        else if (turnState == TurnState::QUESTION)
        {
            if (code == sf::Keyboard::Key::A || code == sf::Keyboard::Key::Num1 || code == sf::Keyboard::Key::Numpad1)
                submitAnswer(0);
            else if (code == sf::Keyboard::Key::B || code == sf::Keyboard::Key::Num2 || code == sf::Keyboard::Key::Numpad2)
                submitAnswer(1);
            else if (code == sf::Keyboard::Key::C || code == sf::Keyboard::Key::Num3 || code == sf::Keyboard::Key::Numpad3)
                submitAnswer(2);
            else if (code == sf::Keyboard::Key::D || code == sf::Keyboard::Key::Num4 || code == sf::Keyboard::Key::Numpad4)
                submitAnswer(3);
        }
        else if (turnState == TurnState::ANSWERED)
        {
            if (code == sf::Keyboard::Key::Enter)
                continueAfterAnswer();
        }
    }

    if (const auto click = leftClick(event, window))
    {
        if (turnState == TurnState::WAIT_ROLL)
        {
            if (activateButtonAt("roll", *click))
                startRoll();
        }
        else if (turnState == TurnState::QUESTION)
        {
            for (int i = 0; i < 4; i++)
            {
                if (answerRect(i).contains(*click))
                {
                    submitAnswer(i);
                    break;
                }
            }
        }
        else if (turnState == TurnState::ANSWERED)
        {
            if (activateButtonAt("continue", *click))
                continueAfterAnswer();
        }
    }
}

