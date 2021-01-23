/*
 * NesDev:
 * Emulator for the Nintendo Entertainment System (R) Archetecture.
 * Written by and Copyright (C) 2020 Shingo OKAWA shingo.okawa.g.h.c@gmail.com
 * Trademarks are owned by their respect owners.
 */
#ifndef _NESDEV_CORE_DETAIL_RP2C02_H_
#define _NESDEV_CORE_DETAIL_RP2C02_H_
#include <cstdint>
#include <memory>
#include "nesdev/core/exceptions.h"
#include "nesdev/core/ppu.h"
#include "nesdev/core/macros.h"
#include "nesdev/core/mmu.h"
#include "nesdev/core/types.h"

namespace nesdev {
namespace core {
namespace detail {

#define REG(x)    registers_->x.value
#define BIT(x, y) registers_->x.y
#define MSK(x)    registers_->ppustatus.x.mask

class RP2C02 final : public PPU {
 public:
  enum class MemoryMap : Address {
    PPUCTRL   = 0x0000,
    PPUMASK   = 0x0001,
    PPUSTATUS = 0x0002,
    OAMADDR   = 0x0003,
    OAMDATA   = 0x0004,
    PPUSCROLL = 0x0005,
    PPUADDR   = 0x0006,
    PPUDATA   = 0x0007
  };

  enum class SpriteSize : Byte {
    PIXELS_8x8,
    PIXELS_8x16
  };

  /*
   * The following registers are defined according to the folloing Loopy's archetecture.
   * [SEE] https://wiki.nesdev.com/w/index.php/PPU_scrolling
   */
  struct Registers {
    // $2000
    union {
      Byte value;
      Bitfield<0, 2, Byte> nametable;                  // Base nametable address (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
      Bitfield<0, 1, Byte> nametable_x;
      Bitfield<1, 1, Byte> nametable_y;
      Bitfield<2, 1, Byte> increment;                  // VRAM address increment per CPU read/write of PPUDATA (0: add 1, going across; 1: add 32, going down)
      Bitfield<3, 1, Byte> sprite_tile;                // Sprite pattern table address for 8x8 sprites (0: $0000; 1: $1000; ignored in 8x16 mode)
      Bitfield<4, 1, Byte> background_tile;            // Background pattern table address (0: $0000; 1: $1000)
      Bitfield<5, 1, Byte> sprite_height;              // Sprite size (0: 8x8 pixels; 1: 8x16 pixels)
      Bitfield<6, 1, Byte> ppu_master_slave;           // PPU master/slave select (0: read backdrop from EXT pins; 1: output color on EXT pins)
      Bitfield<7, 1, Byte> nmi_enable;                 // Generate an NMI at the start of the vertical blanking interval (0: off; 1: on)
    } ppuctrl = {0x00};
    // $2001
    union {
      Byte value;
      Bitfield<0, 1, Byte> greyscale;                  // Greyscale (0: normal color, 1: produce a greyscale display)
      Bitfield<1, 1, Byte> background_leftmost_enable; // 1: Show background in leftmost 8 pixels of screen, 0: Hide
      Bitfield<2, 1, Byte> sprite_leftmost_enable;     // 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
      Bitfield<3, 1, Byte> background_enable;          // 1: Show background
      Bitfield<4, 1, Byte> sprite_enable;              // 1: Show sprites
      Bitfield<5, 1, Byte> emphasize_red;              // Emphasize red (green on PAL/Dendy)
      Bitfield<6, 1, Byte> emphasize_green;            // Emphasize green (red on PAL/Dendy)
      Bitfield<7, 1, Byte> emphasize_blue;             // Emphasize blue
    } ppumask = {0x00};
    // $2002
    union {
      Byte value;
      Bitfield<0, 5, Byte> previous_lsb;               // Least significant bits previously written into a PPU register
      Bitfield<5, 1, Byte> sprite_overflow;            // Sprite overflow
      Bitfield<6, 1, Byte> sprite_zero_hit;            // Sprite 0 Hit
      Bitfield<7, 1, Byte> vblank_start;               // Vertical blank has started (0: not in vblank; 1: in vblank)
    } ppustatus = {0x00};
    // $2003 OAM read/write address
    union {
      Byte value;
    } oamaddr = {0x00};
    // $2004 OAM data read/write
    union {
      Byte value;
    } oamdata = {0x00};
    // $2005 fine scroll position (two writes: X scroll, Y scroll), CREDIT: Loopy
    union {
      Byte value;
    } fine_x = {0x00};
    // $2006 PPU read/write address (two writes: most significant byte, least significant byte), CREDIT: Loopy
    union {
      Address value;
      Bitfield< 0, 12, Address> tile_id;
      Bitfield< 0,  5, Address> coarse_x;
      Bitfield< 5,  5, Address> coarse_y;
      Bitfield<10,  1, Address> nametable_x;
      Bitfield<11,  1, Address> nametable_y;
      Bitfield<12,  3, Address> fine_y;
      Bitfield<15,  1, Address> unused;
      Bitfield< 0,  8, Address> lo;
      Bitfield< 8,  8, Address> hi;
    } vramaddr = {0x0000};
    // $2006 PPU read/write address (two writes: most significant byte, least significant byte), CREDIT: Loopy
    union {
      Address value;
      Bitfield< 0, 12, Address> tile_id;
      Bitfield< 0,  5, Address> coarse_x;
      Bitfield< 5,  5, Address> coarse_y;
      Bitfield<10,  1, Address> nametable_x;
      Bitfield<11,  1, Address> nametable_y;
      Bitfield<12,  3, Address> fine_y;
      Bitfield<15,  1, Address> unused;
      Bitfield< 0,  8, Address> lo;
      Bitfield< 8,  8, Address> hi;
    } tramaddr = {0x0000};
    // $2007 PPU data read/write
    union {
      Byte value;
    } ppudata = {0x00};
    // $2014 OAM DMA high address
    union {
      Byte value;
    } oamdma = {0x00};
  };

