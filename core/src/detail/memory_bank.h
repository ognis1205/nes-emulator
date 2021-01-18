/*
 * NesDev:
 * Emulator for the Nintendo Entertainment System (R) Archetecture.
 * Written by and Copyright (C) 2020 Shingo OKAWA shingo.okawa.g.h.c@gmail.com
 * Trademarks are owned by their respect owners.
 */
#ifndef _NESDEV_CORE_DETAIL_MEMORY_BANK_H_
#define _NESDEV_CORE_DETAIL_MEMORY_BANK_H_
#include <cstddef>
#include <vector>
#include "nesdev/core/exceptions.h"
#include "nesdev/core/macros.h"
#include "nesdev/core/memory_bank.h"
#include "nesdev/core/types.h"

namespace nesdev {
namespace core {
namespace detail {

template <Address From, Address To>
class MemoryBank final : public nesdev::core::MemoryBank {
 public:
  static_assert(
    From <= To,
    "Start address must be greater than end address");

 public:
  MemoryBank(std::size_t size) {
    NESDEV_CORE_CASSERT((To - From + 1u) % size == 0, "Size does not match address range");
    data_.resize(size);
  }

  [[nodiscard]]
  bool HasValidAddress(Address address) const noexcept override {
    if constexpr (From == 0) return address <= To;
    else return address >= From && address <= To;
  }

  Byte Read(Address address) const override {
    if (HasValidAddress(address)) return *PtrTo(address);
    else NESDEV_CORE_THROW(InvalidAddress::Occur("Invalid address specified to Read", address));
  }

  void Write(Address address, Byte byte) override {
    if (HasValidAddress(address)) *PtrTo(address) = byte;
    else NESDEV_CORE_THROW(InvalidAddress::Occur("Invalid address specified to Write", address));
  }

  std::size_t Size() const noexcept override {
    return data_.size();
  }

  Byte* Data() override {
    return const_cast<Byte*>(std::as_const(*this).Data());
  }

  const Byte* Data() const override {
    return data_.data();
  }

 NESDEV_CORE_PRIVATE_UNLESS_TESTED:
  Byte* PtrTo(Address address) {
    return const_cast<Byte*>(std::as_const(*this).PtrTo(address));
  }

  const Byte* PtrTo(Address address) const {
    return &data_.data()[address % Size()];
  }

 NESDEV_CORE_PRIVATE_UNLESS_TESTED:
  std::vector<Byte> data_;
};

class VoidMemory final : public nesdev::core::MemoryBank {
 public:
  VoidMemory() = default;

  [[nodiscard]]
  bool HasValidAddress([[maybe_unused]] Address address) const noexcept override {
    return false;
  }

  Byte Read(Address address) const override {
    NESDEV_CORE_THROW(InvalidAddress::Occur("Invalid address specified to Read", address));
  }

  void Write(Address address, [[maybe_unused]] Byte byte) override {
    NESDEV_CORE_THROW(InvalidAddress::Occur("Invalid address specified to Write", address));
  }

  std::size_t Size() const noexcept override {
    return 0;
  }

  Byte* Data() override {
    NESDEV_CORE_THROW(InvalidOperation::Occur("Invalid operation specified to void memory"));
  }

  const Byte* Data() const override {
    NESDEV_CORE_THROW(InvalidOperation::Occur("Invalid operation specified to void memory"));
  }
};

}  // namespace detail
}  // namespace core
}  // namespace nesdev
#endif  // ifndef _NESDEV_CORE_DETAIL_MEMORY_BANK_H_
