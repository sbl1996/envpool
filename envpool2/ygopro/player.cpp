#include <algorithm>

#include "envpool2/ygopro/player.h"


namespace ygopro {

inline std::string getline() {
    char *line = nullptr;
    size_t len = 0;
    ssize_t read;

    read = getline(&line, &len, stdin);

    if (read != -1) {
        // Remove line ending character(s)
        if (line[read - 1] == '\n')
            line[read - 1] = '\0';  // Replace newline character with null terminator
        else if (line[read - 2] == '\r' && line[read - 1] == '\n') {
            line[read - 2] = '\0';  // Replace carriage return and newline characters with null terminator
            line[read - 1] = '\0';
        }

        std::string input(line);
        free(line);
        return input;
    }

    free(line);
    return "";
}

Player::Player(const std::string &nickname, int init_lp, PlayerId duel_player,
        bool verbose)
    : nickname_(nickname), init_lp_(init_lp), duel_player_(duel_player),
    verbose_(verbose) {}

void Player::notify(const std::string &text) const {
    if (verbose_) {
        printf("%d %s\n", duel_player_, text.c_str());
    }
}

const int &Player::init_lp() const { return init_lp_; }

const std::string &Player::nickname() const { return nickname_; }


GreedyAI::GreedyAI(const std::string &nickname, int init_lp, PlayerId duel_player,
        bool verbose)
    : Player(nickname, init_lp, duel_player, verbose) {}
int GreedyAI::think(const std::vector<std::string> &options) { return 0; }

HumanPlayer::HumanPlayer(const std::string &nickname, int init_lp, PlayerId duel_player,
        bool verbose)
    : Player(nickname, init_lp, duel_player, verbose) {}
int HumanPlayer::think(const std::vector<std::string> &options) {
    while (true) {
      std::string input = getline();
      if (input == "quit") {
        exit(0);
      }
      // check if option in options
      auto it = std::find(options.begin(), options.end(), input);
      if (it != options.end()) {
        return std::distance(options.begin(), it);
      } else {
        printf("Choose from");
        for (const auto &option : options) {
          printf(" %s", option.c_str());
        }
        printf("\n");
      }
    }
  }
} // namespace ygopro



