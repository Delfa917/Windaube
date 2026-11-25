/**
 * @file tests.cpp
 *
 * @brief Implementation of test functions for testing the functions implemented in 'functions.cpp'.
 *
 * ============================= TEST SUITE DOCUMENTATION =============================
 *
 * This file aggregates all unit tests for the Hnefatafl project. Tests are grouped
 * by function and aim to cover both happy paths and edge cases.
 *
 * Display options (settings SHOW_... at the beginning of main.cpp, given to configureTestDisplay()):
 *   - DISPLAY_BOARDS : show the boards used by the tests (drawn by the test suite itself,
 *                      so it works even if displayBoard() is not written yet).
 *   - DISPLAY_PROMPTS: show or mute the messages printed by the tested functions (shown with
 *                      the result of their test; for a passed test only if DISPLAY_PASSED is true).
 *   - DISPLAY_PASSED : list the passed tests (true) or only the failed ones (false).
 *
 * Results:
 *   - PASS : the test succeeds.
 *   - FAIL : the value obtained (Got) differs from the expected value (Expected),
 *            or the test crashed / timed out (a hint about the cause is displayed).
 *   - NOT IMPLEMENTED : the tested function (or a function it uses) still contains
 *            markNotImplemented(__func__): its tests are skipped, neither passed nor failed.
 *   - At the end, a table summarizes the status of every function.
 *
 * Test isolation and protection (POSIX systems only: macOS, Linux, WSL):
 *   - Process isolation: each test runs in a separate subprocess (fork) to survive crashes
 *   - Crash protection: segfaults and other fatal signals are caught and reported
 *   - Infinite loop protection: tests time out after TEST_TIMEOUT_SECONDS (3 seconds)
 *   - Windows (native): only C++ exceptions are caught (use WSL for full protection)
 *
 * Helpers used in this file only:
 *   - TestBoard: board owned by the tests (freed automatically)
 *   - resetBoard(board, size): sets all cells to {NORMAL, NONE}
 *   - printBoard(board, size, title, highlights): board display used by the tests
 *   - printBoards(views): several boards side by side (e.g. before / after), if the terminal is wide enough
 *
 * Capture logic semantics used by tests:
 *   - Simple capture: only the 4 orthogonal neighbors of the KING are considered. They are
 *     hostile if they are SWORD, border, FORTRESS or CASTLE. SHIELD is not hostile.
 *   - Recursive capture: considers the connected region of KING+SHIELD. If the region touches
 *     an empty NORMAL cell, the KING is NOT captured; otherwise it is captured.
 * ====================================================================================
 *
 * @author JMB - IUT Informatique La Rochelle
 * @date 14/10/2026
 */

#include <iostream>
#include <sstream>
#include <functional>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

// System headers BEFORE "using namespace std": otherwise, on Windows, std::byte (C++17)
// and the byte type of windows.h are ambiguous ("reference to 'byte' is ambiguous").
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX   // already defined by MinGW: avoids the "NOMINMAX redefined" warning
#endif
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

using namespace std;

#include "typeDef.h"
#include "functions.h"
#include "view.h"
#include "tests.h"

// ========================= GLOBAL VARIABLES =========================
// Display options (set by configureTestDisplay)
bool DISPLAY_BOARDS = false;
bool DISPLAY_PROMPTS = false;
bool DISPLAY_PASSED = true;

// Global counters for aggregating test results across all test functions
static int G_TOTAL_PASSED = 0;
static int G_TOTAL_FAILED = 0;
static int G_TOTAL_SKIPPED = 0;

// ========================= FORWARD DECLARATIONS =========================
// Helper functions defined at the end of this file
void printTestHeader(const string& testName);
void printTestResult(int testNum, const string& description, bool passed,
                     const string& expected = "", const string& actual = "");
void printTestException(int testNum, const string& description, const string& exceptionMsg);
void printTestSummary(const string& testName, int passed, int failed);
void resetBoard(Cell**& aBoard, const BoardSize& aBoardSize);
/** @brief One board to display: its cells, size, title and highlighted cells (cells checked by the test). */
struct BoardView {
    Cell** itsCells;
    int itsSize;
    string itsTitle;
    vector<Position> itsHighlights;
};
void printBoard(Cell** aBoard, int aSize, const string& title = "", const vector<Position>& highlights = {});
void printBoards(const vector<BoardView>& views);
string cellName(int row, int col);
string pieceName(PieceType piece);

// ========================================================================================
// ==================== MEMORY CHECKS: new[] AND delete[] =================================
// ========================================================================================
// The test program replaces the array allocation functions (operator new[] / delete[]):
//   - the memory given by new[] is filled with a "garbage" value: a forgotten initialization
//     (e.g. createBoard that does not initialize its cells) is visible on every computer;
//   - the arrays still allocated are counted: a forgotten delete[] (memory leak) is visible.
// ========================================================================================

static const unsigned char GARBAGE_BYTE = 0xA5;
static long G_LIVE_ARRAYS = 0;   // Number of arrays allocated with new[] and not freed yet

void* operator new[](std::size_t size) {
    void* memory = std::malloc(size > 0 ? size : 1);
    if (memory == nullptr) throw std::bad_alloc();
    std::memset(memory, GARBAGE_BYTE, size);
    G_LIVE_ARRAYS++;
    return memory;
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    void* memory = std::malloc(size > 0 ? size : 1);
    if (memory != nullptr) {
        std::memset(memory, GARBAGE_BYTE, size);
        G_LIVE_ARRAYS++;
    }
    return memory;
}

void operator delete[](void* memory) noexcept {
    if (memory != nullptr) {
        G_LIVE_ARRAYS--;
        std::free(memory);
    }
}

void operator delete[](void* memory, std::size_t) noexcept {
    operator delete[](memory);
}

void operator delete[](void* memory, const std::nothrow_t&) noexcept {
    operator delete[](memory);
}

// ========================================================================================
// ==================== "NOT IMPLEMENTED" DETECTION =======================================
// ========================================================================================
// Each empty function of the student version starts with markNotImplemented(__func__).
// When a test calls such a function, the test is reported as "not implemented" (skipped)
// instead of passed/failed. The name of the function is transmitted from the test
// subprocess to the parent process through the exit code (NOT_IMPLEMENTED_EXIT_BASE + index).
// ========================================================================================

static const char* const KNOWN_FUNCTIONS[] = {
    "displayHnefataflLogo", "displayBoard",
    "moveStatusMessage", "readLine", "askBoardSize", "askPosition", "askMove",
    "stringToBoardSize", "createBoard", "deleteBoard", "initializeBoard",
    "isValidPosition", "stringToPosition", "isEmptyCell",
    "checkMovement", "movePiece", "capturePieces", "switchCurrentPlayer", "playTurn",
    "isSwordLeft", "getKingPosition", "isKingEscaped", "isKingCapturedSimple",
    "isKingCapturedRecursive", "canCurrentPlayerMove", "isGameFinished", "whoWon", "playGame"
};
static const int KNOWN_FUNCTIONS_COUNT = sizeof(KNOWN_FUNCTIONS) / sizeof(KNOWN_FUNCTIONS[0]);
static const int NOT_IMPLEMENTED_EXIT_BASE = 100;

static bool G_TEST_RUNNING = false;          // true while a test body is executed
static int G_NOT_IMPLEMENTED_INDEX = -1;      // first not implemented function met by the current test
static bool G_SECTION_MISSING[KNOWN_FUNCTIONS_COUNT] = {};  // not implemented functions met in the current section
static int G_SECTION_SKIPPED = 0;             // number of skipped tests in the current section
static bool G_WARNED[KNOWN_FUNCTIONS_COUNT] = {};           // warnings already displayed (game mode)
static string G_SECTION_NAME;                 // name of the function tested by the current section
static int G_FAILED_BEFORE_TEST = 0;          // failed tests of the section before the current one

/**
 * @brief Tells whether the boards of a test must be displayed.
 * - DISPLAY_BOARDS true: boards of every displayed test (the passed tests are displayed
 *   only if DISPLAY_PASSED is true);
 * - DISPLAY_BOARDS false: boards of the first failed test of each function only
 *   (to help without flooding the console).
 * @param success Result of the test
 */
static bool shouldShowBoards(bool success) {
    if (DISPLAY_BOARDS) {
        return !success || DISPLAY_PASSED;
    }
    return !success && G_FAILED_BEFORE_TEST == 0;
}

void markNotImplemented(const char* aFunctionName) {
    int index = KNOWN_FUNCTIONS_COUNT - 1;
    for (int i = 0; i < KNOWN_FUNCTIONS_COUNT; ++i) {
        if (string(KNOWN_FUNCTIONS[i]) == aFunctionName) {
            index = i;
        }
    }

    if (G_TEST_RUNNING) {
        // Inside a test: remember the first missing function,
        // unless the tested function itself is missing (more useful information)
        if (G_NOT_IMPLEMENTED_INDEX == -1 || G_SECTION_NAME == aFunctionName) {
            G_NOT_IMPLEMENTED_INDEX = index;
        }
    } else if (!G_WARNED[index]) {
        // Inside the game: warn once per function
        G_WARNED[index] = true;
        cerr << COLOR_YELLOW << "⚠ La fonction " << aFunctionName
             << "() n'est pas encore écrite." << COLOR_RESET << endl;
    }
}

/**
 * @brief Records that a test has been skipped because a function is not implemented.
 * @param functionIndex Index of the missing function in KNOWN_FUNCTIONS.
 */
static void recordSkippedTest(int functionIndex) {
    G_SECTION_SKIPPED++;
    if (functionIndex >= 0 && functionIndex < KNOWN_FUNCTIONS_COUNT) {
        G_SECTION_MISSING[functionIndex] = true;
    }
}

// ========================================================================================
// ==================== OUTPUT CONTROL ====================================================
// ========================================================================================
// When DISPLAY_PROMPTS is false, everything written on cout by the tested functions is
// discarded. The test suite writes its own messages on the real console through
// TestOutput, which temporarily restores the real cout buffer.
// ========================================================================================

/** @brief Stream buffer that discards everything written into it. */
class NullBuffer : public streambuf {
protected:
    int overflow(int c) override { return c; }
};

static streambuf* G_REAL_COUT_BUF = nullptr;

// Messages printed by the tested functions (DISPLAY_PROMPTS): kept during the test, then displayed
// with the result of the test (not displayed for a passed test if DISPLAY_PASSED is false).
// A fixed-size array is used so that the messages can still be written if the test crashes.
static char G_PROMPT_BUFFER[1 << 16];
static size_t G_PROMPT_LENGTH = 0;
static bool G_PROMPT_TRUNCATED = false;

/** @brief Stream buffer that keeps the messages of the tested functions in G_PROMPT_BUFFER. */
class PromptBuffer : public streambuf {
protected:
    int overflow(int c) override {
        if (c != EOF) {
            if (G_PROMPT_LENGTH < sizeof(G_PROMPT_BUFFER)) {
                G_PROMPT_BUFFER[G_PROMPT_LENGTH++] = static_cast<char>(c);
            } else {
                G_PROMPT_TRUNCATED = true;
            }
        }
        return c;
    }
};

/**
 * @brief Displays (or discards) the messages printed by the tested functions during the current test.
 * @param show true to display them
 */
static void flushPrompts(bool show) {
    if (show && G_PROMPT_LENGTH > 0) {
        streambuf* saved = cout.rdbuf();
        if (G_REAL_COUT_BUF != nullptr) cout.rdbuf(G_REAL_COUT_BUF);
        cout.write(G_PROMPT_BUFFER, static_cast<streamsize>(G_PROMPT_LENGTH));
        if (G_PROMPT_BUFFER[G_PROMPT_LENGTH - 1] != '\n') cout << endl;
        if (G_PROMPT_TRUNCATED) cout << "(... messages truncated)" << endl;
        cout.flush();
        cout.rdbuf(saved);
    }
    G_PROMPT_LENGTH = 0;
    G_PROMPT_TRUNCATED = false;
}

/** @brief RAII helper: writes on the real console during its lifetime, even if cout is muted. */
class TestOutput {
public:
    TestOutput() : itsSavedBuf(cout.rdbuf()) {
        if (G_REAL_COUT_BUF != nullptr) cout.rdbuf(G_REAL_COUT_BUF);
    }
    ~TestOutput() {
        cout.flush();
        cout.rdbuf(itsSavedBuf);
    }
private:
    streambuf* itsSavedBuf;
};

/**
 * @brief Runs a test body: mutes the tested functions if needed and detects missing functions.
 * @param fn The test body (returns 0 if passed, 1 if failed).
 * @return 0 (passed), 1 (failed) or NOT_IMPLEMENTED_EXIT_BASE + index (not implemented).
 */
