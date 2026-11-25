/**
 * @file tests.h
 *
 * @brief Declarations of test functions for testing the functions implemented in 'functions.cpp'.
 *
 * Each test function checks one function of 'functions.h' on both board sizes,
 * with nominal cases and edge cases. See 'tests.cpp' for how to read the results.
 *
 * @author JMB - IUT Informatique La Rochelle
 * @date 14/10/2026
 */

#ifndef TESTS_H
#define TESTS_H

// ========================= ANSI COLOR CODES =========================
// ANSI Color codes for better test output readability
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

// ========================= GLOBAL VARIABLES =========================
// Global variables to control test display behavior
extern bool DISPLAY_BOARDS;
extern bool DISPLAY_PROMPTS;
extern bool DISPLAY_PASSED;

// ========================= TEST FUNCTIONS =========================
// In the recommended order of development (same order as in launchTests).

// ─────────────────────────────────────────────────────────────────
// Step 1: Start of the game (board size, player names, board)
// ─────────────────────────────────────────────────────────────────

/**
 * @brief Test function for readLine: reads a whole line, or returns false at the end of input.
 */
void test_readLine();

/**
 * @brief Test function for the stringToBoardSize function.
 *
 * Checks the conversion of various texts: valid sizes, other numbers, letters,
 * spaces, signs, leading zeros, empty text.
 */
void test_stringToBoardSize();

/**
 * @brief Test function for askBoardSize: asks again until the answer is valid.
 */
void test_askBoardSize();

/**
 * @brief Test function for the createBoard function.
 *
 * This function tests the createBoard function by verifying that boards are correctly allocated
 * for different sizes. Tests cover valid board sizes, pointer verification, memory allocation
 * checks, and edge cases including size validation and double allocation detection.
 */
void test_createBoard();

/**
 * @brief Test function for the deleteBoard function.
 *
 * This function tests the deleteBoard function by ensuring that boards are correctly deleted
 * and pointers are set to nullptr after deletion.
 */
void test_deleteBoard();

/**
 * @brief Test function for the initializeBoard function.
 *
 * This function tests the initializeBoard function by comparing the generated board with expected board configurations.
 */
void test_initializeBoard();

// ─────────────────────────────────────────────────────────────────
// Step 2: Moves (with playGame, the game is playable: MVP)
// ─────────────────────────────────────────────────────────────────

/**
 * @brief Test function for the isValidPosition function.
 *
 * This function tests the isValidPosition function for different board sizes, valid positions,
 * and invalid positions. Tests include corners, edges, center, out-of-bounds positions, and
 * extreme values for both LITTLE and BIG board sizes.
 */
void test_isValidPosition();

/**
 * @brief Test function for the stringToPosition function.
 *
 * Checks the conversion of various texts into positions: valid cells (uppercase and lowercase),
 * cells outside the board, wrong formats, leading zeros, spaces, empty text, both board sizes.
 */
void test_stringToPosition();

/**
 * @brief Test function for askPosition: asks again until the answer is a valid position.
 */
void test_askPosition();

/**
 * @brief Test function for askMove: asks the start cell, then the end cell.
 */
void test_askMove();

/**
 * @brief Test function for the isEmptyCell function.
 *
 * This function tests the isEmptyCell function by checking various combinations of cell types
 * and piece types. It verifies that the function correctly identifies empty cells (NONE) and
 * non-empty cells (SWORD, SHIELD, or KING) across different board sizes and cell types.
 */
void test_isEmptyCell();

/**
 * @brief Test function for the checkMovement function.
 *
 * Checks the value returned for many moves: valid moves, piece ownership, directions,
 * obstacles, fortress and castle rules, positions out of the board. The reason of each
 * refusal (MoveStatus) is checked too.
 */
void test_checkMovement();

/**
 * @brief Test function for moveStatusMessage: one different, non-empty message for each MoveStatus.
 */
void test_moveStatusMessage();

/**
 * @brief Test the movePiece function with different board sizes and piece types.
 *
 * This function tests the movePiece function by verifying that pieces are correctly moved
 * on the board. Tests cover all piece types (SWORD, SHIELD, KING) with horizontal and vertical
 * moves, and special KING moves to/from FORTRESS and CASTLE cells. Note: movePiece only handles
 * mechanical piece relocation; move validation is tested separately in test_isValidMovement.
 */
void test_movePiece();

/**
 * @brief Test function for switchCurrentPlayer.
 *
 * This function tests the switchCurrentPlayer function by toggling between players
 * and verifying the current player state.
 */
void test_switchCurrentPlayer();

/**
 * @brief Test function for the playTurn function.
 *
 * Checks that a valid move is played completely (move, captures, player switched),
 * and that a refused move changes nothing (same board, same current player).
 */
