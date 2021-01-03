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

  static const Address kResetAddress = 0xFFFC;

  static const Address kNMIAddress = 0xFFFA;

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

  void Fetch() override;

  void Reset() noexcept override;

  void IRQ() noexcept override;

  void NMI() noexcept override;

 NESDEV_CORE_PRIVATE_UNLESS_TESTED:
  class Stack {
   public:
    static const Address kOffset = {0x0100};

    static const Byte kHead = 0xFD;

    Stack(Registers* const registers, MMU* const mmu);

    Byte Pull() const;

    void Push(Byte byte);

   NESDEV_CORE_PRIVATE_UNLESS_TESTED:
    Registers* const registers_;

    MMU* const mmu_;
  };

  class ALU {
   public:
    union Bus {
      std::uint_least16_t concat;
      Bitfield<0, 8, std::uint_least16_t> b;
      Bitfield<8, 8, std::uint_least16_t> a;
    };

    ALU(Registers* const registers);

    Byte ShiftL(Bus bus, bool rotate_carry);

    Byte ShiftR(Bus bus, bool rotate_carry);

   NESDEV_CORE_PRIVATE_UNLESS_TESTED:
    Registers* const registers_;
  };

 NESDEV_CORE_PRIVATE_UNLESS_TESTED:
  void Stage(const std::function<void()>& step) noexcept {
    pipeline_.Push(step);
  }

  void Stage(Pipeline(MOS6502::*addressing_mode)(Instruction, MemoryAccess, const Byte&),
	     Instruction instruction,
	     MemoryAccess memory_access,
	     const Byte& opcode) {
    pipeline_.Append(std::forward<Pipeline>((this->*addressing_mode)(instruction, memory_access, opcode)));
  }

  bool ClearWhenCompletion() noexcept {
    if (pipeline_.Done()) {
      pipeline_.Clear();
      return true;
    }
    return false;
  }

  void Execute() {
    pipeline_.Tick();
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

  Byte ShiftL(Byte operand, bool carry_bit) {
    ALU::Bus bus;
    bus.b = operand;
    return alu_.ShiftL(bus, carry_bit);
  }

  Byte ShiftR(Byte operand, bool carry_bit) {
    ALU::Bus bus;
    bus.a = operand;
    return alu_.ShiftR(bus, carry_bit);
  }

  Pipeline ACC(Instruction instruction, [[maybe_unused]]MemoryAccess memory_access, const Byte& opcode);

  Pipeline IMP(Instruction instruction, [[maybe_unused]]MemoryAccess memory_access, const Byte& opcode);

  Pipeline IMM(Instruction instruction, [[maybe_unused]]MemoryAccess memory_access, const Byte& opcode);

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
