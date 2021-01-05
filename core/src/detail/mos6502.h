/*
 * NesDev:
 * Emulator for the Nintendo Entertainment System (R) Archetecture.
 * Written by and Copyright (C) 2020 Shingo OKAWA shingo.okawa.g.h.c@gmail.com
 * Trademarks are owned by their respect owners.
 */
#ifndef _NESDEV_CORE_DETAIL_MOS6502_H_
#define _NESDEV_CORE_DETAIL_MOS6502_H_
#include <cstdint>
#include <functional>
#include "nesdev/core/cpu.h"
#include "nesdev/core/mmu.h"
#include "nesdev/core/opcodes.h"
#include "nesdev/core/types.h"
#include "macros.h"
#include "pipeline.h"

namespace nesdev {
namespace core {
namespace detail {

class MOS6502 final : public CPU {
 public:
  static const Address kBRKAddress = {0xFFFE};

  static const Address kRSTAddress = {0xFFFC};

  static const Address kNMIAddress = {0xFFFA};

  struct Registers {
    // Accumulator
    union {
      Byte value;
    } a = {0x00};
    // X index register
    union {
      Byte value;
    } x = {0x00};
    // Y index register
    union {
      Byte value;
    } y = {0x00};
    // Stack pointer
    union {
      Byte value;
    } s = {0x00};
    // Program counter
    union {
      Address value;
      Bitfield<0, 8, Address> lo;
      Bitfield<8, 8, Address> hi;
    } pc = {0x0000};
    // Status register
    union {
      Byte value;
      Bitfield<0, 1, Byte> carry;
      Bitfield<1, 1, Byte> zero;
      Bitfield<2, 1, Byte> irq_disable;
      Bitfield<3, 1, Byte> decimal_mode;
      Bitfield<4, 1, Byte> brk_command;
      Bitfield<5, 1, Byte> unused;
      Bitfield<6, 1, Byte> overflow;
      Bitfield<7, 1, Byte> negative;
    } p = {0x00};
  };

 public:
  MOS6502(Registers* const registers, MMU* const mmu);

  ~MOS6502();

  void Tick() override;

  void FetchOpcode() override;

  void RST() noexcept override;

  void IRQ() noexcept override;

  void NMI() noexcept override;

 NESDEV_CORE_PRIVATE_UNLESS_TESTED:
  class Stack {
   public:
    static const Address kOffset = {0x0100};

    static const Byte kHead = {0xFD};

    Stack(Registers* const registers, MMU* const mmu)
      : registers_{registers}, mmu_{mmu} {}

    Byte Pull() const {
      return mmu_->Read(Stack::kOffset + ++registers_->s.value);
    }

    void Push(Byte byte) {
      mmu_->Write(Stack::kOffset + registers_->s.value--, byte);
    }

   NESDEV_CORE_PRIVATE_UNLESS_TESTED:
    Registers* const registers_;

    MMU* const mmu_;
  };

  class ALU {
   public:
    union Bus {
      Word concat;
      Bitfield<0, 8, Word> b;
      Bitfield<8, 8, Word> a;
    };

    static Bus Load(Byte a, Byte b) noexcept {
      return {(Word)(a << 8 | b)};
    };

    ALU(Registers* const registers)
      : registers_{registers} {}

    Byte ShiftL(Bus bus, bool rotate_carry) noexcept {
      bus.concat <<= 1;
      bus.b |= rotate_carry ? registers_->p.carry : 0x00;
      registers_->p.carry    = bus.a  & 0x01;
      registers_->p.zero     = bus.b == 0x00;
      registers_->p.negative = bus.b  & 0x80;
      return bus.b;
    }

    Byte ShiftR(Bus bus, bool rotate_carry) noexcept {
      bus.concat >>= 1;
      bus.a |= rotate_carry ? registers_->p.carry << 7 : 0x00;
      registers_->p.carry    = bus.b  & 0x80;
      registers_->p.zero     = bus.a == 0x00;
      registers_->p.negative = bus.a  & 0x80;
      return bus.a;
    }

    Byte Increment(Bus bus) noexcept {
      ++bus.b;
      registers_->p.zero     = bus.b == 0x00;
      registers_->p.negative = bus.b  & 0x80;
      return bus.b;
    }

    Byte Decrement(Bus bus) noexcept {
      --bus.b;
      registers_->p.zero     = bus.b == 0x00;
      registers_->p.negative = bus.b  & 0x80;
      return bus.b;
    }

    Byte PassThrough(Bus bus, bool check_zero_or_negative) noexcept {
      if (check_zero_or_negative) {
	registers_->p.zero     = bus.b == 0x00;
	registers_->p.negative = bus.b  & 0x80;
      }
      return bus.b;
    }

    Byte Or(Bus bus) noexcept {
      bus.b |= bus.a;
      registers_->p.zero     = bus.b == 0x00;
      registers_->p.negative = bus.b  & 0x80;
      return bus.b;
    }

    Byte And(Bus bus) noexcept {
      bus.b &= bus.a;
      registers_->p.zero     = bus.b == 0x00;
      registers_->p.negative = bus.b  & 0x80;
      return bus.b;
    }