  struct Shifters {
    // Background tile pattern low bits
    union {
      Address value;
      PPU::Shifter<Address> shift;
    } background_pttr_lo = {0x0000};
    // Background tile pattern high bits
    union {
      Address value;
      PPU::Shifter<Address> shift;
    } background_pttr_hi = {0x0000};
    // Background tile palette attributes low bits
    union {
      Address value;
      PPU::Shifter<Address> shift;
    } background_attr_lo = {0x0000};
    // Background tile palette attributes high bits
    union {
      Address value;
      PPU::Shifter<Address> shift;
    } background_attr_hi = {0x0000};
  };

  struct Chips {
    Chips(std::unique_ptr<MemoryBank> oam)
      : oam{std::move(oam)} {}

    const std::unique_ptr<MemoryBank> oam;
  };

 public:
  RP2C02(std::unique_ptr<Chips> chips,
         Registers* const registers,
	 Shifters* const shifters,
         MMU* const mmu,
         const std::vector<Byte>& palette);

  ~RP2C02();

  void Tick() override;

  Byte Read(Address address) override;

  void Write(Address address, Byte byte) override;

 NESDEV_CORE_PRIVATE_UNLESS_TESTED:
  class Latch {
   public:
    Latch(Registers* const registers, MMU* const mmu, Chips* const chips)
      : registers_{registers},
        mmu_{mmu},
        chips_{chips} {}

    [[nodiscard]]
    Byte Latched() const noexcept {
      return latch_;
    }

    Byte Latched(Byte byte) noexcept {
      return latch_ = byte;
    }

    [[nodiscard]]
    Byte Deffered() const noexcept {
      return deffered_;
    }

    Byte Deffered(Byte byte) noexcept {
      return deffered_ = byte;
    }

    void ReadPPUCtrl() {
      /* Do nothing. */
    }

    void ReadPPUMask() {
      /* Do nothing. */
    }
    
