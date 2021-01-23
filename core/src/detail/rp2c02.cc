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
#include "nesdev/core/rom.h"
#include "nesdev/core/types.h"
#include "detail/rp2c02.h"
#include "detail/memory_banks/chip.h"

namespace nesdev {
namespace core {
namespace detail {

#define REG(x)    registers_->x.value
#define BIT(x, y) registers_->x.y
#define MSK(x)    registers_->ppustatus.x.mask

RP2C02::RP2C02(std::unique_ptr<RP2C02::Chips> chips,
               RP2C02::Registers* const registers,
               MMU* const mmu,
	       const std::vector<Byte>& palette)
  : PPU{palette},
    chips_{std::move(chips)},
    registers_{registers},
    mmu_{mmu},
    latch_{registers_, mmu_, chips_.get()} {}

RP2C02::~RP2C02() {}

Byte RP2C02::Read(Address address) {
  switch (Map(address)) {
  case MemoryMap::PPUCTRL:   ReadPPUCtrl();   break;
  case MemoryMap::PPUMASK:   ReadPPUMask();   break;
  case MemoryMap::PPUSTATUS: ReadPPUStatus(); break;
  case MemoryMap::OAMADDR:   ReadOAMAddr();   break;
  case MemoryMap::OAMDATA:   ReadOAMData();   break;
  case MemoryMap::PPUSCROLL: ReadPPUScroll(); break;
  case MemoryMap::PPUADDR:   ReadPPUAddr();   break;
  case MemoryMap::PPUDATA:   ReadPPUData();   break;
  default:                                    break;
  }
  return Latched();
}

void RP2C02::Write(Address address, Byte byte) {
  switch (Map(address)) {
  case MemoryMap::PPUCTRL:   WritePPUCtrl(byte);   break;
  case MemoryMap::PPUMASK:   WritePPUMask(byte);   break;
  case MemoryMap::PPUSTATUS: WritePPUStatus(byte); break;
  case MemoryMap::OAMADDR:   WriteOAMAddr(byte);   break;
  case MemoryMap::OAMDATA:   WriteOAMData(byte);   break;
  case MemoryMap::PPUSCROLL: WritePPUScroll(byte); break;
  case MemoryMap::PPUADDR:   WritePPUAddr(byte);   break;
  case MemoryMap::PPUDATA:   WritePPUData(byte);   break;
  default:                                         break;
  }
}

/*
 * The following instruction timings are defined according to the following article.
 * [SEE] https://wiki.nesdev.com/w/index.php/PPU_rendering
 */
void RP2C02::Tick() {
  if (IsPreRenderOrVisibleLine()) {
    if (Scanline() == 0 && Cycle() == 0 && IsOddFrame() && IsRendering())
      Cycle(1);

    if (Scanline() == -1 && Cycle() == 1) {
      REG(ppustatus) &= ~(MSK(vblank_start) | MSK(sprite_overflow) | MSK(sprite_zero_hit));
      // Clear Shifters
//      for (int i = 0; i < 8; i++) {
//	sprite_shifter_pattern_lo[i] = 0;
//	sprite_shifter_pattern_hi[i] = 0;
//      }
    }

  }

  Ticked();
}

}  // namespace detail
}  // namespace core
}  // namespace nesdev

