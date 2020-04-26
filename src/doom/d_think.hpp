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
#include <variant>

//
// Experimental stuff.
// To compile this as "ANSI C with classes"
//  we will need to handle the various
//  action functions cleanly.
//

struct mobj_thinker;
struct mobj_t;
struct player_t;
struct pspdef_t;
struct vldoor_t;
struct ceiling_t;
struct fireflicker_t;
struct floormove_t;
struct glow_t;
struct plat_t;
struct strobe_t;
struct lightflash_t;
struct thinker_t;

struct action_removed {};

typedef void (*actionf_v)();
typedef void (*actionf_p1)(mobj_t *mo);
typedef void (*actionf_pthink1)(mobj_thinker *mo);
typedef void (*actionf_vldoor)(vldoor_t *mo);
typedef void (*actionf_ceiling)(ceiling_t *mo);
typedef void (*actionf_fireflicker)(fireflicker_t *mo);
typedef void (*actionf_floormove)(floormove_t *mo);
typedef void (*actionf_glow)(glow_t *mo);
typedef void (*actionf_plat)(plat_t *mo);
typedef void (*actionf_strobe)(strobe_t *mo);
typedef void (*actionf_lightflash)(lightflash_t *mo);
typedef void (*actionf_p3)(mobj_t *mo, player_t *player,
                           pspdef_t *psp); // [crispy] let pspr action pointers
                                           // get called from mobj states

struct actionf_t {
  struct action1 {
    std::variant<actionf_p1, actionf_pthink1, actionf_vldoor, actionf_ceiling,
                 actionf_fireflicker, actionf_floormove, actionf_strobe,
                 actionf_glow, actionf_plat, actionf_lightflash>
        action;

    template <typename Arg> constexpr bool operator==(void (*f)(Arg)) const {
      using fptr = void (*)(Arg);
      return std::holds_alternative<fptr>(action) &&
             std::get<fptr>(action) == f;
    }

    struct visitor {
      mobj_thinker *obj;

      template <typename T> void operator()(void (*f)(T)) const {
        f(reinterpret_cast<T>(obj));
      }
    };

    void invoke(mobj_thinker *t) {
      visitor v{t};
      std::visit(v, action);
    }

    void *as_ptr() const {
      return std::visit([](auto &&f) { return reinterpret_cast<void *>(f); },
                        action);
    }
  };
  std::variant<std::monostate, actionf_v, action_removed, actionf_p3, action1>
      value;

public:
  auto &acv() { return std::get<actionf_v>(value); }
  auto acv() const { return std::get<actionf_v>(value); }

  void *as_ptr() { return std::get<action1>(value).as_ptr(); }
  actionf_t &operator=(void *f) {
    value = action1{reinterpret_cast<actionf_p1>(f)};
    return *this;
  }

  constexpr actionf_t() = default;

  template <typename Arg>
  constexpr actionf_t(void (*f)(Arg)) : value{action1{f}} {}
  constexpr actionf_t(actionf_p3 f) : value{f} {}

  constexpr actionf_t &operator=(actionf_v const &f) {
    value = f;
    return *this;
  }

  template <typename Ret, typename Arg>
  constexpr actionf_t &operator=(Ret (*f)(Arg)) {
    value = action1{f};
    return *this;
  }

  template <typename Ret, typename Arg>
  constexpr bool operator==(Ret (*f)(Arg)) const {
    return std::get<action1>(value) == f;
  }

  constexpr bool operator==(actionf_p3 f) const {
    return static_cast<bool>(*this) && std::get<actionf_p3>(value) == f;
  }

  constexpr void reset() { value = std::monostate{}; }

  void remove() { value = action_removed{}; }

  bool is_removed() const {
    return std::holds_alternative<action_removed>(value);
  }

  constexpr operator bool() const {
    return !is_default_initialized() && !holds_empty_void_callback();
  }

  bool contains(actionf_p1 f) const { return (*this) == f; }

