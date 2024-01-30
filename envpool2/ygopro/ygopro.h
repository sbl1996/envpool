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
#include <utility>
#include <vector>

namespace ygopro {

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

  int32_t controler_ = 0;
  uint32_t location_ = 0;
  uint32_t sequence_ = 0;
  uint32_t position_ = 0;

public:
  Card() = default;

  Card(
    uint32_t code, uint32_t alias, uint64_t setcode,
    uint32_t type, uint32_t level, uint32_t lscale, uint32_t rscale,
    int32_t attack, int32_t defense, uint32_t race, uint32_t attribute,
    uint32_t link_marker, const std::string &name, const std::string &desc,
    const std::vector<std::string> &strings) :
    code_(code), alias_(alias), setcode_(setcode),
    type_(type), level_(level), lscale_(lscale), rscale_(rscale),
    attack_(attack), defense_(defense), race_(race), attribute_(attribute),
    link_marker_(link_marker), name_(name), desc_(desc), strings_(strings) {}

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
};

static std::unordered_map<uint32_t, Card> cards;

inline Card query_card_from_db(uint32_t code) {
  SQLite::Statement query1(*db, "SELECT * FROM datas WHERE id=?");
  query1.bind(1, code);
  query1.executeStep();

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
  return Card(code, alias, setcode, type, level, lscale, rscale, attack, defense, race, attribute, link_marker, name, desc, strings);
}

