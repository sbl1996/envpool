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
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "envpool2/core/async_envpool.h"
#include "envpool2/core/env.h"

#include "ygopro-core/common.h"
#include "ygopro-core/card_data.h"
#include "ygopro-core/field.h"
#include "ygopro-core/ocgapi.h"

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

namespace ygopro {

inline std::vector<std::vector<int>> combinations(int n, int r) {
  std::vector<std::vector<int>> combs;
  std::vector<bool> m(n);
  std::fill(m.begin(), m.begin() + r, true);

  do {
    std::vector<int> cs;
    cs.reserve(r);
    for (int i = 0; i < n; ++i) {
      if (m[i]) {
        cs.push_back(i);
      }
    }
    combs.push_back(cs);
  } while (std::prev_permutation(m.begin(), m.end()));

  return combs;
}

inline bool sum_to(const std::vector<int> &w, const std::vector<int> ind, int i, int r) {
  std::vector<int> sums;
  if (r <= 0) {
    return false;
  }
  int n = ind.size();
  if (i == n - 1) {
    return r == 1 || (w[ind[i]] == r); 
  }
  return sum_to(w, ind, i + 1, r - 1) || sum_to(w, ind, i + 1, r - w[ind[i]]);
}

inline bool sum_to(const std::vector<int> &w, const std::vector<int> ind, int r) {
  return sum_to(w, ind, 0, r);
}

inline auto combinations_with_weight(const std::vector<int> &weights, int r) {
  int n = weights.size();
  std::vector<std::vector<int>> results;

  for (int k = 1; k <= n; k++) {
    std::vector<std::vector<int>> combs = combinations(n, k);
    for (const auto &comb : combs) {
      if (sum_to(weights, comb, r)) {
        results.push_back(comb);
      }
    }
  }
  return results;
}

inline std::string reason_to_string(uint8_t reason) {
  // !victory 0x0 Surrendered
  // !victory 0x1 LP reached 0
  // !victory 0x2 Cards can't be drawn
  // !victory 0x3 Time limit up
  // !victory 0x4 Lost connection  
  switch (reason) {
  case 0x0:
    return "Surrendered";
  case 0x1:
    return "LP reached 0";
  case 0x2:
    return "Cards can't be drawn";
  case 0x3:
    return "Time limit up";
  case 0x4:
    return "Lost connection";
  default:
    return "Unknown";
  }
}

inline std::string phase_to_string(int phase) {
  switch (phase) {
  case PHASE_DRAW:
    return "draw phase";
  case PHASE_STANDBY:
    return "standby phase";
  case PHASE_MAIN1:
    return "main1 phase";
  case PHASE_BATTLE_START:
    return "battle start phase";
  case PHASE_BATTLE_STEP:
    return "battle step phase";
  case PHASE_DAMAGE:
    return "damage phase";
  case PHASE_DAMAGE_CAL:
    return "damage calculation phase";
  case PHASE_BATTLE:
    return "battle phase";
  case PHASE_MAIN2:
    return "main2 phase";
  case PHASE_END:
    return "end phase";
  default:
    return "Unknown";
  }
}

inline std::string position_to_string(int position, int location) {
  switch (position) {
  case POS_FACEUP_ATTACK:
    return "face-up attack";
  case POS_FACEDOWN_ATTACK:
    return "face-down attack";
  case POS_FACEUP_DEFENSE:
    if (location & LOCATION_EXTRA) {
      return "face-up";
    }
    return "face-up defense";
  case POS_FACEUP:
    return "face-up";
  case POS_FACEDOWN_DEFENSE:
    if (location & LOCATION_EXTRA) {
      return "face-down";
    }
    return "face-down defense";
  case POS_FACEDOWN:
    return "face-down";
  default:
    return "Unknown";
  }
}

inline std::string msg_to_string(int msg) {
    switch (msg) {
        case MSG_RETRY:
            return "retry";
        case MSG_HINT:
            return "hint";
        case MSG_WIN:
            return "win";
        case MSG_SELECT_BATTLECMD:
            return "select_battlecmd";
        case MSG_SELECT_IDLECMD:
            return "select_idlecmd";
        case MSG_SELECT_EFFECTYN:
            return "select_effectyn";
        case MSG_SELECT_YESNO:
            return "select_yesno";
        case MSG_SELECT_OPTION:
            return "select_option";
        case MSG_SELECT_CARD:
            return "select_card";
        case MSG_SELECT_CHAIN:
            return "select_chain";
        case MSG_SELECT_PLACE:
            return "select_place";
        case MSG_SELECT_POSITION:
            return "select_position";
        case MSG_SELECT_TRIBUTE:
            return "select_tribute";
        case MSG_SELECT_COUNTER:
            return "select_counter";
        case MSG_SELECT_SUM:
            return "select_sum";
        case MSG_SELECT_DISFIELD:
            return "select_disfield";
        case MSG_SORT_CARD:
            return "sort_card";
        case MSG_SELECT_UNSELECT_CARD:
            return "select_unselect_card";
        case MSG_CONFIRM_DECKTOP:
            return "confirm_decktop";
        case MSG_CONFIRM_CARDS:
            return "confirm_cards";
        case MSG_SHUFFLE_DECK:
            return "shuffle_deck";
        case MSG_SHUFFLE_HAND:
            return "shuffle_hand";
        case MSG_SWAP_GRAVE_DECK:
            return "swap_grave_deck";
        case MSG_SHUFFLE_SET_CARD:
            return "shuffle_set_card";
        case MSG_REVERSE_DECK:
            return "reverse_deck";
        case MSG_DECK_TOP:
            return "deck_top";
        case MSG_SHUFFLE_EXTRA:
            return "shuffle_extra";
        case MSG_NEW_TURN:
            return "new_turn";
        case MSG_NEW_PHASE:
            return "new_phase";
        case MSG_CONFIRM_EXTRATOP:
            return "confirm_extratop";
        case MSG_MOVE:
            return "move";
        case MSG_POS_CHANGE:
            return "pos_change";
        case MSG_SET:
            return "set";
        case MSG_SWAP:
            return "swap";
        case MSG_FIELD_DISABLED:
            return "field_disabled";
        case MSG_SUMMONING:
            return "summoning";
        case MSG_SUMMONED:
            return "summoned";
        case MSG_SPSUMMONING:
            return "spsummoning";
        case MSG_SPSUMMONED:
            return "spsummoned";
        case MSG_FLIPSUMMONING:
            return "flipsummoning";
        case MSG_FLIPSUMMONED:
            return "flipsummoned";
        case MSG_CHAINING:
            return "chaining";
        case MSG_CHAINED:
            return "chained";
        case MSG_CHAIN_SOLVING:
            return "chain_solving";
        case MSG_CHAIN_SOLVED:
            return "chain_solved";
        case MSG_CHAIN_END:
            return "chain_end";
        case MSG_CHAIN_NEGATED:
            return "chain_negated";
        case MSG_CHAIN_DISABLED:
            return "chain_disabled";
        case MSG_RANDOM_SELECTED:
            return "random_selected";
        case MSG_BECOME_TARGET:
            return "become_target";
        case MSG_DRAW:
            return "draw";
        case MSG_DAMAGE:
            return "damage";
        case MSG_RECOVER:
            return "recover";
        case MSG_EQUIP:
            return "equip";
        case MSG_LPUPDATE:
            return "lpupdate";
        case MSG_CARD_TARGET:
            return "card_target";
        case MSG_CANCEL_TARGET:
            return "cancel_target";
        case MSG_PAY_LPCOST:
            return "pay_lpcost";
        case MSG_ADD_COUNTER:
            return "add_counter";
        case MSG_REMOVE_COUNTER:
            return "remove_counter";
        case MSG_ATTACK:
            return "attack";
        case MSG_BATTLE:
            return "battle";
        case MSG_ATTACK_DISABLED:
            return "attack_disabled";
        case MSG_DAMAGE_STEP_START:
            return "damage_step_start";
        case MSG_DAMAGE_STEP_END:
            return "damage_step_end";
        case MSG_MISSED_EFFECT:
            return "missed_effect";
        case MSG_TOSS_COIN:
            return "toss_coin";
        case MSG_TOSS_DICE:
            return "toss_dice";
        case MSG_ROCK_PAPER_SCISSORS:
            return "rock_paper_scissors";
        case MSG_HAND_RES:
            return "hand_res";
        case MSG_ANNOUNCE_RACE:
            return "announce_race";
        case MSG_ANNOUNCE_ATTRIB:
            return "announce_attrib";
        case MSG_ANNOUNCE_CARD:
            return "announce_card";
        case MSG_ANNOUNCE_NUMBER:
            return "announce_number";
        case MSG_CARD_HINT:
            return "card_hint";
        case MSG_TAG_SWAP:
            return "tag_swap";
        case MSG_RELOAD_FIELD:
            return "reload_field";
        case MSG_AI_NAME:
            return "ai_name";
        case MSG_SHOW_HINT:
            return "show_hint";
        case MSG_PLAYER_HINT:
            return "player_hint";
        case MSG_MATCH_KILL:
            return "match_kill";
        case MSG_CUSTOM_MSG:
            return "custom_msg";
        default:
            return "unknown_msg";
    }
}


using PlayerId = uint8_t;

static SQLite::Database *db = nullptr;

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

inline std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}
class Card {
  friend class YGOProEnv;

protected:
  uint32_t code_;
  uint32_t alias_;
  uint64_t setcode_;
  uint32_t type_;
  uint32_t level_;
  uint32_t lscale_;
  uint32_t rscale_;
  int32_t attack_;
  int32_t defense_;
  uint32_t race_;
  uint32_t attribute_;
  uint32_t link_marker_;
  // uint32_t category_;
  std::string name_;
  std::string desc_;
  std::vector<std::string> strings_;

