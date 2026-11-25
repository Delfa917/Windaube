/**
 * @file view.h
 *
 * @brief The view of the Hnefatafl game: everything that displays or reads the keyboard.
 *
 * The game functions (functions.h) only compute; this file contains the functions that
 * interact with the players: terminal setup (provided), display, keyboard input and messages.
 *
 * @author JMB - IUT Informatique La Rochelle
 * @date 14/10/2026
 */

#ifndef VIEW_H
#define VIEW_H

#include <string>
#include "typeDef.h"

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
bool enableTerminalFormatting();

/**
 * @brief Clears the console screen based on the operating system.
 *
 * Executes platform-specific system commands: "cls" on Windows, "clear" on Linux/macOS.
 *
 * @return `true` if successful, `false` if an error occurred.
 * @note Provided. If this fails, the game continues but the screen won't be cleared.
 */
bool clearConsole();

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
void displayHnefataflLogo();

/**
 * @brief Displays the game board with piece positions and labels.
 *
 * Shows the board with column numbers (1-11 or 1-13) and row letters (A-K or A-M).
 * Displays pieces (shields, swords, king) and special cells (castle, fortresses).
 *
 * @param aBoard The game board object to display.
 * @note Free display (not tested, not evaluated). Handles both LITTLE (11x11) and BIG (13x13) boards.
 */
void displayBoard(const Board& aBoard);

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
std::string moveStatusMessage(MoveStatus aStatus);

/**
 * @brief Displays a question and reads the answer (a whole line, spaces included).
 *
 * @param aPrompt The question to display (e.g. "Quel est le nom du joueur attaquant ? ").
 * @param aText The line typed by the user, without the final Enter (output).
 * @return `false` if nothing can be read anymore (end of input), `true` otherwise.
 */
bool readLine(const std::string& aPrompt, std::string& aText);

/**
 * @brief Asks the board size until the user types a valid one (11 or 13).
 *
 * Uses readLine() and stringToBoardSize(); displays an error message after an invalid answer
 * and asks again.
 *
 * @param aBoardSize The size typed by the user (output).
 * @return `false` if nothing can be read anymore (end of input), `true` otherwise.
 */
bool askBoardSize(BoardSize& aBoardSize);

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
bool askPosition(const std::string& aPrompt, Position& aPosition, const Board& aBoard);

/**
 * @brief Asks a move: the start cell, then the end cell.
 *
 * Uses askPosition() twice. The move is not checked here (see checkMovement / playTurn).
 *
 * @param aBoard The board (to check the bounds of the positions).
 * @param aMove The move typed by the user (output).
 * @return `false` if nothing can be read anymore (end of input), `true` otherwise.
 */
bool askMove(const Board& aBoard, Move& aMove);

#endif // VIEW_H
