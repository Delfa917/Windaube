/**
 * @file functions.h
 *
 * @brief Declarations of the functions to write for the Hnefatafl game.
 *
 * This file contains the declarations of the functions used in the Hnefatafl game:
 * board management, positions, moves, captures and end of game.
 *
 * These functions only compute: they never read the keyboard and never display messages.
 * The display and the keyboard input are in view.h (the view of the game).
 *
 * @author JMB - IUT Informatique La Rochelle
 * @date 14/10/2026
 */

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>
#include "typeDef.h"

// ============================================================================
// SECTION 1: BOARD MANAGEMENT
// ============================================================================

/**
 * @brief Converts the text typed by the user into a board size.
 *
 * Only "11" (LITTLE) and "13" (BIG) are accepted. Everything else is refused:
 * other numbers, letters, empty text, spaces, signs (+/-), decimal numbers, leading zeros ("011").
 *
 * @param aText The text typed by the user (without the final Enter).
 * @param aBoardSize Reference to store the board size (modified only if the text is valid).
 * @return `true` if the text is a valid size, `false` otherwise.
 * @note No keyboard reading and no display here: see askBoardSize() / askPosition() in view.h.
 */
bool stringToBoardSize(const std::string& aText, BoardSize& aBoardSize);

/**
 * @brief Dynamically creates a game board.
 *
 * Allocates memory for a 2D array of cells and sets every cell to {NORMAL, NONE}.
 * Refuses to allocate if the size is not LITTLE (11) or BIG (13),
 * or if the board is already allocated (prevents memory leaks).
 *
 * @param aBoard Reference to the Board object (`itsSize` must be set, `itsCells` must be nullptr).
 * @return `true` if successful, `false` on invalid size or already allocated board.
 */
bool createBoard(Board& aBoard);

/**
 * @brief Frees the memory allocated for the game board.
 *
 * Deallocates the 2D array of cells and sets `itsCells` to `nullptr`.
 * Safe to call multiple times (does nothing if already freed).
 *
 * @param aBoard Reference to the Board object to deallocate.
 */
void deleteBoard(Board& aBoard);

/**
 * @brief Initializes the game board with starting positions.
 *
 * Places fortresses (4 corners), castle (center), king (center),
 * shields (12 defenders), and swords (24 attackers) based on board size.
 * Every other cell becomes {NORMAL, NONE}, even if the board was already used.
 *
 * @param aBoard The board object to initialize (`itsCells` must be allocated, `itsSize` must be set).
 * @note Does nothing if `itsCells` is nullptr or if the size is not LITTLE or BIG.
 */
void initializeBoard(Board& aBoard);

// ============================================================================
// SECTION 2: POSITION MANAGEMENT
// ============================================================================

/**
 * @brief Checks if a position is valid within the game board.
 *
 * Verifies that row and column indices are within board bounds (0-based indexing).
 *
 * @param aPos The position to check (row and column indices).
 * @param aBoard The game board containing size information.
 * @return `true` if position is within bounds, `false` otherwise.
 */
bool isValidPosition(const Position& aPos, const Board& aBoard);

/**
 * @brief Converts the text typed by the user into a position (format: letter + number).
 *
 * Expects format: letter (row, case-insensitive) + number (column, 1-based), e.g., "A1", "c3", "K11".
 * Rejects: empty text, spaces, leading zeros ("A01"), extra characters, positions outside the board.
 *
 * @param aText The text typed by the user (without the final Enter).
 * @param aPosition Reference to the position to update (0-based: letter A → row 0, number 1 → column 0).
 *                  Modified only if the text is valid.
 * @param aBoard The game board for bounds validation.
 * @return `true` if the text is a valid position on the board, `false` otherwise.
 * @note No keyboard reading and no display here: see askBoardSize() / askPosition() in view.h.
 */
bool stringToPosition(const std::string& aText, Position& aPosition, const Board& aBoard);

