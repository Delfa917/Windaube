/**
 * @file view.cpp
 *
 * @brief Implementation of the view of the Hnefatafl game: display, keyboard input and messages.
 *
 * @author JMB - IUT Informatique La Rochelle
 * @date 14/10/2026
 */

#include <iostream>
#include <string>
#include <cstdlib>   // system

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

#include "typeDef.h"
#include "functions.h"
#include "view.h"

// ============================================================================
// SECTION 1: TERMINAL (provided: nothing to write)
// ============================================================================

/**
 * @brief Enables terminal formatting support for colors and UTF-8 characters.
 *
 * Configures the terminal to display ANSI color codes and UTF-8 characters.
 * On Windows, activates UTF-8 encoding and ANSI escape sequences.
 * On Linux/macOS, no configuration needed (enabled by default).
 *
 * @return `true` if successful, `false` if an error occurred.
 * @note Provided. The game will still work if this fails, but without colors and special characters.
 */
bool enableTerminalFormatting() {
#ifdef _WIN32
    // === STEP 1: Enable UTF-8 output on Windows ===
    // Changes console code page to 65001 (UTF-8)
    // This allows displaying special characters like ⛒ (king), ⏺ (sword), ○ (shield)
    int result = system("chcp 65001 > nul");
    if (result != 0) {
        cerr << "Warning: Failed to set UTF-8 code page (chcp 65001)." << endl;
        // Continue anyway - not critical, game can still run
    }

    // === STEP 2: Enable ANSI color codes on Windows 10+ ===
    // This allows using ANSI escape sequences for colored text output

    // Get the console output handle
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        cerr << "Error: Failed to get console output handle." << endl;
        cout << "Warning: Terminal formatting could not be enabled." << endl;
        cout << "Colors and special characters may not display correctly." << endl;
        return false;
    }

    // Get the current console mode
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        cerr << "Error: Failed to get console mode." << endl;
        cout << "Warning: Terminal formatting could not be enabled." << endl;
        cout << "Colors and special characters may not display correctly." << endl;
        return false;
    }

    // Enable virtual terminal processing (ANSI escape sequences)
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        cerr << "Error: Failed to enable ANSI escape sequences (virtual terminal processing)." << endl;
        cout << "Warning: Terminal formatting could not be enabled." << endl;
        cout << "Colors and special characters may not display correctly." << endl;
        return false;
    }

    return true;
#else
    // On Unix-like systems (Linux, macOS), UTF-8 and ANSI codes are usually enabled by default
    return true;
#endif
}

/**
 * @brief Clears the console screen based on the operating system.
 *
 * Executes platform-specific system commands: "cls" on Windows, "clear" on Linux/macOS.
 *
 * @return `true` if successful, `false` if an error occurred.
 * @note Provided. If this fails, the game continues but the screen won't be cleared.
 */
bool clearConsole() {
    int result;
#ifdef _WIN32
    result = std::system("cls"); // For Windows
#elif defined(__unix__) || defined(__APPLE__) // Linux or macOS
    result = std::system("clear"); // For Linux and macOS
#else
    // Unsupported operating system
    cerr << "Error: Unsupported operating system for console clearing." << endl;
    cerr << "       Supported systems: Windows, Linux, macOS." << endl;
    return false;
#endif

    // Check if the command executed successfully
    if (result != 0) {
        cerr << "Error: Failed to clear the console screen. Exit code: " << result << endl;
        return false;
    }

    return true;
}

// ============================================================================
// SECTION 2: DISPLAY (free display: not tested, not evaluated)
// ============================================================================

/**
 * @brief Displays the game logo in ASCII art.
 *
 * Prints the Hnefatafl game logo with stylized text and credit line.
 * Best called after `clearConsole()` for clean display.
 *
 * @note Free display (not tested, not evaluated). Call `enableTerminalFormatting()` first
 *       for a proper display of special characters on Windows.
 */
void displayHnefataflLogo()
{
    // TODO: Display the logo of the game (free display)
    markNotImplemented(__func__); // Remove this line as soon as you write this function
}

/**
 * @brief Displays the game board with piece positions and labels.
 *
 * Shows the board with column numbers (1-11 or 1-13) and row letters (A-K or A-M).
 * Displays pieces (shields, swords, king) and special cells (castle, fortresses).
 *
 * @param aBoard The game board object to display.
 * @note Free display (not tested, not evaluated). Handles both LITTLE (11x11) and BIG (13x13) boards.
 */
void displayBoard(const Board& aBoard)
{
    // TODO: Display the board (free display)
    markNotImplemented(__func__); // Remove this line as soon as you write this function
}

// ============================================================================
// SECTION 3: MESSAGES AND KEYBOARD INPUT
// ============================================================================

/**
 * @brief Gives the message explaining the result of a move, to display to the player.
 *
 * Each value of MoveStatus has its own message (e.g. MOVE_PATH_BLOCKED: "Une pièce bloque le chemin.").
 *
 * @param aStatus The result of checkMovement() or playTurn().
 * @return The message to display (not empty, different for each value).
 */
string moveStatusMessage(MoveStatus aStatus)
{
    // TODO: Return the message corresponding to the status
    markNotImplemented(__func__); // Remove this line as soon as you write this function
    return "";
}

/**
 * @brief Displays a question and reads the answer (a whole line, spaces included).
 *
 * @param aPrompt The question to display (e.g. "Quel est le nom du joueur attaquant ? ").
 * @param aText The line typed by the user, without the final Enter (output).
 * @return `false` if nothing can be read anymore (end of input), `true` otherwise.
 */
bool readLine(const string& aPrompt, string& aText)
{
    // TODO: Display the question and read a whole line
    markNotImplemented(__func__); // Remove this line as soon as you write this function
    return false;
}

/**
 * @brief Asks the board size until the user types a valid one (11 or 13).
 *
 * Uses readLine() and stringToBoardSize(); displays an error message after an invalid answer
 * and asks again.
 *
 * @param aBoardSize The size typed by the user (output).
 * @return `false` if nothing can be read anymore (end of input), `true` otherwise.
 */
bool askBoardSize(BoardSize& aBoardSize)
{
    // TODO: Ask the board size until it is valid
    markNotImplemented(__func__); // Remove this line as soon as you write this function
    return false;
}

/**
 * @brief Asks a position until the user types a valid one (e.g. "F6").
 *
 * Uses readLine() and stringToPosition(); displays an error message after an invalid answer
 * and asks again.
 *
 * @param aPrompt The question to display.
 * @param aPosition The position typed by the user (output).
 * @param aBoard The board (to check the bounds).
 * @return `false` if nothing can be read anymore (end of input), `true` otherwise.
 */
bool askPosition(const string& aPrompt, Position& aPosition, const Board& aBoard)
{
    // TODO: Ask a position until it is valid
    markNotImplemented(__func__); // Remove this line as soon as you write this function
    return false;
}

/**
 * @brief Asks a move: the start cell, then the end cell.
 *
 * Uses askPosition() twice. The move is not checked here (see checkMovement / playTurn).
 *
 * @param aBoard The board (to check the bounds of the positions).
 * @param aMove The move typed by the user (output).
 * @return `false` if nothing can be read anymore (end of input), `true` otherwise.
 */
bool askMove(const Board& aBoard, Move& aMove)
{
    // TODO: Ask the start cell, then the end cell
    markNotImplemented(__func__); // Remove this line as soon as you write this function
    return false;
}
