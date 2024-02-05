#ifndef ENVPOOL_YGOPRO_COMMON_H_
#define ENVPOOL_YGOPRO_COMMON_H_

#include <algorithm>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <fstream>
#include <map>
#include <unordered_map>

#include "ygopro-core/common.h"

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

inline std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
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

inline std::string ls_to_spec(uint8_t loc, uint8_t seq, uint8_t pos) {
  std::string spec;
  if (loc & LOCATION_HAND) {
    spec += "h";
  } else if (loc & LOCATION_MZONE) {
    spec += "m";
  } else if (loc & LOCATION_SZONE) {
    spec += "s";
  } else if (loc & LOCATION_GRAVE) {
    spec += "g";
  } else if (loc & LOCATION_REMOVED) {
    spec += "r";
  } else if (loc & LOCATION_EXTRA) {
    spec += "x";
  }
  spec += std::to_string(seq + 1);
  if (loc & LOCATION_OVERLAY) {
    spec += "#";
    spec += std::to_string(pos + 1);
  }
  return spec;
}

inline std::string ls_to_spec(uint8_t loc, uint8_t seq, uint8_t pos, bool opponent) {
  std::string spec = ls_to_spec(loc, seq, pos);
  if (opponent) {
    spec.insert(0, 1, 'o');
  }
  return spec;
}

inline std::tuple<uint8_t, uint8_t, uint8_t> spec_to_ls(const std::string spec) {
  uint8_t loc;
  uint8_t seq;
  uint8_t pos = 0;
  int offset = 1;
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
  } else if (std::isdigit(spec[0])) {
    loc = LOCATION_DECK;
    offset = 0;
  } else {
    throw std::runtime_error("Invalid location");
  }
  int end = offset;
  while (end < spec.size() && std::isdigit(spec[end])) {
    end++;
  }
  seq = std::stoi(spec.substr(offset, end - offset)) - 1;
  if (end < spec.size() && spec[end] == '#') {
    pos = std::stoi(spec.substr(end + 1)) - 1;
  }
  return {loc, seq, pos};
}

inline uint32_t ls_to_spec_code(uint8_t loc, uint8_t seq, uint8_t pos, bool opponent) {
  uint32_t c = opponent ? 1 : 0;
  c |= (loc << 8);
  c |= (seq << 16);
  c |= (pos << 24);
  return c;
}

inline uint32_t spec_to_code(const std::string &spec) {
  int offset = 0;
  bool opponent = false;
  if (spec[0] == 'o') {
    opponent = true;
    offset++;
  }
  auto [loc, seq, pos] = spec_to_ls(spec.substr(offset));
  return ls_to_spec_code(loc, seq, pos, opponent);
}


inline std::string code_to_spec(uint32_t spec_code) {
  uint8_t loc = (spec_code >> 8) & 0xff;
  uint8_t seq = (spec_code >> 16) & 0xff;
  uint8_t pos = (spec_code >> 24) & 0xff;
  bool opponent = (spec_code & 0xff) == 1;
  return ls_to_spec(loc, seq, pos, opponent);
}