/**
 * @brief Checks if a cell is empty (contains no piece).
 *
 * A cell is empty if `itsPieceType` is NONE (ignores `itsCellType`).
 *
 * @param aBoard The game board.
 * @param aPos The position of the cell to check.
 * @return `true` if cell is empty, `false` if it contains a piece.
 */
bool isEmptyCell(const Board& aBoard, const Position& aPos);

// ============================================================================
// SECTION 3: MOVEMENT & ACTIONS
// ============================================================================

/**
 * @brief Checks if a move is valid for the current player, and gives the reason if it is not.
 *
 * The checks are done in this order, the first problem found is returned:
 * 1. MOVE_OUT_OF_BOARD   : start or end position outside the board;
 * 2. MOVE_SAME_CELL      : start and end positions are identical;
 * 3. MOVE_NOT_YOUR_PIECE : the start cell does not contain a piece of the current player
 *                          (SWORD for ATTACK, SHIELD or KING for DEFENSE);
 * 4. MOVE_NOT_STRAIGHT   : the move is neither horizontal nor vertical;
 * 5. then the cells of the path are checked one by one, from the start to the end cell (included):
 *    - MOVE_PATH_BLOCKED   : the cell contains a piece;
 *    - MOVE_FORBIDDEN_CELL : the cell is forbidden to this piece. Only the KING can stop on a
 *      FORTRESS or on the CASTLE; only the KING can cross the (empty) CASTLE; nobody crosses a FORTRESS.
 *
 * @param aGame Current game state (player, board).
 * @param aMove The move to check (start and end positions).
 * @return MOVE_OK if the move is valid, otherwise the reason of the refusal.
 * @note No display here: see moveStatusMessage() in view.h.
 */
MoveStatus checkMovement(const Game& aGame, const Move& aMove);

/**
 * @brief Moves a piece on the board.
 *
 * Transfers piece from start position to end position. Clears start cell (sets to NONE).
 * Cell types (NORMAL, FORTRESS, CASTLE) remain unchanged.
 *
 * @param aGame Current game state.
 * @param aMove The move to execute (start and end positions).
 * @note Assumes move is valid. Use `checkMovement()` first to validate.
 */
void movePiece(Game& aGame, const Move& aMove);

/**
 * @brief Removes captured pieces from the board.
 *
 * Checks all 4 cardinal directions from move end position. Applies capture rules:
 * - ATTACK captures SHIELD when sandwiched between SWORD/FORTRESS/empty CASTLE
 * - DEFENSE captures SWORD when sandwiched between SHIELD/KING/FORTRESS/CASTLE
 * The KING is never captured this way (see isKingCapturedSimple / isKingCapturedRecursive),
 * and the piece that has just moved is never captured (moving between two enemies is safe).
 *
 * @param aGame Current game state.
 * @param aMove The executed move (uses end position for capture check).
 * @note Assumes move already validated and executed. Modifies board to remove captured pieces.
 */
void capturePieces(Game& aGame, const Move& aMove);

/**
 * @brief Switches the active player.
 *
 * Toggles between Player1 and Player2 for alternating turns.
 * If there is no current player yet (nullptr), Player1 becomes the current player.
 *
 * @param aGame Reference to the Game object (updates `itsCurrentPlayer`).
 */
void switchCurrentPlayer(Game& aGame);

/**
 * @brief Plays one turn: checks the move, then moves the piece, captures and switches the player.
 *
 * If checkMovement() refuses the move, nothing changes (same board, same current player).
 * Otherwise: movePiece(), capturePieces(), then switchCurrentPlayer().
 *
 * @param aGame Current game state.
 * @param aMove The move entered by the current player.
 * @return The result of checkMovement(): MOVE_OK if the turn has been played.
 */
MoveStatus playTurn(Game& aGame, const Move& aMove);

// ============================================================================
// SECTION 4: ENDGAME & VICTORY
// ============================================================================

