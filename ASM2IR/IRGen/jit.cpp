#include "jit.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>

extern "C" {
#include "sim.h"
}

namespace sim {
namespace {

constexpr std::size_t return_capacity = 65536;

enum class Fault : std::uint64_t {
    Fallthrough, ReturnOverflow, ReturnAddress, ZeroDivisor, DivisionOverflow,
    Shift, AbsOverflow, MemoryExhausted, MemoryAddress, Uninitialized, Pixel
};

const char* fault_message(Fault fault) {
    switch (fault) {
    case Fault::Fallthrough: return "execution reached the end of the program without ret";
    case Fault::ReturnOverflow: return "return stack exhausted";
    case Fault::ReturnAddress: return "invalid return address";
    case Fault::ZeroDivisor: return "division by zero";
    case Fault::DivisionOverflow: return "signed division overflow";
    case Fault::Shift: return "shift count is outside the instruction width";
    case Fault::AbsOverflow: return "abs of -2^63 is not representable";
    case Fault::MemoryExhausted: return "simulated memory exhausted";
    case Fault::MemoryAddress: return "memory access is outside its allocation or unaligned";
    case Fault::Uninitialized: return "read of an uninitialized memory word";
    case Fault::Pixel: return "pixel coordinates are outside the screen";
    }
    return "unknown execution error";
}

const char* handler_name(Opcode opcode) {
    switch (opcode) {
    case Opcode::Mov: return "sim_do_mov";
    case Opcode::Add: return "sim_do_add";
    case Opcode::Sub: return "sim_do_sub";
    case Opcode::Mul: return "sim_do_mul";
    case Opcode::Sdiv: return "sim_do_sdiv";
    case Opcode::Udiv: return "sim_do_udiv";
    case Opcode::Srem: return "sim_do_srem";
    case Opcode::Shl: return "sim_do_shl";
    case Opcode::Ashr: return "sim_do_ashr";
    case Opcode::Or: return "sim_do_or";
    case Opcode::Sext32: return "sim_do_sext32";
    case Opcode::Abs: return "sim_do_abs";
    case Opcode::Umax: return "sim_do_umax";
    case Opcode::Clamp8: return "sim_do_clamp8";
    case Opcode::Mdiv: return "sim_do_mdiv";
    case Opcode::Alloc: return "sim_do_alloc";
    case Opcode::Ld: return "sim_do_ld";
    case Opcode::St: return "sim_do_st";
    case Opcode::Init: return "sim_do_init";
    case Opcode::Exit: return "sim_do_exit";
    case Opcode::Flush: return "sim_do_flush";
    case Opcode::Putpixel: return "sim_do_putpixel";
    case Opcode::Rand: return "sim_do_rand";
    case Opcode::Hasclick: return "sim_do_hasclick";
    case Opcode::Getclick: return "sim_do_getclick";
    default: throw std::logic_error("control instruction has no emulation handler");
    }
}

class Generator {
public:
    Generator(llvm::LLVMContext& context, const Program& program, JitMode mode,
              std::size_t memory_words)
        : module(std::make_unique<llvm::Module>("sim_asm", context)),
          builder(context), program(program), mode(mode), memory_words(memory_words) {}

    std::unique_ptr<llvm::Module> generate();

private:
    std::unique_ptr<llvm::Module> module;
    llvm::IRBuilder<> builder;
    const Program& program;
    JitMode mode;
    std::size_t memory_words;
    llvm::Function* function = nullptr;
    llvm::GlobalVariable* registers = nullptr;
    llvm::GlobalVariable* runtime = nullptr;
    llvm::GlobalVariable* memory = nullptr;
    llvm::GlobalVariable* initialized = nullptr;
    llvm::GlobalVariable* allocation_begin = nullptr;
    llvm::GlobalVariable* allocation_end = nullptr;
    llvm::GlobalVariable* next_word = nullptr;
    llvm::GlobalVariable* ready = nullptr;
    llvm::AllocaInst* return_stack = nullptr;
    llvm::AllocaInst* stack_size = nullptr;
    llvm::AllocaInst* current_line = nullptr;
    llvm::BasicBlock* done = nullptr;
    std::vector<llvm::BasicBlock*> blocks;