inline std::vector<uint32> read_main_deck(const std::string &fp) {
  std::ifstream file(fp);
  std::string line;
  std::vector<uint32> deck;

  if (file.is_open()) {
    while (std::getline(file, line)) {
      if ((line.find("side") != std::string::npos) || line.find("extra") != std::string::npos) {
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


inline std::vector<uint32> read_extra_deck(const std::string &fp) {
  std::ifstream file(fp);
  std::string line;
  std::vector<uint32> deck;

  if (file.is_open()) {
    while (std::getline(file, line)) {
      if (line.find("extra") != std::string::npos) {
        break;
      }
    }

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


template<class K = uint8_t>
inline std::unordered_map<K, uint8_t> make_ids(const std::map<K, std::string> &m, int id_offset = 0, int m_offset = 0) {
  std::unordered_map<K, uint8_t> m2;
  auto i = 0;
  for (const auto &[k, v] : m) {
    if (i < m_offset) {
      i++;
      continue;
    }
    m2[k] = i - m_offset + id_offset;
    i++;
  }
  return m2;
}


template<class K = char>
inline std::unordered_map<K, uint8_t> make_ids(const std::vector<K> &cmds, int id_offset = 0, int m_offset = 0) {
  std::unordered_map<K, uint8_t> m2;
  for (int i = m_offset; i < cmds.size(); i++) {
    m2[cmds[i]] = i - m_offset + id_offset;
  }
  return m2;
}

static const std::map<uint8_t, std::string> location2str = {
  {LOCATION_DECK, "Deck"},
  {LOCATION_HAND, "Hand"},
  {LOCATION_MZONE, "Main Monster Zone"},
  {LOCATION_SZONE, "Spell & Trap Zone"},
  {LOCATION_GRAVE, "Graveyard"},
  {LOCATION_REMOVED, "Banished"},
  {LOCATION_EXTRA, "Extra Deck"},
  // {LOCATION_FZONE, "Field Zone"},
};

static const std::unordered_map<uint8_t, uint8_t> location2id = make_ids(location2str, 1);

#define POS_NONE 0x0  // xyz materials (overlay)

static const std::map<uint8_t, std::string> position2str = {
  {POS_NONE, "none"},
  {POS_FACEUP_ATTACK, "face-up attack"},
  {POS_FACEDOWN_ATTACK, "face-down attack"},
  {POS_ATTACK, "attack"},
  {POS_FACEUP_DEFENSE, "face-up defense"},
  {POS_FACEUP, "face-up"},
  {POS_FACEDOWN_DEFENSE, "face-down defense"},
  {POS_FACEDOWN, "face-down"},
  {POS_DEFENSE, "defense"},
};

static const std::unordered_map<uint8_t, uint8_t> position2id = make_ids(position2str);

#define ATTRIBUTE_NONE 0x0  // token

static const std::map<uint8_t, std::string> attribute2str = {
  {ATTRIBUTE_NONE, "None"},
  {ATTRIBUTE_EARTH, "Earth"},
  {ATTRIBUTE_WATER, "Water"},
  {ATTRIBUTE_FIRE, "Fire"},
  {ATTRIBUTE_WIND, "Wind"},
  {ATTRIBUTE_LIGHT, "Light"},
  {ATTRIBUTE_DARK, "Dark"},
  {ATTRIBUTE_DEVINE, "Divine"},
};

static const std::unordered_map<uint8_t, uint8_t> attribute2id = make_ids(attribute2str);


#define RACE_NONE 0x0  // token

static const std::map<uint32_t, std::string> race2str = {
  {RACE_NONE, "None"},
  {RACE_WARRIOR, "Warrior"},
  {RACE_SPELLCASTER, "Spellcaster"},
  {RACE_FAIRY, "Fairy"},
  {RACE_FIEND, "Fiend"},
  {RACE_ZOMBIE, "Zombie"},
  {RACE_MACHINE, "Machine"},
  {RACE_AQUA, "Aqua"},
  {RACE_PYRO, "Pyro"},
  {RACE_ROCK, "Rock"},
  {RACE_WINDBEAST, "Windbeast"},
  {RACE_PLANT, "Plant"},
  {RACE_INSECT, "Insect"},
  {RACE_THUNDER, "Thunder"},
  {RACE_DRAGON, "Dragon"},
  {RACE_BEAST, "Beast"},
  {RACE_BEASTWARRIOR, "Beast Warrior"},
  {RACE_DINOSAUR, "Dinosaur"},
  {RACE_FISH, "Fish"},
  {RACE_SEASERPENT, "Sea Serpent"},
  {RACE_REPTILE, "Reptile"},
  {RACE_PSYCHO, "Psycho"},
  {RACE_DEVINE, "Divine"},
  {RACE_CREATORGOD, "Creator God"},
  {RACE_WYRM, "Wyrm"},
  {RACE_CYBERSE, "Cyberse"},
  {RACE_ILLUSION, "Illusion'"} 
};

static const std::unordered_map<uint32_t, uint8_t> race2id = make_ids(race2str);

static const std::map<uint32_t, std::string> type2str = {
    {TYPE_MONSTER, "Monster"},
    {TYPE_SPELL, "Spell"},
    {TYPE_TRAP, "Trap"},
    {TYPE_NORMAL, "Normal"},
    {TYPE_EFFECT, "Effect"},
    {TYPE_FUSION, "Fusion"},
    {TYPE_RITUAL, "Ritual"},
    {TYPE_TRAPMONSTER, "Trap Monster"},
    {TYPE_SPIRIT, "Spirit"},
    {TYPE_UNION, "Union"},
    {TYPE_DUAL, "Dual"},
    {TYPE_TUNER, "Tuner"},
    {TYPE_SYNCHRO, "Synchro"},
    {TYPE_TOKEN, "Token"},
    {TYPE_QUICKPLAY, "Quick-play"},
    {TYPE_CONTINUOUS, "Continuous"},
    {TYPE_EQUIP, "Equip"},
    {TYPE_FIELD, "Field"},
    {TYPE_COUNTER, "Counter"},
    {TYPE_FLIP, "Flip"},
    {TYPE_TOON, "Toon"},
    {TYPE_XYZ, "XYZ"},
    {TYPE_PENDULUM, "Pendulum"},
    {TYPE_SPSUMMON, "Special"},
    {TYPE_LINK, "Link"},
};

inline std::vector<uint8_t> type_to_ids(uint32_t type) {
  std::vector<uint8_t> ids;
  ids.reserve(type2str.size());
  for (const auto &[k, v] : type2str) {
    ids.push_back(std::min(1u, type & k));
  }
  return ids;
}

static const std::map<int, std::string> phase2str = {
    {PHASE_DRAW, "draw phase"},
    {PHASE_STANDBY, "standby phase"},
    {PHASE_MAIN1, "main1 phase"},
    {PHASE_BATTLE_START, "battle start phase"},
    {PHASE_BATTLE_STEP, "battle step phase"},
    {PHASE_DAMAGE, "damage phase"},
    {PHASE_DAMAGE_CAL, "damage calculation phase"},
    {PHASE_BATTLE, "battle phase"},
    {PHASE_MAIN2, "main2 phase"},
    {PHASE_END, "end phase"},
};

static const std::unordered_map<int, uint8_t> phase2id = make_ids(phase2str);


static const std::vector<int> _msgs = {
  MSG_SELECT_IDLECMD,
  MSG_SELECT_CHAIN,
  MSG_SELECT_CARD,
  MSG_SELECT_TRIBUTE,
  MSG_SELECT_POSITION,
  MSG_SELECT_EFFECTYN,
  MSG_SELECT_YESNO,
  MSG_SELECT_BATTLECMD,
  MSG_SELECT_UNSELECT_CARD,
  MSG_SELECT_OPTION,
  MSG_SELECT_PLACE,
};

static const std::unordered_map<int, uint8_t> msg2id = make_ids(_msgs, 1);


static const std::unordered_map<char, uint8_t> cmd_act2id =
  make_ids({'t', 'r', 'v', 'c', 's', 'm', 'a'}, 1);

static const std::unordered_map<char, uint8_t> cmd_phase2id =
  make_ids(std::vector<char>({'b', 'm', 'e'}), 1);

static const std::unordered_map<char, uint8_t> cmd_yesno2id =
  make_ids(std::vector<char>({'y', 'n'}), 1);

inline std::string phase_to_string(int phase) {
  auto it = phase2str.find(phase);
  if (it != phase2str.end()) {
    return it->second;
  }
  return "unknown";
}

inline std::string position_to_string(int position) {
  auto it = position2str.find(position);
  if (it != position2str.end()) {
    return it->second;
  }
  return "unknown";
}

inline std::pair<uint8_t, uint8_t> float_transform(int x) {
  x = x % 65536;
  return {x / 256, x % 256};
}

}

#endif // ENVPOOL_YGOPRO_COMMON_H_