void test_playTurn();

// ─────────────────────────────────────────────────────────────────
// Step 3: Complete game (captures, end of the game)
// ─────────────────────────────────────────────────────────────────

/**
 * @brief Test the capturePieces function with various scenarios.
 *
 * This function tests the capturePieces function by setting up different scenarios
 * to capture pieces on the board. It covers ATTACK captures (SWORD captures SHIELD),
 * DEFENSE captures (SHIELD/KING captures SWORD), and non-capture cases where pieces
 * are protected or on the same team.
 */
void test_capturePieces();

/**
 * @brief Test function for getKingPosition.
 *
 * This function tests the getKingPosition function by verifying that it correctly retrieves
 * the position of the king on the board. Tests cover different board sizes and various king
 * positions including corners, edges, center, and special cells (FORTRESS, CASTLE).
 */
void test_getKingPosition();

/**
 * @brief Test function for isKingEscaped.
 *
 * This function tests the isKingEscaped function by verifying that it correctly detects
 * when the king has escaped to a FORTRESS cell. Tests cover different board sizes and
 * various king positions including FORTRESS corners, CASTLE center, and NORMAL cells.
 */
void test_isKingEscaped();

/**
 * @brief Test function for isKingCapturedSimple.
 *
 * This function tests the isKingCapturedSimple function by verifying that it correctly detects
 * when the king is captured. Tests cover scenarios with the king surrounded by SWORD pieces on
 * all four sides, partial encirclement, and edge cases with FORTRESS and CASTLE cells.
 */
void test_isKingCapturedSimple();

/**
 * @brief Test function for isSwordLeft.
 *
 * This function tests the isSwordLeft function by verifying that it correctly detects
 * the presence or absence of SWORD pieces on the board. Tests cover boards with no swords,
 * single sword, and multiple swords across different positions and board sizes.
 */
void test_isSwordLeft();

/**
 * @brief Test function for canCurrentPlayerMove.
 *
 * Checks the detection of a blocked player: pieces surrounded by other pieces, by the castle
 * or by a fortress (allowed to the king only), no piece at all.
 */
void test_canCurrentPlayerMove();

/**
 * @brief Test function for isGameFinished.
 *
 * This function tests the isGameFinished function by verifying that it correctly detects
 * game ending conditions: king captured, king escaped to FORTRESS, or no SWORD pieces left.
 * Tests cover various scenarios for both board sizes.
 */
void test_isGameFinished();

/**
 * @brief Test function for whoWon.
 *
 * This function tests the whoWon function by verifying that it correctly returns the winner
 * based on different game ending scenarios: DEFENSE wins if king escaped or no swords left,
 * ATTACK wins if king is captured.
 */
void test_whoWon();

/**
 * @brief Plays complete games (sequences of moves from the starting position) with playTurn.
 *
 * Checks the end of each game: game finished or not (isGameFinished) and winner (whoWon).
 * These tests use initializeBoard, playTurn, isGameFinished and whoWon together.
 */
void test_completeGames();

// ─────────────────────────────────────────────────────────────────
// Step 4: Bonus
// ─────────────────────────────────────────────────────────────────

/**
 * @brief Test function for isKingCapturedRecursive.
 *
 * This function tests the isKingCapturedRecursive function by verifying that it correctly detects
 * king captures with escape route analysis. Tests include complex scenarios with SHIELD chains,
 * blocked escape routes, and situations where the king has valid escape paths.
 */
void test_isKingCapturedRecursive();

// ========================= HELPER FUNCTIONS =========================

/**
 * @brief Check Windows environment and inform about crash protection options.
 *
 * On Windows, displays information about limited crash protection (try/catch only)
 * and explains how to get full fork() protection via WSL (see README-WINDOWS.md).
 *
 * On macOS/Linux, this is a no-op as fork() is natively available.
 */
void checkWindowsEnvironment();

/**
 * @brief Configures the display settings for tests.
 *
 * Called by launchTests() in main.cpp with the settings written at the beginning of main.cpp.
 * @param showBoards Whether to display the boards used by the tests
 * @param showPrompts Whether to display the messages printed by your functions during tests
 * @param showPassedTests Whether to list the passed tests (false: only failed tests are listed)
 */
void configureTestDisplay(bool showBoards, bool showPrompts, bool showPassedTests = true);

/**
 * @brief Prints the main test suite header.
 * Displays a stylized header at the start of the test suite execution.
 */
void printTestSuiteHeader();

/**
 * @brief Prints the main test suite footer.
 * Displays a summary table (status of every tested function) and the mini report.
 */
void printTestSuiteFooter();

#endif // TESTS_H