    llvm::GlobalVariable* external_array(const char* name, llvm::Type* element, std::size_t count);
    llvm::Value* element(llvm::GlobalVariable* array, llvm::Value* index);
    llvm::Value* read(const Operand& operand, unsigned width = 64);
    void write(const Instruction& instruction, llvm::Value* value);
    void fault(llvm::Value* line, Fault code);
    void guard(llvm::Value* condition, const Instruction& instruction, Fault code);
    llvm::BasicBlock* block(const Operand& operand);
    llvm::Value* comparison(const Instruction& instruction);
    void control(const Instruction& instruction, std::size_t index);
    void native(const Instruction& instruction);
    llvm::Value* division(llvm::Value* a, llvm::Value* b, const Instruction& instruction,
                          bool is_signed, bool remainder = false);
    llvm::Value* address(const Instruction& instruction, std::size_t base_operand);
    void allocate(const Instruction& instruction);
    void ensure_sim();
    void close_sim();
    void sim_call(const Instruction& instruction);
};

llvm::GlobalVariable* Generator::external_array(const char* name, llvm::Type* element,
                                                 std::size_t count) {
    return new llvm::GlobalVariable(*module, llvm::ArrayType::get(element, count), false,
                                    llvm::GlobalValue::ExternalLinkage, nullptr, name);
}

llvm::Value* Generator::element(llvm::GlobalVariable* array, llvm::Value* index) {
    return builder.CreateInBoundsGEP(array->getValueType(), array, {builder.getInt64(0), index});
}

llvm::Value* Generator::read(const Operand& operand, unsigned width) {
    llvm::Value* value = nullptr;
    if (const auto* reg = std::get_if<Register>(&operand)) {
        value = builder.CreateLoad(builder.getInt64Ty(), element(registers, builder.getInt64(reg->index)));
    } else {
        value = builder.getInt64(std::get<Immediate>(operand).bits);
    }
    return builder.CreateTruncOrBitCast(value, builder.getIntNTy(width));
}

void Generator::write(const Instruction& instruction, llvm::Value* value) {
    const auto index = std::get<Register>(instruction.operands[0]).index;
    builder.CreateStore(builder.CreateZExtOrBitCast(value, builder.getInt64Ty()),
                        element(registers, builder.getInt64(index)));
}

void Generator::fault(llvm::Value* line, Fault code) {
    const auto callback = module->getOrInsertFunction(
        "sim_error", builder.getVoidTy(), builder.getPtrTy(), builder.getInt64Ty(), builder.getInt64Ty());
    builder.CreateCall(callback, {builder.CreateLoad(builder.getPtrTy(), runtime), line,
                                 builder.getInt64(static_cast<std::uint64_t>(code))});
    builder.CreateRetVoid();
}

void Generator::guard(llvm::Value* condition, const Instruction& instruction, Fault code) {
    auto* success = llvm::BasicBlock::Create(module->getContext(), "checked", function);
    auto* failure = llvm::BasicBlock::Create(module->getContext(), "fault", function);
    builder.CreateCondBr(condition, success, failure);
    builder.SetInsertPoint(failure);
    fault(builder.getInt64(instruction.source_line), code);
    builder.SetInsertPoint(success);
}

llvm::BasicBlock* Generator::block(const Operand& operand) {
    return blocks.at(std::get<LabelTarget>(operand).instruction_index);
}

llvm::Value* Generator::comparison(const Instruction& instruction) {
    auto* a = read(instruction.operands[0], instruction.width);
    auto* b = read(instruction.operands[1], instruction.width);
    switch (instruction.predicate) {
    case Predicate::Eq: return builder.CreateICmpEQ(a, b);
    case Predicate::Slt: return builder.CreateICmpSLT(a, b);
    case Predicate::Sgt: return builder.CreateICmpSGT(a, b);
    case Predicate::None: throw std::logic_error("comparison has no predicate");
    }
    throw std::logic_error("invalid predicate");
}

void Generator::control(const Instruction& instruction, std::size_t index) {
    switch (instruction.opcode) {
    case Opcode::Jmp:
        builder.CreateBr(block(instruction.operands[0]));
        return;
    case Opcode::Bcmp:
        builder.CreateCondBr(comparison(instruction), block(instruction.operands[2]), blocks[index + 1]);
        return;
    case Opcode::Brect: {
        auto* in_x = builder.CreateICmpULT(read(instruction.operands[0]), builder.getInt64(SIM_X_SIZE));
        auto* in_y = builder.CreateICmpULT(read(instruction.operands[1]), builder.getInt64(SIM_Y_SIZE));
        builder.CreateCondBr(builder.CreateAnd(in_x, in_y), block(instruction.operands[2]), blocks[index + 1]);
        return;
    }
    case Opcode::Call: {
        auto* size = builder.CreateLoad(builder.getInt64Ty(), stack_size);
        guard(builder.CreateICmpULT(size, builder.getInt64(return_capacity)), instruction, Fault::ReturnOverflow);
        auto* slot = builder.CreateInBoundsGEP(return_stack->getAllocatedType(), return_stack,
                                              {builder.getInt64(0), size});
        builder.CreateStore(builder.getInt64(index + 1), slot);
        builder.CreateStore(builder.CreateAdd(size, builder.getInt64(1)), stack_size);
        builder.CreateBr(block(instruction.operands[0]));
        return;
    }
    case Opcode::Ret: {
        auto* size = builder.CreateLoad(builder.getInt64Ty(), stack_size);
        auto* pop = llvm::BasicBlock::Create(module->getContext(), "pop_return", function);
        builder.CreateCondBr(builder.CreateICmpEQ(size, builder.getInt64(0)), done, pop);
        builder.SetInsertPoint(pop);
        auto* reduced = builder.CreateSub(size, builder.getInt64(1));
        builder.CreateStore(reduced, stack_size);
        auto* slot = builder.CreateInBoundsGEP(return_stack->getAllocatedType(), return_stack,
                                              {builder.getInt64(0), reduced});
        auto* destination = builder.CreateLoad(builder.getInt64Ty(), slot);
        auto* invalid = llvm::BasicBlock::Create(module->getContext(), "bad_return", function);
        auto* dispatch = builder.CreateSwitch(destination, invalid);
        for (std::size_t call = 0; call < program.instructions.size(); ++call) {
            if (program.instructions[call].opcode == Opcode::Call) {
                dispatch->addCase(builder.getInt64(call + 1), blocks[call + 1]);
            }
        }
        builder.SetInsertPoint(invalid);
        fault(builder.getInt64(instruction.source_line), Fault::ReturnAddress);
        return;
    }
    default:
        break;
    }

    if (mode == JitMode::HandlerCalls) {
        const auto handler = module->getOrInsertFunction(
            handler_name(instruction.opcode), builder.getInt1Ty(), builder.getPtrTy(), builder.getInt64Ty());
        auto* succeeded = builder.CreateCall(
            handler, {builder.CreateLoad(builder.getPtrTy(), runtime), builder.getInt64(index)});
        builder.CreateCondBr(succeeded, blocks[index + 1], done);
    } else {
        native(instruction);
        builder.CreateBr(blocks[index + 1]);
    }
}

llvm::Value* Generator::division(llvm::Value* a, llvm::Value* b, const Instruction& instruction,
                                bool is_signed, bool remainder) {
    auto* type = llvm::cast<llvm::IntegerType>(a->getType());
    guard(builder.CreateICmpNE(b, llvm::ConstantInt::get(type, 0)), instruction, Fault::ZeroDivisor);
    if (is_signed) {
        auto* minimum = llvm::ConstantInt::get(type, llvm::APInt::getSignedMinValue(type->getBitWidth()));
        auto* minus_one = llvm::ConstantInt::getSigned(type, -1);
        auto* overflow = builder.CreateAnd(builder.CreateICmpEQ(a, minimum), builder.CreateICmpEQ(b, minus_one));
        guard(builder.CreateNot(overflow), instruction, Fault::DivisionOverflow);
        if (remainder) {
            return builder.CreateSRem(a, b);
        }
        return builder.CreateSDiv(a, b);
    }
    return builder.CreateUDiv(a, b);
}

llvm::Value* Generator::address(const Instruction& instruction, std::size_t base_operand) {
    auto* base = read(instruction.operands[base_operand]);
    auto* index = read(instruction.operands[base_operand + 1]);
    auto* word = builder.CreateUDiv(base, builder.getInt64(8));
    auto* aligned = builder.CreateICmpEQ(builder.CreateAnd(base, builder.getInt64(7)), builder.getInt64(0));
    guard(builder.CreateAnd(aligned, builder.CreateICmpULT(word, builder.getInt64(memory_words))),
          instruction, Fault::MemoryAddress);
    auto* begin = builder.CreateLoad(builder.getInt64Ty(), element(allocation_begin, word));
    auto* end = builder.CreateLoad(builder.getInt64Ty(), element(allocation_end, word));
    guard(builder.CreateICmpUGT(end, word), instruction, Fault::MemoryAddress);
    auto* negative = builder.CreateICmpSLT(index, builder.getInt64(0));
    auto* fits_before = builder.CreateICmpULE(builder.CreateNeg(index), builder.CreateSub(word, begin));
    auto* fits_after = builder.CreateICmpULT(index, builder.CreateSub(end, word));
    guard(builder.CreateSelect(negative, fits_before, fits_after), instruction, Fault::MemoryAddress);
    
    return builder.CreateAdd(word, index);
}

void Generator::allocate(const Instruction& instruction) {
    auto* count = read(instruction.operands[1]);
    auto* begin = builder.CreateLoad(builder.getInt64Ty(), next_word);
    guard(builder.CreateICmpULE(begin, builder.getInt64(memory_words)), instruction, Fault::MemoryExhausted);
    guard(builder.CreateICmpULE(count, builder.CreateSub(builder.getInt64(memory_words), begin)),
          instruction, Fault::MemoryExhausted);
    auto* end = builder.CreateAdd(begin, count);
    auto* predecessor = builder.GetInsertBlock();
    auto* loop = llvm::BasicBlock::Create(module->getContext(), "allocate_words", function);
    auto* finish = llvm::BasicBlock::Create(module->getContext(), "allocated", function);
    builder.CreateBr(loop);
    builder.SetInsertPoint(loop);
    auto* cursor = builder.CreatePHI(builder.getInt64Ty(), 2);
    cursor->addIncoming(begin, predecessor);
    builder.CreateStore(begin, element(allocation_begin, cursor));
    builder.CreateStore(end, element(allocation_end, cursor));
    auto* next = builder.CreateAdd(cursor, builder.getInt64(1));
    cursor->addIncoming(next, loop);
    builder.CreateCondBr(builder.CreateICmpULT(next, end), loop, finish);
    builder.SetInsertPoint(finish);
    builder.CreateStore(builder.CreateAdd(end, builder.CreateAnd(end, builder.getInt64(1))), next_word);
    write(instruction, builder.CreateMul(begin, builder.getInt64(8)));
}

void Generator::ensure_sim() {
    auto* initialize = llvm::BasicBlock::Create(module->getContext(), "initialize_sim", function);
    auto* continuation = llvm::BasicBlock::Create(module->getContext(), "sim_ready", function);
    auto* active = builder.CreateLoad(builder.getInt8Ty(), ready);
    builder.CreateCondBr(builder.CreateICmpEQ(active, builder.getInt8(0)), initialize, continuation);
    builder.SetInsertPoint(initialize);
    builder.CreateCall(module->getOrInsertFunction("simInit", builder.getVoidTy()));
    builder.CreateStore(builder.getInt8(1), ready);
    builder.CreateBr(continuation);
    builder.SetInsertPoint(continuation);
}

void Generator::close_sim() {
    auto* close = llvm::BasicBlock::Create(module->getContext(), "close_sim", function);
    auto* continuation = llvm::BasicBlock::Create(module->getContext(), "sim_closed", function);
    auto* active = builder.CreateLoad(builder.getInt8Ty(), ready);
    builder.CreateCondBr(builder.CreateICmpNE(active, builder.getInt8(0)), close, continuation);
    builder.SetInsertPoint(close);
    builder.CreateCall(module->getOrInsertFunction("simExit", builder.getVoidTy()));
    builder.CreateStore(builder.getInt8(0), ready);
    builder.CreateBr(continuation);
    builder.SetInsertPoint(continuation);
}

void Generator::sim_call(const Instruction& instruction) {
    if (instruction.opcode == Opcode::Exit) {
        close_sim();
        return;
    }
    if (instruction.opcode == Opcode::Putpixel) {
        auto* x = read(instruction.operands[0], 32);
        auto* y = read(instruction.operands[1], 32);
        auto* color = read(instruction.operands[2], 32);
        guard(builder.CreateAnd(builder.CreateICmpULT(x, builder.getInt32(SIM_X_SIZE)),
                                builder.CreateICmpULT(y, builder.getInt32(SIM_Y_SIZE))),
              instruction, Fault::Pixel);
        ensure_sim();
        builder.CreateCall(module->getOrInsertFunction("simPutPixel", builder.getVoidTy(),
                                                       builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()),
                           {x, y, color});
        return;
    }
    ensure_sim();
    switch (instruction.opcode) {
    case Opcode::Init: return;
    case Opcode::Flush:
        builder.CreateCall(module->getOrInsertFunction("simFlush", builder.getVoidTy()));
        return;
    case Opcode::Rand:
        write(instruction, builder.CreateCall(module->getOrInsertFunction("simRand", builder.getInt32Ty())));
        return;
    case Opcode::Hasclick:
        write(instruction, builder.CreateCall(module->getOrInsertFunction("simHasClick", builder.getInt32Ty())));
        return;
    case Opcode::Getclick:
        write(instruction, builder.CreateCall(module->getOrInsertFunction("simGetClick", builder.getInt32Ty())));
        return;
    default: throw std::logic_error("invalid simulator instruction");
    }
}

void Generator::native(const Instruction& instruction) {
    const auto& args = instruction.operands;
    const auto width = instruction.width;
    switch (instruction.opcode) {
    case Opcode::Mov:
        write(instruction, read(args[1]));
        return;
    case Opcode::Add:
        write(instruction, builder.CreateAdd(read(args[1], width), read(args[2], width)));
        return;
    case Opcode::Sub:
        write(instruction, builder.CreateSub(read(args[1], width), read(args[2], width)));
        return;
    case Opcode::Mul:
        write(instruction, builder.CreateMul(read(args[1], width), read(args[2], width)));
        return;
    case Opcode::Or:
        write(instruction, builder.CreateOr(read(args[1], width), read(args[2], width)));
        return;
    case Opcode::Sdiv:
        write(instruction, division(read(args[1], width), read(args[2], width), instruction, true));
        return;
    case Opcode::Udiv:
        write(instruction, division(read(args[1], width), read(args[2], width), instruction, false));
        return;
    case Opcode::Srem:
        write(instruction, division(read(args[1], width), read(args[2], width), instruction, true, true));
        return;
    case Opcode::Mdiv: {
        auto* product = builder.CreateMul(read(args[0], width), read(args[1], width));
        write(instruction, division(product, read(args[2], width), instruction, true));
        return;
    }
    case Opcode::Shl:
    case Opcode::Ashr: {
        auto* value = read(args[1], width);
        auto* count = read(args[2], width);
        guard(builder.CreateICmpULT(count, builder.getIntN(width, width)), instruction, Fault::Shift);
        write(instruction, instruction.opcode == Opcode::Shl ?
              builder.CreateShl(value, count) : builder.CreateAShr(value, count));
        return;
    }
    case Opcode::Sext32:
        write(instruction, builder.CreateSExt(read(args[1], 32), builder.getInt64Ty()));
        return;
    case Opcode::Abs: {
        auto* value = read(args[1]);
        guard(builder.CreateICmpNE(value, builder.getInt64(std::uint64_t{1} << 63)),
              instruction, Fault::AbsOverflow);
        write(instruction, builder.CreateSelect(builder.CreateICmpSLT(value, builder.getInt64(0)),
                                                builder.CreateNeg(value), value));
        return;
    }
    case Opcode::Umax: {
        auto* a = read(args[1]);
        auto* b = read(args[2]);
        write(instruction, builder.CreateSelect(builder.CreateICmpUGT(a, b), a, b));
        return;
    }
    case Opcode::Clamp8: {
        auto* value = read(args[1]);
        auto* nonnegative = builder.CreateSelect(builder.CreateICmpSLT(value, builder.getInt64(0)),
                                                 builder.getInt64(0), value);
        write(instruction, builder.CreateSelect(builder.CreateICmpUGT(nonnegative, builder.getInt64(255)),
                                                builder.getInt64(255), nonnegative));
        return;
    }
    case Opcode::Alloc:
        allocate(instruction);
        return;
    case Opcode::Ld: {
        auto* index = address(instruction, 1);
        auto* is_initialized = builder.CreateLoad(builder.getInt8Ty(), element(initialized, index));
        guard(builder.CreateICmpNE(is_initialized, builder.getInt8(0)), instruction, Fault::Uninitialized);
        write(instruction, builder.CreateLoad(builder.getInt64Ty(), element(memory, index)));
        return;
    }
    case Opcode::St: {
        auto* index = address(instruction, 0);
        builder.CreateStore(read(args[2]), element(memory, index));
        builder.CreateStore(builder.getInt8(1), element(initialized, index));
        return;
    }
    default:
        sim_call(instruction);
        return;
    }
}

std::unique_ptr<llvm::Module> Generator::generate() {
    function = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), false),
                                      llvm::GlobalValue::ExternalLinkage, "sim_entry", *module);
    auto* entry = llvm::BasicBlock::Create(module->getContext(), "entry", function);
    builder.SetInsertPoint(entry);
    registers = external_array("reg_file", builder.getInt64Ty(), register_count);
    runtime = new llvm::GlobalVariable(*module, builder.getPtrTy(), false,
                                       llvm::GlobalValue::ExternalLinkage, nullptr, "runtime_context");
    return_stack = builder.CreateAlloca(llvm::ArrayType::get(builder.getInt64Ty(), return_capacity));
    stack_size = builder.CreateAlloca(builder.getInt64Ty());
    current_line = builder.CreateAlloca(builder.getInt64Ty());
    builder.CreateStore(builder.getInt64(0), stack_size);
    builder.CreateStore(builder.getInt64(0), current_line);
    if (mode == JitMode::NativeIR) {
        memory = external_array("memory", builder.getInt64Ty(), memory_words);
        initialized = external_array("initialized", builder.getInt8Ty(), memory_words);
        allocation_begin = external_array("allocation_begin", builder.getInt64Ty(), memory_words);
        allocation_end = external_array("allocation_end", builder.getInt64Ty(), memory_words);
        next_word = new llvm::GlobalVariable(*module, builder.getInt64Ty(), false,
            llvm::GlobalValue::InternalLinkage, builder.getInt64(2), "next_word");
        ready = new llvm::GlobalVariable(*module, builder.getInt8Ty(), false,
            llvm::GlobalValue::ExternalLinkage, nullptr, "sim_ready");
    }
    for (std::size_t i = 0; i <= program.instructions.size(); ++i) {
        blocks.push_back(llvm::BasicBlock::Create(module->getContext(), "asm_" + std::to_string(i), function));
    }
    done = llvm::BasicBlock::Create(module->getContext(), "done", function);
    builder.CreateBr(blocks.at(program.entry_point));
    for (std::size_t i = 0; i < program.instructions.size(); ++i) {
        builder.SetInsertPoint(blocks[i]);
        builder.CreateStore(builder.getInt64(program.instructions[i].source_line), current_line);
        control(program.instructions[i], i);
    }
    builder.SetInsertPoint(blocks.back());
    fault(builder.CreateLoad(builder.getInt64Ty(), current_line), Fault::Fallthrough);
    builder.SetInsertPoint(done);
    builder.CreateRetVoid();
    if (llvm::verifyModule(*module, &llvm::errs())) {
        throw std::runtime_error("generated LLVM IR failed verification");
    }
    return std::move(module);
}

} // namespace