static int runTestBody(const function<int()>& fn) {
    static NullBuffer nullBuffer;
    static PromptBuffer promptBuffer;
    if (G_REAL_COUT_BUF == nullptr) G_REAL_COUT_BUF = cout.rdbuf();

    // The messages of the tested functions are discarded, or kept until the result is known
    streambuf* savedBuf = cout.rdbuf();
    G_PROMPT_LENGTH = 0;
    G_PROMPT_TRUNCATED = false;
    if (DISPLAY_PROMPTS) cout.rdbuf(&promptBuffer);
    else cout.rdbuf(&nullBuffer);

    G_TEST_RUNNING = true;
    G_NOT_IMPLEMENTED_INDEX = -1;
    int result = 1;
    try {
        result = fn();
    } catch (...) {
        G_TEST_RUNNING = false;
        cout.rdbuf(savedBuf);
        flushPrompts(DISPLAY_PROMPTS);   // Messages printed before the exception
        throw;
    }
    G_TEST_RUNNING = false;
    cout.rdbuf(savedBuf);
    flushPrompts(false);   // Already displayed (or discarded) with the result of the test

    if (G_NOT_IMPLEMENTED_INDEX != -1) {
        return NOT_IMPLEMENTED_EXIT_BASE + G_NOT_IMPLEMENTED_INDEX;
    }
    return result;
}

// ========================================================================================
// ==================== PLATFORM DETECTION & TEST ISOLATION SYSTEM ========================
// ========================================================================================
// This section handles platform-specific differences in test crash protection:
//   - Windows: Detection system informing users about WSL options for full protection
//   - POSIX (macOS/Linux): fork()-based subprocess isolation for complete crash handling
//
// The isolation system ensures that if a test causes a system crash (segfault, etc.),
// the test suite continues executing remaining tests. On Windows without WSL, only C++
// exceptions are caught; system crashes will terminate the program.
// ========================================================================================

/**
 * @brief Interprets the result code of a test and updates the counters.
 */
static void handleTestResult(int code, int& pass, int& failed) {
    if (code == 0) {
        pass++;
    } else if (code >= NOT_IMPLEMENTED_EXIT_BASE && code < NOT_IMPLEMENTED_EXIT_BASE + KNOWN_FUNCTIONS_COUNT) {
        recordSkippedTest(code - NOT_IMPLEMENTED_EXIT_BASE);
    } else {
        failed++;
    }
}

// ========================= WINDOWS ENVIRONMENT CHECK =========================
#ifdef _WIN32
#include <cstdio>
#include <limits>

/**
 * @brief Check if WSL is available on Windows system.
 * @return true if WSL is installed and accessible, false otherwise.
 */
static bool isWSLAvailable() {
    FILE* pipe = _popen("wsl --status 2>NUL", "r");
    if (!pipe) return false;

    char buffer[128];
    bool hasOutput = (fgets(buffer, 128, pipe) != nullptr);
    _pclose(pipe);

    return hasOutput;
}

/**
 * @brief Display warning message for Windows users about limited crash protection.
 * Informs users about the options to get better protection via WSL.
 */
static void displayWindowsEnvironmentInfo() {
    cout << "\n";
    cout << COLOR_YELLOW << "╔════════════════════════════════════════════════════════════╗" << COLOR_RESET << "\n";
    cout << COLOR_YELLOW << "║  ⚠️  Windows détecté : Protection limitée des tests       ║" << COLOR_RESET << "\n";
    cout << COLOR_YELLOW << "╚════════════════════════════════════════════════════════════╝" << COLOR_RESET << "\n";
    cout << "\n";

    cout << "🛡️  " << COLOR_CYAN << "Protection actuelle :" << COLOR_RESET << "\n";
    cout << "   ✅ Exceptions C++ (throw/catch)\n";
    cout << "   ❌ Crashs système (segfault, division par zéro, etc.)\n";
    cout << "   ❌ Boucles infinies (pas de limite de temps)\n";
    cout << "\n";

    cout << "💥 " << COLOR_RED << "Si un test cause un crash système :" << COLOR_RESET << "\n";
    cout << "   → La suite de tests s'arrêtera brutalement\n";
    cout << "   → Les tests suivants ne seront pas exécutés\n";
    cout << "\n";

    cout << COLOR_GREEN << "✅ POUR UNE PROTECTION COMPLÈTE : utiliser WSL (voir README-WINDOWS.md)" << COLOR_RESET << "\n";
    cout << "\n";
    cout << COLOR_CYAN << "Option 1 - Terminal WSL (Recommandé, le plus simple)" << COLOR_RESET << "\n";
    cout << "  1. Installer WSL : " << COLOR_YELLOW << "wsl --install" << COLOR_RESET << " (PowerShell Admin), puis redémarrer\n";
    cout << "  2. Dans Ubuntu : " << COLOR_YELLOW << "sudo apt install build-essential" << COLOR_RESET << "\n";
    cout << "  3. Développer dans Qt Creator Windows (normal)\n";
    cout << "  4. Tester dans WSL :\n";
    cout << "     " << COLOR_YELLOW << "cd /mnt/c/Users/VotreNom/Documents/MonProjet/Hnefatafl" << COLOR_RESET << "\n";
    cout << "     " << COLOR_YELLOW << "g++ -std=c++17 -o Hnefatafl *.cpp && ./Hnefatafl" << COLOR_RESET << "\n";
    cout << "\n";
    cout << COLOR_CYAN << "Option 2 - Qt Creator dans WSL (Windows 11, ou Windows 10 avec WSL à jour)" << COLOR_RESET << "\n";
    cout << "  Dans Ubuntu : " << COLOR_YELLOW << "sudo apt install qtcreator build-essential qmake6" << COLOR_RESET
         << " puis " << COLOR_YELLOW << "qtcreator &" << COLOR_RESET << "\n";
    cout << "\n";

    cout << "📚 Documentation : " << COLOR_CYAN << "https://aka.ms/wsl" << COLOR_RESET << "\n";
    cout << "\n";
    cout << COLOR_YELLOW << "Appuyez sur ENTRÉE pour continuer avec la protection limitée Windows..." << COLOR_RESET << "\n";
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

/**
 * @brief Check Windows environment and display appropriate message.
 * Called at the start of the test suite to inform Windows users about their options.
 */
void checkWindowsEnvironment() {
    if (!isWSLAvailable()) {
        displayWindowsEnvironmentInfo();
    } else {
        cout << "\n";
        cout << COLOR_GREEN << "ℹ️  WSL détecté sur votre système Windows !" << COLOR_RESET << "\n";
        cout << "   Pour bénéficier de la protection complète des tests (plantages, boucles infinies) :\n";
        cout << "   • Compilez et exécutez dans WSL : " << COLOR_CYAN
             << "cd /mnt/c/.../Hnefatafl && g++ -std=c++17 -o Hnefatafl *.cpp && ./Hnefatafl" << COLOR_RESET << "\n";
        cout << "   • Ou lancez Qt Creator depuis WSL (voir README-WINDOWS.md)\n";
        cout << "\n";
    }
}
#else
/**
 * @brief No-op function for non-Windows systems (macOS/Linux).
 * These systems have native fork() support, so no warning is needed.
 */
void checkWindowsEnvironment() {
    // fork() protection natively available on macOS/Linux: nothing to display
}
#endif

// ========================= POSIX SUBPROCESS ISOLATION =========================
// On POSIX systems (macOS/Linux/WSL), use fork/waitpid to isolate crashes and timeouts.
// If a test causes a segfault, infinite loop, or other issues, the parent process
// detects it and continues running remaining tests.
//
// Protection features:
//   - Crash isolation: SIGSEGV, SIGFPE, etc. are caught and reported with a hint
//   - Infinite loop protection: tests are terminated after TEST_TIMEOUT_SECONDS
//   - Exception handling: C++ exceptions are caught and reported
//   - Memory safety: child process memory leaks don't affect parent
//
// On Windows (native), only C++ exceptions are caught via try/catch.
// ==============================================================================
#if defined(__unix__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

// Global timeout configuration for test execution
static const int TEST_TIMEOUT_SECONDS = 3; // Timeout per test in seconds
static const int TIMEOUT_EXIT_CODE = 253;

/**
 * @brief Writes the kept messages of the tested functions (async-signal-safe: only write()).
 * Called when the test crashes or times out, so that the messages printed before stay visible.
 */
static void writePromptsOnFailure() {
    if (DISPLAY_PROMPTS && G_PROMPT_LENGTH > 0) {
        ssize_t written = write(STDOUT_FILENO, G_PROMPT_BUFFER, G_PROMPT_LENGTH);
        (void)written;
        if (G_PROMPT_BUFFER[G_PROMPT_LENGTH - 1] != '\n') {
            written = write(STDOUT_FILENO, "\n", 1);
        }
    }
}

// Signal handler for test timeout in child process
static void timeout_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    writePromptsOnFailure();
    _exit(TIMEOUT_EXIT_CODE);
}

// Signal handler for a crash in child process: shows the kept messages, then crashes normally
static void crash_handler(int sig) {
    writePromptsOnFailure();
    signal(sig, SIG_DFL);
    raise(sig);
}

/**
 * @brief Explains a fatal signal in simple words.
 * @param sig The signal number.
 * @return A short explanation with a hint about the probable cause.
 */
static string explainSignal(int sig) {
    string msg = "crash, signal " + to_string(sig);
    switch (sig) {
        case SIGSEGV: return msg + " (SIGSEGV): invalid memory access - index out of the board, nullptr or deleted pointer?";
        case SIGBUS:  return msg + " (SIGBUS): invalid memory access - nullptr or deleted pointer?";
        case SIGFPE:  return msg + " (SIGFPE): arithmetic error - division by zero?";
        case SIGABRT: return msg + " (SIGABRT): program aborted - memory freed twice, or uncaught exception?";
        default:      return msg;
    }
}

/**
 * @brief Run a test block in an isolated subprocess with timeout protection.
 *        If the child crashes (e.g., SIGSEGV) or hangs (infinite loop),
 *        the parent marks the test as failed without crashing the whole test suite.
 *
 * Contract:
 *  - Inputs: test body (returns 0 if passed, 1 if failed), test number, description, counters.
 *  - Output: increments pass/failed (or the skipped counter if a function is not implemented)
 *            and prints a clear message on crash or timeout.
 *  - Timeout: each test is limited to TEST_TIMEOUT_SECONDS.
 */
static void runIsolated(const function<int()>& fn, int testNum, const string& description, int& pass, int& failed) {
    cout.flush(); // Avoid duplicating pending output in the child process
    G_FAILED_BEFORE_TEST = failed;

    pid_t pid = fork();
    if (pid == -1) {
        // Fork failed; fallback to direct execution with try/catch (no timeout protection)
        try {
            handleTestResult(runTestBody(fn), pass, failed);
        } catch (const exception& e) {
            printTestException(testNum, description, string("exception: ") + e.what());
            failed++;
        } catch (...) {
            printTestException(testNum, description, "unknown exception");
            failed++;
        }
        return;
    }
    if (pid == 0) {
        // Child: setup timeout alarm and execute the test body
        signal(SIGALRM, timeout_handler);
        signal(SIGSEGV, crash_handler);
        signal(SIGBUS, crash_handler);
        signal(SIGFPE, crash_handler);
        signal(SIGABRT, crash_handler);
        alarm(TEST_TIMEOUT_SECONDS);

        int result = 1;
        try {
            result = runTestBody(fn);
        } catch (const exception& e) {
            printTestException(testNum, description, string("exception: ") + e.what());
            result = 1;
        } catch (...) {
            printTestException(testNum, description, "unknown exception");
            result = 1;
        }
        alarm(0);
        cout.flush();
        _exit(result);
    }

    // Parent: wait for child with potential timeout detection
    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        printTestException(testNum, description, "waitpid failed");
        failed++;
        return;
    }
    if (WIFSIGNALED(status)) {
        printTestException(testNum, description, explainSignal(WTERMSIG(status)));
        failed++;
    } else if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        if (code == TIMEOUT_EXIT_CODE) {
            printTestException(testNum, description, "TIMEOUT after " + to_string(TEST_TIMEOUT_SECONDS)
                               + " s: infinite loop, or waiting for a keyboard input?");
            failed++;
        } else {
            handleTestResult(code, pass, failed);
        }
    }
}
#else
// Non-POSIX fallback: directly run with try/catch (cannot catch segfaults, but keeps portability).
static void runIsolated(const function<int()>& fn, int testNum, const string& description, int& pass, int& failed) {
    G_FAILED_BEFORE_TEST = failed;
    try {
        handleTestResult(runTestBody(fn), pass, failed);
    } catch (const exception& e) {
        printTestException(testNum, description, string("exception: ") + e.what());
        failed++;
    } catch (...) {
        printTestException(testNum, description, "unknown exception");
        failed++;
    }
}
#endif

