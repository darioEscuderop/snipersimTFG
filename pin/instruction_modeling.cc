#include "instruction_modeling.h"
#include "inst_mode_macros.h"
#include "local_storage.h"
#include "spin_loop_detection.h"

#include "simulator.h"
#include "performance_model.h"
#include "core_manager.h"
#include "core.h"
#include "thread.h"
#include "timer.h"
#include "instruction_decoder.h"
#include "instruction.h"
#include "micro_op.h"
#include "magic_client.h"
#include "inst_mode.h"
#include "dvfs_manager.h"
#include "hooks_manager.h"
#include "branch_predictor.h"

#include <unordered_map>

void InstructionModeling::handleBasicBlock(THREADID thread_id, BasicBlock *sim_basic_block)
{
   Core *core = localStore[thread_id].thread->getCore();
   assert(core);
   PerformanceModel *prfmdl = core->getPerformanceModel();

   prfmdl->queueBasicBlock(sim_basic_block);

   #ifndef ENABLE_PERF_MODEL_OWN_THREAD
   prfmdl->iterate();
   #endif
}

static void handleBranch(THREADID thread_id, ADDRINT eip, BOOL taken, ADDRINT target)
{
   Core *core = localStore[thread_id].thread->getCore();
   assert(core);
   PerformanceModel *prfmdl = core->getPerformanceModel();

   DynamicInstructionInfo info = DynamicInstructionInfo::createBranchInfo(eip, taken, target);
   prfmdl->pushDynamicInstructionInfo(info);
}

static void handleBranchWarming(THREADID thread_id, ADDRINT eip, BOOL taken, ADDRINT target)
{
   Core *core = localStore[thread_id].thread->getCore();
   assert(core);
   PerformanceModel *prfmdl = core->getPerformanceModel();
   BranchPredictor *bp = prfmdl->getBranchPredictor();

   if (bp) {
      bool prediction = bp->predict(eip, target);
      bp->update(prediction, taken, eip, target);
   }
}

static VOID handleMagic(THREADID threadIndex, CONTEXT * ctxt, ADDRINT next_eip)
{
   ADDRINT res = handleMagicInstruction(localStore[threadIndex].thread->getId(),
                    PIN_GetContextReg(ctxt, REG_GAX),
                    #ifdef TARGET_IA32
                       PIN_GetContextReg(ctxt, REG_GDX),
                    #else
                       PIN_GetContextReg(ctxt, REG_GBX),
                    #endif
                    PIN_GetContextReg(ctxt, REG_GCX));
   PIN_SetContextReg(ctxt, REG_GAX, res);
   // Forcefully abort the current trace (Redmine #118).
   PIN_SetContextReg(ctxt, REG_INST_PTR, next_eip);
   PIN_ExecuteAt(ctxt);
}

static void handleRdtsc(THREADID thread_id, PIN_REGISTER * gax, PIN_REGISTER * gdx)
{
   Core *core = localStore[thread_id].thread->getCore();
   assert (core);
   SubsecondTime cycles_fs = core->getPerformanceModel()->getElapsedTime();
   // Convert SubsecondTime to cycles in global clock domain
   const ComponentPeriod *dom_global = Sim()->getDvfsManager()->getGlobalDomain();
   UInt64 cycles = SubsecondTime::divideRounded(cycles_fs, *dom_global);
   // Return in eax and edx
   gdx->dword[0] = cycles >> 32;
   gax->dword[0] = cycles & 0xffffffff;
}

static void handlePause()
{
   // Mostly used Inside spinlocks, use it here to increase the probability
   // that another processor/thread will get some functional execution time
   PIN_Yield();
}