  uint32_t data_ = 0;

  PlayerId controler_ = 0;
  uint32_t location_ = 0;
  uint32_t sequence_ = 0;
  uint32_t position_ = 0;

public:
  Card() = default;

  Card(uint32_t code, uint32_t alias, uint64_t setcode, uint32_t type,
       uint32_t level, uint32_t lscale, uint32_t rscale, int32_t attack,
       int32_t defense, uint32_t race, uint32_t attribute, uint32_t link_marker,
       const std::string &name, const std::string &desc,
       const std::vector<std::string> &strings)
      : code_(code), alias_(alias), setcode_(setcode), type_(type),
        level_(level), lscale_(lscale), rscale_(rscale), attack_(attack),
        defense_(defense), race_(race), attribute_(attribute),
        link_marker_(link_marker), name_(name), desc_(desc), strings_(strings) {
  }

  ~Card() = default;

  void set_location(uint32_t location) {
    controler_ = location & 0xff;
    location_ = (location >> 8) & 0xff;
    sequence_ = (location >> 16) & 0xff;
    position_ = (location >> 24) & 0xff;
  }

  const std::string &name() const { return name_; }
  const std::string &desc() const { return desc_; }
  const std::vector<std::string> &strings() const { return strings_; }

  std::string get_spec(bool opponent) const;
  std::string get_spec(PlayerId player) const;
  uint32_t get_spec_code(PlayerId player) const;
  std::string get_position() const;
  std::string get_effect_description(uint32_t desc, bool existing = false) const {
    std::string s;
    bool e = false;
    auto code = code_;
    if (desc > 10000) {
      code = desc >> 4;
    }
    uint32_t offset = desc - code_ * 16;
    bool in_range = (offset >= 0) && (offset < strings_.size());
    std::string str = "";
    if (in_range) {
      str = ltrim(strings_[offset]);
    }
    if (in_range || desc == 0) {
      if ((desc == 0) || str.empty()) {
        s = "Activate " + name_ + ".";
      }
      else {
        s = str;
        e = true;
      }
    } else {
      s = "TODO: system string " + std::to_string(desc);
      if (!s.empty()) {
        e = true;
      }
    }
    if (existing && !e) {
      s = "";
    }
    return s;
  }
};


inline Card query_card_from_db(uint32_t code) {
  SQLite::Statement query1(*db, "SELECT * FROM datas WHERE id=?");
  query1.bind(1, code);
  bool found = query1.executeStep();
  if (!found) {
    std::string msg = "Card not found: " + std::to_string(code);
    throw std::runtime_error(msg);
  }

  uint32_t alias = query1.getColumn("alias");
  uint64_t setcode = query1.getColumn("setcode").getInt64();
  uint32_t type = query1.getColumn("type");
  uint32_t level_ = query1.getColumn("level");
  uint32_t level = level_ & 0xff;
  uint32_t lscale = (level_ >> 24) & 0xff;
  uint32_t rscale = (level_ >> 16) & 0xff;
  int32_t attack = query1.getColumn("atk");
  int32_t defense = query1.getColumn("def");
  uint32_t link_marker = 0;
  if (type & TYPE_LINK) {
    defense = 0;
    link_marker = defense;
  }
  uint32_t race = query1.getColumn("race");
  uint32_t attribute = query1.getColumn("attribute");

  SQLite::Statement query2(*db, "SELECT * FROM texts WHERE id=?");
  query2.bind(1, code);
  query2.executeStep();

  std::string name = query2.getColumn(1);
  std::string desc = query2.getColumn(2);
  std::vector<std::string> strings;
  for (int i = 3; i < query2.getColumnCount(); ++i) {
    std::string str = query2.getColumn(i);
    if (str.empty()) {
      break;
    }
    strings.push_back(str);
  }
  return Card(code, alias, setcode, type, level, lscale, rscale, attack,
              defense, race, attribute, link_marker, name, desc, strings);
}

static std::unordered_map<uint32_t, Card> cards;
static std::shared_timed_mutex cards_mtx;

inline const Card &get_card_from_db(uint32_t code) {
  // if not found, read from db and cache it
  std::shared_lock<std::shared_timed_mutex> lock(cards_mtx);
  auto it = cards.find(code);
  if (it == cards.end()) {
    lock.unlock();
    std::unique_lock<std::shared_timed_mutex> ulock(cards_mtx);
    cards[code] = query_card_from_db(code);
    return cards.at(code);
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

inline std::vector<std::string> flag_to_usable_cardspecs(uint32_t flag,
                                                         bool reverse = false) {
  std::string zone_names[4] = {"m", "s", "om", "os"};
  std::vector<std::string> specs;
  for (int j = 0; j < 4; j++) {
    uint32_t value = (flag >> (j * 8)) & 0xff;
    for (int i = 0; i < 8; i++) {
      bool avail = (value & (1 << i)) == 0;
      if (reverse) {
        avail = !avail;
      }
      if (avail) {
        specs.push_back(zone_names[j] + std::to_string(i + 1));
      }
    }
  }
  return specs;
}

inline std::string ls_to_spec(uint8_t loc, uint8_t seq) {
  std::string spec;
  if (loc == LOCATION_HAND) {
    spec += "h";
  } else if (loc == LOCATION_MZONE) {
    spec += "m";
  } else if (loc == LOCATION_SZONE) {
    spec += "s";
  } else if (loc == LOCATION_GRAVE) {
    spec += "g";
  } else if (loc == LOCATION_REMOVED) {
    spec += "r";
  } else if (loc == LOCATION_EXTRA) {
    spec += "x";
  }
  spec += std::to_string(seq + 1);
  return spec;
}

inline std::string ls_to_spec(uint8_t loc, uint8_t seq, bool opponent) {
  std::string spec = ls_to_spec(loc, seq);
  if (opponent) {
    spec.insert(0, 1, 'o');
  }
  return spec;
}

inline std::tuple<uint8_t, uint8_t> sepc_to_ls(const std::string spec) {
  uint8_t loc;
  uint8_t seq;
  if (spec[0] == 'h') {
    loc = LOCATION_HAND;
  } else if (spec[0] == 'm') {
    loc = LOCATION_MZONE;
  } else if (spec[0] == 's') {
    loc = LOCATION_SZONE;
  } else if (spec[0] == 'g') {
    loc = LOCATION_GRAVE;
  } else if (spec[0] == 'r') {
    loc = LOCATION_REMOVED;
  } else if (spec[0] == 'x') {
    loc = LOCATION_EXTRA;
  } else {
    throw std::runtime_error("Invalid location");
  }
  seq = std::stoi(spec.substr(1)) - 1;
  return {loc, seq};
}

inline uint32_t ls_to_spec_code(uint8_t loc, uint8_t seq, bool opponent) {
  uint32_t c = opponent ? 1 : 0;
  c |= (loc << 8);
  c |= (seq << 16);
  return c;
}

inline std::string code_to_spec(uint32_t spec_code) {
  uint8_t loc = (spec_code >> 8) & 0xff;
  uint8_t seq = (spec_code >> 16) & 0xff;
  bool opponent = (spec_code & 0xff) == 1;
  return ls_to_spec(loc, seq, opponent);
}

inline std::string Card::get_spec(bool opponent) const {
  return ls_to_spec(location_, sequence_, opponent);
};

inline std::string Card::get_spec(PlayerId player) const {
  return get_spec(player != controler_);
};

inline uint32_t Card::get_spec_code(PlayerId player) const {
  return ls_to_spec_code(location_, sequence_, player != controler_);
};

inline std::string Card::get_position() const {
  return position_to_string(position_, location_);
}

class YGOProEnvFns {
public:
  static decltype(auto) DefaultConfig() {
    return MakeDict("db_path"_.Bind(std::string("vendor/locale/en/cards.db")),
                    "deck1"_.Bind(std::string("vendor/decks/OldSchool.ydk")),
                    "deck2"_.Bind(std::string("vendor/decks/OldSchool.ydk")),
                    "player"_.Bind(-1), "play"_.Bind(false),
                    "verbose"_.Bind(false));
  }
  template <typename Config>
  static decltype(auto) StateSpec(const Config &conf) {
    float fmax = std::numeric_limits<float>::max();
    return MakeDict("obs:global_"_.Bind(Spec<float>({4})),
                    "info:num_options"_.Bind(Spec<int>({}, {0, 100})));
  }
  template <typename Config>
  static decltype(auto) ActionSpec(const Config &conf) {
    return MakeDict("action"_.Bind(Spec<int>({}, {0, 100})));
  }
};

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
         bool verbose = false)
      : nickname_(nickname), init_lp_(init_lp), duel_player_(duel_player),
        verbose_(verbose) {}
  virtual ~Player() = default;

  void notify(const std::string &text) {
    if (verbose_) {
      printf("%d %s\n", duel_player_, text.c_str());
    }
  }

  const int &init_lp() const { return init_lp_; }

  const std::string &nickname() const { return nickname_; }

  virtual int think(const std::vector<std::string> &options) = 0;
};

