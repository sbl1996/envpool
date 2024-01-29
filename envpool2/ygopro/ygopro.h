/*
 * Copyright 2021 Garena Online Private Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ENVPOOL_YGOPRO_YGOPRO_H_
#define ENVPOOL_YGOPRO_YGOPRO_H_

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <functional>
#include <limits>
#include <random>
#include <string>
#include <unordered_map>

#include "envpool2/core/async_envpool.h"
#include "envpool2/core/env.h"

#include "ygopro-core/common.h"
#include "ygopro-core/card_data.h"
#include "ygopro-core/field.h"
#include "ygopro-core/ocgapi.h"

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>
#include <vector>

namespace ygopro {

static SQLite::Database *db = nullptr;
static std::unordered_map<uint32_t, card_data> cards;

inline uint32 card_reader_callback(uint32 code, card_data *card) {
  SQLite::Statement query(*db, "SELECT * FROM datas WHERE id=?");
  query.bind(1, code);
  query.executeStep();
  card->code = code;
  card->alias = query.getColumn("alias");
  card->setcode = query.getColumn("setcode").getInt64();
  card->type = query.getColumn("type");
  uint32_t level_ = query.getColumn("level");
  card->level = level_ & 0xff;
  card->lscale = (level_ >> 24) & 0xff;
  card->rscale = (level_ >> 16) & 0xff;
  card->attack = query.getColumn("atk");
  card->defense = query.getColumn("def");
  if (card->type & TYPE_LINK) {
    card->link_marker = card->defense;
    card->defense = 0;
  } else {
    card->link_marker = 0;
  }
  card->race = query.getColumn("race");
  card->attribute = query.getColumn("attribute");
  return 0;
}

inline byte *script_reader_callback(const char *name, int *lenptr) {
  std::ifstream file(name, std::ios::binary);
  if (!file) {
    return nullptr;
  }
  file.seekg(0, std::ios::end);
  int len = file.tellg();
  file.seekg(0, std::ios::beg);
  byte *buf = new byte[len];
  file.read((char *)buf, len);
  *lenptr = len;
  return buf;
}

inline card_data query_card_from_db(uint32_t code) {
  SQLite::Statement query(*db, "SELECT * FROM datas WHERE id=?");
  query.bind(1, code);
  query.executeStep();
  card_data card;
  card.code = code;
  card.alias = query.getColumn("alias");
  card.setcode = query.getColumn("setcode").getInt64();
  card.type = query.getColumn("type");
  uint32_t level_ = query.getColumn("level");
  card.level = level_ & 0xff;
  card.lscale = (level_ >> 24) & 0xff;
  card.rscale = (level_ >> 16) & 0xff;
  card.attack = query.getColumn("atk");
  card.defense = query.getColumn("def");
  if (card.type & TYPE_LINK) {
    card.link_marker = card.defense;
    card.defense = 0;
  } else {
    card.link_marker = 0;
  }
  card.race = query.getColumn("race");
  card.attribute = query.getColumn("attribute");
  return card;
}

inline const card_data &get_card_from_db(uint32_t code) {
  // if not found, read from db and cache it
  auto it = cards.find(code);
  if (it == cards.end()) {
    cards[code] = query_card_from_db(code);
    return cards[code];
  } else {
    return it->second;
  }
}

inline std::vector<uint32> read_deck(const std::string &fp) {
  std::ifstream file(fp);
  std::string line;
  std::vector<uint32> deck;

  if (file.is_open()) {
    while (std::getline(file, line)) {
      if (line.find("side") != std::string::npos) {
        break;
      }
      // Check if line contains only digits
      if (std::all_of(line.begin(), line.end(), ::isdigit)) {
        deck.push_back(std::stoul(line));
      }
    }
    file.close();
  } else {
    printf("Unable to open deck file\n");
  }

  return deck;
}

class YGOProEnvFns {
public:
  static decltype(auto) DefaultConfig() {
    return MakeDict("db_path"_.Bind(std::string("vendor/locale/en/cards.db")),
                    "deck1"_.Bind(std::string("vendor/decks/OldSchool.ydk")),
                    "deck2"_.Bind(std::string("vendor/decks/OldSchool.ydk")),
                    "player"_.Bind(-1), "play"_.Bind(false), "verbose"_.Bind(false)
                    );
  }
  template <typename Config>
  static decltype(auto) StateSpec(const Config &conf) {
    float fmax = std::numeric_limits<float>::max();
    return MakeDict(
        "obs:global_"_.Bind(Spec<float>({4})),
        "info:num_options"_.Bind(Spec<int>({}, {0, 100})));
  }
  template <typename Config>
  static decltype(auto) ActionSpec(const Config &conf) {
    return MakeDict("action"_.Bind(Spec<int>({}, {0, 100})));
  }
};

class Player {
protected:
  const std::string nickname_;
  const int init_lp_;
  const int duel_player_;
  const bool verbose_;

  bool seen_waiting_ = false;

public:
  Player(const std::string &nickname, int init_lp, int duel_player, bool verbose = false)
      : nickname_(nickname), init_lp_(init_lp), duel_player_(duel_player), verbose_(verbose){}
  virtual ~Player() = default;

  void notify(const std::string &text) {
    if (verbose_) {
      printf("%d %s\n", duel_player_, text.c_str());
    }
  }

  const int &init_lp() const { return init_lp_; }

  virtual int think(const std::vector<std::string> &options) = 0;


};

class GreedyAI : public Player {
protected:

public:
  GreedyAI(const std::string &nickname, int init_lp, int duel_player, bool verbose = false)
      : Player(nickname, init_lp, duel_player, verbose) {}

  int think(const std::vector<std::string> &options) override {
    return 0;
  }
};

class HumanPlayer : public Player {
protected:

public:
  HumanPlayer(const std::string &nickname, int init_lp, int duel_player, bool verbose = false)
      : Player(nickname, init_lp, duel_player, verbose) {}

  int think(const std::vector<std::string> &options) override {
    while (true) {
      std::string input;
      std::getline(std::cin, input);
      // check if option in options
      auto it = std::find(options.begin(), options.end(), input);
      if (it != options.end()) {
        return std::distance(options.begin(), it);
      }
      else {
        printf("Choose from");
        for (const auto &option : options) {
          printf(" %s", option.c_str());
        }
        printf("\n");
      }
    }
  }
};

using YGOProEnvSpec = EnvSpec<YGOProEnvFns>;

class YGOProEnv : public Env<YGOProEnvSpec> {
protected:
  std::vector<uint32> deck1_;
  std::vector<uint32> deck2_;
  int player_;
  bool play_ = false;
  bool verbose_ = false;

  int max_episode_steps_, elapsed_step_;
  intptr_t pduel_;
  int ai_player_;
  Player *players_[2];
  std::uniform_real_distribution<> dist_;
  std::uniform_int_distribution<uint64_t> dist_int_;
  bool done_{true};
  bool duel_started_{false};

  int msg_;
  std::vector<std::string> options_;
  int to_decide_;
  std::function<void(int)> callback_;

  byte data_[4096];
  int dp_ = 0;
  int dl_ = 0;
  uint32_t res_;

public:
  YGOProEnv(const Spec &spec, int env_id)
      : Env<YGOProEnvSpec>(spec, env_id),
        max_episode_steps_(spec.config["max_episode_steps"_]),
        elapsed_step_(max_episode_steps_ + 1), dist_int_(0, 0xffffffff),
        deck1_(read_deck(spec.config["deck1"_])), deck2_(read_deck(spec.config["deck2"_])),
        player_(spec.config["player"_]), play_(spec.config["play"_]), verbose_(spec.config["verbose"_]) {
    SQLite::Database *old_db = db;
    db = new SQLite::Database(spec.config["db_path"_], SQLite::OPEN_READONLY);
    set_card_reader(card_reader_callback);
    set_script_reader(script_reader_callback);
    if (old_db != nullptr) {
      delete old_db;
    }
    if (verbose_) {
      std::cout << "Loaded " << deck1_.size() << " cards in deck 1" << std::endl;
      std::cout << "Loaded " << deck2_.size() << " cards in deck 2" << std::endl;
    }
  }

  bool IsDone() override { return done_; }

  void Reset() override {
    if (player_ == -1) {
      ai_player_ = dist_int_(gen_) & 2;
    }
    else {
      ai_player_ = player_;
    }

    unsigned long duel_seed = dist_int_(gen_);
    pduel_ = create_duel(duel_seed);
    for (int i = 0; i < 2; i++) {
      if (players_[i] != nullptr) {
        delete players_[i];
      }
      std::string nickname = i == 0 ? "Alice" : "Bob";
      if (i != ai_player_ && play_) {
        players_[i] = new HumanPlayer(nickname, 8000, i, verbose_);
      }
      else {
        players_[i] = new GreedyAI(nickname, 8000, i, verbose_);
      }
      set_player_info(pduel_, i, 8000, 5, 1);
      load_deck(i == 0 ? deck1_ : deck2_, i);
    }

    // rules = 1, Traditional
    // rules = 0, Default
    // rules = 4, Link
    // rules = 5, MR5
    int32_t rules = 5;
    int32_t options = ((rules & 0xFF) << 16) + (0 & 0xFFFF);
    start_duel(pduel_, options);
    duel_started_ = true;

    next(true);

    done_ = false;
    elapsed_step_ = 0;
    WriteState(0.0);
  }

  void Step(const Action &action) override {
    done_ = (++elapsed_step_ >= max_episode_steps_);
    int act = action["action"_];

    callback_(act);
    if (verbose_) {
      printf("Msg: %d\n", msg_);
      printf("Player %d chose %s in [", to_decide_, options_[act].c_str());
      for (const auto &option : options_) {
        printf(" %s", option.c_str());
      }
      printf("]\n");
    }

    next(false);
    float reward = 0;
    if (done_) {
      reward = 1;
    }

    WriteState(1.0);
  }

private:
  void WriteState(float reward) {
    State state = Allocate();
    state["obs:global_"_][0] = static_cast<float>(players_[0]->init_lp());
    state["obs:global_"_][1] = static_cast<float>(players_[1]->init_lp());
    state["obs:global_"_][2] = 0;
    state["obs:global_"_][3] = 0;
    state["info:num_options"_] = static_cast<int>(options_.size());
    state["reward"_] = reward;
  }

  void load_deck(std::vector<uint32_t> deck, uint8_t player, bool shuffle = true) {
    std::vector<uint32_t> c;
    std::vector<std::pair<uint32_t, int>> fusion, xyz, synchro, link;

    for (auto code : deck) {
      const card_data &cc = get_card_from_db(code);
      if (cc.type & TYPE_FUSION) {
        fusion.push_back({code, cc.level});
      } else if (cc.type & TYPE_XYZ) {
        xyz.push_back({code, cc.level});
      } else if (cc.type & TYPE_SYNCHRO) {
        synchro.push_back({code, cc.level});
      } else if (cc.type & TYPE_LINK) {
        link.push_back({code, cc.level});
      } else {
        c.push_back(code);
      }
    }

    if (shuffle) {
      std::shuffle(c.begin(), c.end(), gen_);
    }

    auto cmp = [](const std::pair<uint32_t, int> &a, const std::pair<uint32_t, int> &b) {
      return a.second < b.second;
    };
    std::sort(fusion.begin(), fusion.end(), cmp);
    std::sort(xyz.begin(), xyz.end(), cmp);
    std::sort(synchro.begin(), synchro.end(), cmp);
    std::sort(link.begin(), link.end(), cmp);

    for (const auto &tc : fusion) {
      c.push_back(tc.first);
    }
    for (const auto &tc : xyz) {
      c.push_back(tc.first);
    }
    for (const auto &tc : synchro) {
      c.push_back(tc.first);
    }
    for (const auto &tc : link) {
      c.push_back(tc.first);
    }

    for (auto code : c) {
      new_card(pduel_, code, player, player, LOCATION_DECK, 0, POS_FACEDOWN_DEFENSE);
    }
  }

  void next(bool process_first = true) {
    bool skip_process = !process_first;
    while (duel_started_) {
      if (!skip_process) {
        res_ = process(pduel_);
        dl_ = get_message(pduel_, data_);
        dp_ = 0;
      }
      else {
        skip_process = false;
      }
      while (dp_ != dl_) {
        handle_message();
        if (options_.empty()) {
          continue;
        }
        if (to_decide_ == ai_player_) {
          if (msg_ == MSG_SELECT_PLACE) {
            callback_(0);
          }
          else if (options_.size() == 1) {
            callback_(0);
          }
          else {
            return;
          }
        }
        else {
          auto idx = players_[to_decide_]->think(options_);
          callback_(idx);
        }
      }
      if (res_ & 0x20000) {
        break;
      }
    }
    done_ = true;
    options_.clear();
  }

  uint8_t read_u8() {
    return data_[dp_++];
  }

  uint32 read_u32() {
    uint32 v = 0;
    for (int i = 0; i < 4; ++i) {
      v |= data_[dp_++] << (i * 8);
    }
    return v;
  }

  void handle_message() {
    msg_ = int(data_[dp_++]);
    options_ = {};
    if (msg_ == MSG_DRAW) {
      auto player = read_u8();
      auto drawed = read_u8();
      std::vector<uint32> codes;
      for (int i = 0; i < drawed; ++i) {
        uint32 code = read_u32();
        codes.push_back(code & 0x7fffffff);
      }

      if (!verbose_) {
        return;
      }

      const auto &pl = players_[player];
      pl->notify("Drew " + std::to_string(drawed) + " cards:");
      for (int i = 0; i < drawed; ++i) {
        const auto &c = get_card_from_db(codes[i]);
        pl->notify(std::to_string(i + 1) + ": " + std::to_string(c.code));
      }
      const auto &op = players_[1 - player];
      op->notify("Opponent drew " + std::to_string(drawed) + " cards.");
    }
    else if (msg_  == MSG_RETRY) {
      if (verbose_) {
        printf("Retry\n");
        throw std::runtime_error("Retry");
      }
    }
    else if (msg_ == MSG_SELECT_YESNO) {
      auto player = read_u8();
      auto desc = read_u32();
      to_decide_ = player;
      auto callback = [this](int idx) {
        if (idx == 0) {
          set_responsei(pduel_, 1);
        }
        else if (idx == 1) {
          set_responsei(pduel_, 0);
        }
        else {
          throw std::runtime_error("Invalid option");
        }
      };
      callback_ = callback;
    }
    else {
      if (verbose_) {
        printf("Unknown message %d\n", msg_);
      }
      dp_ = dl_;
    }
  }
};

using YGOProEnvPool = AsyncEnvPool<YGOProEnv>;

} // namespace ygopro

#endif // ENVPOOL_YGOPRO_YGOPRO_H_