    void ReadPPUStatus() {
      Latched((Latched() & 0x1F) | (REG(ppustatus) & 0xE0));
      REG(ppustatus) &= ~MSK(vblank_start);
      is_latched_     = false;
    }

    void ReadOAMAddr() {
      /* Do nothing. */
    }

    void ReadOAMData() {
      Latched(chips_->oam->Read(REG(oamaddr)));
    }

    void ReadPPUScroll() {
      /* Do nothing. */
    }

    void ReadPPUAddr() {
      /* Do nothing. */
    }

    void ReadPPUData() {
      if (REG(vramaddr) <= 0x3EFF) {
        // Reads from the NameTable ram get delayed one cycle, 
        // so output buffer which contains the data from the 
        // previous read request.
        Latched(Deffered());
        Deffered(mmu_->Read(REG(vramaddr)));
      } else {
        // However, if the address was in the palette range, the
        // data is not delayed, so it returns immediately.
        Deffered(mmu_->Read(REG(vramaddr)));
        Latched(Deffered());
      }
      REG(vramaddr) += BIT(ppuctrl, increment) ? 0x0020 : 0x0001;
    }

    void WritePPUCtrl(Byte byte) {
      REG(ppuctrl)               = Latched(byte);
      BIT(tramaddr, nametable_x) = BIT(ppuctrl, nametable_x);
      BIT(tramaddr, nametable_y) = BIT(ppuctrl, nametable_y);
    }

    void WritePPUMask(Byte byte) {
      REG(ppumask) = Latched(byte);
    }

    void WritePPUStatus(Byte byte) {
      Latched(byte);
    }

    void WriteOAMAddr(Byte byte) {
      REG(oamaddr) = Latched(byte);
    }

    void WriteOAMData(Byte byte) {
      chips_->oam->Write(REG(oamaddr), Latched(byte));
    }

    void WritePPUScroll(Byte byte) {
      if (!is_latched_) {
        // First write to scroll register contains X offset in pixel space
        // which we split into coarse and fine x values
        REG(fine_x)             = Latched(byte) & 0x07;
        BIT(tramaddr, coarse_x) = Latched(byte) >> 3;
        is_latched_             = true;
      } else {
        // First write to scroll register contains Y offset in pixel space
        // which we split into coarse and fine Y values
        BIT(tramaddr, fine_y)   = Latched(byte) & 0x07;
        BIT(tramaddr, coarse_y) = Latched(byte) >> 3;
        is_latched_             = false;
      }
    }

    void WritePPUAddr(Byte byte) {
      if (!is_latched_) {
        // PPU address bus can be accessed by CPU via the ADDR and DATA
        // registers. The fisrt write to this register latches the high byte
        // of the address, the second is the low byte. Note the writes
        // are stored in the tram register...
        BIT(tramaddr, hi) = Latched(byte) & 0x3F;
        is_latched_       = true;
      } else {
        // ...when a whole address has been written, the internal vram address
        // buffer is updated. Writing to the PPU is unwise during rendering
        // as the PPU will maintam the vram address automatically whilst
        // rendering the scanline position.
        BIT(tramaddr, lo) = Latched(byte);
        REG(vramaddr)     = REG(tramaddr);
        is_latched_       = false;
      }
    }

    void WritePPUData(Byte byte) {
      mmu_->Write(REG(vramaddr), Latched(byte));
      REG(vramaddr) += BIT(ppuctrl, increment) ? 0x0020 : 0x0001;
    }

   NESDEV_CORE_PRIVATE_UNLESS_TESTED:
    Registers* const registers_;

    MMU* const mmu_;

    Chips* const chips_;

    bool is_latched_ = false;

    Byte latch_      = {0x00};

    Byte deffered_   = {0x00};
  };

  class Shift {
   public:
    Shift(Registers* const registers, MMU* const mmu)
      : registers_{registers},
        mmu_{mmu} {}

    void ReadBgTileId() {
      background_.id = mmu_->Read(0x2000 | BIT(vramaddr, tile_id));
    }