class GreedyAI : public Player {
protected:
public:
  GreedyAI(const std::string &nickname, int init_lp, PlayerId duel_player,
           bool verbose = false)
      : Player(nickname, init_lp, duel_player, verbose) {}

  int think(const std::vector<std::string> &options) override { return 0; }
};

class HumanPlayer : public Player {
protected:
public:
  HumanPlayer(const std::string &nickname, int init_lp, PlayerId duel_player,
              bool verbose = false)
      : Player(nickname, init_lp, duel_player, verbose) {}

  int think(const std::vector<std::string> &options) override {
    while (true) {
      std::string input;
      std::getline(std::cin, input);
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
  PlayerId ai_player_;
  Player *players_[2];

  std::uniform_int_distribution<uint64_t> dist_int_;
  bool done_{true};
  bool duel_started_{false};

  PlayerId winner_;
  uint8_t win_reason_;

  int lp_[2];

  // turn player
  PlayerId tp_;
  int current_phase_;

  int msg_;
  std::vector<std::string> options_;
  PlayerId to_decide_;
  std::function<void(int)> callback_;

  byte data_[4096];
  int dp_ = 0;
  int dl_ = 0;
  uint32_t res_;

  byte query_buf_[128];
  int qdp_ = 0;

  byte resp_buf_[128];

  using IdleCardSpec = std::tuple<uint32_t, std::string, uint32_t>;

  // chain
  PlayerId chaining_player_;

public:
  YGOProEnv(const Spec &spec, int env_id)
      : Env<YGOProEnvSpec>(spec, env_id),
        max_episode_steps_(spec.config["max_episode_steps"_]),
        elapsed_step_(max_episode_steps_ + 1), dist_int_(0, 0xffffffff),
        deck1_(read_deck(spec.config["deck1"_])),
        deck2_(read_deck(spec.config["deck2"_])),
        player_(spec.config["player"_]), play_(spec.config["play"_]),
        verbose_(spec.config["verbose"_]) {
    SQLite::Database *old_db = db;
    db = new SQLite::Database(spec.config["db_path"_], SQLite::OPEN_READONLY);
    set_card_reader(card_reader_callback);
    set_script_reader(script_reader_callback);
    if (old_db != nullptr) {
      delete old_db;
    }
    if (verbose_) {
      std::cout << "Loaded " << deck1_.size() << " cards in deck 1"
                << std::endl;
      std::cout << "Loaded " << deck2_.size() << " cards in deck 2"
                << std::endl;
    }
  }

  bool IsDone() override { return done_; }

  void Reset() override {
    if (player_ == -1) {
      ai_player_ = dist_int_(gen_) % 2;
    } else {
      ai_player_ = player_;
    }

    unsigned long duel_seed = dist_int_(gen_);
    pduel_ = create_duel(duel_seed);
    for (int i = 0; i < 2; i++) {
      if (players_[i] != nullptr) {
        delete players_[i];
      }
      std::string nickname = i == 0 ? "Alice" : "Bob";
      int init_lp = 8000;
      if (i != ai_player_ && play_) {
        players_[i] = new HumanPlayer(nickname, init_lp, i, verbose_);
      } else {
        players_[i] = new GreedyAI(nickname, init_lp, i, verbose_);
      }
      set_player_info(pduel_, i, init_lp, 5, 1);
      load_deck(i == 0 ? deck1_ : deck2_, i);
      lp_[i] = players_[i]->init_lp_;
    }

    // rules = 1, Traditional
    // rules = 0, Default
    // rules = 4, Link
    // rules = 5, MR5
    int32_t rules = 5;
    int32_t options = ((rules & 0xFF) << 16) + (0 & 0xFFFF);
    start_duel(pduel_, options);
    duel_started_ = true;
    winner_ = 255;
    win_reason_ = 255;

    next(true);

    done_ = false;
    elapsed_step_ = 0;
    WriteState(0.0);
  }

  void Step(const Action &action) override {
    int act = action["action"_];

    callback_(act);
    if (verbose_) {
      show_decision(act);
    }

    next(false);
    float reward = 0;
    if (done_) {
      reward = winner_ == ai_player_ ? 1.0 : -1.0;
    }

    WriteState(reward);
  }

private:
  void WriteState(float reward) {
    State state = Allocate();
    state["obs:global_"_][0] = static_cast<float>(lp_[to_decide_]);
    state["obs:global_"_][1] = static_cast<float>(lp_[1 - to_decide_]);
    state["obs:global_"_][2] = 0;
    state["obs:global_"_][3] = 0;
    state["info:num_options"_] = static_cast<int>(options_.size());
    state["reward"_] = reward;
  }

  void show_decision(int act) {
    printf("Player %d chose '%s' in [", to_decide_, options_[act].c_str());
    int n = options_.size();
    for (int i = 0; i < n; ++i) {
      printf(" '%s'", options_[i].c_str());
      if (i < n - 1) {
        printf(",");
      }
    }
    printf(" ]\n");
  }

  void load_deck(std::vector<uint32_t> deck, PlayerId player,
                 bool shuffle = true) {
    std::vector<uint32_t> c;
    std::vector<std::pair<uint32_t, int>> fusion, xyz, synchro, link;

    for (auto code : deck) {
      const Card &cc = get_card_from_db(code);
      if (cc.type_ & TYPE_FUSION) {
        fusion.push_back({code, cc.level_});
      } else if (cc.type_ & TYPE_XYZ) {
        xyz.push_back({code, cc.level_});
      } else if (cc.type_ & TYPE_SYNCHRO) {
        synchro.push_back({code, cc.level_});
      } else if (cc.type_ & TYPE_LINK) {
        link.push_back({code, cc.level_});
      } else {
        c.push_back(code);
      }
    }

    if (shuffle) {
      std::shuffle(c.begin(), c.end(), gen_);
    }

    auto cmp = [](const std::pair<uint32_t, int> &a,
                  const std::pair<uint32_t, int> &b) {
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
      new_card(pduel_, code, player, player, LOCATION_DECK, 0,
               POS_FACEDOWN_DEFENSE);
    }
  }

  void next(bool process_first = true) {
    bool skip_process = !process_first;
    while (duel_started_) {
      if (!skip_process) {
        res_ = process(pduel_);
        dl_ = get_message(pduel_, data_);
        dp_ = 0;
      } else {
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
            if (verbose_) {
              show_decision(0);
            }
          } else if (options_.size() == 1) {
            callback_(0);
            if (verbose_) {
              show_decision(0);
            }
          } else {
            return;
          }
        } else {
          auto idx = players_[to_decide_]->think(options_);
          callback_(idx);
          if (verbose_) {
            show_decision(idx);
          }
        }
      }
      if (res_ & 0x20000) {
        break;
      }
    }
    done_ = true;
    options_.clear();
  }