// ========================================================================================
// ==================== BOARDS USED BY THE TESTS ==========================================
// ========================================================================================

/**
 * @brief Board owned by the test suite, independent from createBoard() and deleteBoard().
 *
 * The cells are stored in a vector and freed automatically at the end of the scope.
 * cells() gives the Cell** view expected by the Board structure.
 */
class TestBoard {
public:
    explicit TestBoard(int aSize)
        : itsSize(aSize), itsStorage(static_cast<size_t>(aSize * aSize), Cell{NORMAL, NONE}),
          itsRows(static_cast<size_t>(aSize)) {
        for (int r = 0; r < aSize; ++r) {
            itsRows[r] = &itsStorage[static_cast<size_t>(r * aSize)];
        }
    }
    TestBoard(const TestBoard&) = delete;
    TestBoard& operator=(const TestBoard&) = delete;

    /** @brief The cells, as a Cell** usable in a Board. */
    Cell** cells() { return itsRows.data(); }

    /** @brief A Board structure using these cells. */
    Board board() { return {cells(), static_cast<BoardSize>(itsSize)}; }

private:
    int itsSize;
    vector<Cell> itsStorage;
    vector<Cell*> itsRows;
};

// ========================================================================================
// ==================== TEST DATA: DECODING AND PARSING ===================================
// ========================================================================================
// The test cases (boards, moves, expected results) are stored in tests_data.inc as an
// encoded byte array. Each line of the decoded text is either "[functionName]" (start of
// the test cases of a function) or a test case whose fields are separated by '|'.
// ========================================================================================

#include "tests_data.inc"

/** @brief One test case: its fields, already trimmed. */
typedef vector<string> TestRow;

/**
 * @brief Decodes the test data and checks its integrity.
 * @return The decoded text, or an empty string if the data is corrupted.
 */
static string decodeTestData() {
    const size_t size = sizeof(TEST_DATA);
    string text(size, ' ');
    uint32_t state = 0x5AE10101u;
    uint32_t checksum = 0x811C9DC5u;
    for (size_t i = 0; i < size; ++i) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        unsigned char c = static_cast<unsigned char>(TEST_DATA[i] ^ (state & 0xFFu));
        text[i] = static_cast<char>(c);
        checksum = (checksum ^ c) * 0x01000193u;
    }
    if (checksum != TEST_DATA_CHECKSUM) {
        return "";
    }
    return text;
}

/** @brief Removes the spaces at the beginning and at the end of a string. */
static string trim(const string& s) {
    size_t first = s.find_first_not_of(" \t\r");
    if (first == string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r");
    return s.substr(first, last - first + 1);
}

/** @brief Splits a string on a separator and trims each part. */
static vector<string> split(const string& s, char separator) {
    vector<string> parts;
    string current;
    istringstream stream(s);
    while (getline(stream, current, separator)) {
        parts.push_back(trim(current));
    }
    return parts;
}

/**
 * @brief Returns the test cases of a function.
 * @param section Name of the function (section "[name]" of the test data)
 * @return The test cases (empty if the section does not exist or the data is corrupted)
 */
static const vector<TestRow>& testRows(const string& section) {
    static vector<string> names;
    static vector<vector<TestRow>> sections;
    static bool isLoaded = false;
    static const vector<TestRow> empty;

    if (!isLoaded) {
        isLoaded = true;
        istringstream lines(decodeTestData());
        string line;
        while (getline(lines, line)) {
            line = trim(line);
            if (line.empty()) continue;
            if (line[0] == '[' && line[line.size() - 1] == ']') {
                names.push_back(line.substr(1, line.size() - 2));
                sections.push_back(vector<TestRow>());
            } else if (!sections.empty()) {
                sections.back().push_back(split(line, '|'));
            }
        }
    }
    for (size_t i = 0; i < names.size(); ++i) {
        if (names[i] == section) return sections[i];
    }
    TestOutput output;
    cout << COLOR_RED << "  ✗ Test data not found for " << section
         << " (tests_data.inc missing or corrupted: download it again)" << COLOR_RESET << endl;
    return empty;
}

/**
 * @brief Converts a cell name in game notation ("F6") into a position (row 5, column 5).
 * @return The position, or {-1, -1} if the name is malformed.
 */
static Position parseCell(const string& name) {
    if (name.size() < 2 || name[0] < 'A' || name[0] > 'Z') return {-1, -1};
    int col = 0;
    for (size_t i = 1; i < name.size(); ++i) {
        if (name[i] < '0' || name[i] > '9') return {-1, -1};
        col = col * 10 + (name[i] - '0');
    }
    return {name[0] - 'A', col - 1};
}

/** @brief Converts "ATTACK" / "DEFENSE" into a role. */
static PlayerRole parseRole(const string& role) {
    return (role == "DEFENSE") ? DEFENSE : ATTACK;
}

/** @brief Converts "11" / "13" into a board size. */
static BoardSize parseSize(const string& size) {
    return (size == "13") ? BIG : LITTLE;
}

/** @brief Converts a move "F3 F5" into a Move. */
static Move parseMove(const string& move) {
    vector<string> cells = split(move, ' ');
    Move result = {{-1, -1}, {-1, -1}};
    if (cells.size() >= 2) {
        result.itsStartPosition = parseCell(cells[0]);
        result.itsEndPosition = parseCell(cells[cells.size() - 1]);
    }
    return result;
}

/**
 * @brief Converts a cell content ("X", "O", "K", ".", "F", "C", "FK", "CK") into a Cell.
 */
static Cell parseContent(const string& content) {
    Cell cell = {NORMAL, NONE};
    for (char c : content) {
        switch (c) {
            case 'F': cell.itsCellType = FORTRESS; break;
            case 'C': cell.itsCellType = CASTLE; break;
            case 'X': cell.itsPieceType = SWORD; break;
            case 'O': cell.itsPieceType = SHIELD; break;
            case 'K': cell.itsPieceType = KING; break;
            default: break;
        }
    }
    return cell;
}

/** @brief One "cell=content" item of a board or expectation list. */
struct CellSpec {
    Position itsPos;
    string itsContent;
};

/** @brief Converts "F6=O F7=X" into a list of cell specifications. */
static vector<CellSpec> parseCellSpecs(const string& specs) {
    vector<CellSpec> result;
    for (const string& item : split(specs, ' ')) {
        size_t equal = item.find('=');
        if (item.empty() || equal == string::npos) continue;
        result.push_back({parseCell(item.substr(0, equal)), item.substr(equal + 1)});
    }
    return result;
}

/**
 * @brief Fills a board from its description (other cells are empty NORMAL cells).
 * @param aBoard An allocated board of size aSize
 * @param aSize Board size
 * @param specs Board description, e.g. "F6=O F7=X A1=F"
 */
static void fillBoard(Cell** aBoard, BoardSize aSize, const string& specs) {
    resetBoard(aBoard, aSize);
    for (const CellSpec& spec : parseCellSpecs(specs)) {
        if (spec.itsPos.itsRow >= 0 && spec.itsPos.itsRow < aSize
            && spec.itsPos.itsCol >= 0 && spec.itsPos.itsCol < aSize) {
            aBoard[spec.itsPos.itsRow][spec.itsPos.itsCol] = parseContent(spec.itsContent);
        }
    }
}

/** @brief Positions of the cells of a list "F6=O F7=X" (used to highlight them). */
static vector<Position> specPositions(const string& specs) {
    vector<Position> positions;
    for (const CellSpec& spec : parseCellSpecs(specs)) {
        positions.push_back(spec.itsPos);
    }
    return positions;
}

/**
 * @brief Compares a board with a list of expected cells.
 * @param aBoard The board to check
 * @param specs Expected cells, e.g. "F6=. F7=X"
 * @return Description of the differences (empty string if the board is as expected)
 */
static string checkCells(Cell** aBoard, const string& specs) {
    string problems;
    for (const CellSpec& spec : parseCellSpecs(specs)) {
        const Cell expected = parseContent(spec.itsContent);
        const Cell& actual = aBoard[spec.itsPos.itsRow][spec.itsPos.itsCol];
        bool checkType = !spec.itsContent.empty() && (spec.itsContent[0] == 'F' || spec.itsContent[0] == 'C');
        if (actual.itsPieceType != expected.itsPieceType || (checkType && actual.itsCellType != expected.itsCellType)) {
            string name = cellName(spec.itsPos.itsRow, spec.itsPos.itsCol);
            problems += (problems.empty() ? "" : "; ") + name + " contains " + pieceName(actual.itsPieceType)
                        + " instead of " + pieceName(expected.itsPieceType);
            if (checkType && actual.itsCellType != expected.itsCellType) problems += " (wrong cell type)";
        }
    }
    return problems;
}

/**
 * @brief Compares the whole board after a move with the expected board.
 *
 * Expected board = the board before the move, with the move applied (if moved is true),
 * then the expected cells of the test case. Every other cell must not change.
 *
 * @param actual The board after the call
 * @param size Board size
 * @param beforeSpecs Board before the call
 * @param move The move
 * @param moved Whether the piece must have been moved
 * @param expectedSpecs Expected cells, e.g. "F6=. F7=X"
 * @return Description of the differences (empty string if the board is as expected)
 */
static string checkBoardAfterMove(Cell** actual, BoardSize size, const string& beforeSpecs,
                                  const Move& move, bool moved, const string& expectedSpecs) {
    TestBoard expected(size);
    Cell** cells = expected.cells();
    fillBoard(cells, size, beforeSpecs);
    if (moved) {
        Cell& start = cells[move.itsStartPosition.itsRow][move.itsStartPosition.itsCol];
        cells[move.itsEndPosition.itsRow][move.itsEndPosition.itsCol].itsPieceType = start.itsPieceType;
        start.itsPieceType = NONE;
    }
    for (const CellSpec& spec : parseCellSpecs(expectedSpecs)) {
        Cell content = parseContent(spec.itsContent);
        Cell& cell = cells[spec.itsPos.itsRow][spec.itsPos.itsCol];
        cell.itsPieceType = content.itsPieceType;
        if (!spec.itsContent.empty() && (spec.itsContent[0] == 'F' || spec.itsContent[0] == 'C')) {
            cell.itsCellType = content.itsCellType;
        }
    }

    // The expected cells first (most useful messages), then any other unexpected change
    string problems = checkCells(actual, expectedSpecs);
    vector<Position> listed = specPositions(expectedSpecs);
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            bool isListed = false;
            for (const Position& pos : listed) {
                if (pos.itsRow == r && pos.itsCol == c) isListed = true;
            }
            const Cell& a = actual[r][c];
            const Cell& e = cells[r][c];
            if (!isListed && (a.itsPieceType != e.itsPieceType || a.itsCellType != e.itsCellType)) {
                problems += (problems.empty() ? "" : "; ") + string("unexpected change in ") + cellName(r, c)
                            + ": " + pieceName(a.itsPieceType) + " instead of " + pieceName(e.itsPieceType);
                if (a.itsCellType != e.itsCellType) problems += " (wrong cell type)";
            }
        }
    }
    return problems;
}


/** @brief Returns a field of a test case, or "" if it does not exist. */
static string field(const TestRow& row, size_t index) {
    return (index < row.size()) ? row[index] : "";
}

// ========================================================================================
// ================================= TEST FUNCTIONS =======================================
// ========================================================================================
// One test function per function of functions.h (except the display functions, which are free).
// Each test uses runIsolated() to protect against crashes and exceptions.
// All results are aggregated in the final summary table (printTestSuiteFooter).
// ========================================================================================

/**
 * @brief Runs all the test cases of a section of the test data, each one in an isolated process.
 *
 * Displays the header and the summary of the section, and counts the results.
 *
 * @param section Name of the tested function (section of the test data, title of the results)
 * @param descriptionField Index of the field containing the description of the test case
 * @param testCase Executes one test case, prints its result and returns 0 (passed) or 1 (failed)
 */
static void runTestSection(const string& section, size_t descriptionField,
                           const function<int(const TestRow&, int, const string&)>& testCase) {
    printTestHeader(section);
    int pass = 0;
    int failed = 0;
    int testNum = 0;

    for (const TestRow& row : testRows(section)) {
        testNum++;
        const int currentTestNum = testNum;
        const string description = field(row, descriptionField);
        runIsolated([&testCase, &row, currentTestNum, &description]() -> int {
            return testCase(row, currentTestNum, description);
        }, currentTestNum, description, pass, failed);
    }

    printTestSummary(section, pass, failed);
}