    void ReadBgTileAttr() {
      background_.attr = mmu_->Read(0x23C0
                                    | ( BIT(vramaddr, nametable_y)    << 11)
                                    | ( BIT(vramaddr, nametable_x)    << 10) 
                                    | ((BIT(vramaddr, coarse_y) >> 2) <<  3) 
                                    | ( BIT(vramaddr, coarse_x)       >>  2));
      // Since we know we can access a tile directly from the 12 bit address, we
      // can analyse the bottom bits of the coarse coordinates to provide us with
      // the correct offset into the 8-bit word, to yield the 2 bits we are
      // actually interested in which specifies the palette for the 2x2 group of
      // tiles. We know if "coarse y % 4" < 2 we are in the top half else bottom half.
      // Likewise if "coarse x % 4" < 2 we are in the left half else right half.
      // Ultimately we want the bottom two bits of our attribute word to be the
      // palette selected. So shift as required...              
      if (BIT(vramaddr, coarse_y) & 0x02) background_.attr >>= 4;
      if (BIT(vramaddr, coarse_x) & 0x02) background_.attr >>= 2;
      background_.attr &= 0x03;
    }

    void ReadBgTileLSB() {
      background_.id = mmu_->Read((BIT(ppuctrl, background_tile) << 12)
                                  + (static_cast<Address>(background_.id) << 4)
                                  + BIT(vramaddr, fine_y));
    }

    void ReadBgTileMSB() {
      background_.id = mmu_->Read((BIT(ppuctrl, background_tile) << 12)
                                  + (static_cast<Address>(background_.id) << 4)
                                  + BIT(vramaddr, fine_y)
                                  + 8);
    }

   NESDEV_CORE_PRIVATE_UNLESS_TESTED:
    struct Background {
      Byte id   = {0x00};

      Byte attr = {0x00};

      Byte lsb  = {0x00};

      Byte msb  = {0x00};
    };

   NESDEV_CORE_PRIVATE_UNLESS_TESTED:
    Registers* const registers_;

    MMU* const mmu_;

    Background background_;
  };

 NESDEV_CORE_PRIVATE_UNLESS_TESTED:
  static MemoryMap Map(Address address) {
    switch (address % 0x08) {
    case 0x0000: return MemoryMap::PPUCTRL;
    case 0x0001: return MemoryMap::PPUMASK;
    case 0x0002: return MemoryMap::PPUSTATUS;
    case 0x0003: return MemoryMap::OAMADDR;
    case 0x0004: return MemoryMap::OAMDATA;
    case 0x0005: return MemoryMap::PPUSCROLL;
    case 0x0006: return MemoryMap::PPUADDR;
    case 0x0007: return MemoryMap::PPUDATA;
    default:     NESDEV_CORE_THROW(InvalidAddress::Occur("Invalid address specified to Map", address));
    }
  }

  [[nodiscard]]
  RP2C02::SpriteSize SpriteSize() const {
    switch (BIT(ppuctrl, sprite_height)) {
    case 0:  return RP2C02::SpriteSize::PIXELS_8x8;
    case 1:  return RP2C02::SpriteSize::PIXELS_8x16;
    default: NESDEV_CORE_THROW(InvalidRegister::Occur("Invalid value specified to PPUCTRL/SPRITE_HEIGHT", REG(ppuctrl)));
    }
  }

  [[nodiscard]]
  bool IsMaster() const noexcept {
    return BIT(ppuctrl, ppu_master_slave);
  }

  [[nodiscard]]
  bool IsNMIEnable() const noexcept {
    return BIT(ppuctrl, nmi_enable);
  }

 NESDEV_CORE_PRIVATE_UNLESS_TESTED:
  /* [SEE] https://wiki.nesdev.com/w/index.php/PPU_rendering */
  void Ticked() noexcept {
    ++Cycle();
    if (IsRendering() && (Cycle() == 260 && Scanline() < 240))
      rom_->mapper->Callback();
    if (Cycle() >= 341) {
      Cycle(0); ++Scanline();
      if (Scanline() >= 261) {
        Scanline(-1); TransitFrame();
      }
    }
  }

