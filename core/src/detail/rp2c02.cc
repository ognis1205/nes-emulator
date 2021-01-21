/*
 * NesDev:
 * Emulator for the Nintendo Entertainment System (R) Archetecture.
 * Written by and Copyright (C) 2020 Shingo OKAWA shingo.okawa.g.h.c@gmail.com
 * Trademarks are owned by their respect owners.
 */
#include <memory>
#include "nesdev/core/ppu.h"
#include "nesdev/core/exceptions.h"
#include "nesdev/core/macros.h"
#include "nesdev/core/types.h"
#include "detail/rp2c02.h"
#include "detail/memory_banks/chip.h"

namespace nesdev {
namespace core {
namespace detail {

RP2C02::RP2C02(std::unique_ptr<RP2C02::Chips> chips, RP2C02::Registers* const registers, MMU* const mmu)
  : chips_{std::move(chips)},
    registers_{registers},
    mmu_{mmu},
    latch_{&context_, registers_, mmu_, chips_.get()} {}

RP2C02::~RP2C02() {}

Byte RP2C02::Read(Address address) {
  switch (Map(address)) {
  case MemoryMap::PPUCTRL:
    ReadPPUCtrl();
    break;
  case MemoryMap::PPUMASK:
    ReadPPUMask();
    break;
  case MemoryMap::PPUSTATUS:
    ReadPPUStatus();
    break;
  case MemoryMap::OAMADDR:
    ReadOAMAddr();
    break;
  case MemoryMap::OAMDATA:
    ReadOAMData();
    break;
  case MemoryMap::PPUSCROLL:
    ReadPPUScroll();
    break;
  case MemoryMap::PPUADDR:
    ReadPPUAddr();
    break;
  case MemoryMap::PPUDATA:
    ReadPPUData();
    break;
  default:
    break;
  }
  return Latched();
}

}  // namespace detail
}  // namespace core
}  // namespace nesdev