/**
 * @brief Checks if any SWORD pieces remain on the board.
 *
 * Scans entire board for at least one SWORD piece.
 *
 * @param aBoard The game board to check.
 * @return `true` if at least one SWORD exists, `false` if none remain.
 */
bool isSwordLeft(const Board& aBoard);

/**
 * @brief Gets the king's position on the board.
 *
 * Scans board to locate the KING piece.
 *
 * @param aBoard The game board to scan.
 * @return Position of the king, or {-1, -1} if not found.
 */
Position getKingPosition(const Board& aBoard);

/**
 * @brief Checks if the king has escaped to a fortress.
 *
 * Victory condition for DEFENSE: king reaches any corner FORTRESS cell.
 *
 * @param aBoard The game board to check.
 * @return `true` if king is on a FORTRESS cell, `false` otherwise.
 */
bool isKingEscaped(const Board& aBoard);

/**
 * @brief Checks if the king is captured (simple version - 4 directions).
 *
 * King captured if all 4 adjacent cells (up/down/left/right) are hostile:
 * out of bounds, SWORD pieces, FORTRESS, or CASTLE cells.
 *
 * @param aBoard The game board to check.
 * @return `true` if king surrounded on all 4 sides, `false` otherwise.
 * @note Simpler version. Checks only immediate neighbors.
 */
bool isKingCapturedSimple(const Board& aBoard);

/**
 * @brief Checks if the king is captured (recursive version: the king and its group of shields).
 *
 * The group made of the KING and of the SHIELDs connected to it (orthogonally, step by step)
 * is captured if it touches no empty NORMAL cell: all the neighbours of the group are
 * SWORD pieces, borders, FORTRESS or CASTLE cells.
 *
 * @param aBoard The game board to check.
 * @return `true` if the group of the king is completely surrounded, `false` otherwise
 *         (or if there is no king on the board).
 * @note Optional (bonus): write a recursive helper function that explores the group,
 *       remembering the cells already visited.
 */
bool isKingCapturedRecursive(const Board& aBoard);

/**
 * @brief Checks if the current player (the one who must play now) can move at least one of his pieces.
 *
 * A piece can move if one of its 4 neighbouring cells is a cell where it may stop:
 * an empty NORMAL cell, or (for the KING only) an empty FORTRESS or CASTLE.
 * Only the pieces of the current player are considered (SWORD for ATTACK, SHIELD and KING for DEFENSE).
 *
 * @param aGame The current game state.
 * @return `true` if at least one move exists, `false` if the current player is blocked.
 */
bool canCurrentPlayerMove(const Game& aGame);

/**
 * @brief Checks if the game has ended.
 *
 * Game ends when: king escaped to FORTRESS, king captured, no SWORD pieces left,
 * or the current player (the one who must play now) cannot move any of his pieces
 * (see canCurrentPlayerMove).
 *
 * @param aGame The current game state.
 * @return `true` if game finished, `false` otherwise.
 */
bool isGameFinished(const Game& aGame);

/**
 * @brief Determines the winner of the game.
 *
 * Returns winner if game finished:
 * - DEFENSE wins if king escaped or no swords left
 * - ATTACK wins if king captured
 * - if the current player cannot move any piece, the other player wins
 *
 * @param aGame The current game state.
 * @return Pointer to winning Player, or `nullptr` if game not finished.
 * @note Call after `isGameFinished()` returns true.
 */
const Player* whoWon(const Game& aGame);

// ============================================================================
// DEVELOPMENT HELPER (defined in tests.cpp)
// ============================================================================

/**
 * @brief Marks a function as "not implemented yet".
 *
 * Placed at the beginning of each empty function provided in the student version.
 * The test suite then reports the function as "not implemented" instead of
 * counting its tests as passed or failed.
 *
 * @param aFunctionName Name of the function (use `__func__`).
 * @note Remove this call as soon as you start writing the function.
 */
void markNotImplemented(const char* aFunctionName);

#endif // FUNCTIONS_H
