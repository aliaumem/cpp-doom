//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  MapObj data. Map Objects or mobjs are actors, entities,
//  thinker, take-your-pick... anything that moves, acts, or
//  suffers state changes of more or less violent nature.
//

#pragma once

#include <cstdint>
#include <list>
#include <variant>

//
// Experimental stuff.
// To compile this as "ANSI C with classes"
//  we will need to handle the various
//  action functions cleanly.
//

struct mobj_t;
struct player_t;
struct pspdef_t;

typedef void (*actionf_p1)(mobj_t *mo);
typedef void (*actionf_p3)(mobj_t *mo, player_t *player,
                           pspdef_t *psp); // [crispy] let pspr action pointers
                                           // get called from mobj states

struct actionf_t {
  std::variant<std::monostate, actionf_p3, actionf_p1> value;

public:
  constexpr actionf_t() = default;
  constexpr actionf_t(actionf_p1 a) : value(a) {}
  constexpr actionf_t(actionf_p3 f) : value{f} {}

  void *as_ptr() const {
    return reinterpret_cast<void *>(std::get<actionf_p1>(value));
  }

  constexpr actionf_t &operator=(actionf_p1 f) {
    value = f;
    return *this;
  }

  constexpr bool operator==(actionf_p1 f) const {
    return std::get<actionf_p1>(value) == f;
  }

  constexpr bool operator==(actionf_p3 f) const {
    return static_cast<bool>(*this) && std::get<actionf_p3>(value) == f;
  }

  constexpr void reset() { value = std::monostate{}; }

  constexpr operator bool() const {
    return !std::holds_alternative<std::monostate>(value);
  }

  constexpr bool call_if(mobj_t *mo) const {
    if (std::holds_alternative<actionf_p1>(value)) {
      std::get<actionf_p1>(value)(mo);
      return true;
    }
    return false;
  }

  constexpr bool call_if(mobj_t *mo, player_t *player, pspdef_t *psp) {
    if (std::holds_alternative<actionf_p3>(value)) {
      (*this)(mo, player, psp);
      return true;
    }
    return false;
  }

  constexpr void operator()(mobj_t *mo, player_t *player, pspdef_t *psp) const {
    std::get<actionf_p3>(value)(mo, player, psp);
  }
};

struct mobj_thinker {
  // List: thinker links.
  bool is_removed = false;

  virtual void perform() = 0;

  constexpr bool needs_removal() { return is_removed; }

  constexpr void mark_for_removal() { is_removed = true; }
};

template <typename T> T *thinker_cast(mobj_thinker *obj) {
  return dynamic_cast<T *>(obj);
}

template <typename T> T *is_a(T *obj) {
  return thinker_cast<T>(static_cast<mobj_thinker *>(obj));
}

class thinker_list {
  std::list<mobj_thinker *> thinkers;

public:
  static thinker_list instance;

  using iterator = decltype(thinkers)::iterator;

  [[nodiscard]] iterator begin() noexcept { return thinkers.begin(); }
  [[nodiscard]] iterator end() noexcept { return thinkers.end(); }

  constexpr void push_back(mobj_thinker *thinker) {
    thinkers.push_back(thinker);
  }

  void run();

  uint32_t index_of(mobj_t *mobj);
  mobj_t *mobj_at(uintptr_t index);

private:
  void clear_removed();
};

inline thinker_list thinker_list::instance{};
