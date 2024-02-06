#ifndef ENVPOOL_YGOPRO_COMMON_H_
#define ENVPOOL_YGOPRO_COMMON_H_

#include <algorithm>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <fstream>
#include <map>

#include <ankerl/unordered_dense.h>

#include "ygopro-core/common.h"

namespace ygopro {

std::vector<std::vector<int>> combinations(int n, int r);

bool sum_to(const std::vector<int> &w, const std::vector<int> ind, int i, int r);

bool sum_to(const std::vector<int> &w, const std::vector<int> ind, int r);

std::vector<std::vector<int>> combinations_with_weight(const std::vector<int> &weights, int r);

std::string reason_to_string(uint8_t reason);

std::string msg_to_string(int msg);

std::string ltrim(std::string s);


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


std::vector<uint32> read_main_deck(const std::string &fp);


std::vector<uint32> read_extra_deck(const std::string &fp);


template<class K = uint8_t>
ankerl::unordered_dense::map<K, uint8_t> make_ids(const std::map<K, std::string> &m, int id_offset = 0, int m_offset = 0);


template<class K = char>
ankerl::unordered_dense::map<K, uint8_t> make_ids(const std::vector<K> &cmds, int id_offset = 0, int m_offset = 0);

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

static const ankerl::unordered_dense::map<uint8_t, uint8_t> location2id = make_ids(location2str, 1);

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

static const ankerl::unordered_dense::map<uint8_t, uint8_t> position2id = make_ids(position2str);

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

static const ankerl::unordered_dense::map<uint8_t, uint8_t> attribute2id = make_ids(attribute2str);


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

static const ankerl::unordered_dense::map<uint32_t, uint8_t> race2id = make_ids(race2str);

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

static const ankerl::unordered_dense::map<int, uint8_t> phase2id = make_ids(phase2str);


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

static const ankerl::unordered_dense::map<int, uint8_t> msg2id = make_ids(_msgs, 1);


static const ankerl::unordered_dense::map<char, uint8_t> cmd_act2id =
  make_ids({'t', 'r', 'v', 'c', 's', 'm', 'a'}, 1);

static const ankerl::unordered_dense::map<char, uint8_t> cmd_phase2id =
  make_ids(std::vector<char>({'b', 'm', 'e'}), 1);

static const ankerl::unordered_dense::map<char, uint8_t> cmd_yesno2id =
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