JitRunner::JitRunner(const Program& program, std::size_t memory_words)
    : machine_(program, memory_words) {}

void JitRunner::report_error(void* context, std::uint64_t line, std::uint64_t code) noexcept {
    auto& runner = *static_cast<JitRunner*>(context);
    try {
        throw ExecutionError(line, fault_message(static_cast<Fault>(code)));
    } catch (...) {
        runner.error_ = std::current_exception();
    }
}

void JitRunner::map_symbols(llvm::ExecutionEngine& engine, llvm::Module& module) {
    const auto map = [&](const char* name, auto pointer) {
        if (auto* function = module.getFunction(name)) {
            engine.addGlobalMapping(function, reinterpret_cast<void*>(pointer));
        }
    };
    map("sim_error", &JitRunner::report_error);
    map("sim_do_mov", &invoke<&Simulator::handle_mov>);
    map("sim_do_add", &invoke<&Simulator::handle_add>);
    map("sim_do_sub", &invoke<&Simulator::handle_sub>);
    map("sim_do_mul", &invoke<&Simulator::handle_mul>);
    map("sim_do_sdiv", &invoke<&Simulator::handle_sdiv>);
    map("sim_do_udiv", &invoke<&Simulator::handle_udiv>);
    map("sim_do_srem", &invoke<&Simulator::handle_srem>);
    map("sim_do_shl", &invoke<&Simulator::handle_shl>);
    map("sim_do_ashr", &invoke<&Simulator::handle_ashr>);
    map("sim_do_or", &invoke<&Simulator::handle_or>);
    map("sim_do_sext32", &invoke<&Simulator::handle_sext32>);
    map("sim_do_abs", &invoke<&Simulator::handle_abs>);
    map("sim_do_umax", &invoke<&Simulator::handle_umax>);
    map("sim_do_clamp8", &invoke<&Simulator::handle_clamp8>);
    map("sim_do_mdiv", &invoke<&Simulator::handle_mdiv>);
    map("sim_do_alloc", &invoke<&Simulator::handle_alloc>);
    map("sim_do_ld", &invoke<&Simulator::handle_ld>);
    map("sim_do_st", &invoke<&Simulator::handle_st>);
    map("sim_do_init", &invoke<&Simulator::handle_init>);
    map("sim_do_exit", &invoke<&Simulator::handle_exit>);
    map("sim_do_flush", &invoke<&Simulator::handle_flush>);
    map("sim_do_putpixel", &invoke<&Simulator::handle_putpixel>);
    map("sim_do_rand", &invoke<&Simulator::handle_rand>);
    map("sim_do_hasclick", &invoke<&Simulator::handle_hasclick>);
    map("sim_do_getclick", &invoke<&Simulator::handle_getclick>);
    map("simInit", &simInit);
    map("simExit", &simExit);
    map("simFlush", &simFlush);
    map("simPutPixel", &simPutPixel);
    map("simRand", &simRand);
    map("simHasClick", &simHasClick);
    map("simGetClick", &simGetClick);
}

