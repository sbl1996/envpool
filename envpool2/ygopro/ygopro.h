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

// clang-format off
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iostream>
#include <optional>
#include <random>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "envpool2/core/array.h"
#include "envpool2/core/async_envpool.h"
#include "envpool2/core/env.h"

#include "envpool2/core/spec.h"
#include "ygopro-core/common.h"
#include "ygopro-core/card_data.h"
#include "ygopro-core/field.h"
#include "ygopro-core/ocgapi.h"

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

#include "envpool2/ygopro/common.h"
// clang-format on

namespace ygopro {

using PlayerId = uint8_t;

// TODO: 7% performance loss
static std::shared_timed_mutex duel_mtx;

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
  const uint32_t &type() const { return type_; }
  const uint32_t &level() const { return level_; }
  const std::vector<std::string> &strings() const { return strings_; }

  std::string get_spec(bool opponent) const {
    return ls_to_spec(location_, sequence_, position_, opponent);
  }

  std::string get_spec(PlayerId player) const {
    return get_spec(player != controler_);
  }

  uint32_t get_spec_code(PlayerId player) const {
    return ls_to_spec_code(location_, sequence_, position_, player != controler_);
  }

  std::string get_position() const { return position_to_string(position_); }

  std::string get_effect_description(uint32_t desc,
                                     bool existing = false) const {
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
      } else {
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

inline Card db_query_card(const SQLite::Database &db, uint32_t code) {
  SQLite::Statement query1(db, "SELECT * FROM datas WHERE id=?");
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

  SQLite::Statement query2(db, "SELECT * FROM texts WHERE id=?");
  query2.bind(1, code);
  query2.executeStep();

  std::string name = query2.getColumn(1);
  std::string desc = query2.getColumn(2);
  std::vector<std::string> strings;
  for (int i = 3; i < query2.getColumnCount(); ++i) {
    std::string str = query2.getColumn(i);
    strings.push_back(str);
  }
  return Card(code, alias, setcode, type, level, lscale, rscale, attack,
              defense, race, attribute, link_marker, name, desc, strings);
}

inline card_data db_query_card_data(const SQLite::Database &db, uint32_t code) {
  SQLite::Statement query(db, "SELECT * FROM datas WHERE id=?");
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

// TODO: use faster map (martinus/unordered_dense)
static std::unordered_map<uint32_t, Card> cards_;
static std::unordered_map<uint32_t, uint32_t> card_ids_;
static std::unordered_map<uint32_t, card_data> cards_data_;
static std::unordered_map<std::string, std::vector<uint32_t>> main_decks_;
static std::unordered_map<std::string, std::vector<uint32_t>> extra_decks_;

inline const Card &c_get_card(uint32_t code) { return cards_.at(code); }

inline uint32_t &c_get_card_id(uint32_t code) { return card_ids_.at(code); }

inline void sort_extra_deck(std::vector<uint32_t> &deck) {
  std::vector<uint32_t> c;
  std::vector<std::pair<uint32_t, int>> fusion, xyz, synchro, link;

  for (auto code : deck) {
    const Card &cc = c_get_card(code);
    if (cc.type() & TYPE_FUSION) {
      fusion.push_back({code, cc.level()});
    } else if (cc.type() & TYPE_XYZ) {
      xyz.push_back({code, cc.level()});
    } else if (cc.type() & TYPE_SYNCHRO) {
      synchro.push_back({code, cc.level()});
    } else if (cc.type() & TYPE_LINK) {
      link.push_back({code, cc.level()});
    } else {
      throw std::runtime_error("Not extra deck card");
    }
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

  deck = c;
}

inline void preload_deck(const SQLite::Database &db,
                         const std::vector<uint32_t> &deck) {
  for (const auto &code : deck) {
    auto it = cards_.find(code);
    if (it == cards_.end()) {
      cards_[code] = db_query_card(db, code);
      if (card_ids_.find(code) == card_ids_.end()) {
        throw std::runtime_error("Card not found in code list: " +
                                 std::to_string(code));
      }
    }

    auto it2 = cards_data_.find(code);
    if (it2 == cards_data_.end()) {
      cards_data_[code] = db_query_card_data(db, code);
    }
  }
}

inline uint32 card_reader_callback(uint32 code, card_data *card) {
  auto it = cards_data_.find(code);
  if (it == cards_data_.end()) {
    throw std::runtime_error("Card not found: " + std::to_string(code));
  }
  *card = it->second;
  return 0;
}

static void
init_module(const std::string &db_path, const std::string &code_list_file,
            const std::unordered_map<std::string, std::string> &decks) {
  // parse code from code_list_file
  std::ifstream file(code_list_file);
  std::string line;
  int i = 0;
  while (std::getline(file, line)) {
    i++;
    uint32_t code = std::stoul(line);
    card_ids_[code] = i;
  }

  SQLite::Database db(db_path, SQLite::OPEN_READONLY);

  for (const auto &[name, deck] : decks) {
    std::vector<uint32_t> main_deck = read_main_deck(deck);
    std::vector<uint32_t> extra_deck = read_extra_deck(deck);
    main_decks_[name] = main_deck;
    extra_decks_[name] = extra_deck;

    preload_deck(db, main_deck);
    preload_deck(db, extra_deck);
  }

  for (auto &[name, deck] : extra_decks_) {
    sort_extra_deck(deck);
  }

  set_card_reader(card_reader_callback);
  set_script_reader(script_reader_callback);
}

class YGOProEnvFns {
public:
  static decltype(auto) DefaultConfig() {
    return MakeDict("deck1"_.Bind(std::string("OldSchool")),
                    "deck2"_.Bind(std::string("OldSchool")), "player"_.Bind(-1),
                    "play_mode"_.Bind(std::string("bot")),
                    "verbose"_.Bind(false), "max_options"_.Bind(16),
                    "max_cards"_.Bind(55), "n_history_actions"_.Bind(8));
  }
  template <typename Config>
  static decltype(auto) StateSpec(const Config &conf) {
    return MakeDict(
        "obs:cards_"_.Bind(Spec<uint8_t>({conf["max_cards"_] * 2, 37})),
        "obs:global_"_.Bind(Spec<uint8_t>({8})),
        "obs:actions_"_.Bind(Spec<uint8_t>({conf["max_options"_], 7})),
        "obs:history_actions_"_.Bind(Spec<uint8_t>({conf["n_history_actions"_], 7})),
        "info:num_options"_.Bind(Spec<int>({}, {0, conf["max_options"_] - 1})));
  }
  template <typename Config>
  static decltype(auto) ActionSpec(const Config &conf) {
    return MakeDict(
        "action"_.Bind(Spec<int>({}, {0, conf["max_options"_] - 1})));
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
  std::vector<uint32> main_deck1_;
  std::vector<uint32> main_deck2_;
  std::vector<uint32> extra_deck1_;
  std::vector<uint32> extra_deck2_;

  int player_;
  const std::string play_mode_;
  bool verbose_ = false;

  int max_episode_steps_, elapsed_step_;
  intptr_t pduel_;
  PlayerId ai_player_;
  Player *players_[2];

  std::uniform_int_distribution<uint64_t> dist_int_;
  bool done_{true};
  bool duel_started_{false};
  bool keep_processing_{false};
  uint32_t eng_flag_{0};

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

  byte query_buf_[4096];
  int qdp_ = 0;

  byte resp_buf_[128];

  using IdleCardSpec = std::tuple<uint32_t, std::string, uint32_t>;

  // chain
  PlayerId chaining_player_;

  // feature
  int max_options_;
  int max_cards_;

  double step_time_ = 0;
  uint64_t step_time_count_ = 0;

  double reset_time_ = 0;
  uint64_t reset_time_count_ = 0;

  const int n_history_actions_;
  TArray<uint8_t> history_actions_;
  int ha_p_ = 0;
  std::vector<uint32_t> prev_code_ids_;

public:
  YGOProEnv(const Spec &spec, int env_id)
      : Env<YGOProEnvSpec>(spec, env_id),
        max_episode_steps_(spec.config["max_episode_steps"_]),
        elapsed_step_(max_episode_steps_ + 1), dist_int_(0, 0xffffffff),
        main_deck1_(main_decks_.at(spec.config["deck1"_])),
        main_deck2_(main_decks_.at(spec.config["deck2"_])),
        extra_deck1_(extra_decks_.at(spec.config["deck1"_])),
        extra_deck2_(extra_decks_.at(spec.config["deck2"_])),
        player_(spec.config["player"_]), play_mode_(spec.config["play_mode"_]),
        verbose_(spec.config["verbose"_]), max_options_(spec.config["max_options"_]),
        max_cards_(spec.config["max_cards"_]), n_history_actions_(spec.config["n_history_actions"_]) {

    history_actions_ = TArray<uint8_t>(Array(ShapeSpec(sizeof(uint8_t), {n_history_actions_, 7})));
    prev_code_ids_ = std::vector<uint32_t>(max_options_, 0);
    // if (verbose_) {
    //   std::cout << "Loaded " << main_deck1_.size()
    //             << " cards in main deck 1 and " << extra_deck1_.size()
    //             << " cards in extra deck 1" << std::endl;
    //   std::cout << "Loaded " << main_deck1_.size()
    //             << " cards in main deck 1 and " << extra_deck1_.size()
    //             << " cards in extra deck 1" << std::endl;
    // }
  }

  ~YGOProEnv() {
    for (int i = 0; i < 2; i++) {
      if (players_[i] != nullptr) {
        delete players_[i];
      }
    }
  }

  bool IsDone() override { return done_; }

  void Reset() override {
    // clock_t start = clock();

    if (player_ == -1) {
      ai_player_ = dist_int_(gen_) % 2;
    } else {
      ai_player_ = player_;
    }

    history_actions_.Zero();

    unsigned long duel_seed = dist_int_(gen_);

    std::unique_lock<std::shared_timed_mutex> ulock(duel_mtx);
    pduel_ = create_duel(duel_seed);
    ulock.unlock();

    for (PlayerId i = 0; i < 2; i++) {
      if (players_[i] != nullptr) {
        delete players_[i];
      }
      std::string nickname = i == 0 ? "Alice" : "Bob";
      int init_lp = 8000;
      if ((i != ai_player_) && (play_mode_ == "human")) {
        players_[i] = new HumanPlayer(nickname, init_lp, i, verbose_);
      } else {
        players_[i] = new GreedyAI(nickname, init_lp, i, verbose_);
      }
      set_player_info(pduel_, i, init_lp, 5, 1);
      load_deck(i);
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

    next();

    done_ = false;
    elapsed_step_ = 0;
    WriteState(0.0);

    // double seconds = static_cast<double>(clock() - start) / CLOCKS_PER_SEC;
    // // update reset_time by moving average
    // reset_time_ = reset_time_* (static_cast<double>(reset_time_count_) / (reset_time_count_ + 1)) + seconds / (reset_time_count_ + 1);
    // reset_time_count_++;
    // if (reset_time_count_ % 20 == 0) {
    //   printf("Reset time: %.3f\n", reset_time_);
    // }

  }

  void update_history_actions(int idx, int msg, const std::string &option, uint32_t code_id = 0) {
    _set_obs_action(history_actions_, ha_p_, msg, option, {}, code_id);
    ha_p_ = (ha_p_ + 1) % n_history_actions_;
  }

  void Step(const Action &action) override {
    // clock_t start = clock();

    int idx = action["action"_];
    callback_(idx);
    update_history_actions(idx, msg_, options_[idx], prev_code_ids_[idx]);

    if (verbose_) {
      show_decision(idx);
    }

    next();
    float reward = 0;
    if (done_) {
      reward = winner_ == ai_player_ ? 1.0 : -1.0;
    }

    WriteState(reward);

    // double seconds = static_cast<double>(clock() - start) / CLOCKS_PER_SEC;
    // // update step_time by moving average
    // step_time_ = step_time_* (static_cast<double>(step_time_count_) / (step_time_count_ + 1)) + seconds / (step_time_count_ + 1);
    // step_time_count_++;
    // if (step_time_count_ % 500 == 0) {
    //   printf("Step time: %.3f\n", step_time_);
    // }
  }

private:
  void _set_obs_cards(TArray<uint8_t> &feat,
                      std::unordered_map<std::string, int> &spec2index,
                      PlayerId player, bool opponent) {
    const auto &shape = feat.Shape();
    auto n1 = shape[0];
    auto n2 = shape[1];
    int offset = opponent ? n1 / 2 : 0;
    std::vector<std::pair<uint8_t, bool>> configs = {
        {LOCATION_DECK, true},   {LOCATION_HAND, true},
        {LOCATION_MZONE, false}, {LOCATION_SZONE, false},
        {LOCATION_GRAVE, false}, {LOCATION_REMOVED, false},
        {LOCATION_EXTRA, true},
    };
    for (const auto &[location, hidden_for_opponent] : configs) {
      if (opponent && hidden_for_opponent) {
        auto n_cards = query_field_count(pduel_, player, location);
        for (auto i = 0; i < n_cards; i++) {
          feat(offset, 1) = location2id.at(location);
          feat(offset, 3) = 1;
          offset++;
        }
      } else {
        std::vector<Card> cards = get_cards_in_location(player, location);
        for (int i = 0; i < cards.size(); ++i) {
          const auto &c = cards[i];
          bool hide = opponent && (c.position_ & POS_FACEDOWN);
          bool overlay = c.location_ & LOCATION_OVERLAY;
          if (overlay) {
            hide = false;
          }

          if (!hide) {
            feat(offset, 0) = c_get_card_id(c.code_);
          }
          feat(offset, 1) = location2id.at(location);
          feat(offset, 2) = c.sequence_ + 1;
          feat(offset, 3) = opponent ? 1 : 0;
          if (overlay) {
            feat(offset, 4) = position2id.at(POS_FACEUP);
            feat(offset, 5) = 1;
          } else {
            feat(offset, 4) = position2id.at(c.position_);
          }
          if (!hide) {
            feat(offset, 6) = attribute2id.at(c.attribute_);
            feat(offset, 7) = race2id.at(c.race_);
            feat(offset, 8) = c.level_;
            auto [atk1, atk2] = float_transform(c.attack_);
            feat(offset, 9) = atk1;
            feat(offset, 10) = atk2;

            auto [def1, def2] = float_transform(c.defense_);
            feat(offset, 11) = def1;
            feat(offset, 12) = def2;

            auto type_ids = type_to_ids(c.type_);
            for (int j = 0; j < type_ids.size(); ++j) {
              feat(offset, 13 + j) = type_ids[j];
            }
          }

          offset++;

          spec2index[c.get_spec(opponent)] = offset;
        }
      }
    }
  }

  void _set_obs_global(TArray<uint8_t> &feat, PlayerId player) {
    uint8_t me = player;
    uint8_t op = 1 - player;

    auto [me_lp_1, me_lp_2] = float_transform(lp_[me]);
    feat(0) = me_lp_1;
    feat(1) = me_lp_2;

    auto [op_lp_1, op_lp_2] = float_transform(lp_[op]);
    feat(2) = op_lp_1;
    feat(3) = op_lp_2;

    feat(4) = phase2id.at(current_phase_);
    feat(5) = (me == 0) ? 1 : 0;
    feat(6) = (me == tp_) ? 1 : 0;
  }

  void _set_obs_action_spec(
    TArray<uint8_t> &feat, int i, const std::string &spec,
    const std::unordered_map<std::string, int> &spec2index, uint8_t code_id = 0) {
    if (spec2index.empty()) {
      feat(i, 0) = code_id;
    } else {
      feat(i, 0) = spec2index.at(spec);
    }
  }

  void _set_obs_action_msg(TArray<uint8_t> &feat, int i, int msg) {
    feat(i, 1) = msg2id.at(msg);
  }

  void _set_obs_action_act(TArray<uint8_t> &feat, int i, char act) {
    feat(i, 2) = cmd_act2id.at(act);
  }

  void _set_obs_action_yesno(TArray<uint8_t> &feat, int i, char yesno) {
    feat(i, 3) = cmd_yesno2id.at(yesno);
  }

  void _set_obs_action_phase(TArray<uint8_t> &feat, int i, char phase) {
    feat(i, 4) = cmd_phase2id.at(phase);
  }

  void _set_obs_action_cancel(TArray<uint8_t> &feat, int i, bool cancel) {
    feat(i, 5) = cancel;
  }

  void _set_obs_action_position(TArray<uint8_t> &feat, int i, char position) {
    position = 1 << (position - '0' - 1);
    feat(i, 6) = position2id.at(position);
  }

  void _set_obs_action(TArray<uint8_t> &feat, int i, int msg, const std::string &option,
                       const std::unordered_map<std::string, int> &spec2index, uint8_t code_id = 0) {
    _set_obs_action_msg(feat, i, msg);
    if (msg == MSG_SELECT_IDLECMD) {
      if (option == "b" || option == "e") {
        _set_obs_action_phase(feat, i, option[0]);
      } else {
        auto act = option[0];
        _set_obs_action_act(feat, i, act);

        auto spec = option.substr(2);
        _set_obs_action_spec(feat, i, spec, spec2index, code_id);
      }
    } else if (msg == MSG_SELECT_CHAIN) {
      if (option == "c") {
        _set_obs_action_cancel(feat, i, true);
      } else {
        _set_obs_action_spec(feat, i, option, spec2index, code_id);
      }
    } else if (msg == MSG_SELECT_CARD || msg == MSG_SELECT_TRIBUTE) {
      // TODO: Multi-select
      auto idx = option.find_first_of(" ");
      if (idx == std::string::npos) {
        _set_obs_action_spec(feat, i, option, spec2index, code_id);
      } else {
        _set_obs_action_spec(feat, i, option.substr(0, idx), spec2index, code_id);
      }
    } else if (msg == MSG_SELECT_UNSELECT_CARD) {
      _set_obs_action_spec(feat, i, option, spec2index, code_id);
    } else if (msg == MSG_SELECT_POSITION) {
      _set_obs_action_position(feat, i, option[0]);
    } else if (msg == MSG_SELECT_EFFECTYN || msg == MSG_SELECT_YESNO) {
      _set_obs_action_yesno(feat, i, option[0]);
    } else if (msg == MSG_SELECT_BATTLECMD) {
      if (option == "m" || option == "e") {
        _set_obs_action_phase(feat, i, option[0]);
      } else {
        auto act = option[0];
        auto spec = option.substr(2);
        _set_obs_action_act(feat, i, act);
        _set_obs_action_spec(feat, i, spec, spec2index, code_id);
      }
    } else {
      if (verbose_) {
        printf("Unsupported message %s\n", msg_to_string(msg).c_str());
      } else {
        throw std::runtime_error("Unsupported message " + std::to_string(msg));
      }
    }
  }

  uint8_t spec_to_card_id(const std::string &spec) {
    PlayerId player = ai_player_;
    int offset = 0;
    if (spec[0] == 'o') {
      player = 1 - ai_player_;
      offset++;
    }
    auto [loc, seq, pos] = spec_to_ls(spec.substr(offset));
    return card_ids_.at(get_card_code(player, loc, seq));
  }

  uint8_t get_prev_code_id(const std::string &option) {
    uint8_t code_id = 0;
    if (msg_ == MSG_SELECT_IDLECMD) {
      if (!(option == "b" || option == "e")) {
        code_id = spec_to_card_id(option.substr(2));
      }
    } else if (msg_ == MSG_SELECT_CHAIN) {
      if (option != "c") {
        code_id = spec_to_card_id(option);
      }
    } else if (msg_ == MSG_SELECT_CARD || msg_ == MSG_SELECT_TRIBUTE) {
      // TODO: Multi-select
      auto idx = option.find_first_of(" ");
      if (idx == std::string::npos) {
        code_id = spec_to_card_id(option);
      } else {
        code_id = spec_to_card_id(option.substr(0, idx));
      }
    } else if (msg_ == MSG_SELECT_UNSELECT_CARD) {
      code_id = spec_to_card_id(option);
    } else if (msg_ == MSG_SELECT_BATTLECMD) {
      if (!(option == "m" || option == "e")) {
        code_id = spec_to_card_id(option.substr(2));
      }
    }
    return code_id;
  }

  void _set_obs_actions(TArray<uint8_t> &feat,
                        const std::unordered_map<std::string, int> &spec2index,
                        int msg, const std::vector<std::string> &options) {
    for (int i = 0; i < options.size(); ++i) {
      _set_obs_action(feat, i, msg, options[i], spec2index);
    }
  }

  void WriteState(float reward) {
    State state = Allocate();

    int n_options = options_.size();
    state["reward"_] = reward;

    if (n_options == 0) {
      state["info:num_options"_] = 1;
      state["obs:global_"_][7] = uint8_t(1);
      return;
    }

    std::unordered_map<std::string, int> spec2index;
    _set_obs_cards(state["obs:cards_"_], spec2index, ai_player_, false);
    _set_obs_cards(state["obs:cards_"_], spec2index, 1 - ai_player_, true);
    _set_obs_global(state["obs:global_"_], ai_player_);

    // we can't shuffle because idx must be stable in callback
    if (n_options > max_options_) {
      options_.resize(max_options_);
    }

    // print spec2index
    // for (auto const& [key, val] : spec2index) {
    //   printf("%s %d\n", key.c_str(), val);
    // }


    n_options = options_.size();
    state["info:num_options"_] = n_options;

    _set_obs_actions(state["obs:actions_"_], spec2index, msg_, options_);

    for (int i = 0; i < n_options; ++i) {
      uint8_t spec_index = state["obs:actions_"_](i, 0);
      if (spec_index == 0) {
        prev_code_ids_[i] = 0;
      } else {
        prev_code_ids_[i] = state["obs:cards_"_](spec_index, 0);
      }
    }

    int n1 = n_history_actions_ - ha_p_;
    state["obs:history_actions_"_].Assign((uint8_t *)history_actions_[ha_p_].Data(), 7 * n1);
    state["obs:history_actions_"_][n1].Assign((uint8_t *)history_actions_.Data(), 7 * ha_p_);

  }

  void show_decision(int idx) {
    printf("Player %d chose '%s' in [", to_decide_, options_[idx].c_str());
    int n = options_.size();
    for (int i = 0; i < n; ++i) {
      printf(" '%s'", options_[i].c_str());
      if (i < n - 1) {
        printf(",");
      }
    }
    printf(" ]\n");
  }

  void load_deck(PlayerId player, bool shuffle = true) {
    std::vector<uint32_t> &main_deck = player == 0 ? main_deck1_ : main_deck2_;
    std::vector<uint32_t> &extra_deck =
        player == 0 ? extra_deck1_ : extra_deck2_;

    if (shuffle) {
      std::shuffle(main_deck.begin(), main_deck.end(), gen_);
    }

    // add main deck in reverse order following ygopro
    // but since we have shuffled deck, so just add in order
    for (int i = 0; i < main_deck.size(); i++) {
      new_card(pduel_, main_deck[i], player, player, LOCATION_DECK, 0,
               POS_FACEDOWN_DEFENSE);
    }

    // add extra deck in reverse order following ygopro
    for (int i = extra_deck.size() - 1; i >= 0; --i) {
      new_card(pduel_, extra_deck[i], player, player, LOCATION_EXTRA, 0,
               POS_FACEDOWN_DEFENSE);
    }
  }

  void next() {
    while (duel_started_) {
      if (eng_flag_ == PROCESSOR_END) {
        break;
      }
      uint32_t res = process(pduel_);
      dl_ = res & PROCESSOR_BUFFER_LEN;
      eng_flag_ = res & PROCESSOR_FLAG;

      if (dl_ == 0) {
        continue;
      }
      get_message(pduel_, data_);
      dp_ = 0;
      while (dp_ != dl_) {
        handle_message();
        if (options_.empty()) {
          continue;
        }
        if (to_decide_ == ai_player_) {
          if (msg_ == MSG_SELECT_PLACE) {
            callback_(0);
            // update_history_actions(0, msg_, options_[0]);
            if (verbose_) {
              show_decision(0);
            }
          } else if (options_.size() == 1) {
            callback_(0);
            prev_code_ids_[0] = get_prev_code_id(options_[0]);
            update_history_actions(0, msg_, options_[0], prev_code_ids_[0]);
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
    }
    done_ = true;
    options_.clear();
  }

  uint8_t read_u8() { return data_[dp_++]; }

  uint16_t read_u16() {
    uint16_t v = *reinterpret_cast<uint16_t*>(data_ + dp_);
    dp_ += 2;
    return v;
  }

  uint32 read_u32() {
    uint32 v = *reinterpret_cast<uint32_t*>(data_ + dp_);
    dp_ += 4;
    return v;
  }

  uint32 q_read_u8() {
    uint8_t v = *reinterpret_cast<uint8_t*>(query_buf_ + qdp_);
    qdp_ += 1;
    return v;
  }

  uint32 q_read_u32() {
    uint32_t v = *reinterpret_cast<uint32_t*>(query_buf_ + qdp_);
    qdp_ += 4;
    return v;
  }

  uint32_t get_card_code(PlayerId player, uint8_t loc, uint8_t seq) {
    int32_t flags = QUERY_CODE;
    int32_t bl = query_card(pduel_, player, loc, seq, flags, query_buf_, 0);
    qdp_ = 0;
    if (bl <= 0) {
      throw std::runtime_error("Invalid card");
    }
    qdp_ += 8;
    uint32_t code = q_read_u32();
    return code;
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
    Card c = c_get_card(code);
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

  std::vector<Card> get_cards_in_location(PlayerId player, uint8_t loc) {
    int32_t flags = QUERY_CODE | QUERY_POSITION | QUERY_LEVEL | QUERY_RANK |
                    QUERY_ATTACK | QUERY_DEFENSE | QUERY_EQUIP_CARD |
                    QUERY_OVERLAY_CARD | QUERY_COUNTERS | QUERY_LSCALE |
                    QUERY_RSCALE | QUERY_LINK;
    int32_t bl = query_field_card(pduel_, player, loc, flags, query_buf_, 0);
    qdp_ = 0;
    std::vector<Card> cards;
    while (true) {
      if (qdp_ >= bl) {
        break;
      }
      uint32_t f = q_read_u32();
      if (f == LEN_EMPTY) {
        continue;
        ;
      }
      f = q_read_u32();
      uint32_t code = q_read_u32();
      Card c = c_get_card(code);

      uint8_t controller = q_read_u8();
      uint8_t location = q_read_u8();
      uint8_t sequence = q_read_u8();
      uint8_t position = q_read_u8();
      c.controler_ = controller;
      c.location_ = location;
      c.sequence_ = sequence;
      c.position_ = position;

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

      // TODO: equip_target
      if (f & QUERY_EQUIP_CARD) {
        q_read_u32();
      }

      // TODO: xyz
      uint32_t n_xyz = q_read_u32();
      for (int i = 0; i < n_xyz; ++i) {
        auto code = q_read_u32();
        Card c_ = c_get_card(code);
        c_.controler_ = controller;
        c_.location_ = location | LOCATION_OVERLAY;
        c_.sequence_ = sequence;
        c_.position_ = i;
        cards.push_back(c_);
      }

      // TODO: counters
      uint32_t n_counters = q_read_u32();
      for (int i = 0; i < n_counters; ++i) {
        q_read_u32();
      }

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
      cards.push_back(c);
    }
    return cards;
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
      card_specs.push_back({code, ls_to_spec(loc, seq, 0), data});
    }
    return card_specs;
  }

  void handle_message() {
    msg_ = int(data_[dp_++]);
    options_ = {};

    if (verbose_) {
      printf("Message %s, length %d, dp %d\n",
              msg_to_string(msg_).c_str(), dl_, dp_);
    }

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
        const auto &c = c_get_card(codes[i]);
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
      Card card = c_get_card(code);
      card.set_location(location);
      Card cnew = c_get_card(code);
      cnew.set_location(newloc);
      auto pl = players_[card.controler_];
      auto op = players_[1 - card.controler_];

      auto plspec = card.get_spec(false);
      auto opspec = card.get_spec(true);
      auto plnewspec = cnew.get_spec(false);
      auto opnewspec = cnew.get_spec(true);

      auto getspec = [&](Player *p) { return p == pl ? plspec : opspec; };
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
      } else if ((card.location_ == cnew.location_) &&
                 (card.location_ & LOCATION_ONFIELD)) {
        if (card.controler_ != cnew.controler_) {
          pl->notify("Your card " + plspec + " (" + card.name_ +
                     ") changed controller to " + op->nickname() +
                     " and is now located at " + plnewspec + ".");
          op->notify("You now control " + pl->nickname() + "'s card " + opspec +
                     " (" + card.name_ + ") and its located at " + opnewspec +
                     ".");
        } else {
          pl->notify("Your card " + plspec + " (" + card.name_ +
                     ") switched its zone to " + plnewspec + ".");
          op->notify(pl->nickname() + "'s card " + opspec + " (" + card.name_ +
                     ") changed its zone to " + opnewspec + ".");
        }
      } else if ((reason & REASON_DISCARD) &&
                 (card.location_ != cnew.location_)) {
        pl->notify("You discarded " + plspec + " (" + card.name_ + ").");
        op->notify(pl->nickname() + " discarded " + opspec + " (" + card.name_ +
                   ").");
      } else if ((card.location_ == LOCATION_REMOVED) &&
                 (cnew.location_ & LOCATION_ONFIELD)) {
        pl->notify("Your banished card " + plspec + " (" + card.name_ +
                   ") returns to the field at " + plnewspec + ".");
        op->notify(pl->nickname() + "'s banished card " + opspec + " (" +
                   card.name_ + ") returned to their field at " + opnewspec +
                   ".");
      } else if ((card.location_ == LOCATION_GRAVE) &&
                 (cnew.location_ & LOCATION_ONFIELD)) {
        pl->notify("Your card " + plspec + " (" + card.name_ +
                   ") returns from the graveyard to the field at " + plnewspec +
                   ".");
        op->notify(pl->nickname() + "'s card " + opspec + " (" + card.name_ +
                   ") returns from the graveyard to the field at " + opnewspec +
                   ".");
      } else if ((cnew.location_ == LOCATION_HAND) &&
                 (card.location_ != cnew.location_)) {
        pl->notify("Card " + plspec + " (" + card.name_ +
                   ") returned to hand.");
      } else if ((reason & (REASON_RELEASE | REASON_SUMMON)) &&
                 (card.location_ != cnew.location_)) {
        pl->notify("You tribute " + plspec + " (" + card.name_ + ").");
        op->notify(pl->nickname() + " tributes " + opspec + " (" +
                   getvisiblename(op) + ").");
      } else if ((card.location_ == (LOCATION_OVERLAY | LOCATION_MZONE)) &&
                 (cnew.location_ & LOCATION_GRAVE)) {
        pl->notify("You detached " + card.name_ + ".");
        op->notify(pl->nickname() + " detached " + card.name_ + ".");
      } else if ((card.location_ != cnew.location_) &&
                 (cnew.location_ == LOCATION_GRAVE)) {
        pl->notify("Your card " + plspec + " (" + card.name_ +
                   ") was sent to the graveyard.");
        op->notify(pl->nickname() + "'s card " + opspec + " (" + card.name_ +
                   ") was sent to the graveyard.");
      } else if ((card.location_ != cnew.location_) &&
                 (cnew.location_ == LOCATION_REMOVED)) {
        pl->notify("Your card " + plspec + " (" + card.name_ +
                   ") was banished.");
        op->notify(pl->nickname() + "'s card " + opspec + " (" +
                   getvisiblename(op) + ") was banished.");
      } else if ((card.location_ != cnew.location_) &&
                 (cnew.location_ == LOCATION_DECK)) {
        pl->notify("Your card " + plspec + " (" + card.name_ +
                   ") returned to your deck.");
        op->notify(pl->nickname() + "'s card " + opspec + " (" +
                   getvisiblename(op) + ") returned to their deck.");
      } else if ((card.location_ != cnew.location_) &&
                 (cnew.location_ == LOCATION_EXTRA)) {
        pl->notify("Your card " + plspec + " (" + card.name_ +
                   ") returned to your extra deck.");
        op->notify(pl->nickname() + "'s card " + opspec + " (" + card.name_ +
                   ") returned to their extra deck.");
      } else if ((card.location_ == LOCATION_DECK) &&
                 (cnew.location_ == LOCATION_SZONE) &&
                 (cnew.position_ != POS_FACEDOWN)) {
        pl->notify("Activating " + plnewspec + " (" + cnew.name_ + ")");
        op->notify(pl->nickname() + " activating " + opnewspec + " (" +
                   cnew.name_ + ")");
      }
    } else if (msg_ == MSG_SET) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      uint32_t code = read_u32();
      uint32_t location = read_u32();
      Card card = c_get_card(code);
      card.set_location(location);
      auto c = card.controler_;
      auto cpl = players_[c];
      auto opl = players_[1 - c];
      auto x = 1u - c;
      cpl->notify("You set " + card.get_spec(c) + " (" + card.name_ + ") in " +
                  card.get_position() + " position.");
      opl->notify(cpl->nickname() + " sets " + card.get_spec(PlayerId(1 - c)) +
                  " in " + card.get_position() + " position.");
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
                                   c_get_card(code).name_);
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
      uint8_t type = read_u8();
      uint32_t value = read_u32();
      Card card = get_card(player, loc, seq);
      if (type == CHINT_RACE) {
        std::string races_str = "TODO";
        for (PlayerId pl = 0; pl < 2; pl++) {
          players_[pl]->notify(card.get_spec(pl) + " (" + card.name_ +
                               ") selected " + races_str + ".");
        }
      } else if (type == CHINT_ATTRIBUTE) {
        std::string attributes_str = "TODO";
        for (PlayerId pl = 0; pl < 2; pl++) {
          players_[pl]->notify(card.get_spec(pl) + " (" + card.name_ +
                               ") selected " + attributes_str + ".");
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
      Card card = c_get_card(code);
      card.set_location(read_u32());
      uint8_t prevpos = card.position_;
      card.position_ = read_u8();

      auto pl = players_[card.controler_];
      auto op = players_[1 - card.controler_];
      auto plspec = card.get_spec(false);
      auto opspec = card.get_spec(true);
      auto prevpos_str = position_to_string(prevpos);
      auto pos_str = position_to_string(card.position_);
      pl->notify("The position of card " + plspec + " (" + card.name_ +
                 ") changed from " + prevpos_str + " to " + pos_str + ".");
      op->notify("The position of card " + opspec + " (" + card.name_ +
                 ") changed from " + prevpos_str + " to " + pos_str + ".");
    } else if (msg_ == MSG_BECOME_TARGET) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto u = read_u8();
      uint32_t target = read_u32();
      uint8_t tc = target & 0xff;
      uint8_t tl = (target >> 8) & 0xff;
      uint8_t tseq = (target >> 16) & 0xff;
      Card card = get_card(tc, tl, tseq);
      auto name = players_[chaining_player_]->nickname_;
      for (PlayerId pl = 0; pl < 2; pl++) {
        auto spec = card.get_spec(pl);
        auto tcname = card.name_;
        if ((card.controler_ != pl) && (card.position_ & POS_FACEDOWN)) {
          tcname = position_to_string(card.position_) + " card";
        }
        players_[pl]->notify(name + " targets " + spec + " (" + tcname + ")");
      }
    } else if (msg_ == MSG_CONFIRM_CARDS) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto player = read_u8();
      auto size = read_u8();
      std::vector<Card> cards;
      for (int i = 0; i < size; ++i) {
        read_u32();
        auto c = read_u8();
        auto loc = read_u8();
        auto seq = read_u8();
        cards.push_back(get_card(c, loc, seq));
      }

      auto pl = players_[player];
      auto op = players_[1 - player];

      op->notify(pl->nickname() + " shows you " + std::to_string(size) +
                 " cards.");
      for (int i = 0; i < size; ++i) {
        pl->notify(std::to_string(i + 1) + ": " + cards[i].name_);
      }
    } else if (msg_ == MSG_SHUFFLE_DECK) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      auto player = read_u8();
      auto pl = players_[player];
      auto op = players_[1 - player];
      pl->notify("You shuffled your deck.");
      op->notify(pl->nickname() + " shuffled their deck.");
    } else if (msg_ == MSG_SHUFFLE_HAND) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }

      auto player = read_u8();
      dp_ = dl_;

      auto pl = players_[player];
      auto op = players_[1 - player];
      pl->notify("You shuffled your hand.");
      op->notify(pl->nickname() + " shuffled their hand.");
    } else if (msg_ == MSG_SUMMONED) {
      dp_ = dl_;
    } else if (msg_ == MSG_SUMMONING) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      uint32_t code = read_u32();
      Card card = c_get_card(code);
      card.set_location(read_u32());
      const auto &nickname = players_[card.controler_]->nickname();
      for (auto pl : players_) {
        pl->notify(nickname + " summoning " + card.name_ + " (" +
                   std::to_string(card.attack_) + "/" +
                   std::to_string(card.defense_) + ") in " +
                   card.get_position() + " position.");
      }
    } else if (msg_ == MSG_SPSUMMONED) {
      dp_ = dl_;
    } else if (msg_ == MSG_FLIPSUMMONED) {
      dp_ = dl_;
    } else if (msg_ == MSG_FLIPSUMMONING) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }

      auto code = read_u32();
      auto location = read_u32();
      Card card = c_get_card(code);
      card.set_location(location);

      auto cpl = players_[card.controler_];
      for (PlayerId pl = 0; pl < 2; pl++) {
        auto spec = card.get_spec(pl);
        players_[1 - pl]->notify(cpl->nickname() + " flip summons " + spec +
                                " (" + card.name_ + ")");
      }
    } else if (msg_ == MSG_SPSUMMONING) {
      if (!verbose_) {
        dp_ = dl_;
        return;
      }
      uint32_t code = read_u32();
      Card card = c_get_card(code);
      card.set_location(read_u32());
      const auto &nickname = players_[card.controler_]->nickname();
      for (auto pl : players_) {
        auto pos = card.get_position();
        auto atk = std::to_string(card.attack_);
        auto def = std::to_string(card.defense_);
        if (card.type_ & TYPE_LINK) {
          pl->notify(nickname + " special summoning " + card.name_ + " (" +
                     atk + ") in " + pos + " position.");
        } else {
          pl->notify(nickname + " special summoning " + card.name_ + " (" +
                     atk + "/" + def + ") in " + pos + " position.");
        }
      }
    } else if (msg_ == MSG_CHAIN_NEGATED) {
      dp_ = dl_;
    } else if (msg_ == MSG_CHAIN_DISABLED) {
      dp_ = dl_;
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
      Card card = c_get_card(code);
      card.set_location(read_u32());
      auto tc = read_u8();
      auto tl = read_u8();
      auto ts = read_u8();
      uint32_t desc = read_u32();
      auto cs = read_u8();
      auto c = card.controler_;
      PlayerId o = 1 - c;
      chaining_player_ = c;
      players_[c]->notify("Activating " + card.get_spec(c) + " (" + card.name_ +
                          ")");
      players_[o]->notify(players_[c]->nickname_ + " activating " +
                          card.get_spec(o) + " (" + card.name_ + ")");
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
      pl->notify("You pay " + std::to_string(cost) + " LP. Your LP is now " +
                 std::to_string(lp_[player]) + ".");
      players_[1 - player]->notify(
          pl->nickname() + " pays " + std::to_string(cost) + " LP. " +
          pl->nickname() + "'s LP is now " + std::to_string(lp_[player]) + ".");
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
          players_[i]->notify(name + " prepares to attack with " +
                              acard.get_spec(i) + " (" + acard.name_ + ")");
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
        players_[i]->notify(name + " prepares to attack " + tspec + " (" +
                            tcname + ") with " + aspec + " (" + acard.name_ +
                            ")");
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
          pl->notify(acard.name_ + "(" + attacker_points + ")" + " attacks " +
                     tcard->name_ + " (" + defender_points + ")");
        } else {
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
          const auto &c = c_get_card(code);
          pl->notify("v " + spec + ": activate " + c.name_ + " (" +
                     std::to_string(c.attack_) + "/" +
                     std::to_string(c.defense_) + ")");
        }
      }
      for (const auto [code, spec, data] : attackable) {
        options_.push_back("a " + spec);
        if (verbose_) {
          const auto &c = c_get_card(code);
          if (c.type_ & TYPE_LINK) {
            pl->notify("a " + spec + ": " + c.name_ + " (" +
                       std::to_string(c.attack_) + ") attack");
          } else {
            pl->notify("a " + spec + ": " + c.name_ + " (" +
                       std::to_string(c.attack_) + "/" +
                       std::to_string(c.defense_) + ") attack");
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
    } else if (msg_ == MSG_SELECT_UNSELECT_CARD) {
      // TODO: multi select in one action
      auto player = read_u8();
      to_decide_ = player;
      bool finishable = read_u8();
      bool cancelable = read_u8();
      auto min = read_u8();
      auto max = read_u8();
      auto select_size = read_u8();

      std::vector<std::string> select_specs;
      select_specs.reserve(select_size);
      if (verbose_) {
        std::vector<Card> cards;
        for (int i = 0; i < select_size; ++i) {
          auto code = read_u32();
          auto loc = read_u32();
          Card card = c_get_card(code);
          card.set_location(loc);
          cards.push_back(card);
        }
        auto pl = players_[player];
        pl->notify("Select " + std::to_string(min) + " to " + std::to_string(max) + " cards:");
        for (const auto &card : cards) {
          auto spec = card.get_spec(player);
          select_specs.push_back(spec);
          pl->notify(spec + ": " + card.name_);
        }
      } else {
        for (int i = 0; i < select_size; ++i) {
          dp_ += 4;
          auto controller = read_u8();
          auto loc = read_u8();
          auto seq = read_u8();
          auto pos = read_u8();
          auto spec = ls_to_spec(loc, seq, pos, controller != player);
          select_specs.push_back(spec);
        }
      }

      auto unselect_size = read_u8();

      // unselect not allowed (no regrets!)
      dp_ += 8 * unselect_size;

      if (min != max) {
        printf("Min(%d) != Max(%d) not implemented, select_size: %d, unselect_size: %d\n",
               min, max, select_size, unselect_size);

        // throw std::runtime_error(
        //     "Min(" + std::to_string(min) + ") != Max(" + std::to_string(max) +
        //     ") not implemented, select_size: " + std::to_string(select_size) +
        //     ", unselect_size: " + std::to_string(unselect_size));
      }

      for (int j = 0; j < select_specs.size(); ++j) {
        options_.push_back(select_specs[j]);
      }

      // cancelable and finishable not needed

      callback_ = [this](int idx) {
        resp_buf_[0] = 1;
        resp_buf_[1] = idx;
        set_responseb(pduel_, resp_buf_);
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
          Card card = c_get_card(code);
          card.set_location(loc);
          cards.push_back(card);
        }
        auto pl = players_[player];
        pl->notify("Select " + std::to_string(min) + " to " +
                   std::to_string(max) + " cards separated by spaces:");
        for (const auto &card : cards) {
          auto spec = card.get_spec(player);
          specs.push_back(spec);
          if (card.controler_ != player && card.position_ & POS_FACEDOWN) {
            pl->notify(spec + ": " + card.get_position() + " card");
          } else {
            pl->notify(spec + ": " + card.name_);
          }
        }
      } else {
        for (int i = 0; i < size; ++i) {
          dp_ += 4;
          auto controller = read_u8();
          auto loc = read_u8();
          auto seq = read_u8();
          auto pos = read_u8();
          auto spec = ls_to_spec(loc, seq, pos, controller != player);
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
        pl->notify("Select " + std::to_string(min) + " to " +
                   std::to_string(max) +
                   " cards to tribute separated by spaces:");
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

          auto spec = ls_to_spec(loc, seq, 0, controller != player);
          specs.push_back(spec);

          release_params.push_back(release_param);
        }
      }

      bool has_weight =
          std::any_of(release_params.begin(), release_params.end(),
                      [](int i) { return i != 1; });

      if (min != max) {
        auto err_str =
            "min: " + std::to_string(min) + ", max: " + std::to_string(max);
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
          Card card = c_get_card(code);
          card.set_location(loc);
          cards.push_back(card);
          spec_codes.push_back(card.get_spec_code(player));
        } else {
          PlayerId c = read_u8();
          uint8_t loc = read_u8();
          uint8_t seq = read_u8();
          uint8_t pos = read_u8();
          spec_codes.push_back(ls_to_spec_code(loc, seq, pos, c != player));
        }
        uint32_t desc = read_u32();
        descs.push_back(desc);
      }

      if ((size == 0) && (spe_count == 0)) {
        keep_processing_ = true;

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
        } else {
          pl->notify("Select chain (c to cancel):");
        }
        for (int i = 0; i < size; i++) {
          const auto &effect_desc = effect_descs[i];
          if (effect_desc.empty()) {
            pl->notify(chain_specs[i] + ": " + cards[i].name_);
          } else {
            pl->notify(chain_specs[i] + " (" + cards[i].name_ +
                       "): " + effect_desc);
          }
        }
      }

      for (const auto &spec : chain_specs) {
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
          auto card = c_get_card(code);
          auto opt_idx = desc & 0xf;
          if (opt_idx < card.strings_.size()) {
            opt = card.strings_[opt_idx];
          }
          if (opt.empty()) {
            opt = "Unknown question from " + card.name_ + ". Yes or no?";
          }
        } else {
          opt = "TODO: system string " + std::to_string(desc) + ". Yes or no?";
        }
        pl->notify(opt);
        pl->notify("Please enter y or n.");
      } else {
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
        Card card = c_get_card(code);
        card.set_location(loc);
        auto desc = read_u32();
        auto pl = players_[player];
        auto spec = card.get_spec(player);
        auto name = card.name_;
        std::string s;
        if (desc == 221) {
          s = "On " + card.get_spec(player) + ", Activate Trigger Effect of " +
              name + "?";
        } else if (desc == 0) {
          // From [%ls], activate [%ls]?
          s = "From " + card.get_spec(player) + ", activate " + name + "?";
        } else if (desc < 2048) {
          s = "TODO: system string " + std::to_string(desc) + "";
        } else {
          throw std::runtime_error("Unknown effectyn desc " +
                                   std::to_string(desc) + " of " + name);
        }
        pl->notify("Please enter y or n.");
      } else {
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
    } else if (msg_ == MSG_SELECT_OPTION) {
      auto player = read_u8();
      to_decide_ = player;
      auto size = read_u8();
      if (verbose_) {
        auto pl = players_[player];
        pl->notify("Select an option:");
        for (int i = 0; i < size; ++i) {
          auto opt = read_u32();
          std::string s;
          if (opt > 10000) {
            uint32_t code = opt >> 4;
            s = c_get_card(code).strings_[opt & 0xf];
          } else {
            s = "TODO: system string " + std::to_string(opt);
          }
          std::string option = std::to_string(i + 1);
          options_.push_back(option);
          pl->notify(option + ": " + s);
        }
      } else {
        dp_ += 4 * size;
      }
      callback_ = [this](int idx) {
        if (verbose_) {
          players_[to_decide_]->notify("You selected option " + options_[idx] + ".");
          players_[1 - to_decide_]->notify(players_[to_decide_]->nickname_ +
                                           " selected option " + options_[idx] +
                                           ".");
        }

        set_responsei(pduel_, idx);
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
      for (const auto &[code, spec, data] : summonable_) {
        std::string option = "s " + spec;
        options_.push_back(option);
        if (verbose_) {
          const auto &name = c_get_card(code).name_;
          pl->notify(option + ": Summon " + name +
                     " in face-up attack position.");
        }
      }
      for (const auto &[code, spec, data] : idle_set_) {
        std::string option = "t " + spec;
        options_.push_back(option);
        if (verbose_) {
          const auto &name = c_get_card(code).name_;
          pl->notify(option + ": Set " + name + ".");
        }
      }
      for (const auto &[code, spec, data] : idle_mset_) {
        std::string option = "m " + spec;
        options_.push_back(option);
        if (verbose_) {
          const auto &name = c_get_card(code).name_;
          pl->notify(option + ": Summon " + name +
                     " in face-down defense position.");
        }
      }
      for (const auto &[code, spec, data] : repos_) {
        std::string option = "r " + spec;
        options_.push_back(option);
        if (verbose_) {
          const auto &name = c_get_card(code).name_;
          pl->notify(option + ": Reposition " + name + ".");
        }
      }
      for (const auto &[code, spec, data] : spsummon_) {
        std::string option = "c " + spec;
        options_.push_back(option);
        if (verbose_) {
          const auto &name = c_get_card(code).name_;
          pl->notify(option + ": Special summon " + name + ".");
        }
      }
      std::unordered_map<std::string, int> idle_activate_count;
      for (const auto &[code, spec, data] : idle_activate_) {
        idle_activate_count[spec] += 1;
      }
      for (const auto &[code, spec, data] : idle_activate_) {
        int count = idle_activate_count[spec];
        if (count > 1) {
          Card card = c_get_card(code);
          printf("%s has %d effects\n", card.name_.c_str(), count);
          throw std::runtime_error("Activate more than one effect.");
        }
        std::string option = "v " + spec;
        options_.push_back(option);
        if (verbose_) {
          pl->notify(option + ": " +
                     c_get_card(code).get_effect_description(data));
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
      callback_ = [this, summonable_, spsummon_, repos_, idle_mset_, idle_set_,
                   idle_activate_](int idx) {
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
        auto [loc, seq, pos] = spec_to_ls(spec);
        resp_buf_[0] = plr;
        resp_buf_[1] = loc;
        resp_buf_[2] = seq;
        set_responseb(pduel_, resp_buf_);
      };
    } else if (msg_ == MSG_SELECT_POSITION) {
      auto player = read_u8();
      to_decide_ = player;
      auto code = read_u32();
      auto valid_pos = read_u8();

      if (verbose_) {
        auto pl = players_[player];
        auto card = c_get_card(code);
        pl->notify("Select position for " + card.name_ + ":");
      }

      std::vector<uint8_t> positions;
      int i = 1;
      for (auto pos : {POS_FACEUP_ATTACK, POS_FACEDOWN_ATTACK,
                       POS_FACEUP_DEFENSE, POS_FACEDOWN_DEFENSE}) {
        if (valid_pos & pos) {
          positions.push_back(pos);
          options_.push_back(std::to_string(i));
          if (verbose_) {
            auto pl = players_[player];
            pl->notify(std::to_string(i) + ": " + position_to_string(pos));
          }
        }
        i++;
      }

      callback_ = [this](int idx) {
        uint8_t pos = options_[idx][0] - '0' - 1;
        set_responsei(pduel_, 1 << pos);
      };
    } else {
      if (verbose_) {
        printf("Unknown message %s, length %d, dp %d\n",
               msg_to_string(msg_).c_str(), dl_, dp_);
      }
      dp_ = dl_;
    }
  }

  void _damage(uint8_t player, uint32_t amount) {
    lp_[player] -= amount;
    if (verbose_) {
      auto lp = players_[player];
      lp->notify("Your lp decreased by " + std::to_string(amount) + ", now " +
                 std::to_string(lp_[player]));
      players_[1 - player]->notify(lp->nickname_ + "'s lp decreased by " +
                                   std::to_string(amount) + ", now " +
                                   std::to_string(lp_[player]));
    }
  }

  void _recover(uint8_t player, uint32_t amount) {
    lp_[player] += amount;
    if (verbose_) {
      auto lp = players_[player];
      lp->notify("Your lp increased by " + std::to_string(amount) + ", now " +
                 std::to_string(lp_[player]));
      players_[1 - player]->notify(lp->nickname_ + "'s lp increased by " +
                                   std::to_string(amount) + ", now " +
                                   std::to_string(lp_[player]));
    }
  }

  void _duel_end(uint8_t player, uint8_t reason) {
    winner_ = player;
    win_reason_ = reason;

    std::unique_lock<std::shared_timed_mutex> ulock(duel_mtx);
    end_duel(pduel_);
    ulock.unlock();

    duel_started_ = false;
  }
};

using YGOProEnvPool = AsyncEnvPool<YGOProEnv>;

} // namespace ygopro

#endif // ENVPOOL_YGOPRO_YGOPRO_H_