  [[nodiscard]]
  bool IsRendering() const noexcept {
    return BIT(ppumask, background_enable) || BIT(ppumask, sprite_enable);
  }

  [[nodiscard]]
  Byte Latched() const noexcept {
    return latch_.Latched();
  }

  void ReadPPUCtrl() {
    latch_.ReadPPUCtrl();
  }

  void ReadPPUMask() {
    latch_.ReadPPUMask();
  }
    
  void ReadPPUStatus() {
    latch_.ReadPPUStatus();
  }

  void ReadOAMAddr() {
    latch_.ReadOAMAddr();
  }

  void ReadOAMData() {
    latch_.ReadOAMData();
  }

  void ReadPPUScroll() {
    latch_.ReadPPUScroll();
  }

  void ReadPPUAddr() {
    latch_.ReadPPUAddr();
  }

  void ReadPPUData() {
    latch_.ReadPPUData();
  }

  void WritePPUCtrl(Byte byte) {
    latch_.WritePPUCtrl(byte);
  }

  void WritePPUMask(Byte byte) {
    latch_.WritePPUMask(byte);
  }

  void WritePPUStatus(Byte byte) {
    latch_.WritePPUStatus(byte);
  }

  void WriteOAMAddr(Byte byte) {
    latch_.WriteOAMAddr(byte);
  }

  void WriteOAMData(Byte byte) {
    latch_.WriteOAMData(byte);
  }

  void WritePPUScroll(Byte byte) {
    latch_.WritePPUScroll(byte);
  }

  void WritePPUAddr(Byte byte) {
    latch_.WritePPUAddr(byte);
  }

  void WritePPUData(Byte byte) {
    latch_.WritePPUData(byte);
  }

  void ScrollX() noexcept {
    if (IsRendering()) {
      // A single name table is 32 x 30 tiles.
      if (BIT(vramaddr, coarse_x) == 31) {
        BIT(vramaddr, coarse_x)    = 0;
        BIT(vramaddr, nametable_x) = ~BIT(vramaddr, nametable_x);
      } else {
        BIT(vramaddr, coarse_x)++;
      }
    }
  }

  void ScrollY() noexcept {
    if (IsRendering()) {
      // A single name table is 32 x 30 tiles.                
      // According to the existence of attribute memory, an increment in Yx first adjusts the fine offset.
      if (BIT(vramaddr, fine_y) < 7) {
        BIT(vramaddr, fine_y)++;
      } else {
        BIT(vramaddr, fine_y) = 0;
        if (BIT(vramaddr, coarse_y) == 29) {
          BIT(vramaddr, coarse_y)    = 0;
          BIT(vramaddr, nametable_y) = ~BIT(vramaddr, nametable_y);
        } else if (BIT(vramaddr, coarse_y) == 31) {
          BIT(vramaddr, coarse_y) = 0;
        } else {
          BIT(vramaddr, coarse_y)++;
        }
      }
    }
  }

  void TransferX() noexcept {
    if (IsRendering()) {
      BIT(vramaddr, nametable_x) = BIT(tramaddr, nametable_x);
      BIT(vramaddr, coarse_x)    = BIT(tramaddr, coarse_x);
    }
  }

  void TransferY() noexcept {
    if (IsRendering()) {
      BIT(vramaddr, fine_y)      = BIT(tramaddr, fine_y);
      BIT(vramaddr, nametable_y) = BIT(tramaddr, nametable_y);
      BIT(vramaddr, coarse_y)    = BIT(tramaddr, coarse_y);
    }
  }

 NESDEV_CORE_PRIVATE_UNLESS_TESTED:
  const std::unique_ptr<Chips> chips_;

  Registers* const registers_;

  Shifters* const shifters_;

  MMU* const mmu_;

  Latch latch_;

//  ShiftRegister shifter_;
};

#undef REG
#undef BIT
#undef MSK

}  // namespace detail
}  // namespace core
}  // namespace nesdev
#endif  // ifndef _NESDEV_CORE_DETAIL_RP2C02_H_