static void fillOperandListMemOps(OperandList *list, INS ins)
{
   if (INS_IsMemoryRead (ins) || INS_IsMemoryWrite (ins))
   {
      // first all reads (dyninstrinfo pushed from redirectMemOp)
      for (unsigned int i = 0; i < INS_MemoryOperandCount(ins); i++)
      {
         if (INS_MemoryOperandIsRead(ins, i))
            list->push_back(Operand(Operand::MEMORY, 0, Operand::READ));
      }
      // then all writes (dyninstrinfo pushed from completeMemWrite)
      for (unsigned int i = 0; i < INS_MemoryOperandCount(ins); i++)
      {
         if (INS_MemoryOperandIsWritten(ins, i))
            list->push_back(Operand(Operand::MEMORY, 0, Operand::WRITE));
      }
   }
}

static void fillOperandList(OperandList *list, INS ins)
{
   // memory
   fillOperandListMemOps(list, ins);

   // for handling register operands
   unsigned int max_read_regs = INS_MaxNumRRegs(ins);
   unsigned int max_write_regs = INS_MaxNumWRegs(ins);

   for (unsigned int i = 0; i < max_read_regs; i++)
   {
      REG reg_i = INS_RegR(ins, i);
      if (REG_valid(reg_i))
         list->push_back(Operand(Operand::REG, reg_i, Operand::READ, REG_StringShort(reg_i).c_str(), INS_RegRContain(ins, reg_i)));
   }

   for (unsigned int i = 0; i < max_write_regs; i++)
   {
      REG reg_i = INS_RegW(ins, i);
      if (REG_valid(reg_i))
         list->push_back(Operand(Operand::REG, reg_i, Operand::WRITE, REG_StringShort(reg_i).c_str(), INS_RegWContain(ins, reg_i)));
   }

   // immediate
   for (unsigned int i = 0; i < INS_OperandCount(ins); i++)
   {
      if (INS_OperandIsImmediate(ins, i))
      {
         list->push_back(Operand(Operand::IMMEDIATE, INS_OperandImmediate(ins, i), Operand::READ));
      }
   }
}

std::unordered_map<ADDRINT, const std::vector<const MicroOp *> *> instruction_cache;