/**
 * @brief Converts a text written between double quotes ("11", "\t13") into the real text.
 * Supported escapes: \n (new line), \t (tabulation), \\ and \".
 */
static string parseQuoted(const string& quoted) {
    string text;
    size_t first = quoted.find('"');
    size_t last = quoted.rfind('"');
    if (first == string::npos || last == first) return quoted;
    for (size_t i = first + 1; i < last; ++i) {
        if (quoted[i] == '\\' && i + 1 < last) {
            ++i;
            switch (quoted[i]) {
                case 'n': text += '\n'; break;
                case 't': text += '\t'; break;
                default:  text += quoted[i]; break;
            }
        } else {
            text += quoted[i];
        }
    }
    return text;
}

/**
 * @brief Simulates the keyboard: cin reads from a string while the function is executed.
 * @param input The text "typed" by the user
 * @param fn The code to execute
 */
static void withKeyboardInput(const string& input, const function<void()>& fn) {
    istringstream stream(input);
    streambuf* oldCinBuf = cin.rdbuf(stream.rdbuf());
    cin.clear();
    fn();
    cin.rdbuf(oldCinBuf);
    cin.clear();
}

/** @brief Converts "true" / "false" into a boolean. */
static bool parseBool(const string& value) {
    return value.substr(0, 4) == "true";
}

/** @brief Names of the MoveStatus values, in the order of the enum. */
static const char* const MOVE_STATUS_NAMES[] = {
    "MOVE_OK", "MOVE_OUT_OF_BOARD", "MOVE_SAME_CELL", "MOVE_NOT_YOUR_PIECE",
    "MOVE_NOT_STRAIGHT", "MOVE_PATH_BLOCKED", "MOVE_FORBIDDEN_CELL"
};
static const int MOVE_STATUS_COUNT = sizeof(MOVE_STATUS_NAMES) / sizeof(MOVE_STATUS_NAMES[0]);

/** @brief Name of a MoveStatus value, e.g. MOVE_OK → "MOVE_OK". */
static string moveStatusName(MoveStatus status) {
    int index = static_cast<int>(status);
    if (index < 0 || index >= MOVE_STATUS_COUNT) return "invalid value " + to_string(index);
    return MOVE_STATUS_NAMES[index];
}

/** @brief Converts a name ("MOVE_PATH_BLOCKED") into a MoveStatus value. */
static MoveStatus parseMoveStatus(const string& name) {
    for (int i = 0; i < MOVE_STATUS_COUNT; ++i) {
        if (name == MOVE_STATUS_NAMES[i]) return static_cast<MoveStatus>(i);
    }
    return MOVE_OK;
}

/** @brief Converts "DEFENSE" / "ATTACK" into the name of the player of a test game. */
static string roleName(PlayerRole role) {
    return (role == ATTACK) ? "ATTACK" : "DEFENSE";
}

/** @brief Converts a position "(row,col)" or a cell name "F6" into a Position. */
static Position parsePosition(const string& text) {
    if (!text.empty() && text[0] == '(') {
        vector<string> numbers = split(text.substr(1, text.size() - 2), ',');
        if (numbers.size() == 2) return {stoi(numbers[0]), stoi(numbers[1])};
    }
    return parseCell(text);
}

/** @brief Displays a position for the test messages: cell name if it is on a board, (row,col) otherwise. */
static string positionName(const Position& pos) {
    if (pos.itsRow >= 0 && pos.itsRow < 26 && pos.itsCol >= 0) return cellName(pos.itsRow, pos.itsCol);
    return "(" + to_string(pos.itsRow) + "," + to_string(pos.itsCol) + ")";
}

/**
 * @brief Runs the test cases of a function whose result is a boolean computed from a board.
 *
 * Test case fields: size | board | expected (true/false) | description
 *
 * @param section Name of the tested function (section of the test data)
 * @param fn Calls the tested function on the board
 * @param trueText Text displayed for a true result (e.g. "captured")
 * @param falseText Text displayed for a false result (e.g. "not captured")
 */
static void runBoardPredicateTests(const string& section, const function<bool(const Board&)>& fn,
                                   const string& trueText, const string& falseText) {
    runTestSection(section, 3, [&fn, &trueText, &falseText](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        TestBoard storage(size);
        Board board = storage.board();
        fillBoard(board.itsCells, size, field(row, 1));

        const bool expected = parseBool(field(row, 2));
        const bool result = fn(board);
        const bool success = (result == expected);

        printTestResult(currentTestNum, description, success,
                        expected ? trueText : falseText, result ? trueText : falseText);
        if (shouldShowBoards(success)) {
            printBoard(board.itsCells, size, "Board:");
        }
        return success ? 0 : 1;
    });
}

/**
 * @brief Builds a game from a test case: board, players (Player1 attacker, Player2 defender)
 *        and current player.
 * @param game The game to fill
 * @param storage The cells used by the board of the game
 * @param role Role of the current player
 * @param boardSpecs Board description
 */
static void buildGame(Game& game, TestBoard& storage, PlayerRole role, const string& boardSpecs) {
    game.itsBoard = storage.board();
    fillBoard(game.itsBoard.itsCells, game.itsBoard.itsSize, boardSpecs);
    game.itsPlayer1 = {"Attacker", ATTACK};
    game.itsPlayer2 = {"Defender", DEFENSE};
    game.itsCurrentPlayer = (role == ATTACK) ? &game.itsPlayer1 : &game.itsPlayer2;
}

/**
 * @brief Test function for the stringToBoardSize function.
 *
 * Test coverage:
 * - Valid texts: "11" (LITTLE), "13" (BIG)
 * - Invalid sizes, huge numbers (overflow), letters, empty text, spaces, tabs,
 *   decimal numbers, leading zeros, signs (+/-)
 */
void test_stringToBoardSize()
{
    // Fields: text | expected ("true <size>" or "false") | description
    runTestSection("stringToBoardSize", 2, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const string expectedText = field(row, 1);
        const bool expectedReturn = parseBool(expectedText);
        const BoardSize expectedSize = expectedReturn ? parseSize(expectedText.substr(5)) : LITTLE;

        BoardSize yourBoardSize = static_cast<BoardSize>(0);
        bool ok = stringToBoardSize(parseQuoted(field(row, 0)), yourBoardSize);

        bool success = expectedReturn ? (ok && yourBoardSize == expectedSize) : !ok;
        string actual = ok ? "true and size " + to_string(yourBoardSize) : "false";
        string expected = expectedReturn ? "true and size " + to_string(expectedSize) : "false";
        printTestResult(currentTestNum, description, success, expected, actual);
        return success ? 0 : 1;
    });
}


/** @brief Converts a size field ("11", "13", "0", "-1", "1000"...) into a BoardSize, invalid values included. */
static BoardSize parseRawSize(const string& size) {
    return static_cast<BoardSize>(stoi(size));
}

/**
 * @brief Test function for the createBoard function.
 *
 * Each test case is a scenario (see the [createBoard] section of the test data):
 * returned value and pointer, allocated rows, double allocation, initial content of the cells.
 */
void test_createBoard() {
    // Fields: size | scenario | expected | description
    runTestSection("createBoard", 3, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseRawSize(field(row, 0));
        const string scenario = field(row, 1);
        const string expectedText = field(row, 2);
        Board board = {nullptr, size};
        bool success = false;
        string expected, actual;

        if (scenario == "create") {
            bool result = createBoard(board);
            bool expectedResult = parseBool(expectedText);
            success = (result == expectedResult) && ((board.itsCells != nullptr) == expectedResult);
            expected = expectedResult ? "true and non-null pointer" : "false and itsCells == nullptr";
            actual = string(result ? "true" : "false") + " and " + (board.itsCells ? "non-null pointer" : "nullptr");
            if (board.itsCells != nullptr && !expectedResult) {
                // Accepted by mistake: rows may be missing, free only the array of rows
                delete[] board.itsCells;
                board.itsCells = nullptr;
            }
        }
        else if (scenario == "rows") {
            createBoard(board);
            success = (board.itsCells != nullptr);
            for (int r = 0; success && r < size; ++r) {
                if (board.itsCells[r] == nullptr) success = false;
            }
            expected = "all rows allocated";
            actual = "nullptr board or row";
        }
        else if (scenario == "double") {
            createBoard(board);
            Cell** firstAllocation = board.itsCells;
            bool secondResult = createBoard(board);
            success = firstAllocation != nullptr && secondResult == parseBool(expectedText)
                      && board.itsCells == firstAllocation;
            expected = "second call returns " + expectedText + ", pointer unchanged";
            actual = (firstAllocation == nullptr) ? "first allocation failed"
                   : string("second call returns ") + (secondResult ? "true" : "false")
                     + (board.itsCells == firstAllocation ? ", pointer unchanged" : ", pointer modified (memory leak)");
        }
        else if (scenario == "cells") {
            createBoard(board);
            const Cell expectedCell = parseContent(expectedText);
            int wrongCells = (board.itsCells == nullptr) ? size * size : 0;
            string firstWrong;
            for (int r = 0; board.itsCells != nullptr && r < size; ++r) {
                for (int c = 0; c < size; ++c) {
                    const Cell& cell = board.itsCells[r][c];
                    if (cell.itsCellType != expectedCell.itsCellType || cell.itsPieceType != expectedCell.itsPieceType) {
                        if (wrongCells == 0) firstWrong = cellName(r, c);
                        wrongCells++;
                    }
                }
            }
            success = (wrongCells == 0);
            expected = "all cells {NORMAL, NONE}";
            actual = to_string(wrongCells) + " cells not initialized" + (firstWrong.empty() ? "" : " (first: " + firstWrong + ")");
        }

        // Free the board allocated by createBoard (rows first, then the array of rows)
        if (board.itsCells != nullptr) {
            for (int r = 0; r < size; ++r) {
                delete[] board.itsCells[r];
            }
            delete[] board.itsCells;
        }
        printTestResult(currentTestNum, description, success, expected, actual);
        return success ? 0 : 1;
    });
}


/**
 * @brief Test function for the deleteBoard function.
 *
 * Each test case is a scenario (see the [deleteBoard] section of the test data):
 * delete once, delete twice, delete a board never allocated. The pointer must become nullptr.
 */
void test_deleteBoard() {
    // Fields: size | scenario | expected pointer | description
    runTestSection("deleteBoard", 3, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseRawSize(field(row, 0));
        const string scenario = field(row, 1);
        Board board = {nullptr, size};

        const long arraysBefore = G_LIVE_ARRAYS;
        if (scenario != "nullptr") {
            // Allocated row by row, as createBoard does: deleteBoard must free it
            board.itsCells = new Cell*[size];
            for (int r = 0; r < size; ++r) {
                board.itsCells[r] = new Cell[size];
            }
        }
        deleteBoard(board);
        if (scenario == "twice") {
            deleteBoard(board);
        }
        const long arraysNotFreed = G_LIVE_ARRAYS - arraysBefore;

        bool expectNull = (field(row, 2) == "nullptr");
        string problem;
        if ((board.itsCells == nullptr) != expectNull) {
            problem = (board.itsCells == nullptr) ? "nullptr" : "non-null pointer";
        } else if (arraysNotFreed > 0) {
            problem = "memory leak: " + to_string(arraysNotFreed) + " array(s) not freed (each row, then the array of rows)";
        }
        bool success = problem.empty();
        printTestResult(currentTestNum, description, success, field(row, 2) + ", all arrays freed", problem);
        return success ? 0 : 1;
    });
}


/**
 * @brief Fills a board with the expected starting position (section [initialPosition] of the test data).
 * @param aBoard An allocated board
 * @param aSize Board size
 */
static void fillInitialBoard(Cell** aBoard, BoardSize aSize) {
    string specs;
    for (const TestRow& row : testRows("initialPosition")) {
        if (parseSize(field(row, 0)) == aSize) specs = field(row, 1);
    }
    fillBoard(aBoard, aSize, specs);
}

/**
 * @brief Compares a board with the starting position.
 * @return Description of the differences (empty string if the board is the starting position)
 */