  uint8_t read_u8() { return data_[dp_++]; }

  uint16_t read_u16() {
    uint16_t v = 0;
    for (int i = 0; i < 2; ++i) {
      v |= data_[dp_++] << (i * 8);
    }
    return v;
  }

  uint32 read_u32() {
    uint32 v = 0;
    for (int i = 0; i < 4; ++i) {
      v |= data_[dp_++] << (i * 8);
    }
    return v;
  }

  uint32 q_read_u32() {
    uint32 v = 0;
    for (int i = 0; i < 4; ++i) {
      v |= query_buf_[qdp_++] << (i * 8);
    }
    return v;
  }

  Card get_card(PlayerId player, uint8_t loc, uint8_t seq) {
    int32_t flags = QUERY_CODE | QUERY_ATTACK | QUERY_DEFENSE | QUERY_POSITION |
                    QUERY_LEVEL | QUERY_RANK | QUERY_LSCALE | QUERY_RSCALE |
                    QUERY_LINK;
    int32_t bl = query_card(pduel_, player, loc, seq, flags, query_buf_, 0);
    qdp_ = 0;
    if (bl <= 0) {
      throw std::runtime_error("Invalid card");
    }
    uint32_t f = q_read_u32();
    if (f == LEN_EMPTY) {
      throw std::runtime_error("Invalid card");
    }
    f = q_read_u32();
    uint32_t code = q_read_u32();
    Card c = get_card_from_db(code);
    uint32_t position = q_read_u32();
    c.set_location(position);
    uint32_t level = q_read_u32();
    if ((level & 0xff) > 0) {
      c.level_ = level & 0xff;
    }
    uint32_t rank = q_read_u32();
    if ((rank & 0xff) > 0) {
      c.level_ = rank & 0xff;
    }
    c.attack_ = q_read_u32();
    c.defense_ = q_read_u32();
    c.lscale_ = q_read_u32();
    c.rscale_ = q_read_u32();
    uint32_t link = q_read_u32();
    uint32_t link_marker = q_read_u32();
    if ((link & 0xff) > 0) {
      c.level_ = link & 0xff;
    }
    if (link_marker > 0) {
      c.defense_ = link_marker;
    }
    return c;
  }

  std::vector<Card> read_cardlist(bool extra = false, bool extra8 = false) {
    std::vector<Card> cards;
    auto count = read_u8();
    cards.reserve(count);
    for (int i = 0; i < count; ++i) {
      auto code = read_u32();
      auto controller = read_u8();
      auto loc = read_u8();
      auto seq = read_u8();
      auto card = get_card(controller, loc, seq);
      if (extra) {
        if (extra8) {
          card.data_ = read_u8();
        } else {
          card.data_ = read_u32();
        }
      }
      cards.push_back(card);
    }
    return cards;
  }

  std::vector<IdleCardSpec> read_cardlist_spec(bool extra = false,
                                               bool extra8 = false) {
    std::vector<IdleCardSpec> card_specs;
    auto count = read_u8();
    card_specs.reserve(count);
    for (int i = 0; i < count; ++i) {
      auto code = read_u32();
      auto controller = read_u8();
      auto loc = read_u8();
      auto seq = read_u8();
      uint32_t data = -1;
      if (extra) {
        if (extra8) {
          data = read_u8();
        } else {
          data = read_u32();
        }
      }
      card_specs.push_back({code, ls_to_spec(loc, seq), data});
    }
    return card_specs;
  }

  void handle_message() {
    msg_ = int(data_[dp_++]);
    if (verbose_) {
      printf("Msg: %s, dp: %d, dl: %d\n", msg_to_string(msg_).c_str(), dp_, dl_);
    }
    options_ = {};
    if (msg_ == MSG_DRAW) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto player = read_u8();
      auto drawed = read_u8();
      std::vector<uint32> codes;
      for (int i = 0; i < drawed; ++i) {
        uint32 code = read_u32();
        codes.push_back(code & 0x7fffffff);
      }
      const auto &pl = players_[player];
      pl->notify("Drew " + std::to_string(drawed) + " cards:");
      for (int i = 0; i < drawed; ++i) {
        const auto &c = get_card_from_db(codes[i]);
        pl->notify(std::to_string(i + 1) + ": " + c.name_);
      }
      const auto &op = players_[1 - player];
      op->notify("Opponent drew " + std::to_string(drawed) + " cards.");
    } else if (msg_ == MSG_NEW_TURN) {
      tp_ = int(read_u8());
      if (!verbose_) {
        return;
      }
      auto player = players_[tp_];
      player->notify("Your turn.");
      players_[1 - tp_]->notify(player->nickname() + "'s turn.");
    } else if (msg_ == MSG_NEW_PHASE) {
      current_phase_ = int(read_u16());
      if (!verbose_) {
        return;
      }
      auto phase_str = phase_to_string(current_phase_);
      for (int i = 0; i < 2; ++i) {
        players_[i]->notify("entering " + phase_str + ".");
      }
    } else if (msg_ == MSG_MOVE) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      uint32_t code = read_u32();
      uint32_t location = read_u32();
      uint32_t newloc = read_u32();
      uint32_t reason = read_u32();
      Card card = get_card_from_db(code);
      card.set_location(location);
      Card cnew = get_card_from_db(code);
      cnew.set_location(newloc);
      auto pl = players_[card.controler_];
      auto op = players_[1 - card.controler_];

      auto plspec = card.get_spec(false);
      auto opspec = card.get_spec(true);
      auto plnewspec = cnew.get_spec(false);
      auto opnewspec = cnew.get_spec(true);

      auto getspec = [&](Player *p) {
        return p == pl ? plspec : opspec;
      };
      auto getnewspec = [&](Player *p) {
        return p == pl ? plnewspec : opnewspec;
      };
      bool card_visible = true;
      if ((card.position_ & POS_FACEDOWN) && (cnew.position_ & POS_FACEDOWN)) {
        card_visible = false;
      }
      auto getvisiblename = [&](Player *p) {
        return card_visible ? card.name_ : "Face-down card";
      };

