#include <iostream>
#include <string>
#include <array>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <csignal>
#include <cstdlib>

// Manage terminal raw mode and screen buffers
struct TerminalSession {
    static inline termios orig_termios;

    static void restore() {
        // Reset colors, restore cursor, return to primary screen buffer
        std::cout << "\033[0m\033[?25h\033[?1049l" << std::flush;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }

    static void signal_handler(int) {
        restore();
        std::exit(0);
    }

    static void init() {
        tcgetattr(STDIN_FILENO, &orig_termios);
        std::atexit(restore);
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        termios raw = orig_termios;
        // Disable ECHO, canonical mode, and default signal interrupts
        raw.c_lflag &= ~(ECHO | ICANON | ISIG);
        // Disable software flow control (Ctrl+S, Ctrl+Q)
        raw.c_iflag &= ~(IXON | ICRNL);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

        // Switch to alternate screen buffer, clear screen, and hide cursor
        std::cout << "\033[?1049h\033[2J\033[H\033[?25l" << std::flush;
    }
};

// Non-blocking key parser supporting Vim keys, arrows, ESC, and Ctrl+C
char read_key() {
    char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) return 0;

    // Handle ESC and ANSI escape sequences
    if (c == '\033') {
        struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
        // Check if additional bytes follow immediately (within 25ms)
        if (poll(&pfd, 1, 25) > 0) {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) > 0 && read(STDIN_FILENO, &seq[1], 1) > 0) {
                if (seq[0] == '[') {
                    switch (seq[1]) {
                        case 'A': return 'k'; // Up arrow -> k
                        case 'B': return 'j'; // Down arrow -> j
                        case 'C': return 'l'; // Right arrow -> l
                        case 'D': return 'h'; // Left arrow -> h
                    }
                }
            }
        }
        return 'q'; // Standalone ESC key triggers exit
    }
    return c;
}

const int GRID_SIZE = 3;
const int CARD_WIDTH = 14;

// 3x3 menu items
const std::array<std::array<std::string, GRID_SIZE>, GRID_SIZE> MENU_ITEMS = {{
    { "Terminal", "Editor",  "Files"    },
    { "Network",  "System",  "Settings" },
    { "Blender",  "Music",   "About"    }
}};

// Render monochrome 3x3 grid
void render_ui(int sel_r, int sel_c, const std::string& status_msg) {
    // Move cursor to home position for flicker-free redrawing
    std::string out = "\033[H";

    // Title banner
    out += "\033[1m=== MINIMAL TUI MENU (3x3) ===\033[0m\n\n";

    // Build the grid
    for (int r = 0; r < GRID_SIZE; ++r) {
        std::string top_line = "  ";
        std::string mid_line = "  ";
        std::string bot_line = "  ";

        for (int c = 0; c < GRID_SIZE; ++c) {
            bool is_selected = (r == sel_r && c == sel_c);
            const std::string& label = MENU_ITEMS[r][c];
            
            int pad_total = CARD_WIDTH - static_cast<int>(label.length());
            int pad_left = pad_total / 2;
            int pad_right = pad_total - pad_left;

            if (is_selected) {
                // Highlighted active box: double borders and inverted text colors
                top_line += "\033[1m╔";
                for (int i = 0; i < CARD_WIDTH; ++i) top_line += "═";
                top_line += "╗\033[0m ";

                mid_line += "\033[1m║\033[7m";
                for (int i = 0; i < pad_left; ++i) mid_line += " ";
                mid_line += label;
                for (int i = 0; i < pad_right; ++i) mid_line += " ";
                mid_line += "\033[27m║\033[0m ";

                bot_line += "\033[1m╚";
                for (int i = 0; i < CARD_WIDTH; ++i) bot_line += "═";
                bot_line += "╝\033[0m ";
            } else {
                // Normal inactive box: single border lines
                top_line += "┌";
                for (int i = 0; i < CARD_WIDTH; ++i) top_line += "─";
                top_line += "┐ ";

                mid_line += "│";
                for (int i = 0; i < pad_left; ++i) mid_line += " ";
                mid_line += label;
                for (int i = 0; i < pad_right; ++i) mid_line += " ";
                mid_line += "│ ";

                bot_line += "└";
                for (int i = 0; i < CARD_WIDTH; ++i) bot_line += "─";
                bot_line += "┘ ";
            }
        }
        out += top_line + "\n" + mid_line + "\n" + bot_line + "\n\n";
    }

    // Help & keybindings
    out += "\033[2mNavigation: [h] Left  [j] Down  [k] Up  [l] Right (or Arrows)\033[0m\n";
    out += "\033[2mAction:     [Enter / Space] Select    [q / ESC / Ctrl+C] Quit\033[0m\n\n";

    if (!status_msg.empty()) {
        out += "\033[1;37m> " + status_msg + "\033[0m\n";
    } else {
        out += "\033[2m> Ready.\033[0m\n";
    }

    std::cout << out << std::flush;
}

int main() {
    TerminalSession::init();

    int sel_r = 0;
    int sel_c = 0;
    std::string status_msg = "";

    while (true) {
        render_ui(sel_r, sel_c, status_msg);

        char key = read_key();

        // Exit on 'q', ESC, or Ctrl+C (ASCII 3)
        if (key == 'q' || key == 3) {
            break;
        }

        switch (key) {
            case 'h': // Move left
                if (sel_c > 0) sel_c--;
                status_msg = "";
                break;
            case 'l': // Move right
                if (sel_c < GRID_SIZE - 1) sel_c++;
                status_msg = "";
                break;
            case 'k': // Move up
                if (sel_r > 0) sel_r--;
                status_msg = "";
                break;
            case 'j': // Move down
                if (sel_r < GRID_SIZE - 1) sel_r++;
                status_msg = "";
                break;
            case '\r': // Enter
            case '\n':
            case ' ':  // Space
                status_msg = "Selected: [" + MENU_ITEMS[sel_r][sel_c] + 
                             "] at (" + std::to_string(sel_r) + ", " + std::to_string(sel_c) + ")";
                break;
            default:
                break;
        }
    }

    return 0;
}