static string compareWithInitialBoard(Cell** aBoard, BoardSize aSize) {
    TestBoard expectedStorage(aSize);
    Cell** expectedBoard = expectedStorage.cells();
    fillInitialBoard(expectedBoard, aSize);
    int diffCount = 0;
    string firstDiff;
    for (int r = 0; r < aSize; ++r) {
        for (int c = 0; c < aSize; ++c) {
            const Cell& a = aBoard[r][c];
            const Cell& e = expectedBoard[r][c];
            if (a.itsCellType != e.itsCellType || a.itsPieceType != e.itsPieceType) {
                if (diffCount == 0) {
                    firstDiff = cellName(r, c) + ": expected " + pieceName(e.itsPieceType) + ", got " + pieceName(a.itsPieceType);
                    if (a.itsCellType != e.itsCellType) firstDiff += " (wrong cell type)";
                }
                diffCount++;
            }
        }
    }
    return diffCount == 0 ? "" : to_string(diffCount) + " wrong cell(s), first at " + firstDiff;
}

/**
 * @brief Test function for the initializeBoard function.
 *
 * Each test case is a scenario (see the [initializeBoard] section of the test data):
 * whole board compared with the starting position, fortresses, king on the castle,
 * piece counts, empty cells, double call, board in the middle of a game,
 * nullptr board and invalid size.
 */
void test_initializeBoard()
{
    // Fields: size | scenario | board before the call | expected | description
    runTestSection("initializeBoard", 4, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseRawSize(field(row, 0));
        const string scenario = field(row, 1);
        const string expectedText = field(row, 3);
        Board board = {nullptr, size};
        bool success = false;
        string actual;

        // Cells of the board (11x11 if the size of the test case is invalid)
        TestBoard storage((size == LITTLE || size == BIG) ? size : LITTLE);

        if (scenario == "nullptr") {
            initializeBoard(board);
            success = (board.itsCells == nullptr);
            actual = "modified pointer";
        }
        else if (scenario == "unchanged") {
            // Invalid size: the board has 11x11 cells but declares another size
            board.itsCells = storage.cells();
            initializeBoard(board);
            board.itsSize = LITTLE;
            success = true;
            for (int r = 0; r < LITTLE; ++r) {
                for (int c = 0; c < LITTLE; ++c) {
                    if (board.itsCells[r][c].itsCellType != NORMAL || board.itsCells[r][c].itsPieceType != NONE) {
                        success = false;
                    }
                }
            }
            actual = "board was modified";
        }
        else {
            board.itsCells = storage.cells();
            fillBoard(board.itsCells, size, field(row, 2));
            initializeBoard(board);
            if (scenario == "twice") {
                initializeBoard(board);
            }

            if (scenario == "compare" || scenario == "twice") {
                actual = compareWithInitialBoard(board.itsCells, size);
                success = actual.empty();
            }
            else if (scenario == "fortresses" || scenario == "king") {
                vector<string> cells = split(expectedText, ' ');
                int correct = 0;
                for (const string& name : cells) {
                    Position pos = parseCell(name);
                    const Cell& cell = board.itsCells[pos.itsRow][pos.itsCol];
                    bool ok = (scenario == "fortresses") ? (cell.itsCellType == FORTRESS)
                                                         : (cell.itsCellType == CASTLE && cell.itsPieceType == KING);
                    if (ok) correct++;
                    else if (actual.empty()) actual = name + " contains " + pieceName(cell.itsPieceType)
                                                      + (cell.itsCellType == FORTRESS ? " on FORTRESS" : cell.itsCellType == CASTLE ? " on CASTLE" : " on NORMAL cell");
                }
                success = (correct == static_cast<int>(cells.size()));
            }
            else if (scenario == "counts") {
                int kings = 0, shields = 0, swords = 0, fortresses = 0, castles = 0;
                for (int r = 0; r < size; ++r) {
                    for (int c = 0; c < size; ++c) {
                        const Cell& cell = board.itsCells[r][c];
                        if (cell.itsPieceType == KING) kings++;
                        if (cell.itsPieceType == SHIELD) shields++;
                        if (cell.itsPieceType == SWORD) swords++;
                        if (cell.itsCellType == FORTRESS) fortresses++;
                        if (cell.itsCellType == CASTLE) castles++;
                    }
                }
                actual = to_string(kings) + "/" + to_string(shields) + "/" + to_string(swords) + "/"
                         + to_string(fortresses) + "/" + to_string(castles);
                success = (actual == expectedText);
            }
            else if (scenario == "empty") {
                int emptyCells = 0;
                for (int r = 0; r < size; ++r) {
                    for (int c = 0; c < size; ++c) {
                        if (board.itsCells[r][c].itsCellType == NORMAL && board.itsCells[r][c].itsPieceType == NONE) emptyCells++;
                    }
                }
                actual = to_string(emptyCells);
                success = (actual == expectedText);
            }
        }

        string expected = (expectedText == "initial") ? "board identical to the starting position"
                        : (scenario == "counts") ? expectedText + " (kings/shields/swords/fortresses/castles)"
                        : expectedText;
        printTestResult(currentTestNum, description, success, expected, actual);

        if (shouldShowBoards(success) && board.itsCells != nullptr) {
            // Highlight the cells that differ from the starting position
            vector<Position> differences;
            if (board.itsSize == size && (size == LITTLE || size == BIG)) {
                TestBoard initial(size);
                fillInitialBoard(initial.cells(), size);
                for (int r = 0; r < size; ++r) {
                    for (int c = 0; c < size; ++c) {
                        const Cell& a = board.itsCells[r][c];
                        const Cell& e = initial.cells()[r][c];
                        if (a.itsCellType != e.itsCellType || a.itsPieceType != e.itsPieceType) {
                            differences.push_back({r, c});
                        }
                    }
                }
                if (success) {
                    printBoard(board.itsCells, board.itsSize, "Board after initializeBoard:", differences);
                } else {
                    printBoards({{board.itsCells, board.itsSize, "Board after initializeBoard:", differences},
                                 {initial.cells(), size, "Expected board:", differences}});
                }
            } else {
                printBoard(board.itsCells, board.itsSize, "Board after initializeBoard:");
            }
        }
        return success ? 0 : 1;
    });
}



/**
 * @brief Test function for the isValidPosition function.
 *
 * Test coverage:
 * - Valid positions: corners, center, edges, just inside the corners (LITTLE and BIG)
 * - Invalid positions: negative coordinates, just outside the board, extreme values
 */
void test_isValidPosition()
{
    // Fields: size | position (row,col) | expected | description
    runTestSection("isValidPosition", 3, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        const Position pos = parsePosition(field(row, 1));
        const bool expected = parseBool(field(row, 2));

        bool result = isValidPosition(pos, {nullptr, size});
        bool success = (result == expected);
        printTestResult(currentTestNum, description, success,
                        expected ? "valid" : "invalid", result ? "valid" : "invalid");
        return success ? 0 : 1;
    });
}


/**
 * @brief Test function for the stringToPosition function.
 *
 * Test coverage:
 * - Valid texts, uppercase and lowercase, corners and center (LITTLE and BIG)
 * - Positions outside the board, wrong formats (1A, AA, A, A-1, A 1, @5, A1B...)
 * - Column 0, leading zeros, overflow, spaces, empty text
 */
void test_stringToPosition() {
    // Fields: size | text | expected ("true <cell>" or "false") | description
    runTestSection("stringToPosition", 3, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        const string expectedText = field(row, 2);
        const bool expectedResult = parseBool(expectedText);

        Position position = {-1, -1};
        bool result = stringToPosition(parseQuoted(field(row, 1)), position, {nullptr, size});

        if (result != expectedResult) {
            printTestResult(currentTestNum, description, false,
                            expectedResult ? "valid (true)" : "invalid (false)",
                            result ? "valid (true)" : "invalid (false)");
            return 1;
        }
        if (result) {
            const Position expectedPos = parseCell(expectedText.substr(5));
            if (position.itsRow != expectedPos.itsRow || position.itsCol != expectedPos.itsCol) {
                printTestResult(currentTestNum, description, false,
                                "(" + to_string(expectedPos.itsRow) + "," + to_string(expectedPos.itsCol) + ")",
                                "(" + to_string(position.itsRow) + "," + to_string(position.itsCol) + ")");
                return 1;
            }
        }
        printTestResult(currentTestNum, description, true);
        return 0;
    });
}


/**
 * @brief Test function for the isEmptyCell function.
 *
 * Test coverage:
 * - Empty cells (NONE piece) on NORMAL, FORTRESS and CASTLE cells
 * - Cells occupied by a SWORD, a SHIELD or the KING, whatever the cell type
 * - Both board sizes
 */
void test_isEmptyCell()
{
    // Fields: size | board | cell | expected | description
    runTestSection("isEmptyCell", 4, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        TestBoard storage(size);
        Board board = storage.board();
        fillBoard(board.itsCells, size, field(row, 1));
        const Position pos = parsePosition(field(row, 2));
        const bool expected = parseBool(field(row, 3));

        bool result = isEmptyCell(board, pos);
        bool success = (result == expected);
        printTestResult(currentTestNum, description, success,
                        expected ? "empty" : "not empty", result ? "empty" : "not empty");
        if (shouldShowBoards(success)) {
            printBoard(board.itsCells, size, "Board (cell " + cellName(pos.itsRow, pos.itsCol) + " checked):", {pos});
        }
        return success ? 0 : 1;
    });
}

/**
 * @brief Test function for the checkMovement function.
 *
 * Test coverage (both board sizes), with the reason of each refusal (MoveStatus):
 * - Defensive validation: same start/end, positions out of the board
 * - Piece ownership: ATTACK moves SWORD, DEFENSE moves SHIELD/KING, empty start cell
 * - Direction: the 4 directions are allowed, diagonals are forbidden
 * - Path obstruction: short/long distance, several obstacles, occupied destination
 * - FORTRESS and CASTLE rules
 */
void test_checkMovement()
{
    // Fields: size | player | move | board | expected MoveStatus | description
    runTestSection("checkMovement", 5, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        TestBoard storage(size);
        Game game;
        buildGame(game, storage, parseRole(field(row, 1)), field(row, 3));

        vector<string> cells = split(field(row, 2), ' ');
        const Move move = {parsePosition(field(cells, 0)), parsePosition(field(cells, 1))};
        const MoveStatus expected = parseMoveStatus(field(row, 4));

        MoveStatus result = checkMovement(game, move);
        bool success = (result == expected);
        printTestResult(currentTestNum, description, success, moveStatusName(expected), moveStatusName(result));
        if (shouldShowBoards(success)) {
            printBoard(game.itsBoard.itsCells, size, "Board (move " + positionName(move.itsStartPosition)
                       + "→" + positionName(move.itsEndPosition) + "):", {move.itsStartPosition, move.itsEndPosition});
        }
        return success ? 0 : 1;
    });
}

/**
 * @brief Test function for the movePiece function.
 *
 * movePiece only moves the piece: the move is supposed valid (see checkMovement).
 *
 * Test coverage (both board sizes):
 * - SWORD, SHIELD and KING moves in the 4 directions, short and long distance
 * - KING moves to/from FORTRESS and CASTLE: the cell types must not change
 */
void test_movePiece()
{
    // Fields: size | move | board | expected cells | description
    runTestSection("movePiece", 4, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        TestBoard storage(size);
        Game game;
        buildGame(game, storage, ATTACK, field(row, 2));

        movePiece(game, parseMove(field(row, 1)));

        string problems = checkBoardAfterMove(game.itsBoard.itsCells, size, field(row, 2), parseMove(field(row, 1)),
                                              true, field(row, 3));
        bool success = problems.empty();
        printTestResult(currentTestNum, description, success, "piece moved, cell types kept", problems);
        if (shouldShowBoards(success)) {
            TestBoard before(size);
            fillBoard(before.cells(), size, field(row, 2));
            const Move move = parseMove(field(row, 1));
            printBoards({{before.cells(), size, "Before move " + field(row, 1) + ":", {move.itsStartPosition, move.itsEndPosition}},
                         {game.itsBoard.itsCells, size, "After movePiece:", specPositions(field(row, 3))}});
        }
        return success ? 0 : 1;
    });
}

/**
 * @brief Test function for the capturePieces function.
 *
 * Each test case (see the [capturePieces] section of the test data) describes a board,
 * the current player and a move. The test executes movePiece then capturePieces, and
 * checks the expected cells: captured pieces removed, other pieces kept.
 *
 * Test coverage:
 * - ATTACK captures (SWORD captures SHIELD) and DEFENSE captures (SHIELD/KING captures SWORD)
 * - Every kind of partner: friendly piece, KING, FORTRESS, empty CASTLE
 * - The 4 directions, multiple captures, board borders, BIG board
 * - Non-captures: same team, protected pieces, KING never captured like a normal piece,
 *   a piece moving between two enemies is not captured
 */