      if ((reason & REASON_DESTROY) && (card.location_ != cnew.location_)) {
        pl->notify("Card " + plspec + " (" + card.name_ + ") destroyed.");
        op->notify("Card " + opspec + " (" + card.name_ + ") destroyed.");
      }
      else if ((card.location_ == cnew.location_) && (card.location_ & LOCATION_ONFIELD)) {
        if (card.controler_ != cnew.controler_) {
          pl->notify("Your card " + plspec + " (" + card.name_ + ") changed controller to " + op->nickname() + " and is now located at " + plnewspec + ".");
          op->notify("You now control " + pl->nickname() + "'s card " + opspec + " (" + card.name_ + ") and its located at " + opnewspec + ".");
        }
        else {
          pl->notify("Your card " + plspec + " (" + card.name_ + ") switched its zone to " + plnewspec + ".");
          op->notify(pl->nickname() + "'s card " + opspec + " (" + card.name_ + ") changed its zone to " + opnewspec + ".");
        }
      }
      else if ((reason & REASON_DISCARD) && (card.location_ != cnew.location_)) {
        pl->notify("You discarded " + plspec + " (" + card.name_ + ").");
        op->notify(pl->nickname() + " discarded " + opspec + " (" + card.name_ + ").");
      }
      else if ((card.location_ == LOCATION_REMOVED) && (cnew.location_ & LOCATION_ONFIELD)) {
        pl->notify("Your banished card " + plspec + " (" + card.name_ + ") returns to the field at " + plnewspec + ".");
        op->notify(pl->nickname() + "'s banished card " + opspec + " (" + card.name_ + ") returned to their field at " + opnewspec + ".");
      }
      else if ((card.location_ == LOCATION_GRAVE) && (cnew.location_ & LOCATION_ONFIELD)) {
        pl->notify("Your card " + plspec + " (" + card.name_ + ") returns from the graveyard to the field at " + plnewspec + ".");
        op->notify(pl->nickname() + "'s card " + opspec + " (" + card.name_ + ") returns from the graveyard to the field at " + opnewspec + ".");
      }
      else if ((cnew.location_ == LOCATION_HAND) && (card.location_ != cnew.location_)) {
        pl->notify("Card " + plspec + " (" + card.name_ + ") returned to hand.");
      }
      else if ((reason & (REASON_RELEASE | REASON_SUMMON)) && (card.location_ != cnew.location_)) {
        pl->notify("You tribute " + plspec + " (" + card.name_ + ").");
        op->notify(pl->nickname() + " tributes " + opspec + " (" + getvisiblename(op) + ").");
      }
      else if ((card.location_ == (LOCATION_OVERLAY | LOCATION_MZONE)) && (cnew.location_ & LOCATION_GRAVE)) {
        pl->notify("You detached " + card.name_ + ".");
        op->notify(pl->nickname() + " detached " + card.name_ + ".");
      }
      else if ((card.location_ != cnew.location_) && (cnew.location_ == LOCATION_GRAVE)) {
        pl->notify("Your card " + plspec + " (" + card.name_ + ") was sent to the graveyard.");
        op->notify(pl->nickname() + "'s card " + opspec + " (" + card.name_ + ") was sent to the graveyard.");
      }
      else if ((card.location_ != cnew.location_) && (cnew.location_ == LOCATION_REMOVED)) {
        pl->notify("Your card " + plspec + " (" + card.name_ + ") was banished.");
        op->notify(pl->nickname() + "'s card " + opspec + " (" + getvisiblename(op) + ") was banished.");
      }
      else if ((card.location_ != cnew.location_) && (cnew.location_ == LOCATION_DECK)) {
        pl->notify("Your card " + plspec + " (" + card.name_ + ") returned to your deck.");
        op->notify(pl->nickname() + "'s card " + opspec + " (" + getvisiblename(op) + ") returned to their deck.");
      }
      else if ((card.location_ != cnew.location_) && (cnew.location_ == LOCATION_EXTRA)) {
        pl->notify("Your card " + plspec + " (" + card.name_ + ") returned to your extra deck.");
        op->notify(pl->nickname() + "'s card " + opspec + " (" + card.name_ + ") returned to their extra deck.");
      }
      else if ((card.location_ == LOCATION_DECK) && (cnew.location_ == LOCATION_SZONE) && (cnew.position_ != POS_FACEDOWN)) {
        pl->notify("Activating " + plnewspec + " (" + cnew.name_ + ")");
        op->notify(pl->nickname() + " activating " + opnewspec + " (" + cnew.name_ + ")");
      }
    } else if (msg_ == MSG_SET) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      uint32_t code = read_u32();
      uint32_t location = read_u32();
      Card card = get_card_from_db(code);
      card.set_location(location);
      auto c = card.controler_;
      auto cpl = players_[c];
      auto opl = players_[1 - c];
      auto x = 1u - c;
      cpl->notify("You set " + card.get_spec(c) +
                  " (" + card.name_ + ") in " + card.get_position() + " position.");
      opl->notify(cpl->nickname() + " sets " + card.get_spec(PlayerId(1-c)) + " in " + card.get_position() +
                  " position.");
    } else if (msg_ == MSG_HINT) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto hint_type = int(read_u8());
      auto player = read_u8();
      auto value = read_u32();
      // non-GUI don't need hint
      return;
      if (hint_type == HINT_SELECTMSG) {
        if (value > 2000) {
          uint32_t code = value;
          players_[player]->notify(players_[player]->nickname() + " select " +
                                   get_card_from_db(code).name_);
        } else {
          players_[player]->notify("TODO: system string " +
                                   std::to_string(value));
        }
      } else if (hint_type == HINT_NUMBER) {
        players_[1 - player]->notify("Choice of player: [" +
                                     std::to_string(value) + "]");
      } else {
        printf("Unknown hint type %d with value %d\n", hint_type, value);
      }
    } else if (msg_ == MSG_CARD_HINT) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      uint8_t player = read_u8();
      uint8_t loc = read_u8();
      uint8_t seq = read_u8();
      uint8_t pos = read_u8();
      printf("player: %d, loc: %d, seq: %d, pos: %d\n", player, loc, seq, pos);
      uint8_t type = read_u8();
      uint32_t value = read_u32();
      Card card = get_card(player, loc, seq);
      if (type == CHINT_RACE) {
        std::string races_str = "TODO";
        for (PlayerId pl = 0; pl < 2; pl++) {
          players_[pl]->notify(card.get_spec(pl) + " (" + card.name_ + ") selected " + races_str + ".");
        }
      } else if (type == CHINT_ATTRIBUTE) {
        std::string attributes_str = "TODO";
        for (PlayerId pl = 0; pl < 2; pl++) {
          players_[pl]->notify(card.get_spec(pl) + " (" + card.name_ + ") selected " + attributes_str + ".");
        }
      } else {
        printf("Unknown card hint type %d with value %d\n", type, value);
      }
    } else if (msg_ == MSG_POS_CHANGE) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      uint32_t code = read_u32();
      Card card = get_card_from_db(code);
      card.set_location(read_u32());
      uint8_t prevpos = card.position_;
      card.position_ = read_u8();

      auto pl = players_[card.controler_];
      auto op = players_[1 - card.controler_];
      auto plspec = card.get_spec(false);
      auto opspec = card.get_spec(true);
      auto prevpos_str = position_to_string(prevpos, card.location_);
      auto pos_str = position_to_string(card.position_, card.location_);
      pl->notify("The position of card " + plspec + " (" + card.name_ + ") changed from " +
                  prevpos_str + " to " + pos_str + ".");
      op->notify("The position of card " + opspec + " (" + card.name_ + ") changed from " +
                  prevpos_str + " to " + pos_str + ".");
    } else if (msg_ == MSG_SUMMONED) {
      dp_ = dl_;
    } else if (msg_ == MSG_SUMMONING) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      uint32_t code = read_u32();
      Card card = get_card_from_db(code);
      card.set_location(read_u32());
      const auto &nickname = players_[card.controler_]->nickname();
      for (auto pl : players_) {
        pl->notify(
          nickname + " summoning " + card.name_ + " (" +
          std::to_string(card.attack_) + "/" + std::to_string(card.defense_) +
          ") in " + card.get_position() + " position.");
      }
    } else if (msg_ == MSG_SPSUMMONED) {
      dp_ = dl_;
    } else if (msg_ == MSG_SPSUMMONING) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      uint32_t code = read_u32();
      Card card = get_card_from_db(code);
      card.set_location(read_u32());
      const auto &nickname = players_[card.controler_]->nickname();
      for (auto pl : players_) {
        auto pos = card.get_position();
        auto atk = std::to_string(card.attack_);
        auto def = std::to_string(card.defense_);
        if (card.type_ & TYPE_LINK) {
          pl->notify(
            nickname + " special summoning " + card.name_ + " (" + atk + ") in " + pos + " position.");
        } else {
          pl->notify(
            nickname + " special summoning " + card.name_ + " (" + atk + "/" + def + ") in " + pos + " position.");
        }
      }
    } else if (msg_ == MSG_CHAIN_SOLVED) {
      dp_ = dl_;
    } else if (msg_ == MSG_CHAIN_SOLVING) {
      dp_ = dl_;
    } else if (msg_ == MSG_CHAINED) {
      dp_ = dl_;
    } else if (msg_ == MSG_CHAIN_END) {
      dp_ = dl_;      
    } else if (msg_ == MSG_CHAINING) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      uint32_t code = read_u32();
      Card card = get_card_from_db(code);
      card.set_location(read_u32());
      auto tc = read_u8();
      auto tl = read_u8();
      auto ts = read_u8();
      uint32_t desc = read_u32();
      auto cs = read_u8();
      auto c = card.controler_;
      PlayerId o = 1 - c;
      chaining_player_ = c;
      players_[c]->notify("Activating " + card.get_spec(c) + " (" + card.name_ + ")");
      players_[o]->notify(players_[c]->nickname_ + " activating " + card.get_spec(o) + " (" + card.name_ + ")");
    } else if (msg_ == MSG_DAMAGE) {
      auto player = read_u8();
      auto amount = read_u32();
      _damage(player, amount);
    } else if (msg_ == MSG_RECOVER) {
      auto player = read_u8();
      auto amount = read_u32();
      _recover(player, amount);
    } else if (msg_ == MSG_LPUPDATE) {
      auto player = read_u8();
      auto lp = read_u32();
      if (lp >= lp_[player]) {
        _recover(player, lp - lp_[player]);
      } else {
        _damage(player, lp_[player] - lp);
      }
    } else if (msg_ == MSG_PAY_LPCOST) {
      auto player = read_u8();
      auto cost = read_u32();
      lp_[player] -= cost;
      if (!verbose_) {
        return;
      }
      auto pl = players_[player];
      pl->notify("You pay " + std::to_string(cost) + " LP. Your LP is now " + std::to_string(lp_[player]) + ".");
      players_[1-player]->notify(pl->nickname() + " pays " + std::to_string(cost) + " LP. " + pl->nickname() + "'s LP is now " + std::to_string(lp_[player]) + ".");
    } else if (msg_ == MSG_ATTACK) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto attacker = read_u32();
      PlayerId ac = attacker & 0xff;
      auto aloc = (attacker >> 8) & 0xff;
      auto aseq = (attacker >> 16) & 0xff;
      auto apos = (attacker >> 24) & 0xff;
      auto target = read_u32();
      PlayerId tc = target & 0xff;
      auto tloc = (target >> 8) & 0xff;
      auto tseq = (target >> 16) & 0xff;
      auto tpos = (target >> 24) & 0xff;

      if ((ac == 0) && (aloc == 0) && (aseq == 0) && (apos == 0)) {
        return;
      }

      Card acard = get_card(ac, aloc, aseq);
      auto name = players_[ac]->nickname_;
      if ((tc == 0) && (tloc == 0) && (tseq == 0) && (tpos == 0)) {
        for (PlayerId i = 0; i < 2; i++) {
          players_[i]->notify(name + " prepares to attack with " + acard.get_spec(i) + " (" + acard.name_ + ")");
        }
        return;
      }

      Card tcard = get_card(tc, tloc, tseq);
      for (PlayerId i = 0; i < 2; i++) {
        auto aspec = acard.get_spec(i);
        auto tspec = tcard.get_spec(i);
        auto tcname = tcard.name_;
        if ((tcard.controler_ != i) && (tcard.position_ & POS_FACEDOWN)) {
          tcname = tcard.get_position() + " card";
        }
        players_[i]->notify(name + " prepares to attack " + tspec + " (" + tcname + ") with " + aspec + " (" + acard.name_ + ")");
      }
    } else if (msg_ == MSG_DAMAGE_STEP_START) {
      if (!verbose_) {
        return;
      }
      for (int i = 0; i < 2; i++) {
        players_[i]->notify("begin damage");
      }
    } else if (msg_ == MSG_DAMAGE_STEP_END) {
      if (!verbose_) {
        return;
      }
      for (int i = 0; i < 2; i++) {
        players_[i]->notify("end damage");
      }
    } else if (msg_ == MSG_BATTLE) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto attacker = read_u32();
      auto aa = read_u32();
      auto ad = read_u32();
      auto bd0 = read_u8();
      auto target = read_u32();
      auto da = read_u32();
      auto dd = read_u32();
      auto bd1 = read_u8();

      auto ac = attacker & 0xff;
      auto aloc = (attacker >> 8) & 0xff;
      auto aseq = (attacker >> 16) & 0xff;

      auto tc = target & 0xff;
      auto tloc = (target >> 8) & 0xff;
      auto tseq = (target >> 16) & 0xff;
      auto tpos = (target >> 24) & 0xff;

      Card acard = get_card(ac, aloc, aseq);
      std::optional<Card> tcard;
      if (tloc != 0) {
        tcard = get_card(tc, tloc, tseq);
      }
      for (int i = 0; i < 2; i++) {
        auto pl = players_[i];
        std::string attacker_points;
        if (acard.type_ & TYPE_LINK) {
          attacker_points = std::to_string(aa);
        } else {
          attacker_points = std::to_string(aa) + "/" + std::to_string(ad);
        }
        if (tcard.has_value()) {
          std::string defender_points;
          if (tcard->type_ & TYPE_LINK) {
            defender_points = std::to_string(da);
          } else {
            defender_points = std::to_string(da) + "/" + std::to_string(dd);
          }
          pl->notify(acard.name_ + "(" + attacker_points + ")" + " attacks " + tcard->name_ + " (" + defender_points + ")");
        }
        else {
          pl->notify(acard.name_ + "(" + attacker_points + ")" + " attacks");
        }
      }
    } else if (msg_ == MSG_WIN) {
      auto player = read_u8();
      auto reason = read_u8();
      auto winner = players_[player];
      auto loser = players_[1 - player];

      _duel_end(player, reason);

      auto l_reason = reason_to_string(reason);
      if (verbose_) {
        winner->notify("You won (" + l_reason + ").");
        loser->notify("You lost (" + l_reason + ").");
      }
    } else if (msg_ == MSG_RETRY) {
      if (verbose_) {
        printf("Retry\n");
        throw std::runtime_error("Retry");
      }
    } else if (msg_ == MSG_SELECT_BATTLECMD) {
      auto player = read_u8();
      to_decide_ = player;
      auto activatable = read_cardlist_spec(true);
      auto attackable = read_cardlist_spec(true, true);
      bool to_m2 = read_u8();
      bool to_ep = read_u8();

      auto pl = players_[player];
      if (verbose_) {
        pl->notify("Battle menu:");
      }
      for (const auto [code, spec, data] : activatable) {
        options_.push_back("v " + spec);
        if (verbose_) {
          const auto &c = get_card_from_db(code);
          pl->notify("v " + spec + ": activate " + c.name_ + " (" + std::to_string(c.attack_) + "/" + std::to_string(c.defense_) + ")");
        }
      }
      for (const auto [code, spec, data] : attackable) {
        options_.push_back("a " + spec);
        if (verbose_) {
          const auto &c = get_card_from_db(code);
          if (c.type_ & TYPE_LINK) {
            pl->notify("a " + spec + ": " + c.name_ + " (" + std::to_string(c.attack_) + ") attack");
          } else {
            pl->notify("a " + spec + ": " + c.name_ + " (" + std::to_string(c.attack_) + "/" + std::to_string(c.defense_) + ") attack");
          }
        }
      }
      if (to_m2) {
        options_.push_back("m");
        if (verbose_) {
          pl->notify("m: Main phase 2.");
        }
      }
      if (to_ep) {
        if (!to_m2) {
          options_.push_back("e");
          if (verbose_) {
            pl->notify("e: End phase.");
          }
        }
      }
      int n_activatables = activatable.size();
      int n_attackables = attackable.size();
      callback_ = [this, n_activatables, n_attackables, to_ep, to_m2](int idx) {
        if (idx < n_activatables) {
          set_responsei(pduel_, idx << 16);
        } else if (idx < (n_activatables + n_attackables)) {
          idx = idx - n_activatables;
          set_responsei(pduel_, (idx << 16) + 1);
        } else if ((options_[idx] == "e") && to_ep) {
          set_responsei(pduel_, 3);
        } else if ((options_[idx] == "m") && to_m2) {
          set_responsei(pduel_, 2);
        } else {
          throw std::runtime_error("Invalid option");
        }
      };
    } else if (msg_ == MSG_SELECT_CARD) {
      auto player = read_u8();
      to_decide_ = player;
      bool cancelable = read_u8();
      auto min = read_u8();
      auto max = read_u8();
      auto size = read_u8();
      std::vector<std::string> specs;
      specs.reserve(size);
      if (verbose_) {
        std::vector<Card> cards;
        for (int i = 0; i < size; ++i) {
          auto code = read_u32();
          auto loc = read_u32();
          Card card = get_card_from_db(code);
          card.set_location(loc);
          cards.push_back(card);
        }
        auto pl = players_[player];
        pl->notify("Select " + std::to_string(min) + " to " + std::to_string(max) + " cards separated by spaces:");
        for (const auto &card : cards) {
          auto spec = card.get_spec(player);
          specs.push_back(spec);
          pl->notify(spec + ": " + card.name_);
        }
      } else {
        for (int i = 0; i < size; ++i) {
          dp_ += 4;
          auto controller = read_u8();
          auto loc = read_u8();
          auto seq = read_u8();
          dp_++;
          auto spec = ls_to_spec(loc, seq, controller != player);
          specs.push_back(spec);
        }
      }
      
      std::vector<std::vector<int>> combs;
      for (int i = min; i <= max; ++i) {
        for (const auto &comb : combinations(size, i)) {
          combs.push_back(comb);
          std::string option = "";
          for (int j = 0; j < i; ++j) {
            option += specs[comb[j]];
            if (j < i - 1) {
              option += " ";
            }
          }
          options_.push_back(option);
        }
      }

      callback_ = [this, combs](int idx) {
        const auto &comb = combs[idx];
        resp_buf_[0] = comb.size();
        for (int i = 0; i < comb.size(); ++i) {
          resp_buf_[i + 1] = comb[i];
        }
        set_responseb(pduel_, resp_buf_);
      };
    } else if (msg_ == MSG_SELECT_TRIBUTE) {
      auto player = read_u8();
      to_decide_ = player;
      bool cancelable = read_u8();
      auto min = read_u8();
      auto max = read_u8();
      auto size = read_u8();
      std::vector<int> release_params;
      release_params.reserve(size);
      std::vector<std::string> specs;
      specs.reserve(size);
      if (verbose_) {
        std::vector<Card> cards;
        for (int i = 0; i < size; ++i) {
          auto code = read_u32();
          auto controller = read_u8();
          auto loc = read_u8();
          auto seq = read_u8();
          auto release_param = read_u8();
          Card card = get_card(controller, loc, seq);
          cards.push_back(card);
          release_params.push_back(release_param);
        }
        auto pl = players_[player];
        pl->notify("Select " + std::to_string(min) + " to " + std::to_string(max) + " cards to tribute separated by spaces:");
        for (const auto &card : cards) {
          auto spec = card.get_spec(player);
          specs.push_back(spec);
          pl->notify(spec + ": " + card.name_);
        }
      } else {
        for (int i = 0; i < size; ++i) {
          dp_ += 4;
          auto controller = read_u8();
          auto loc = read_u8();
          auto seq = read_u8();
          auto release_param = read_u8();

          auto spec = ls_to_spec(loc, seq, controller != player);
          specs.push_back(spec);

          release_params.push_back(release_param);
        }
      }

      bool has_weight = std::any_of(
        release_params.begin(), release_params.end(), [](int i) { return i != 1; });
      
      if (min != max) {
        auto err_str = "min: " + std::to_string(min) + ", max: " + std::to_string(max);
        throw std::runtime_error(err_str + ", not implemented");
      }

      std::vector<std::vector<int>> combs;
      if (has_weight) {
        combs = combinations_with_weight(release_params, min);
      } else {
        combs = combinations(size, min);
      }
      for (const auto &comb : combs) {
        std::string option = "";
        for (int j = 0; j < min; ++j) {
          option += specs[comb[j]];
          if (j < min - 1) {
            option += " ";
          }
        }
        options_.push_back(option);
      }

      callback_ = [this, combs](int idx) {
        const auto &comb = combs[idx];
        resp_buf_[0] = comb.size();
        for (int i = 0; i < comb.size(); ++i) {
          resp_buf_[i + 1] = comb[i];
        }
        set_responseb(pduel_, resp_buf_);
      };
    } else if (msg_ == MSG_SELECT_CHAIN) {
      auto player = read_u8();
      to_decide_ = player;
      auto size = read_u8();
      auto spe_count = read_u8();
      bool forced = read_u8();
      dp_ += 8;
      // auto hint_timing = read_u32();
      // auto other_timing = read_u32();

      std::vector<Card> cards;
      std::vector<uint32_t> descs;
      std::vector<uint32_t> spec_codes;
      for (int i = 0; i < size; ++i) {
        auto et = read_u8();
        uint32_t code = read_u32();
        if (verbose_) {
          uint32_t loc = read_u32();
          Card card = get_card_from_db(code);
          card.set_location(loc);          
          cards.push_back(card);
          spec_codes.push_back(card.get_spec_code(player));
        } else {
          PlayerId c = read_u8();
          uint8_t loc = read_u8();
          uint8_t seq = read_u8();
          uint8_t pos = read_u8();
          spec_codes.push_back(ls_to_spec_code(loc, seq, c != player));
        }
        uint32_t desc = read_u32();
        descs.push_back(desc);
      }

      if ((size == 0) && (spe_count == 0)) {
        // keep_processing = true;

        // non-GUI don't need this
        // if (verbose_) {
        //   printf("keep processing\n");
        // }
        set_responsei(pduel_, -1);
        return;
      }

      auto pl = players_[player];
      auto op = players_[1 - player];
      chaining_player_ = player;
      if (!op->seen_waiting_) {
        if (verbose_) {
          op->notify("Waiting for opponent.");
        }
        op->seen_waiting_ = true;
      }

      std::vector<int> chain_index;
      std::unordered_map<uint32_t, int> chain_counts;
      std::unordered_map<uint32_t, int> chain_orders;
      std::vector<std::string> chain_specs;
      std::vector<std::string> effect_descs;
      for (int i = 0; i < size; i++) {
        chain_index.push_back(i);
        chain_counts[spec_codes[i]] += 1;
      }
      for (int i = 0; i < size; i++) {
        auto spec_code = spec_codes[i];
        auto cs = code_to_spec(spec_code);
        auto chain_count = chain_counts[spec_code];
        if (chain_count > 1) {
          cs.push_back('a' + chain_orders[spec_code]);
        }
        chain_orders[spec_code]++;
        chain_specs.push_back(cs);
        if (verbose_) {
          const auto &card = cards[i];
          effect_descs.push_back(card.get_effect_description(descs[i], true));
        }
      }

      if (verbose_) {
        if (forced) {
          pl->notify("Select chain:");
        }
        else {
          pl->notify("Select chain (c to cancel):");
        }
        for (int i = 0; i < size; i++) {
          const auto &effect_desc = effect_descs[i];
          if (effect_desc.empty()) {
            pl->notify(chain_specs[i] + ": " + cards[i].name_);
          } else {
            pl->notify(chain_specs[i] + " (" + cards[i].name_ + "): " + effect_desc);
          }
        }
      }

      for (const auto& spec : chain_specs) {
        options_.push_back(spec);
      }
      if (!forced) {
        options_.push_back("c");
      }
      callback_ = [this, forced](int idx) {
        const auto &option = options_[idx];
        if ((option == "c") && (!forced)) {
          set_responsei(pduel_, -1);
          return;
        }
        set_responsei(pduel_, idx);
      };
    } else if (msg_ == MSG_SELECT_YESNO) {
      auto player = read_u8();
      to_decide_ = player;

      if (verbose_) {
        auto desc = read_u32();
        auto pl = players_[player];
        std::string opt;
        if (desc > 10000) {
          auto code = desc >> 4;
          auto card = get_card_from_db(code);
          opt = card.strings_[desc & 0xf];
          if (opt.empty()) {
            opt = "Unknown question from " + card.name_ + ". Yes or no?";
          }
        } else {
          opt = "TODO: system string " + std::to_string(desc) + ". Yes or no?";
        }
        pl->notify(opt);
        pl->notify("Please enter y or n.");
      }
      else {
        dp_ += 4;
      }
      options_ = {"y", "n"};
      callback_ = [this](int idx) {
        if (idx == 0) {
          set_responsei(pduel_, 1);
        } else if (idx == 1) {
          set_responsei(pduel_, 0);
        } else {
          throw std::runtime_error("Invalid option");
        }
      };
    } else if (msg_ == MSG_SELECT_EFFECTYN) {
      auto player = read_u8();
      to_decide_ = player;

      if (verbose_) {
        uint32_t code = read_u32();
        uint32_t loc = read_u32();
        Card card = get_card_from_db(code);
        card.set_location(loc);
        auto desc = read_u32();
        auto pl = players_[player];
        auto spec = card.get_spec(player);
        auto name = card.name_;
        std::string s;
        if (desc == 221) {
          s = "On " + card.get_spec(player) + ", Activate Trigger Effect of " + name + "?";
        } else if (desc == 0) {
          // From [%ls], activate [%ls]?
          s = "From " + card.get_spec(player) + ", activate " + name + "?";
        } else if (desc < 2048) {
          s = "TODO: system string " + std::to_string(desc) + "";
        } else {
          throw std::runtime_error("Unknown effectyn desc " + std::to_string(desc) + " of " + name);
        }
        pl->notify("Please enter y or n.");
      }
      else {
        dp_ += 12;
      }
      options_ = {"y", "n"};
      callback_ = [this](int idx) {
        if (idx == 0) {
          set_responsei(pduel_, 1);
        } else if (idx == 1) {
          set_responsei(pduel_, 0);
        } else {
          throw std::runtime_error("Invalid option");
        }
      };
    } else if (msg_ == MSG_SELECT_IDLECMD) {
      int32_t player = read_u8();
      to_decide_ = player;
      auto summonable_ = read_cardlist_spec();
      auto spsummon_ = read_cardlist_spec();
      auto repos_ = read_cardlist_spec();
      auto idle_mset_ = read_cardlist_spec();
      auto idle_set_ = read_cardlist_spec();
      auto idle_activate_ = read_cardlist_spec(true);
      bool to_bp_ = read_u8();
      bool to_ep_ = read_u8();
      read_u8(); // can_shuffle

      auto pl = players_[player];
      if (verbose_) {
        pl->notify("Select a card and action to perform.");
      }
      for (const auto &card : summonable_) {
        auto [code, spec, data] = card;
        std::string option = "s " + spec;
        options_.push_back(option);
        if (verbose_) {
          const auto &name = get_card_from_db(code).name_;
          pl->notify(option + ": Summon " + name +
                     " in face-up attack position.");
        }
      }
      for (const auto &card : idle_set_) {
        auto [code, spec, data] = card;
        std::string option = "t " + spec;
        options_.push_back(option);
        if (verbose_) {
          const auto &name = get_card_from_db(code).name_;
          pl->notify(option + ": Set " + name + ".");
        }
      }
      for (const auto &card : idle_mset_) {
        auto [code, spec, data] = card;
        std::string option = "m " + spec;
        options_.push_back(option);
        if (verbose_) {
          const auto &name = get_card_from_db(code).name_;
          pl->notify(option + ": Summon " + name +
                     " in face-down defense position.");
        }
      }
      for (const auto &card : repos_) {
        auto [code, spec, data] = card;
        std::string option = "r " + spec;
        options_.push_back(option);
        if (verbose_) {
          const auto &name = get_card_from_db(code).name_;
          pl->notify(option + ": Reposition " + name + ".");
        }
      }
      for (const auto &card : spsummon_) {
        auto [code, spec, data] = card;
        std::string option = "c " + spec;
        options_.push_back(option);
        if (verbose_) {
          const auto &name = get_card_from_db(code).name_;
          pl->notify(option + ": Special summon " + name + ".");
        }
      }
      std::map<std::string, int> idle_activate_count;
      for (const auto &[code, spec, data] : idle_activate_) {
        idle_activate_count[spec] += 1;
      }
      for (const auto &[code, spec, data] : idle_activate_) {
        int count = idle_activate_count[spec];
        if (count > 1) {
          Card card = get_card_from_db(code);
          printf("%s has %d effects\n", card.name_.c_str(), count);
          throw std::runtime_error("Activate more than one effect.");
        }
        std::string option = "v " + spec;
        options_.push_back(option);
        if (verbose_) {
          pl->notify(option + ": " + get_card_from_db(code).get_effect_description(data));
        }
      }

      if (to_bp_) {
        std::string cmd = "b";
        options_.push_back(cmd);
        if (verbose_) {
          pl->notify(cmd + ": Enter the battle phase.");
        }
      }
      if (to_ep_) {
        if (!to_bp_) {
          std::string cmd = "e";
          options_.push_back(cmd);
          if (verbose_) {
            pl->notify(cmd + ": End phase.");
          }
        }
      }
      callback_ = [this, summonable_, spsummon_, repos_, idle_mset_, idle_set_, idle_activate_](int idx) {

        const auto &option = options_[idx];
        char cmd = option[0];
        if (cmd == 'b') {
          set_responsei(pduel_, 6);
        } else if (cmd == 'e') {
          set_responsei(pduel_, 7);
        } else {
          auto spec = option.substr(2);
          if (cmd == 's') {
            auto it = std::find_if(
                summonable_.begin(), summonable_.end(),
                [&](const auto &card) { return std::get<1>(card) == spec; });
            if (it == summonable_.end()) {
              throw std::runtime_error("Invalid option for summon");
            }
            uint32_t idx = std::distance(summonable_.begin(), it);
            set_responsei(pduel_, idx << 16);
          } else if (cmd == 't') {
            auto it = std::find_if(
                idle_set_.begin(), idle_set_.end(),
                [&](const auto &card) { return std::get<1>(card) == spec; });
            if (it == idle_set_.end()) {
              throw std::runtime_error("Invalid option for set");
            }
            uint32_t idx = std::distance(idle_set_.begin(), it);
            set_responsei(pduel_, (idx << 16) + 4);
          } else if (cmd == 'm') {
            auto it = std::find_if(
                idle_mset_.begin(), idle_mset_.end(),
                [&](const auto &card) { return std::get<1>(card) == spec; });
            if (it == idle_mset_.end()) {
              throw std::runtime_error("Invalid option for mset");
            }
            uint32_t idx = std::distance(idle_mset_.begin(), it);
            set_responsei(pduel_, (idx << 16) + 3);
          } else if (cmd == 'r') {
            auto it = std::find_if(
                repos_.begin(), repos_.end(),
                [&](const auto &card) { return std::get<1>(card) == spec; });
            if (it == repos_.end()) {
              throw std::runtime_error("Invalid option for repos");
            }
            uint32_t idx = std::distance(repos_.begin(), it);
            set_responsei(pduel_, (idx << 16) + 2);
          } else if (cmd == 'c') {
            auto it = std::find_if(
                spsummon_.begin(), spsummon_.end(),
                [&](const auto &card) { return std::get<1>(card) == spec; });
            if (it == spsummon_.end()) {
              throw std::runtime_error("Invalid option for spsummon");
            }
            uint32_t idx = std::distance(spsummon_.begin(), it);
            set_responsei(pduel_, (idx << 16) + 1);
          } else if (cmd == 'v') {
            auto it = std::find_if(
                idle_activate_.begin(), idle_activate_.end(),
                [&](const auto &card) { return std::get<1>(card) == spec; });
            if (it == idle_activate_.end()) {
              throw std::runtime_error("Invalid option for activate");
            }
            uint32_t idx = std::distance(idle_activate_.begin(), it);
            set_responsei(pduel_, (idx << 16) + 5);
          } else {
            throw std::runtime_error("Invalid option: " + option);
          }
        }
      };
    } else if (msg_ == MSG_SELECT_PLACE) {
      auto player = read_u8();
      to_decide_ = player;
      auto count = read_u8();
      if (count == 0) {
        count = 1;
      }
      auto flag = read_u32();
      options_ = flag_to_usable_cardspecs(flag);
      if (verbose_) {
        std::string specs_str = options_[0];
        for (int i = 1; i < options_.size(); ++i) {
          specs_str += ", " + options_[i];
        }
        if (count == 1) {
          players_[player]->notify("Select place for card, one of " +
                                   specs_str + ".");
        } else {
          players_[player]->notify("Select " + std::to_string(count) +
                                   " places for card, from " + specs_str + ".");
        }
      }
      callback_ = [this, player](int idx) {
        int y = player + 1;
        std::string spec = options_[idx];
        auto plr = player;
        if (spec[0] == 'o') {
          plr = 1 - player;
          spec = spec.substr(1);
        }
        auto [loc, seq] = sepc_to_ls(spec);
        resp_buf_[0] = plr;
        resp_buf_[1] = loc;
        resp_buf_[2] = seq;
        set_responseb(pduel_, resp_buf_);
      };
    } else {
      if (verbose_) {
        printf("Unknown message %s, length %d, dp %d\n", msg_to_string(msg_).c_str(), dl_, dp_);
      }
      dp_ = dl_;
    }
  }

  void _damage(uint8_t player, uint32_t amount) {
    lp_[player] -= amount;
    if (verbose_) {
      auto lp = players_[player];
      lp->notify("Your lp decreased by " + std::to_string(amount) + ", now " + std::to_string(lp_[player]));
      players_[1-player]->notify(lp->nickname_ + "'s lp decreased by " + std::to_string(amount) + ", now " + std::to_string(lp_[player]));
    }
  }

  void _recover(uint8_t player, uint32_t amount) {
    lp_[player] += amount;
    if (verbose_) {
      auto lp = players_[player];
      lp->notify("Your lp increased by " + std::to_string(amount) + ", now " + std::to_string(lp_[player]));
      players_[1-player]->notify(lp->nickname_ + "'s lp increased by " + std::to_string(amount) + ", now " + std::to_string(lp_[player]));
    }
  }

  void _duel_end(uint8_t player, uint8_t reason) {
    winner_ = player;
    win_reason_ = reason;

    end_duel(pduel_);
    duel_started_ = false;
  }

};

using YGOProEnvPool = AsyncEnvPool<YGOProEnv>;

} // namespace ygopro

#endif // ENVPOOL_YGOPRO_YGOPRO_H_
