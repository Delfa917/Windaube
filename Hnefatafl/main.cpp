/**
 * @file main.cpp
 *
 * @brief Main program of the Hnefatafl game: play a game or run the tests.
 *
 * playGame() organizes a complete game: it uses the view (view.h) to display and to read
 * the keyboard, and the game functions (functions.h) to play. launchTests() runs the test suite.
 *
 * @author JMB - IUT Informatique La Rochelle
 * @date 14/10/2026
 */

#include <iostream>
#include <string>
#include "typeDef.h"
#include "functions.h"
#include "view.h"
#include "tests.h"

using namespace std;

// ============================================================================
// SETTINGS: change these values (true or false), then run the program
// ============================================================================
const bool PLAY_GAME = false;          // true: play a game, false: run the tests
const bool SHOW_PASSED_TESTS = true;   // false: only the failed tests are displayed
const bool SHOW_TEST_BOARDS = false;   // true: display the board of each displayed test (false: first failure only)
const bool SHOW_TEST_PROMPTS = false;  // true: display the messages written by your functions in the displayed tests
// ============================================================================

/**
 * @brief Function to play the Hnefatafl game.
 *
 * Asks the board size and the names of the players, then lets them play
 * until the end of the game, and displays the winner.
 */
void playGame()
{
    // TODO: Implement the complete game
    markNotImplemented(__func__); // Remove this line as soon as you write this function
    // Hint: create a Game structure, configure the terminal, ask the board size (askBoardSize)
    // and the player names (readLine), create and initialize the board,
    // then repeat until the game is finished:
    //   display the board, ask a move (askMove) and play it (playTurn); if the move is refused,
    //   display moveStatusMessage(...) WITHOUT clearing the screen, and ask again.
    // Finally display the winner (whoWon) and free the memory (deleteBoard).
}

/**
 * @brief Launches the testing suite.
 *
 * This function runs a suite of tests to validate various game functions.
 */
void launchTests(){
    // Configure terminal for colors and UTF-8 support (important on Windows)
    enableTerminalFormatting();

    // Check Windows environment and display WSL options if needed
    // (No-op on macOS/Linux where fork() is available natively)
    checkWindowsEnvironment();

    // Display of the tests: see the settings at the beginning of this file
    configureTestDisplay(SHOW_TEST_BOARDS, SHOW_TEST_PROMPTS, SHOW_PASSED_TESTS);

    // Display test suite header
    printTestSuiteHeader();

    // The tests are launched in the recommended order of development:
    // a playable game first (MVP), then the complete game, then the bonus.

    // ─────────────────────────────────────────────────────────────────
    // Step 1: Start of the game (board size, player names, board)
    // ─────────────────────────────────────────────────────────────────
    test_readLine();
    test_stringToBoardSize();
    test_askBoardSize();
    test_createBoard();
    test_deleteBoard();
    test_initializeBoard();

    // ─────────────────────────────────────────────────────────────────
    // Step 2: Moves → with playGame(), the game is playable (MVP)
    // ─────────────────────────────────────────────────────────────────
    test_isValidPosition();
    test_stringToPosition();
    test_askPosition();
    test_askMove();
    test_isEmptyCell();
    test_checkMovement();
    test_moveStatusMessage();
    test_movePiece();
    test_switchCurrentPlayer();
    test_playTurn();

    // ─────────────────────────────────────────────────────────────────
    // Step 3: Complete game (captures, end of the game)
    // ─────────────────────────────────────────────────────────────────
    test_capturePieces();
    test_getKingPosition();
    test_isKingEscaped();
    test_isKingCapturedSimple();
    test_isSwordLeft();
    test_canCurrentPlayerMove();
    test_isGameFinished();
    test_whoWon();
    test_completeGames();

    // ─────────────────────────────────────────────────────────────────
    // Step 4: Bonus (optional function: "not implemented" if you skip it)
    // ─────────────────────────────────────────────────────────────────
    test_isKingCapturedRecursive();

    // Display test suite footer (summary per function)
    printTestSuiteFooter();
}

/**
 * @brief Main function of the game.
 *
 * The main function serves as the entry point for the Hnefatafl game.
 * It can be used to start the game or run tests.
 *
 * @return 0 for successful execution.
 */
int main() {

    // Choose at the beginning of this file: PLAY_GAME = true (play a game) or false (run the tests)
    if (PLAY_GAME) {
        playGame();
    } else {
        launchTests();
    }

    return 0;
}