void test_capturePieces()
{
    // Fields: size | player | move | board | expected cells | description
    runTestSection("capturePieces", 5, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        const Move move = parseMove(field(row, 2));

        TestBoard storage(size);
        Game game;
        game.itsBoard = storage.board();
        fillBoard(game.itsBoard.itsCells, size, field(row, 3));
        game.itsPlayer1.itsRole = ATTACK;
        game.itsPlayer2.itsRole = DEFENSE;
        game.itsCurrentPlayer = (parseRole(field(row, 1)) == ATTACK) ? &game.itsPlayer1 : &game.itsPlayer2;

        // Board before the move (displayed if requested, or for the first failure)
        TestBoard before(size);
        Cell** boardBefore = before.cells();
        fillBoard(boardBefore, size, field(row, 3));

        movePiece(game, move);
        capturePieces(game, move);

        string problems = checkBoardAfterMove(game.itsBoard.itsCells, size, field(row, 3), move, true, field(row, 4));
        bool testPassed = problems.empty();
        printTestResult(currentTestNum, description, testPassed, "all captures/non-captures correct", problems);

        if (shouldShowBoards(testPassed)) {
            printBoards({{boardBefore, size, "Before move " + field(row, 2) + ":", {move.itsStartPosition, move.itsEndPosition}},
                         {game.itsBoard.itsCells, size, "After movePiece + capturePieces:", specPositions(field(row, 4))}});
        }

        return testPassed ? 0 : 1;
    });
}


/**
 * @brief Test function for switchCurrentPlayer.
 *
 * Each test case (see the [switchCurrentPlayer] section of the test data) gives the current
 * player at the start, a number of switches and the expected current player at the end.
 * "same players" checks that switching does not modify the names and roles of the players.
 */
void test_switchCurrentPlayer() {
    // Fields: player at start | number of switches | expected | description
    runTestSection("switchCurrentPlayer", 3, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        Game game;
        game.itsPlayer1 = {"Alice", ATTACK};
        game.itsPlayer2 = {"Bob", DEFENSE};
        const string start = field(row, 0);
        game.itsCurrentPlayer = (start == "Player1") ? &game.itsPlayer1
                              : (start == "Player2") ? &game.itsPlayer2 : nullptr;

        const int switchCount = stoi(field(row, 1));
        for (int i = 0; i < switchCount; ++i) {
            switchCurrentPlayer(game);
        }

        const string expected = field(row, 2);
        string actual;
        if (expected == "same players") {
            actual = string("P1:") + (game.itsPlayer1.itsRole == ATTACK ? "ATTACK" : "DEFENSE") + "/" + game.itsPlayer1.itsName
                   + ", P2:" + (game.itsPlayer2.itsRole == ATTACK ? "ATTACK" : "DEFENSE") + "/" + game.itsPlayer2.itsName;
            bool success = (actual == "P1:ATTACK/Alice, P2:DEFENSE/Bob");
            printTestResult(currentTestNum, description, success, "P1:ATTACK/Alice, P2:DEFENSE/Bob", actual);
            return success ? 0 : 1;
        }

        actual = (game.itsCurrentPlayer == &game.itsPlayer1) ? "Player1"
               : (game.itsCurrentPlayer == &game.itsPlayer2) ? "Player2" : "nullptr";
        bool success = (actual == expected);
        printTestResult(currentTestNum, description, success, expected, actual);
        return success ? 0 : 1;
    });
}


/**
 * @brief Test function for the playTurn function.
 *
 * Each test case (see the [playTurn] section of the test data) gives a board, the current
 * player and a move. playTurn must return the expected MoveStatus; if the move is valid,
 * the piece is moved, the captures are done and the other player becomes the current player;
 * if the move is refused, nothing changes.
 */
void test_playTurn()
{
    // Fields: size | player | move | board | expected MoveStatus | expected cells | expected current player | description
    runTestSection("playTurn", 7, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        TestBoard storage(size);
        Game game;
        buildGame(game, storage, parseRole(field(row, 1)), field(row, 3));

        vector<string> cells = split(field(row, 2), ' ');
        const Move move = {parsePosition(field(cells, 0)), parsePosition(field(cells, 1))};
        const MoveStatus expectedStatus = parseMoveStatus(field(row, 4));

        MoveStatus status = playTurn(game, move);

        string problems;
        if (status != expectedStatus) {
            problems = "returned " + moveStatusName(status) + " instead of " + moveStatusName(expectedStatus);
        }
        string cellProblems = checkBoardAfterMove(game.itsBoard.itsCells, size, field(row, 3), move,
                                                  expectedStatus == MOVE_OK, field(row, 5));
        if (!cellProblems.empty()) {
            problems += (problems.empty() ? "" : "; ") + cellProblems;
        }
        string currentPlayer = (game.itsCurrentPlayer == &game.itsPlayer1) ? "ATTACK"
                             : (game.itsCurrentPlayer == &game.itsPlayer2) ? "DEFENSE" : "nullptr";
        if (currentPlayer != field(row, 6)) {
            problems += (problems.empty() ? "" : "; ") + string("current player ") + currentPlayer
                        + " instead of " + field(row, 6);
        }

        bool success = problems.empty();
        printTestResult(currentTestNum, description, success,
                        moveStatusName(expectedStatus) + ", board and current player correct", problems);
        if (shouldShowBoards(success)) {
            TestBoard before(size);
            fillBoard(before.cells(), size, field(row, 3));
            printBoards({{before.cells(), size, "Before playTurn " + field(row, 2) + " (" + field(row, 1) + " to play):",
                          {move.itsStartPosition, move.itsEndPosition}},
                         {game.itsBoard.itsCells, size, "After playTurn:", specPositions(field(row, 5))}});
        }
        return success ? 0 : 1;
    });
}



/**
 * @brief Test function for isSwordLeft.
 *
 * Test coverage: empty board, only shields/king, one or several swords anywhere, both board sizes.
 */
void test_isSwordLeft()
{
    runBoardPredicateTests("isSwordLeft", [](const Board& board) { return isSwordLeft(board); },
                           "true (swords present)", "false (no swords)");
}

/**
 * @brief Test function for getKingPosition.
 *
 * Test coverage: king on corners, edges, center, special cells, with other pieces,
 * no king (returns (-1,-1)), both board sizes.
 */
void test_getKingPosition()
{
    // Fields: size | board | expected cell (or none) | description
    runTestSection("getKingPosition", 3, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        TestBoard storage(size);
        Board board = storage.board();
        fillBoard(board.itsCells, size, field(row, 1));
        const Position expected = (field(row, 2) == "none") ? Position{-1, -1} : parseCell(field(row, 2));

        Position result = getKingPosition(board);
        bool success = (result.itsRow == expected.itsRow && result.itsCol == expected.itsCol);
        printTestResult(currentTestNum, description, success,
                        "(" + to_string(expected.itsRow) + "," + to_string(expected.itsCol) + ")",
                        "(" + to_string(result.itsRow) + "," + to_string(result.itsCol) + ")");
        if (shouldShowBoards(success)) {
            printBoard(board.itsCells, size, "Board:");
        }
        return success ? 0 : 1;
    });
}

/**
 * @brief Test function for isKingEscaped.
 *
 * Test coverage: king on each fortress, on the castle, on normal cells, no king, both board sizes.
 */
void test_isKingEscaped()
{
    runBoardPredicateTests("isKingEscaped", [](const Board& board) { return isKingEscaped(board); },
                           "escaped", "not escaped");
}

/**
 * @brief Test function for isKingCapturedSimple.
 *
 * Simple rule: the king is captured if its 4 neighbours are hostile
 * (SWORD, border, FORTRESS, CASTLE). A SHIELD or an empty cell protects it.
 */
void test_isKingCapturedSimple()
{
    runBoardPredicateTests("isKingCapturedSimple", [](const Board& board) { return isKingCapturedSimple(board); },
                           "captured", "not captured");
}

/**
 * @brief Test function for isKingCapturedRecursive.
 *
 * Recursive rule: the group KING + connected SHIELDs is captured if it touches no empty
 * NORMAL cell (all its neighbours are SWORD, border, FORTRESS or CASTLE).
 */
void test_isKingCapturedRecursive()
{
    runBoardPredicateTests("isKingCapturedRecursive", [](const Board& board) { return isKingCapturedRecursive(board); },
                           "captured", "not captured");
}

/**
 * @brief Test function for canCurrentPlayerMove.
 *
 * Test coverage: initial boards, pieces surrounded by pieces, by the castle or by a fortress
 * (forbidden to the swords and shields, allowed to the king), no piece at all, both board sizes,
 * both players.
 */
void test_canCurrentPlayerMove()
{
    // Fields: size | player who must play | board | expected | description
    runTestSection("canCurrentPlayerMove", 4, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        TestBoard storage(size);
        Game game;
        buildGame(game, storage, parseRole(field(row, 1)), field(row, 2));
        const bool expected = parseBool(field(row, 3));

        bool result = canCurrentPlayerMove(game);
        bool success = (result == expected);
        printTestResult(currentTestNum, description, success,
                        expected ? "can move" : "blocked", result ? "can move" : "blocked");
        if (shouldShowBoards(success)) {
            printBoard(game.itsBoard.itsCells, size, "Board (" + field(row, 1) + " to play):");
        }
        return success ? 0 : 1;
    });
}

/**
 * @brief Test function for isGameFinished.
 *
 * Test coverage:
 * - Game in progress: initial boards, pieces present but no end condition
 * - King captured, king escaped to a FORTRESS, no SWORD left
 * - The player who must play cannot move any piece
 * - Edge cases: no KING present with/without SWORDs
 *
 * Note: tests pass with isKingCapturedSimple or isKingCapturedRecursive.
 */
void test_isGameFinished()
{
    // Fields: size | player who must play | board | expected | description
    runTestSection("isGameFinished", 4, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        TestBoard storage(size);
        Game game;
        buildGame(game, storage, parseRole(field(row, 1)), field(row, 2));
        const bool expected = parseBool(field(row, 3));

        bool result = isGameFinished(game);
        bool success = (result == expected);
        printTestResult(currentTestNum, description, success,
                        expected ? "finished" : "not finished", result ? "finished" : "not finished");
        if (shouldShowBoards(success)) {
            printBoard(game.itsBoard.itsCells, size, "Board (" + field(row, 1) + " to play):");
        }
        return success ? 0 : 1;
    });
}

/**
 * @brief Test function for whoWon.
 *
 * Victory rules verified:
 * - DEFENSE wins when the KING reaches a FORTRESS or when all SWORDs are eliminated
 * - ATTACK wins when the KING is captured
 * - The player who must play and cannot move any piece loses
 * - No winner (nullptr) when the game is not finished
 */
void test_whoWon()
{
    // Fields: size | player who must play | board | expected winner (ATTACK, DEFENSE or none) | description
    runTestSection("whoWon", 4, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        TestBoard storage(size);
        Game game;
        buildGame(game, storage, parseRole(field(row, 1)), field(row, 2));

        const string expectedText = field(row, 3);
        const Player* expected = nullptr;
        if (expectedText == "ATTACK") expected = &game.itsPlayer1;
        if (expectedText == "DEFENSE") expected = &game.itsPlayer2;

        const Player* winner = whoWon(game);
        bool success = (winner == expected);

        string expectedStr = expected ? expected->itsName : "nullptr (no winner)";
        string actualStr = (winner == &game.itsPlayer1 || winner == &game.itsPlayer2) ? winner->itsName
                         : (winner == nullptr ? "nullptr (no winner)" : "unknown pointer");
        printTestResult(currentTestNum, description, success, expectedStr, actualStr);
        if (shouldShowBoards(success)) {
            printBoard(game.itsBoard.itsCells, size, "Board (" + field(row, 1) + " to play):");
        }
        return success ? 0 : 1;
    });
}


/**
 * @brief Plays complete games (sequences of moves from the starting position) with playTurn.
 *
 * Each test case (see the [completeGames] section of the test data) gives the board size and
 * the list of moves, the attacker playing first. Every move must be accepted by playTurn, the game
 * must not be finished before the last move, and isGameFinished / whoWon must give the expected
 * result after the last move.
 */
