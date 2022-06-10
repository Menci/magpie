#include "Ensure.h"

#include <cstring>

#include "TerminalColor/TerminalColor.h"

void _ensure(bool success, const char *file, int line, const char *expr) {
    if (success) return;
    std::cerr << TerminalColor::ForegroundRed << TerminalColor::Bold
              << "Error: "
              << TerminalColor::Reset
              << " on "
              << TerminalColor::ForegroundYellow
              << expr
              << TerminalColor::Reset
              << " at "
              << TerminalColor::Bold
              << file << ":" << line
              << TerminalColor::Reset
              << std::endl;
    exit(EXIT_FAILURE);
}

void _ensure_errno(int err, const char *file, int line, const char *expr) {
    if (err == 0) return;
    std::cerr << TerminalColor::ForegroundRed << TerminalColor::Bold
              << "Error: "
              << TerminalColor::Reset << TerminalColor::Bold
              << strerror(err)
              << TerminalColor::Reset
              << " on "
              << TerminalColor::ForegroundYellow
              << expr
              << TerminalColor::Reset
              << " at "
              << TerminalColor::Bold
              << file << ":" << line
              << TerminalColor::Reset
              << std::endl;
    exit(EXIT_FAILURE);
}

void _error(const char *message, const char *file, int line) {
    std::cerr << TerminalColor::ForegroundRed << TerminalColor::Bold
              << "Error: "
              << TerminalColor::Reset << TerminalColor::Bold
              << message
              << TerminalColor::Reset
              << " at "
              << TerminalColor::Bold
              << file << ":" << line
              << TerminalColor::Reset
              << std::endl;
    exit(EXIT_FAILURE);
}