void JitRunner::close_runtime() {
    machine_.close_sim();
    if (sim_ready_) {
        simExit();
        sim_ready_ = 0;
    }
}

void JitRunner::run(JitMode mode, const std::filesystem::path& ir_output) {
    if (llvm::InitializeNativeTarget() || llvm::InitializeNativeTargetAsmPrinter()) {
        throw std::runtime_error("cannot initialize LLVM native target");
    }
    machine_.reset();
    error_ = nullptr;
    const auto words = machine_.memory_.size();
    if (mode == JitMode::NativeIR) {
        initialized_.assign(words, 0);
        allocation_begin_.assign(words, 0);
        allocation_end_.assign(words, 0);
    }
    llvm::LLVMContext context;
    auto module = Generator(context, machine_.program_, mode, words).generate();
    if (!ir_output.empty()) {
        std::error_code error;
        llvm::raw_fd_ostream output(ir_output.string(), error, llvm::sys::fs::OF_Text);
        if (error) {
            throw std::runtime_error("cannot write IR: " + error.message());
        }
        module->print(output, nullptr);
        output.flush();
        if (output.has_error()) {
            throw std::runtime_error("failed to write IR");
        }
    }
    auto* module_ptr = module.get();
    std::string engine_error;
    std::unique_ptr<llvm::ExecutionEngine> engine(
        llvm::EngineBuilder(std::move(module)).setEngineKind(llvm::EngineKind::JIT)
            .setErrorStr(&engine_error).create());
    if (!engine) {
        throw std::runtime_error("cannot create Execution Engine: " + engine_error);
    }
    map_symbols(*engine, *module_ptr);
    const auto map_global = [&](const char* name, void* address) {
        if (auto* global = module_ptr->getNamedGlobal(name)) {
            engine->addGlobalMapping(global, address);
        }
    };
    void* runtime_context = this;
    map_global("runtime_context", &runtime_context);
    map_global("reg_file", machine_.registers_.data());
    map_global("memory", machine_.memory_.data());
    map_global("initialized", initialized_.data());
    map_global("allocation_begin", allocation_begin_.data());
    map_global("allocation_end", allocation_end_.data());
    map_global("sim_ready", &sim_ready_);
    engine->finalizeObject();
    const auto entry = engine->getFunctionAddress("sim_entry");
    if (entry == 0) {
        throw std::runtime_error("Execution Engine could not resolve sim_entry");
    }
    reinterpret_cast<void (*)()>(entry)();
    close_runtime();
    if (error_) {
        std::rethrow_exception(error_);
    }
}

} // namespace sim