void test_completeGames()
{
    // Fields: size | moves (separated by commas) | expected finished | expected winner | description
    runTestSection("completeGames", 4, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        TestBoard storage(size);
        Game game;
        buildGame(game, storage, ATTACK, "");
        initializeBoard(game.itsBoard);

        const vector<string> moves = split(field(row, 1), ',');
        string problem;
        Move lastMove = {{-1, -1}, {-1, -1}};
        for (size_t i = 0; i < moves.size() && problem.empty(); ++i) {
            if (isGameFinished(game)) {
                problem = "game finished too early, before move " + to_string(i + 1) + " (" + moves[i] + ")";
                break;
            }
            PlayerRole role = (game.itsCurrentPlayer != nullptr) ? game.itsCurrentPlayer->itsRole : ATTACK;
            lastMove = parseMove(moves[i]);
            MoveStatus status = playTurn(game, lastMove);
            if (status != MOVE_OK) {
                problem = "move " + to_string(i + 1) + " (" + moves[i] + ", " + roleName(role)
                          + ") refused: " + moveStatusName(status);
            }
        }

        if (problem.empty()) {
            bool expectedFinished = parseBool(field(row, 2));
            bool finished = isGameFinished(game);
            const Player* winner = whoWon(game);
            string winnerName = (winner == &game.itsPlayer1) ? "ATTACK"
                              : (winner == &game.itsPlayer2) ? "DEFENSE" : "none";
            if (finished != expectedFinished) {
                problem = string("after the last move, game ") + (finished ? "finished" : "not finished");
            } else if (winnerName != field(row, 3)) {
                problem = "winner " + winnerName + " instead of " + field(row, 3);
            }
        }

        bool success = problem.empty();
        printTestResult(currentTestNum, description, success,
                        string(parseBool(field(row, 2)) ? "finished" : "not finished") + ", winner " + field(row, 3),
                        problem);
        if (shouldShowBoards(success)) {
            printBoard(game.itsBoard.itsCells, size, "Board at the end of the test (last move highlighted):",
                       {lastMove.itsStartPosition, lastMove.itsEndPosition});
        }
        return success ? 0 : 1;
    });
}


/**
 * @brief Test function for the moveStatusMessage function.
 *
 * Every MoveStatus value must have a message which is not empty and different from
 * the messages of the other values.
 */
void test_moveStatusMessage()
{
    // Fields: MoveStatus | description
    runTestSection("moveStatusMessage", 1, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const MoveStatus status = parseMoveStatus(field(row, 0));
        const string message = moveStatusMessage(status);

        string problem;
        if (message.empty()) {
            problem = "empty message";
        }
        for (int i = 0; i < MOVE_STATUS_COUNT && problem.empty(); ++i) {
            MoveStatus other = static_cast<MoveStatus>(i);
            if (other != status && moveStatusMessage(other) == message) {
                problem = "same message as " + moveStatusName(other) + ": \"" + message + "\"";
            }
        }
        bool success = problem.empty();
        printTestResult(currentTestNum, description, success, "its own message", problem);
        return success ? 0 : 1;
    });
}

/**
 * @brief Test function for the readLine function (keyboard simulated).
 *
 * Test coverage: one line, spaces inside the line, empty line, only the first line is read,
 * end of input.
 */
void test_readLine()
{
    // Fields: keyboard input | expected ("true" + text, or "false") | description
    runTestSection("readLine", 2, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const string expectedText = field(row, 1);
        const bool expectedResult = parseBool(expectedText);

        string text = "(nothing)";
        bool result = false;
        withKeyboardInput(parseQuoted(field(row, 0)), [&result, &text]() {
            result = readLine("Question ? ", text);
        });

        bool success = (result == expectedResult) && (!result || text == parseQuoted(expectedText.substr(4)));
        printTestResult(currentTestNum, description, success, expectedText,
                        result ? "true \"" + text + "\"" : "false");
        return success ? 0 : 1;
    });
}

/**
 * @brief Test function for the askBoardSize function (keyboard simulated).
 *
 * Test coverage: valid answers, invalid answers asked again, stops at the first valid answer,
 * end of input.
 */
void test_askBoardSize()
{
    // Fields: keyboard input | expected ("true <size>" or "false") | description
    runTestSection("askBoardSize", 2, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const string expectedText = field(row, 1);
        const bool expectedResult = parseBool(expectedText);
        const BoardSize expectedSize = expectedResult ? parseSize(expectedText.substr(5)) : LITTLE;

        BoardSize size = static_cast<BoardSize>(0);
        bool result = false;
        withKeyboardInput(parseQuoted(field(row, 0)), [&result, &size]() {
            result = askBoardSize(size);
        });

        bool success = (result == expectedResult) && (!result || size == expectedSize);
        printTestResult(currentTestNum, description, success,
                        expectedResult ? "true and size " + to_string(expectedSize) : "false",
                        result ? "true and size " + to_string(size) : "false");
        return success ? 0 : 1;
    });
}

/**
 * @brief Test function for the askPosition function (keyboard simulated).
 *
 * Test coverage: valid answers, invalid answers asked again, stops at the first valid answer,
 * position outside the board, end of input, both board sizes.
 */
void test_askPosition()
{
    // Fields: size | keyboard input | expected ("true <cell>" or "false") | description
    runTestSection("askPosition", 3, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        const string expectedText = field(row, 2);
        const bool expectedResult = parseBool(expectedText);
        const Position expectedPos = expectedResult ? parseCell(expectedText.substr(5)) : Position{-1, -1};

        Position position = {-1, -1};
        bool result = false;
        withKeyboardInput(parseQuoted(field(row, 1)), [&result, &position, size]() {
            result = askPosition("Case ? ", position, {nullptr, size});
        });

        bool success = (result == expectedResult) && (!result || (position.itsRow == expectedPos.itsRow
                                                                   && position.itsCol == expectedPos.itsCol));
        printTestResult(currentTestNum, description, success,
                        expectedResult ? "true and " + positionName(expectedPos) : "false",
                        result ? "true and " + positionName(position) : "false");
        return success ? 0 : 1;
    });
}

/**
 * @brief Test function for the askMove function (keyboard simulated).
 *
 * Test coverage: start then end cell, invalid answers asked again, end of input
 * before the end cell, both board sizes.
 */
void test_askMove()
{
    // Fields: size | keyboard input | expected ("true <start> <end>" or "false") | description
    runTestSection("askMove", 3, [](const TestRow& row, int currentTestNum, const string& description) -> int {
        const BoardSize size = parseSize(field(row, 0));
        const string expectedText = field(row, 2);

        Move move = {{-1, -1}, {-1, -1}};
        bool result = false;
        withKeyboardInput(parseQuoted(field(row, 1)), [&result, &move, size]() {
            result = askMove({nullptr, size}, move);
        });

        string actual = result ? "true " + positionName(move.itsStartPosition) + " " + positionName(move.itsEndPosition)
                               : "false";
        bool success = (actual == expectedText);
        printTestResult(currentTestNum, description, success, expectedText, actual);
        return success ? 0 : 1;
    });
}


// ========================================================================================
// ========================== HELPER & UTILITY FUNCTIONS ==================================
// ========================================================================================
// This section contains all helper/utility functions used by the test suite.
//
//   TEST OUTPUT FORMATTING
//      - printTestSuiteHeader() / printTestSuiteFooter() : banners and final summary table
//      - printTestHeader() / printTestSummary()          : per-function header and summary
//      - printTestResult() / printTestException()        : one test result
//      - printBoard() / printBoards()                    : board display used by the tests
//
//   BOARD MEMORY MANAGEMENT
//      - resetBoard()    : reset all cells to {NORMAL, NONE}
//
//   TEST CONFIGURATION
//      - configureTestDisplay()  : enable/disable boards, prompts and passed tests display
// ========================================================================================

// Results of each tested function, displayed in the final summary table
struct FunctionReport {
    string itsName;
    int itsPassed;
    int itsFailed;
    int itsSkipped;
    string itsMissing;  // Not implemented functions met by the tests
};
static const int MAX_REPORTS = 32;
static FunctionReport G_REPORTS[MAX_REPORTS];
static int G_REPORT_COUNT = 0;

/**
 * @brief Resets all cells of a board to {NORMAL, NONE}.
 * @param aBoard The board to reset
 * @param aBoardSize The size of the board
 */
void resetBoard(Cell**& aBoard, const BoardSize& aBoardSize)
{
    for (int i = 0; i < aBoardSize; ++i) {
        for (int j = 0; j < aBoardSize; ++j) {
            aBoard[i][j] = {NORMAL, NONE};
        }
    }
}

/**
 * @brief Name of a cell in the game notation (row letter + column number), e.g. (2,5) → "C6".
 */
string cellName(int row, int col) {
    return string(1, static_cast<char>('A' + row)) + to_string(col + 1);
}

/**
 * @brief Name of a piece type, e.g. SHIELD → "SHIELD".
 */
string pieceName(PieceType piece) {
    switch (piece) {
        case NONE:   return "NONE";
        case SHIELD: return "SHIELD";
        case SWORD:  return "SWORD";
        case KING:   return "KING";
    }
    return "?";
}

// Background colors of the special cells
static const string BG_FORTRESS = "\033[44m";
static const string BG_CASTLE = "\033[45m";

/**
 * @brief Builds the lines of a board drawn as a grid (independent from the student's displayBoard).
 *
 * Columns and rows are separated by lines, column numbers and row letters are displayed around the grid.
 * Pieces: X sword (red), O shield (white), K king (yellow). Special cells have a colored
 * background: fortress (blue, F when empty), castle (magenta, C when empty).
 * The highlighted cells (the cells checked by the test) are displayed between brackets: [X].
 *
 * @param view The board to draw (its cells may be nullptr)
 * @return The lines (title first), with color codes
 */
static vector<string> boardLines(const BoardView& view) {
    vector<string> lines;
    lines.push_back(string(COLOR_BOLD) + view.itsTitle + COLOR_RESET);
    if (view.itsCells == nullptr) {
        lines.push_back("(nullptr board)");
        return lines;
    }
    const int size = view.itsSize;

    // Column numbers, centered above each cell
    string header = "  ";
    for (int c = 1; c <= size; ++c) {
        header += (c < 10 ? "  " : " ") + to_string(c) + " ";
    }
    lines.push_back(header);

    // Horizontal line of the grid: left, middle and right characters
    auto border = [size](const string& left, const string& middle, const string& right) {
        string line = "  " + left;
        for (int c = 0; c < size; ++c) line += "───" + (c < size - 1 ? middle : right);
        return line;
    };

    lines.push_back(border("┌", "┬", "┐"));
    for (int r = 0; r < size; ++r) {
        string line = string(1, static_cast<char>('A' + r)) + " │";
        for (int c = 0; c < size; ++c) {
            const Cell& cell = view.itsCells[r][c];

            bool isHighlighted = false;
            for (const Position& pos : view.itsHighlights) {
                if (pos.itsRow == r && pos.itsCol == c) isHighlighted = true;
            }

            // Symbol and color of the piece (or of the empty special cell)
            string symbol = " ";
            string color;
            switch (cell.itsPieceType) {
                case SWORD:  symbol = "X"; color = string(COLOR_BOLD) + COLOR_RED; break;
                case SHIELD: symbol = "O"; color = COLOR_BOLD; break;
                case KING:   symbol = "K"; color = string(COLOR_BOLD) + COLOR_YELLOW; break;
                case NONE:
                    if (cell.itsCellType == FORTRESS) symbol = "F";
                    if (cell.itsCellType == CASTLE) symbol = "C";
                    break;
            }
            const string background = (cell.itsCellType == FORTRESS) ? BG_FORTRESS
                                    : (cell.itsCellType == CASTLE) ? BG_CASTLE : "";

            line += background + (isHighlighted ? string(COLOR_CYAN) + "[" : " ") + COLOR_RESET
                  + background + color + symbol + COLOR_RESET
                  + background + (isHighlighted ? string(COLOR_CYAN) + "]" : " ") + COLOR_RESET + "│";
        }
        lines.push_back(line);
        if (r < size - 1) lines.push_back(border("├", "┼", "┤"));
    }
    lines.push_back(border("└", "┴", "┘"));
    return lines;
}

/**
 * @brief Number of characters displayed by a line (color codes ignored, UTF-8 characters counted once).
 */
static size_t visibleWidth(const string& line) {
    size_t width = 0;
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '\033') {
            while (i < line.size() && line[i] != 'm') ++i;   // Skip the color code
        } else if ((static_cast<unsigned char>(line[i]) & 0xC0) != 0x80) {
            ++width;                                          // First byte of a character
        }
    }
    return width;
}

/**
 * @brief Width of the terminal in characters (120 if it cannot be known, e.g. output redirected).
 */
static int terminalWidth() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        return info.srWindow.Right - info.srWindow.Left + 1;
    }
#else
    struct winsize size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0) {
        return size.ws_col;
    }
#endif
    const char* columns = getenv("COLUMNS");
    if (columns != nullptr && atoi(columns) > 0) {
        return atoi(columns);
    }
    return 120;
}

