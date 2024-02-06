#include "envpool2/ygopro/common.h"

namespace ygopro {

std::vector<std::vector<int>> combinations(int n, int r) {
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

bool sum_to(const std::vector<int> &w, const std::vector<int> ind, int i, int r) {
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

bool sum_to(const std::vector<int> &w, const std::vector<int> ind, int r) {
  return sum_to(w, ind, 0, r);
}

std::vector<std::vector<int>> combinations_with_weight(const std::vector<int> &weights, int r) {
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


std::string reason_to_string(uint8_t reason) {
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

std::string msg_to_string(int msg) {
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

std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}


std::vector<uint32> read_main_deck(const std::string &fp) {
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


std::vector<uint32> read_extra_deck(const std::string &fp) {
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


template<class K>
ankerl::unordered_dense::map<K, uint8_t> make_ids(const std::map<K, std::string> &m, int id_offset, int m_offset) {
  ankerl::unordered_dense::map<K, uint8_t> m2;
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


template<class K>
ankerl::unordered_dense::map<K, uint8_t> make_ids(const std::vector<K> &cmds, int id_offset, int m_offset) {
  ankerl::unordered_dense::map<K, uint8_t> m2;
  for (int i = m_offset; i < cmds.size(); i++) {
    m2[cmds[i]] = i - m_offset + id_offset;
  }
  return m2;
}

} // namespace ygopro