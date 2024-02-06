#ifndef ENVPOOL_YGOPRO_PLAYER_H_
#define ENVPOOL_YGOPRO_PLAYER_H_

#include <string>
#include <vector>


namespace ygopro {
using PlayerId = uint8_t;

class Player {
  friend class YGOProEnv;

protected:
  const std::string nickname_;
  const int init_lp_;
  const PlayerId duel_player_;
  const bool verbose_;

  bool seen_waiting_ = false;

public:
  Player(const std::string &nickname, int init_lp, PlayerId duel_player,
         bool verbose = false);
  virtual ~Player() = default;

  void notify(const std::string &text) const;

  const int &init_lp() const;

  const std::string &nickname() const;

  virtual int think(const std::vector<std::string> &options) = 0;
};

class GreedyAI : public Player {
public:
  GreedyAI(const std::string &nickname, int init_lp, PlayerId duel_player,
           bool verbose = false);
  int think(const std::vector<std::string> &options) override;
};

class HumanPlayer : public Player {
public:
  HumanPlayer(const std::string &nickname, int init_lp, PlayerId duel_player,
              bool verbose = false);
  int think(const std::vector<std::string> &options) override;
};

}  // namespace ygopro

#endif  // ENVPOOL_YGOPRO_PLAYER_H_