/**
 * @brief Displays several boards (e.g. before / after a move), side by side if the terminal is
 *        wide enough, one under the other otherwise. The legend is displayed once, below.
 * @param views The boards to display
 */
void printBoards(const vector<BoardView>& views) {
    if (G_NOT_IMPLEMENTED_INDEX != -1 || views.empty()) return;   // Skipped test: nothing to show
    TestOutput output;

    const string MARGIN = "      ";
    const string GAP = "    ";

    vector<vector<string>> blocks;
    vector<size_t> widths;
    size_t totalWidth = MARGIN.size();
    bool hasHighlights = false;
    for (const BoardView& view : views) {
        blocks.push_back(boardLines(view));
        size_t width = 0;
        for (const string& line : blocks.back()) width = max(width, visibleWidth(line));
        widths.push_back(width);
        totalWidth += width + (blocks.size() > 1 ? GAP.size() : 0);
        if (!view.itsHighlights.empty()) hasHighlights = true;
    }

    if (totalWidth <= static_cast<size_t>(terminalWidth())) {
        // Side by side: line i of every board on the same line
        size_t height = 0;
        for (const vector<string>& block : blocks) height = max(height, block.size());
        for (size_t i = 0; i < height; ++i) {
            string line = MARGIN;
            for (size_t b = 0; b < blocks.size(); ++b) {
                const string part = (i < blocks[b].size()) ? blocks[b][i] : "";
                line += part;
                if (b < blocks.size() - 1) line += string(widths[b] - visibleWidth(part), ' ') + GAP;
            }
            cout << line << endl;
        }
    } else {
        // Terminal too narrow: one board under the other
        for (const vector<string>& block : blocks) {
            for (const string& line : block) cout << MARGIN << line << endl;
        }
    }

    // Legend (once for all the boards)
    cout << MARGIN << "  " << COLOR_BOLD << COLOR_RED << "X" << COLOR_RESET << " sword  "
         << COLOR_BOLD << "O" << COLOR_RESET << " shield  "
         << COLOR_BOLD << COLOR_YELLOW << "K" << COLOR_RESET << " king  "
         << BG_FORTRESS << "F" << COLOR_RESET << " fortress  "
         << BG_CASTLE << "C" << COLOR_RESET << " castle";
    if (hasHighlights) {
        cout << "  " << COLOR_CYAN << "[ ]" << COLOR_RESET << " cells checked by the test";
    }
    cout << endl;
    if (!DISPLAY_BOARDS) {
        cout << MARGIN << "  (boards are shown for the first failure only: SHOW_TEST_BOARDS = true in main.cpp shows all of them)" << endl;
    }
    cout << endl;
}

/**
 * @brief Displays one board (see printBoards).
 * @param aBoard The cells to display (may be nullptr)
 * @param aSize Number of rows and columns
 * @param title Title displayed above the board
 * @param highlights Cells to highlight
 */
void printBoard(Cell** aBoard, int aSize, const string& title, const vector<Position>& highlights) {
    printBoards({{aBoard, aSize, title, highlights}});
}

/**
 * @brief Configures the display settings for tests.
 * @param showBoards Whether to display the boards used by the tests
 * @param showPrompts Whether to display the messages printed by the tested functions
 * @param showPassedTests Whether to list the passed tests (false: only failures are listed)
 */
void configureTestDisplay(bool showBoards, bool showPrompts, bool showPassedTests) {
    DISPLAY_BOARDS = showBoards;
    DISPLAY_PROMPTS = showPrompts;
    DISPLAY_PASSED = showPassedTests;
}

/**
 * @brief Prints the main test suite header.
 * Displays a stylized header at the start of the test suite execution.
 */
void printTestSuiteHeader() {
    G_REAL_COUT_BUF = cout.rdbuf();

    cout << COLOR_BOLD << COLOR_CYAN << endl;
    cout << "╔════════════════════════════════════════════════════╗" << endl;
    cout << "║          HNEFATAFL - TEST SUITE EXECUTION          ║" << endl;
    cout << "╚════════════════════════════════════════════════════╝" << endl;
    cout << COLOR_RESET << endl;

    // Reset global counters at the start of a run
    G_TOTAL_PASSED = 0;
    G_TOTAL_FAILED = 0;
    G_TOTAL_SKIPPED = 0;
    G_REPORT_COUNT = 0;
}

/**
 * @brief Prints the main test suite footer.
 * Displays a summary table (one line per tested function) and the mini report.
 */
void printTestSuiteFooter() {
    cout << COLOR_BOLD << COLOR_CYAN << endl;
    cout << "╔════════════════════════════════════════════════════╗" << endl;
    cout << "║                TEST SUITE COMPLETED                ║" << endl;
    cout << "╚════════════════════════════════════════════════════╝" << endl;
    cout << COLOR_RESET << endl;

    // Summary table: one line per tested function
    const int NAME_WIDTH = 26;
    for (int i = 0; i < G_REPORT_COUNT; ++i) {
        const FunctionReport& r = G_REPORTS[i];
        string padding(max(1, NAME_WIDTH - static_cast<int>(r.itsName.size())), ' ');
        int total = r.itsPassed + r.itsFailed + r.itsSkipped;

        if (r.itsFailed > 0) {
            cout << COLOR_RED << "  ✗ " << r.itsName << padding << r.itsFailed << "/" << total << " failed";
        } else if (r.itsSkipped == total && r.itsMissing.find(r.itsName) == string::npos) {
            cout << COLOR_YELLOW << "  ⏳ " << r.itsName << padding << "not tested yet";
        } else if (r.itsSkipped == total) {
            cout << COLOR_YELLOW << "  ⏳ " << r.itsName << padding << "not implemented";
        } else if (r.itsSkipped > 0) {
            cout << COLOR_YELLOW << "  ⏳ " << r.itsName << padding << r.itsPassed << "/" << total << " passed";
        } else {
            cout << COLOR_GREEN << "  ✓ " << r.itsName << padding << r.itsPassed << "/" << total << " passed";
        }
        if (r.itsSkipped > 0 && r.itsSkipped < total) {
            cout << ", " << r.itsSkipped << " skipped";
        }
        // Other missing functions needed by the tests of this function
        string needed;
        size_t start = 0;
        while (start < r.itsMissing.size()) {
            size_t end = r.itsMissing.find(", ", start);
            if (end == string::npos) end = r.itsMissing.size();
            string name = r.itsMissing.substr(start, end - start);
            if (name != r.itsName) needed += (needed.empty() ? "" : ", ") + name;
            start = end + 2;
        }
        if (!needed.empty()) {
            cout << " (needs " << needed << ")";
        }
        cout << COLOR_RESET << endl;
    }
    cout << endl;

    // Mini auto-report: aggregate totals across all suites
    int total = G_TOTAL_PASSED + G_TOTAL_FAILED + G_TOTAL_SKIPPED;
    cout << COLOR_BOLD << "Mini report: " << COLOR_RESET
         << "Total: " << total << ", "
         << COLOR_GREEN << "Passed: " << G_TOTAL_PASSED << COLOR_RESET << ", ";
    if (G_TOTAL_FAILED > 0) {
        cout << COLOR_RED << "Failed: " << G_TOTAL_FAILED << COLOR_RESET;
    } else {
        cout << "Failed: 0";
    }
    if (G_TOTAL_SKIPPED > 0) {
        cout << ", " << COLOR_YELLOW << "Not implemented: " << G_TOTAL_SKIPPED << COLOR_RESET;
    }
    cout << endl;

    // Overall status line
    if (G_TOTAL_FAILED == 0 && G_TOTAL_SKIPPED == 0) {
        cout << COLOR_GREEN << "✓ All activated tests executed successfully!" << COLOR_RESET << endl;
    } else if (G_TOTAL_FAILED == 0) {
        cout << COLOR_GREEN << "✓ No failed test." << COLOR_RESET
             << COLOR_YELLOW << " Some functions are not implemented yet (⏳)." << COLOR_RESET << endl;
    } else {
        cout << COLOR_YELLOW << "⚠ Some tests failed. Check the details above." << COLOR_RESET << endl;
    }
    cout << COLOR_YELLOW << "ℹ  Display settings at the beginning of main.cpp: SHOW_PASSED_TESTS, SHOW_TEST_BOARDS, SHOW_TEST_PROMPTS" << COLOR_RESET << endl;
    cout << COLOR_YELLOW << "ℹ  To play a game instead of running the tests: PLAY_GAME = true at the beginning of main.cpp" << COLOR_RESET << endl << endl;
}

/**
 * @brief Prints a test header with formatting.
 * @param testName Name of the test function
 */
void printTestHeader(const string& testName) {
    if (G_REAL_COUT_BUF == nullptr) G_REAL_COUT_BUF = cout.rdbuf();
    cout << COLOR_CYAN << "\n┌─ " << testName << " ─┐" << COLOR_RESET << endl;

    // Reset "not implemented" information of the section
    G_SECTION_NAME = testName;
    G_SECTION_SKIPPED = 0;
    for (int i = 0; i < KNOWN_FUNCTIONS_COUNT; ++i) {
        G_SECTION_MISSING[i] = false;
    }
}

/**
 * @brief Prints a test result with formatting and colors.
 * Nothing is printed if the test met a not implemented function (it is reported as skipped).
 * @param testNum Test number
 * @param description Description of the test
 * @param passed Whether the test passed
 * @param expected Expected value (optional)
 * @param actual Actual value (optional)
 */
void printTestResult(int testNum, const string& description, bool passed,
                     const string& expected, const string& actual) {
    // Messages printed by the tested functions: shown only if this test is shown
    flushPrompts(DISPLAY_PROMPTS && G_NOT_IMPLEMENTED_INDEX == -1 && (!passed || DISPLAY_PASSED));
    if (G_NOT_IMPLEMENTED_INDEX != -1) return;   // Skipped test: reported in the summary
    if (passed && !DISPLAY_PASSED) return;        // Compact mode: only failures are listed

    TestOutput output;
    cout << "  " << COLOR_CYAN << "[Test " << testNum << "]" << COLOR_RESET << " ";
    if (passed) {
        cout << COLOR_GREEN << description << ": ✓ PASS" << COLOR_RESET << endl;
    } else {
        cout << COLOR_RED << description << ": ✗ FAIL" << COLOR_RESET;
        if (!expected.empty()) {
            cout << " (Expected: " << expected << ", Got: " << actual << ")";
        }
        cout << endl;
    }
}

/**
 * @brief Prints a test summary with formatting.
 * @param testName Name of the test
 * @param passed Number of tests passed
 * @param failed Number of tests failed
 */
void printTestSummary(const string& testName, int passed, int failed) {
    // List of the not implemented functions met by the tests of this section
    string missing;
    for (int i = 0; i < KNOWN_FUNCTIONS_COUNT; ++i) {
        if (G_SECTION_MISSING[i]) {
            missing += (missing.empty() ? "" : ", ") + string(KNOWN_FUNCTIONS[i]);
        }
    }

    if (G_SECTION_SKIPPED > 0) {
        cout << COLOR_YELLOW << "  ⏳ " << G_SECTION_SKIPPED << " test(s) skipped: not implemented yet ("
             << missing << ")" << COLOR_RESET << endl;
    }

    cout << COLOR_CYAN << "└─ " << testName << " Summary: " << COLOR_RESET;
    cout << COLOR_GREEN << passed << " passed" << COLOR_RESET << ", ";
    if (failed > 0) {
        cout << COLOR_RED << failed << " failed" << COLOR_RESET;
    } else {
        cout << failed << " failed";
    }
    if (G_SECTION_SKIPPED > 0) {
        cout << ", " << COLOR_YELLOW << G_SECTION_SKIPPED << " not implemented" << COLOR_RESET;
    }
    cout << endl;

    // Aggregate into global counters and the summary table
    G_TOTAL_PASSED += passed;
    G_TOTAL_FAILED += failed;
    G_TOTAL_SKIPPED += G_SECTION_SKIPPED;
    if (G_REPORT_COUNT < MAX_REPORTS) {
        G_REPORTS[G_REPORT_COUNT++] = {testName, passed, failed, G_SECTION_SKIPPED, missing};
    }
}

/**
 * @brief Prints an exception, a crash or a timeout that occurred during a test.
 * @param testNum The test number
 * @param description The test description
 * @param exceptionMsg The explanation of the problem
 */
void printTestException(int testNum, const string& description, const string& exceptionMsg) {
    TestOutput output;
    cout << "  " << COLOR_CYAN << "[Test " << testNum << "]" << COLOR_RESET << " ";
    cout << COLOR_RED << description << ": ✗ FAIL (" << exceptionMsg << ")" << COLOR_RESET << endl;
}
