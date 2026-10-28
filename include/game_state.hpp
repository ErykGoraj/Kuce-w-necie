#pragma once

// ============================================================================
//  STAN GRY
// ============================================================================
enum class TurnState { WAIT_ROLL, ROLLING, MOVING, LANDING, QUESTION, ANSWERED, SURPRISE };

TurnState turnState = TurnState::WAIT_ROLL;
int currentPlayer = 0;
int displayedDice = 0;
int stepsLeft = 0;
float stepProgress = 0.f;
float rollTimer = 0.f;
float faceTimer = 0.f;

const Question* currentQuestion = nullptr;
int chosenAnswer = -1;
bool lastCorrect = false;
int lastGained = 0;
bool bonusRoll = false;
sf::String levelUpMessage;
bool skullLanding = false;
int winner = -1;

// ============================================================================