BOOL InstructionModeling::addInstructionModeling(TRACE trace, INS ins, BasicBlock *basic_block, InstMode::inst_mode_t inst_mode)
{
   // For all LOCK-prefixed and atomic-update instructions, for timing purposes we add an MFENCE
   // instruction before and after the atomic instruction to force waiting for loads and stores
   // in the timing model
   static Instruction *mfence_instruction = NULL;
   if (! mfence_instruction) {
      OperandList list;
      mfence_instruction = new GenericInstruction(list);
      MicroOp *uop = new MicroOp();
      uop->makeDynamic("MFENCE", 1);
      uop->setMemBarrier(true);
      uop->setFirst(true);
      uop->setLast(true);
      std::vector<const MicroOp*> *uops = new std::vector<const MicroOp*>();
      uops->push_back(uop);
      mfence_instruction->setMicroOps(uops);
   }

   // Functional modeling

   // Simics-style magic instruction: xchg bx, bx
   if (INS_IsXchg(ins) && INS_OperandReg(ins, 0) == REG_BX && INS_OperandReg(ins, 1) == REG_BX)
   {
      INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)handleMagic, IARG_THREAD_ID, IARG_CONTEXT, IARG_FALLTHROUGH_ADDR, IARG_END);
      // Trace will be aborted after MAGIC (Redmine #118), so don't add the subsequent instructions to the basic-block list.
      return false;
   }

   if (INS_IsRDTSC(ins))
      INS_InsertPredicatedCall(ins, IPOINT_AFTER, (AFUNPTR)handleRdtsc, IARG_THREAD_ID, IARG_REG_REFERENCE, REG_GAX, IARG_REG_REFERENCE, REG_GDX, IARG_END);

   if (INS_Opcode(ins) == XED_ICLASS_PAUSE)
      INS_InsertPredicatedCall(ins, IPOINT_AFTER, (AFUNPTR)handlePause, IARG_END);

   if (INS_IsBranch(ins) && INS_HasFallThrough(ins))
   {
      // In warming mode, warm up the branch predictors
      INSTRUMENT_PREDICATED(
         INSTR_IF_CACHEONLY(inst_mode),
         trace, ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)handleBranchWarming,
         IARG_THREAD_ID,
         IARG_ADDRINT, INS_Address(ins),
         IARG_BOOL, TRUE,
         IARG_BRANCH_TARGET_ADDR,
         IARG_END);

      INSTRUMENT_PREDICATED(
         INSTR_IF_CACHEONLY(inst_mode),
         trace, ins, IPOINT_AFTER, (AFUNPTR)handleBranchWarming,
         IARG_THREAD_ID,
         IARG_ADDRINT, INS_Address(ins),
         IARG_BOOL, FALSE,
         IARG_BRANCH_TARGET_ADDR,
         IARG_END);

      // In detailed mode, push a DynamicInstructionInfo
      INSTRUMENT_PREDICATED(
         INSTR_IF_DETAILED(inst_mode),
         trace, ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)handleBranch,
         IARG_THREAD_ID,
         IARG_ADDRINT, INS_Address(ins),
         IARG_BOOL, TRUE,
         IARG_BRANCH_TARGET_ADDR,
         IARG_END);

      INSTRUMENT_PREDICATED(
         INSTR_IF_DETAILED(inst_mode),
         trace, ins, IPOINT_AFTER, (AFUNPTR)handleBranch,
         IARG_THREAD_ID,
         IARG_ADDRINT, INS_Address(ins),
         IARG_BOOL, FALSE,
         IARG_BRANCH_TARGET_ADDR,
         IARG_END);
   }


   if (!basic_block)
      return true;


   // Spin loop detection

   if (Sim()->getConfig()->getEnableSpinLoopDetection())
      addSpinLoopDetection(trace, ins, inst_mode);


   // Timing modeling

   if (INS_IsAtomicUpdate(ins))
      basic_block->push_back(mfence_instruction);

   OperandList list;
   fillOperandList(&list, ins);

   // branches
   if (INS_IsBranch(ins) && INS_HasFallThrough(ins))
   {
      basic_block->push_back(new BranchInstruction(list));
   }

   // Now handle instructions which have a static cost
   else
   {
      switch(INS_Opcode(ins))
      {
      case XED_ICLASS_DIV:
         basic_block->push_back(new ArithInstruction(INST_DIV, list));
         break;
      case XED_ICLASS_MUL:
         basic_block->push_back(new ArithInstruction(INST_MUL, list));
         break;
      case XED_ICLASS_FDIV:
         basic_block->push_back(new ArithInstruction(INST_FDIV, list));
         break;
      case XED_ICLASS_FMUL:
         basic_block->push_back(new ArithInstruction(INST_FMUL, list));
         break;

      default:
         basic_block->push_back(new GenericInstruction(list));
      }
   }

   ADDRINT addr = INS_Address(ins);

   basic_block->back()->setAddress(addr);
   basic_block->back()->setSize(INS_Size(ins));
   basic_block->back()->setAtomic(INS_IsAtomicUpdate(ins));
   basic_block->back()->setDisassembly(INS_Disassemble(ins).c_str());


   const std::vector<const MicroOp *> * inst = NULL;
   if (!Sim()->getConfig()->getEnableSMCSupport()
       && instruction_cache.count(addr) > 0)
   {
      inst = instruction_cache[addr];
   }
   else
   {
      inst = InstructionDecoder::decode(INS_Address(ins), INS_XedDec(ins), basic_block->back());
      if (!Sim()->getConfig()->getEnableSMCSupport())
         instruction_cache[addr] = inst;
   }
   basic_block->back()->setMicroOps(inst);

   if (INS_IsAtomicUpdate(ins))
      basic_block->push_back(mfence_instruction);

   return true;
}

VOID InstructionModeling::countInstructions(THREADID thread_id, ADDRINT address, INT32 count)
{
   Core* core = localStore[thread_id].thread->getCore();
   core->countInstructions(address, count);
}

VOID InstructionModeling::accessInstructionCacheWarmup(THREADID threadid, ADDRINT address, UINT32 size)
{
   Core *core = localStore[threadid].thread->getCore();
   assert(core);
   core->readInstructionMemory(address, size);
}