  bool call_if(mobj_thinker *mo) const {
    if (!(*this))
      return false;

    std::visit(
        [&mo](auto val) {
          if constexpr (std::is_same_v<action1, decltype(val)>) {
            static_cast<action1>(val).invoke(mo);
          }
        },
        value);
    return true;
  }

  bool call_if(thinker_t *t) const {
    return call_if(reinterpret_cast<mobj_thinker *>(t));
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

private:
  constexpr bool is_default_initialized() const {
    return std::holds_alternative<std::monostate>(value);
  }

  constexpr bool holds_empty_void_callback() const {
    return std::holds_alternative<actionf_v>(value) &&
           std::get<actionf_v>(value) == nullptr;
  }
};

// Historically, "think_t" is yet another
//  function pointer to a routine to handle
//  an actor.
// typedef actionf_t  think_t;

struct thinker_t;
extern void P_MobjThinker(mobj_t *);

// Doubly linked list of actors.
struct thinker_t {
  struct thinker_t *prev;
  struct thinker_t *next;
  actionf_t function;

  //
  // P_RemoveThinker
  // Deallocation is lazy -- it will not actually be freed
  // until its thinking turn comes up.
  //
  constexpr void mark_for_removal() { function.remove(); }

  constexpr bool needs_removal() const { return function.is_removed(); }

  constexpr void reset() { function.reset(); }

  template <typename Arg> constexpr thinker_t &operator=(void (*f)(Arg)) {
    function = f;
    return *this;
  }

  constexpr bool has_any() const { return function; }
};

template <typename T> struct thinker_trait;

template <typename T> T *thinker_cast(thinker_t *th) {
  if (th->function == thinker_trait<T>::func)
    return reinterpret_cast<T *>(th);
  return nullptr;
}

template <typename T> T *thinker_cast(thinker_t &th) {
  return thinker_cast<T>(&th);
}

template <> struct thinker_trait<mobj_t> {
  static constexpr const auto func = P_MobjThinker;
};

struct mobj_thinker {
  // List: thinker links.
  thinker_t thinker;
};

class thinker_list {
  thinker_t sentinel;

public:
  static thinker_list instance;

  class iterator {
    thinker_t *current;

  public:
    using value_type = thinker_t *;
    using iterator_category = std::forward_iterator_tag;
    using difference_type = ptrdiff_t;
    using reference = thinker_t *;

    constexpr iterator(thinker_t *val) : current(val) {}

    constexpr iterator &operator++() {
      current = current->next;
      return *this;
    }

    constexpr thinker_t *operator*() { return current; }

    constexpr bool operator==(iterator const &other) const {
      return current == other.current;
    }

    constexpr bool operator!=(iterator const &other) const {
      return !operator==(other);
    }
  };

  constexpr thinker_list() noexcept : sentinel{} { init(); }

  [[nodiscard]] constexpr iterator begin() noexcept { return {sentinel.next}; }

  [[nodiscard]] constexpr iterator end() noexcept { return {&sentinel}; }

  constexpr void push_back(mobj_thinker *thinker) {
    sentinel.prev->next = &thinker->thinker;
    thinker->thinker.next = &sentinel;
    thinker->thinker.prev = sentinel.prev;
    sentinel.prev = &thinker->thinker;
  }

  constexpr void init() { sentinel.prev = sentinel.next = &sentinel; }

  void run();

  uint32_t index_of(thinker_t *thinker);
  constexpr thinker_t *mobj_at(uintptr_t index);

private:
  void remove(thinker_t *thinker) {
    thinker->next->prev = thinker->prev;
    thinker->prev->next = thinker->next;
  }
};

inline thinker_list thinker_list::instance{};

namespace std {
template <> struct iterator_traits<thinker_list::iterator> {
  using value_type = thinker_list::iterator::value_type;
  using reference = thinker_list::iterator::reference;
  using difference_type = thinker_list::iterator::difference_type;
  using iterator_category = thinker_list::iterator::iterator_category;
};
} // namespace std