    Byte Xor(Bus bus) noexcept {
      bus.b ^= bus.a;
      registers_->p.zero     = bus.b == 0x00;
      registers_->p.negative = bus.b  & 0x80;
      return bus.b;
    }

    Byte Add(Bus bus) noexcept {
      bus.concat = CheckOverflow(bus.a, bus.b, bus.a + bus.b + registers_->p.carry);
      registers_->p.carry    = bus.a  & 0x01;
      registers_->p.zero     = bus.b == 0x00;
      registers_->p.negative = bus.b  & 0x80;
      return bus.b;
    }

    void Cmp(Bus bus) noexcept {
      registers_->p.carry    = bus.a >= bus.b;
      bus.concat = bus.a - bus.b;
      registers_->p.zero     = bus.b == 0x00;
      registers_->p.negative = bus.b  & 0x80;
    }

    void Bit(Bus bus) noexcept {
      bus.concat = (bus.a & bus.b) << 8 | bus.b;
      registers_->p.zero     = bus.a == 0x00;
      registers_->p.negative = bus.b  & 1u << 7;
      registers_->p.overflow = bus.b  & 1u << 6;
    }

   NESDEV_CORE_PRIVATE_UNLESS_TESTED:
    // [SEE] http://www.righto.com/2012/12/the-6502-overflow-flag-explained.html
    Word CheckOverflow(Word a, Word b, Word r) noexcept {
      registers_->p.overflow = (a ^ r) & (b ^ r) & 0x80;
      return r;
    };

   NESDEV_CORE_PRIVATE_UNLESS_TESTED:
    Registers* const registers_;
  };

 NESDEV_CORE_PRIVATE_UNLESS_TESTED:
  Byte Fetch(bool immediate) noexcept override {
    if (immediate)
      SetAddress(registers_->pc.value++);
    return context_.fetched = Read(GetAddress());
  }

  void Stage(const std::function<void()>& step) noexcept {
    pipeline_.Push(step);
  }

  bool ClearWhenCompleted() noexcept {
    if (pipeline_.Done()) {
      pipeline_.Clear();
      return true;
    }
    return false;
  }

  void Execute() {
    pipeline_.Tick();
  }

  void ReadOpcode() noexcept {
    context_.opcode_byte = Read(registers_->pc.value++);
    context_.opcode = nesdev::core::Decode(context_.opcode_byte);
  }

  Byte Read(Address address) const {
    return mmu_->Read(address);
  }

  void Write(Address address, Byte byte) {
    mmu_->Write(address, byte);
  }

  Byte Pull() const {
    return stack_.Pull();
  }

  void Push(Byte byte) {
    stack_.Push(byte);
  }

  Byte ShiftL(Byte b, bool rotate_carry) noexcept {
    return alu_.ShiftL(ALU::Load(0x00, b), rotate_carry);
  }

  Byte ShiftR(Byte a, bool rotate_carry) noexcept {
    return alu_.ShiftR(ALU::Load(a, 0x00), rotate_carry);
  }

  Byte Increment(Byte b) noexcept {
    return alu_.Increment(ALU::Load(0x00, b));
  }

  Byte Decrement(Byte b) noexcept {
    return alu_.Decrement(ALU::Load(0x00, b));
  }

  Byte PassThrough(Byte b, bool check_zero_or_negative) noexcept {
    return alu_.PassThrough(ALU::Load(0x00, b), check_zero_or_negative);
  }

  Byte Or(Byte a, Byte b) noexcept {
    return alu_.Or(ALU::Load(a, b));
  }

  Byte And(Byte a, Byte b) noexcept {
    return alu_.And(ALU::Load(a, b));
  }

  Byte Xor(Byte a, Byte b) noexcept {
    return alu_.Xor(ALU::Load(a, b));
  }

  Byte Add(Byte a, Byte b) noexcept {
    return alu_.Add(ALU::Load(a, b));
  }

  Byte Sub(Byte a, Byte b) noexcept {
    return alu_.Add(ALU::Load(a, ~b));
  }

  void Cmp(Byte a, Byte b) noexcept {
    return alu_.Cmp(ALU::Load(a, b));
  }

  void Bit(Byte a, Byte b) noexcept {
    return alu_.Bit(ALU::Load(a, b));
  }

  void WithACC(Instruction instruction, [[maybe_unused]]MemoryAccess memory_access, Byte opcode);

  void WithIMP(Instruction instruction, [[maybe_unused]]MemoryAccess memory_access, Byte opcode);

  void WithIMM(Instruction instruction, [[maybe_unused]]MemoryAccess memory_access, Byte opcode);

  void WithABS(Instruction instruction, [[maybe_unused]]MemoryAccess memory_access, Byte opcode);

 NESDEV_CORE_PRIVATE_UNLESS_TESTED:
  Registers* const registers_;

  MMU* const mmu_;

  Stack stack_;

  ALU alu_;

  Pipeline pipeline_;
};

}  // namespace detail
}  // namespace core
}  // namespace nesdev
#endif  // ifndef _NESDEV_CORE_DETAIL_CPU_H_