inline const Card &get_card_from_db(uint32_t code) {
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

inline std::vector<std::string> flag_to_usable_cardspecs(uint32_t flag, bool reverse = false) {
    uint32_t pm = flag & 0xff;
    uint32_t ps = (flag >> 8) & 0xff;
    uint32_t om = (flag >> 16) & 0xff;
    uint32_t os = (flag >> 24) & 0xff;
    std::string zone_names[4] = {"m", "s", "om", "os"};
    uint32_t values[4] = {pm, ps, om, os};
    std::vector<std::string> specs;
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 8; i++) {
            bool avail;
            if (reverse) {
                avail = values[j] & ((1 << i) != 0);
            } else {
                avail = values[j] & ((1 << i) == 0);
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
  }
  else if (loc == LOCATION_MZONE) {
    spec += "m";
  }
  else if (loc == LOCATION_SZONE) {
    spec += "s";
  }
  else if (loc == LOCATION_GRAVE) {
    spec += "g";
  }
  else if (loc == LOCATION_REMOVED) {
    spec += "r";
  }
  else if (loc == LOCATION_EXTRA) {
    spec += "x";
  }
  spec += std::to_string(seq + 1);
  return spec;
}

inline std::tuple<uint8_t, uint8_t> sepc_to_ls(const std::string spec) {
  uint8_t loc;
  uint8_t seq;
  if (spec[0] == 'h') {
    loc = LOCATION_HAND;
  }
  else if (spec[0] == 'm') {
    loc = LOCATION_MZONE;
  }
  else if (spec[0] == 's') {
    loc = LOCATION_SZONE;
  }
  else if (spec[0] == 'g') {
    loc = LOCATION_GRAVE;
  }
  else if (spec[0] == 'r') {
    loc = LOCATION_REMOVED;
  }
  else if (spec[0] == 'x') {
    loc = LOCATION_EXTRA;
  }
  else {
    throw std::runtime_error("Invalid location");
  }
  seq = std::stoi(spec.substr(1)) - 1;
  return {loc, seq};
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

  const std::string &nickname() const { return nickname_; }

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

  // turn player
  int tp_;
  int current_phase_;

  int msg_;
  std::vector<std::string> options_;
  int to_decide_;
  std::function<void(int)> callback_;

  byte data_[4096];
  int dp_ = 0;
  int dl_ = 0;
  uint32_t res_;

  byte query_buf_[4096];

  byte resp_buf_[128];

  using IdleCardSpec = std::tuple<uint32_t, std::string, uint32_t>;

  // select_idlecmd
  std::vector<IdleCardSpec> summonable_;
  std::vector<IdleCardSpec> spsummon_;
  std::vector<IdleCardSpec> repos_;
  std::vector<IdleCardSpec> idle_mset_;
  std::vector<IdleCardSpec> idle_set_;
  std::vector<IdleCardSpec> idle_activate_;
  bool to_bp_;
  bool to_ep_;

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
    int act = action["action"_];

    callback_(act);
    if (verbose_) {
      printf("Msg: %d\n", msg_);
      printf("Player %d chose %s in [", to_decide_, options_[act].c_str());
      for (const auto &option : options_) {
        printf(" %s,", option.c_str());
      }
      printf(" ]\n");
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

  Card get_card(uint8_t player, uint8_t loc, uint8_t seq) {
    int32_t flags = QUERY_CODE | QUERY_ATTACK | QUERY_DEFENSE | QUERY_POSITION | QUERY_LEVEL | QUERY_RANK | QUERY_LSCALE | QUERY_RSCALE | QUERY_LINK;
    int32_t bl = query_card(pduel_, player, loc, seq, flags, query_buf_, 0);
    if (bl <= 0) {
      throw std::runtime_error("Invalid card");
    }
    uint32_t f = read_u32();
    if (f == 4) {
      throw std::runtime_error("Invalid card");
    }
    f = read_u32();
    uint32_t code = read_u32();
    Card c = get_card_from_db(code);
    uint32_t position = read_u32();
    c.set_location(position);
    uint32_t level = read_u32();
    if ((level & 0xff) > 0) {
      c.level_ = level & 0xff;
    }
    uint32_t rank = read_u32();
    if ((rank & 0xff) > 0) {
      c.level_ = rank & 0xff;
    }
    c.attack_ = read_u32();
    c.defense_ = read_u32();
    c.lscale_ = read_u32();
    c.rscale_ = read_u32();
    uint32_t link = read_u32();
    uint32_t link_marker = read_u32();
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
        }
        else {
          card.data_ = read_u32();
        }
      }
      cards.push_back(card);
    }
    return cards;
  }

  std::vector<IdleCardSpec> read_cardlist_spec(bool extra = false, bool extra8 = false) {
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
        }
        else {
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
      printf("Msg: %d, dp: %d, dl: %d\n", msg_, dp_, dl_);
    }
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
        pl->notify(std::to_string(i + 1) + ": " + c.name_);
      }
      const auto &op = players_[1 - player];
      op->notify("Opponent drew " + std::to_string(drawed) + " cards.");
    }
    else if (msg_ == MSG_NEW_TURN) {
      tp_ = int(read_u8());
      if (!verbose_) {
        return;
      }
      auto player = players_[tp_];
      player->notify("Your turn.");
      players_[1 - tp_]->notify(player->nickname() + "'s turn.");
    }
    else if (msg_ == MSG_NEW_PHASE) {
      current_phase_ = int(read_u16());
      if (!verbose_) {
        return;
      }
      auto phase_str = phase_to_string(current_phase_);
      for (int i = 0; i < 2; ++i) {
        players_[i]->notify("entering " + phase_str + ".");
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
    else if (msg_ == MSG_SELECT_IDLECMD) {
      int32_t player = read_u8();
      summonable_ = read_cardlist_spec();
      spsummon_ = read_cardlist_spec();
      repos_ = read_cardlist_spec();
      idle_mset_ = read_cardlist_spec();
      idle_set_ = read_cardlist_spec();
      idle_activate_ = read_cardlist_spec();
      to_bp_ = read_u8();
      to_ep_ = read_u8();
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
          pl->notify(option + ": Summon " + name + " in face-up attack position.");
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
          pl->notify(option + ": Summon " + name + " in face-down defense position.");
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
      std::map<uint32_t, int> idle_activate_count;
      for (const auto &card : idle_activate_) {
        auto [code, spec, data] = card;
        idle_activate_count[code] += 1;
      }
      for (const auto &card : idle_activate_) {
        auto [code, spec, data] = card;
        int count = idle_activate_count[code];
        if (count > 1) {
          throw std::runtime_error("Activate more than one effect.");
        }
        std::string option = "v " + spec;
        options_.push_back(option);
        if (verbose_) {
          // TODO: effect description
          pl->notify(option + ": Activate " + get_card_from_db(code).name_ + ".");
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
      auto callback = [this](int idx) {
        const auto &option = options_[idx];
        char cmd = option[0];
        if (cmd == 'b') {
          set_responsei(pduel_, 6);
        }
        else if (cmd == 'e') {
          set_responsei(pduel_, 7);
        }
        else {
          auto spec = option.substr(2);
          if (cmd == 's') {
            auto it = std::find_if(summonable_.begin(), summonable_.end(), [&](const auto &card) {
              return std::get<1>(card) == spec;
            });
            if (it == summonable_.end()) {
              throw std::runtime_error("Invalid option for summon");
            }
            uint32_t idx = std::distance(summonable_.begin(), it);
            set_responsei(pduel_, idx << 16);
          }
          else if (cmd == 't') {
            auto it = std::find_if(idle_set_.begin(), idle_set_.end(), [&](const auto &card) {
              return std::get<1>(card) == spec;
            });
            if (it == idle_set_.end()) {
              throw std::runtime_error("Invalid option for set");
            }
            uint32_t idx = std::distance(idle_set_.begin(), it);
            set_responsei(pduel_, (idx << 16) + 4);
          }
          else if (cmd == 'm') {
            auto it = std::find_if(idle_mset_.begin(), idle_mset_.end(), [&](const auto &card) {
              return std::get<1>(card) == spec;
            });
            if (it == idle_mset_.end()) {
              throw std::runtime_error("Invalid option for mset");
            }
            uint32_t idx = std::distance(idle_mset_.begin(), it);
            set_responsei(pduel_, (idx << 16) + 3);
          }
          else if (cmd == 'r') {
            auto it = std::find_if(repos_.begin(), repos_.end(), [&](const auto &card) {
              return std::get<1>(card) == spec;
            });
            if (it == repos_.end()) {
              throw std::runtime_error("Invalid option for repos");
            }
            uint32_t idx = std::distance(repos_.begin(), it);
            set_responsei(pduel_, (idx << 16) + 2);
          }
          else if (cmd == 'c') {
            auto it = std::find_if(spsummon_.begin(), spsummon_.end(), [&](const auto &card) {
              return std::get<1>(card) == spec;
            });
            if (it == spsummon_.end()) {
              throw std::runtime_error("Invalid option for spsummon");
            }
            uint32_t idx = std::distance(spsummon_.begin(), it);
            set_responsei(pduel_, (idx << 16) + 1);
          }
          else if (cmd == 'v') {
            auto it = std::find_if(idle_activate_.begin(), idle_activate_.end(), [&](const auto &card) {
              return std::get<1>(card) == spec;
            });
            if (it == idle_activate_.end()) {
              throw std::runtime_error("Invalid option for activate");
            }
            uint32_t idx = std::distance(idle_activate_.begin(), it);
            set_responsei(pduel_, (idx << 16) + 5);
          }
          else {
            throw std::runtime_error("Invalid option: " + option);
          }
        }
      };
      callback_ = callback;
    }
    else if (msg_ == MSG_SELECT_PLACE) {
      auto player = read_u8();
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
          players_[player]->notify("Select place for card, one of " + specs_str + ".");
        }
        else {
          players_[player]->notify("Select " + std::to_string(count) + " places for card, from " + specs_str + ".");
        }
      }
      callback_ = [this, player1 = player](int idx) {
        printf("Select place %d, player %d\n", idx, player1);
        std::string spec = options_[idx];
        auto plr = player1;
        if (spec[0] == 'o') {
          plr = 1 - player1;
          spec = spec.substr(1);
        }
        auto [loc, seq] = sepc_to_ls(spec);
        resp_buf_[0] = plr;
        resp_buf_[1] = loc;
        resp_buf_[2] = seq;
        set_responseb(pduel_, resp_buf_);
      };
    }
    else if (msg_ == MSG_HINT) {
      auto hint_type = int(read_u8());
      auto player = read_u8();
      auto value = read_u32();
      if (!verbose_) {
        return;
      }
      if (hint_type == HINT_SELECTMSG) {
        if (value > 2000) {
          uint32_t code = value;
          players_[player]->notify(std::to_string(player) + " select " + get_card_from_db(code).name_);
        }
        else {
          players_[player]->notify("TODO: system string " + std::to_string(value));
        }
      }
      else if (hint_type == HINT_NUMBER) {
        players_[1-player]->notify("Choice of player: [" + std::to_string(value) + "]");
      }
      else {
        printf("Unknown hint type %d with value %d\n", hint_type, value);
      }
    }
    else if (msg_  == MSG_RETRY) {
      if (verbose_) {
        printf("Retry\n");
        throw std::runtime_error("Retry");
      }
    }
    else {
      if (verbose_) {
        printf("Unknown message %d, length %d, dp %d\n", msg_, dl_, dp_);
      }
      dp_ = dl_;
    }
  }
};

using YGOProEnvPool = AsyncEnvPool<YGOProEnv>;

} // namespace ygopro

#endif // ENVPOOL_YGOPRO_YGOPRO_H_
