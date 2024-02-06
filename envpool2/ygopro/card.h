#ifndef ENVPOOL_YGOPRO_CARD_H_
#define ENVPOOL_YGOPRO_CARD_H_

#include <string>
#include <vector>
#include "envpool2/ygopro/player.h"

namespace ygopro {

class YGOProEnv;

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
       const std::vector<std::string> &strings);
  ~Card() = default;

  void set_location(uint32_t location);

  const std::string &name() const;
  const std::string &desc() const;
  const uint32_t &type() const;
  const uint32_t &level() const;
  const std::vector<std::string> &strings() const;

  std::string get_spec(bool opponent) const;

  std::string get_spec(PlayerId player) const;

  uint32_t get_spec_code(PlayerId player) const;

  std::string get_position() const;

  std::string get_effect_description(uint32_t desc, bool existing = false) const;
};

} // namespace ygopro

#endif // ENVPOOL_YGOPRO_CARD_H_