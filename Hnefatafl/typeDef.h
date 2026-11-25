/**
 * @file typeDef.h
 *
 * @brief This file defines various enums and structures used in the game.
 *
 * @author JMB - IUT Informatique La Rochelle
 * @date 14/10/2026
 */

#ifndef TYPEDEF_H
#define TYPEDEF_H

#include <string>


/**
 * @enum BoardSize
 * @brief Represents possible board sizes for the game.
 *
 * The two board sizes available are:
 * - `LITTLE`: 11x11 board.
 * - `BIG`: 13x13 board.
 *
 * The underlying type is fixed (int) so that any integer value can be stored
 * (e.g. an invalid size given to createBoard in the tests).
 */
enum BoardSize : int
{
    LITTLE = 11,  /**< Represents the smaller board size (11x11). */
    BIG = 13      /**< Represents the larger board size (13x13). */
};

/**
 * @enum PlayerRole
 * @brief Represents the roles of the players in the game.
 *
 * The two roles available are:
 * - `ATTACK`: Represents the attacker.
 * - `DEFENSE`: Represents the defender.
 */
enum PlayerRole
{
    ATTACK,   /**< Represents the role of the attacker. */
    DEFENSE   /**< Represents the role of the defender. */
};

/**
 * @enum CellType
 * @brief Represents the types of cells on the game board.
 *
 * - `NORMAL`: A regular cell without any special properties.
 * - `FORTRESS`: A cell that acts as a fortress.
 * - `CASTLE`: A cell that acts as a castle.
 */
enum CellType
{
    NORMAL,    /**< Represents a normal game board cell. */
    FORTRESS,  /**< Represents a fortress cell. */
    CASTLE     /**< Represents a castle cell. */
};

/**
 * @enum PieceType
 * @brief Represents the types of pieces that can occupy a cell.
 *
 * - `NONE`: Indicates no piece is present.
 * - `SHIELD`: Represents a shield piece.
 * - `SWORD`: Represents an attacker (sword) piece.
 * - `KING`: Represents the king piece.
 */
enum PieceType
{
    NONE,    /**< Represents an empty cell (no piece). */
    SHIELD,  /**< Represents a shield piece. */
    SWORD,   /**< Represents a sword piece. */
    KING     /**< Represents the king piece. */
};

/**
 * @enum MoveStatus
 * @brief Result of the validation of a move (see checkMovement and playTurn).
 *
 * MOVE_OK means the move is valid; the other values give the reason of the refusal,
 * so that playGame() can display an explicit message.
 */
enum MoveStatus
{
    MOVE_OK,              /**< The move is valid. */
    MOVE_OUT_OF_BOARD,    /**< Start or end position outside the board. */
    MOVE_SAME_CELL,       /**< Start and end positions are identical. */
    MOVE_NOT_YOUR_PIECE,  /**< The start cell does not contain a piece of the current player. */
    MOVE_NOT_STRAIGHT,    /**< The move is neither horizontal nor vertical. */
    MOVE_PATH_BLOCKED,    /**< A piece is on the path or on the end cell. */
    MOVE_FORBIDDEN_CELL   /**< The path crosses, or ends on, a cell forbidden to this piece (fortress, castle). */
};

/**
 * @struct Cell
 * @brief Structure to represent the state of a single cell on the board.
 *
 * Each cell has a type (`itsCellType`) and a piece (`itsPieceType`),
 * which can be empty or occupied by a specific piece.
 */
struct Cell
{
    CellType itsCellType;   /**< The type of the cell (e.g., NORMAL, FORTRESS, CASTLE). */
    PieceType itsPieceType; /**< The type of piece occupying the cell (e.g., NONE, SHIELD, SWORD, KING). */
};

/**
 * @struct Board
 * @brief Structure representing the game board as a 2D grid of `Cell` structures.
 *
 * The board contains a set of cells arranged in a grid with a size defined by `itsSize`.
 */
struct Board
{
    Cell** itsCells = nullptr;  /**< 2D array representing the cells on the board. */
    BoardSize itsSize = LITTLE; /**< The size of the board (LITTLE or BIG). */
};

/**
 * @struct Position
 * @brief Structure to represent the coordinates of a cell on the board.
 *
 * Each position is defined by a row (`itsRow`) and a column (`itsCol`) within the board grid.
 */
struct Position
{
    int itsRow;  /**< The row (vertical position) of a cell. */
    int itsCol;  /**< The column (horizontal position) of a cell. */
};

/**
 * @struct Move
 * @brief Structure to represent a move made by a player.
 *
 * A move is defined by the starting position (`itsStartPosition`)
 * and the ending position (`itsEndPosition`).
 */
struct Move
{
    Position itsStartPosition; /**< The starting position of the move. */
    Position itsEndPosition;   /**< The ending position of the move. */
};

/**
 * @struct Player
 * @brief Structure representing a player in the game.
 *
 * Each player has a name (`itsName`) and a role (`itsRole`),
 * which can either be ATTACK or DEFENSE.
 */
struct Player
{
    std::string itsName;    /**< The name of the player. */
    PlayerRole itsRole;     /**< The role of the player (ATTACK or DEFENSE). */
};

/**
 * @struct Game
 * @brief Structure representing the state of the game.
 *
 * The game consists of a `Board` and two players (`itsPlayer1` and `itsPlayer2`),
 * with a pointer to the current player (`itsCurrentPlayer`).
 *
 * @warning Always pass a Game by reference (Game& or const Game&), never by value:
 * a copy of a Game keeps `itsCurrentPlayer` pointing to the players of the ORIGINAL game,
 * not to the players of the copy.
 */
struct Game
{
    Board itsBoard;             /**< The game board. */
    Player itsPlayer1 = {"Player 1", ATTACK}; /**< The first player (attacker). */
    Player itsPlayer2 = {"Player 2", DEFENSE}; /**< The second player (defender). */
    Player* itsCurrentPlayer = &itsPlayer1; /**< A pointer to the current player. */
};

#endif // TYPEDEF_H
