#include "envpool2/ygopro/card.h"
#include "envpool2/ygopro/common.h"

namespace ygopro {

Card::Card(uint32_t code, uint32_t alias, uint64_t setcode, uint32_t type,
           uint32_t level, uint32_t lscale, uint32_t rscale, int32_t attack,
           int32_t defense, uint32_t race, uint32_t attribute,
           uint32_t link_marker, const std::string &name,
           const std::string &desc, const std::vector<std::string> &strings)
    : code_(code), alias_(alias), setcode_(setcode), type_(type), level_(level),
      lscale_(lscale), rscale_(rscale), attack_(attack), defense_(defense),
      race_(race), attribute_(attribute), link_marker_(link_marker),
      name_(name), desc_(desc), strings_(strings) {}

void Card::set_location(uint32_t location) {
    controler_ = location & 0xff;
    location_ = (location >> 8) & 0xff;
    sequence_ = (location >> 16) & 0xff;
    position_ = (location >> 24) & 0xff;
  }

  const std::string &Card::name() const { return name_; }
  const std::string &Card::desc() const { return desc_; }
  const uint32_t &Card::type() const { return type_; }
  const uint32_t &Card::level() const { return level_; }
  const std::vector<std::string> &Card::strings() const { return strings_; }

  std::string Card::get_spec(bool opponent) const {
    return ls_to_spec(location_, sequence_, position_, opponent);
  }

  std::string Card::get_spec(PlayerId player) const {
    return get_spec(player != controler_);
  }

  uint32_t Card::get_spec_code(PlayerId player) const {
    return ls_to_spec_code(location_, sequence_, position_, player != controler_);
  }

  std::string Card::get_position() const { return position_to_string(position_); }

  std::string Card::get_effect_description(uint32_t desc, bool existing) const {
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
        s = name_ + " (" + str + ")";
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

  
} // namespace ygopro