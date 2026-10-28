extern "C" {
    #include "../include/sim.h"   
}

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <memory>

using namespace llvm;

static constexpr int64_t N_STARS = 5000;
static constexpr int64_t N_STEPS = 10000;
static constexpr int64_t PI_FIX = 205887; // pi * 65536
static constexpr int64_t K_INV = 39797;   // 1/K for CORDIC
static constexpr int64_t Z_MIN = 16000;
static constexpr int64_t Z_MAX = 100000;
static constexpr int64_t Z_RANGE = Z_MAX - Z_MIN;
static constexpr int64_t DZ = 700;
static constexpr int64_t SWIRL = 6500;
static constexpr int64_t SCALE = SIM_X_SIZE * 55 / 100; // 844
static constexpr int64_t CX = SIM_X_SIZE / 2;           // 768
static constexpr int64_t CY = SIM_Y_SIZE / 2;           // 384

static constexpr uint64_t AtanTableData[16] = {
    51472, 30386, 16055, 8149, 4091, 2047, 1024, 512,
    256,   128,   64,    32,   16,   8,    4,    2};

int main() {
  LLVMContext context;
  Module *module = new Module("app.c", context);
  IRBuilder<> builder(context);

  Type *voidType = Type::getVoidTy(context);
  Type *int32Type = Type::getInt32Ty(context);
  Type *int64Type = Type::getInt64Ty(context);
  Type *ptrType = PointerType::get(context, 0);

  // declare void @simPutPixel(i32, i32, i32)
  FunctionCallee simPutPixelFunc = module->getOrInsertFunction(
      "simPutPixel",
      FunctionType::get(voidType, {int32Type, int32Type, int32Type}, false));

  // declare void @simFlush()
  FunctionCallee simFlushFunc =
      module->getOrInsertFunction("simFlush", FunctionType::get(voidType, false));

  // declare i32 @simRand()
  FunctionCallee simRandFunc =
      module->getOrInsertFunction("simRand", FunctionType::get(int32Type, false));

  // declare i32 @simHasClick()
  FunctionCallee simHasClickFunc = module->getOrInsertFunction(
      "simHasClick", FunctionType::get(int32Type, false));

  // define i64 @colorShift()
  Function *colorShiftFunc = Function::Create(
      FunctionType::get(int64Type, false), Function::ExternalLinkage,
      "colorShift", module);

  // define void @putCross(i64, i64, i64)
  Function *putCrossFunc = Function::Create(
      FunctionType::get(voidType, {int64Type, int64Type, int64Type}, false),
      Function::ExternalLinkage, "putCross", module);

  // define void @setSinCosTables(ptr, ptr, ptr)
  Function *setSinCosFunc = Function::Create(
      FunctionType::get(voidType, {ptrType, ptrType, ptrType}, false),
      Function::ExternalLinkage, "setSinCosTables", module);

  // define void @app()
  Function *appFunc =
      Function::Create(FunctionType::get(voidType, false),
                       Function::ExternalLinkage, "app", module);

  GlobalVariable *atanTable = new GlobalVariable(
      *module, ArrayType::get(int64Type, 16),
      /*isConstant=*/true, GlobalValue::ExternalLinkage,
      ConstantDataArray::get(context, ArrayRef<uint64_t>(AtanTableData, 16)),
      "atan_table");
  atanTable->setAlignment(Align(16));

  // define i64 @colorShift() { return (simRand() % 61) - 40; }
  BasicBlock *csEntry = BasicBlock::Create(context, "", colorShiftFunc);
  builder.SetInsertPoint(csEntry);
  Value *csRand = builder.CreateCall(simRandFunc);                    // %1
  Value *csMod = builder.CreateSRem(csRand, builder.getInt32(61));    // %2
  Value *csShift = builder.CreateNSWAdd(csMod, builder.getInt32(-40)); // %3
  Value *csShift64 = builder.CreateSExt(csShift, int64Type);          // %4
  builder.CreateRet(csShift64);

  // define void @putCross(i64, i64, i64)
  Value *pcX = putCrossFunc->getArg(0);   // %0
  Value *pcY = putCrossFunc->getArg(1);   // %1
  Value *pcArgb = putCrossFunc->getArg(2); // %2

  BasicBlock *pcEntry = BasicBlock::Create(context, "", putCrossFunc);
  BasicBlock *pcBB8 = BasicBlock::Create(context, "", putCrossFunc);
  BasicBlock *pcBB11 = BasicBlock::Create(context, "", putCrossFunc);
  BasicBlock *pcBB13 = BasicBlock::Create(context, "", putCrossFunc);
  BasicBlock *pcBB15 = BasicBlock::Create(context, "", putCrossFunc);
  BasicBlock *pcBB18 = BasicBlock::Create(context, "", putCrossFunc);
  BasicBlock *pcBB20 = BasicBlock::Create(context, "", putCrossFunc);

  builder.SetInsertPoint(pcEntry);
  Value *pcX32 = builder.CreateTrunc(pcX, int32Type);       // %4
  Value *pcY32 = builder.CreateTrunc(pcY, int32Type);       // %5
  Value *pcArgb32 = builder.CreateTrunc(pcArgb, int32Type); // %6
  // simPutPixel(x, y, argb)
  builder.CreateCall(simPutPixelFunc, {pcX32, pcY32, pcArgb32});
  Value *pcRightOk = builder.CreateICmpSLT(pcX, builder.getInt64(SIM_X_SIZE - 1)); // %7
  builder.CreateCondBr(pcRightOk, pcBB8, pcBB11);

  builder.SetInsertPoint(pcBB8);
  Value *pcXR = builder.CreateAdd(pcX32, builder.getInt32(1)); // %9
  builder.CreateCall(simPutPixelFunc, {pcXR, pcY32, pcArgb32});
  Value *pcLeftOk = builder.CreateICmpSGT(pcX, builder.getInt64(0)); // %10
  builder.CreateCondBr(pcLeftOk, pcBB11, pcBB13);

  builder.SetInsertPoint(pcBB11);
  Value *pcXL = builder.CreateAdd(pcX32, builder.getInt32(-1)); // %12
  builder.CreateCall(simPutPixelFunc, {pcXL, pcY32, pcArgb32});
  builder.CreateBr(pcBB13);

  builder.SetInsertPoint(pcBB13);
  Value *pcDownOk = builder.CreateICmpSLT(pcY, builder.getInt64(SIM_Y_SIZE - 1)); // %14
  builder.CreateCondBr(pcDownOk, pcBB15, pcBB18);

  builder.SetInsertPoint(pcBB15);
  Value *pcYD = builder.CreateAdd(pcY32, builder.getInt32(1)); // %16
  builder.CreateCall(simPutPixelFunc, {pcX32, pcYD, pcArgb32});
  Value *pcUpOk = builder.CreateICmpSGT(pcY, builder.getInt64(0)); // %17
  builder.CreateCondBr(pcUpOk, pcBB18, pcBB20);

  builder.SetInsertPoint(pcBB18);
  Value *pcYU = builder.CreateAdd(pcY32, builder.getInt32(-1)); // %19
  builder.CreateCall(simPutPixelFunc, {pcX32, pcYU, pcArgb32});
  builder.CreateBr(pcBB20);

  builder.SetInsertPoint(pcBB20);
  builder.CreateRetVoid();

  // define void @setSinCosTables(ptr %sinTable, ptr %cosTable, ptr %atan)
  Value *scSinTable = setSinCosFunc->getArg(0);  // %0
  Value *scCosTable = setSinCosFunc->getArg(1);  // %1
  Value *scAtanTable = setSinCosFunc->getArg(2); // %2

  BasicBlock *scEntry = BasicBlock::Create(context, "", setSinCosFunc);
  BasicBlock *scBB4 = BasicBlock::Create(context, "", setSinCosFunc);
  BasicBlock *scBB5 = BasicBlock::Create(context, "", setSinCosFunc);
  BasicBlock *scBB13 = BasicBlock::Create(context, "", setSinCosFunc);
  BasicBlock *scBB23 = BasicBlock::Create(context, "", setSinCosFunc);
  BasicBlock *scBB27 = BasicBlock::Create(context, "", setSinCosFunc);
  BasicBlock *scBB31 = BasicBlock::Create(context, "", setSinCosFunc);
  BasicBlock *scBB37 = BasicBlock::Create(context, "", setSinCosFunc);

  builder.SetInsertPoint(scEntry);
  builder.CreateBr(scBB5);

  builder.SetInsertPoint(scBB4);
  builder.CreateRetVoid();

  builder.SetInsertPoint(scBB5);
  PHINode *scI = builder.CreatePHI(int64Type, 2);                            // %6
  Value *scI32 = builder.CreateTrunc(scI, int32Type);                        // %7
  Value *scZq = builder.CreateMul(scI32, builder.getInt32(Z_RANGE), "", true, true);  // %8
  Value *scZq2 = builder.CreateUDiv(scZq, builder.getInt32(255));            // %9
  Value *scZ = builder.CreateAdd(scZq2, builder.getInt32(Z_MIN), "", true, true);     // %10
  Value *scT = builder.CreateUDiv(builder.getInt32(SWIRL << 16), scZ);       // %11
  Value *scT64 = builder.CreateZExt(scT, int64Type);                         // %12
  builder.CreateBr(scBB13);

  builder.SetInsertPoint(scBB13);
  PHINode *scJ = builder.CreatePHI(int64Type, 2);       // %14
  PHINode *scZcPhi = builder.CreatePHI(int64Type, 2);   // %15
  PHINode *scYPhi = builder.CreatePHI(int64Type, 2);    // %16
  PHINode *scXPhi = builder.CreatePHI(int64Type, 2);    // %17
  Value *scZcGe0 = builder.CreateICmpSGT(scZcPhi, builder.getInt64(-1)); // %18
  Value *scYShift = builder.CreateAShr(scYPhi, scJ);    // %19 (y >> j)
  Value *scXShift = builder.CreateAShr(scXPhi, scJ);    // %20 (x >> j)
  Value *scAtanPtr = builder.CreateInBoundsGEP(int64Type, scAtanTable, scJ); // %21
  Value *scAtanVal = builder.CreateLoad(int64Type, scAtanPtr);               // %22
  builder.CreateCondBr(scZcGe0, scBB23, scBB27);

  builder.SetInsertPoint(scBB23); // zc >= 0
  Value *scXnPos = builder.CreateNSWSub(scXPhi, scYShift);  // %24 x - (y >> j)
  Value *scYnPos = builder.CreateNSWAdd(scXShift, scYPhi);  // %25 (x >> j) + y
  Value *scZcPos = builder.CreateNSWSub(scZcPhi, scAtanVal); // %26
  builder.CreateBr(scBB31);

  builder.SetInsertPoint(scBB27); // zc < 0
  Value *scXnNeg = builder.CreateNSWAdd(scYShift, scXPhi);  // %28 (y >> j) + x
  Value *scYnNeg = builder.CreateNSWSub(scYPhi, scXShift);  // %29
  Value *scZcNeg = builder.CreateNSWAdd(scAtanVal, scZcPhi); // %30
  builder.CreateBr(scBB31);

  builder.SetInsertPoint(scBB31);
  PHINode *scZcNew = builder.CreatePHI(int64Type, 2); // %32
  PHINode *scXNew = builder.CreatePHI(int64Type, 2);  // %33
  PHINode *scYNew = builder.CreatePHI(int64Type, 2);  // %34
  Value *scJNext = builder.CreateAdd(scJ, builder.getInt64(1), "", true, true); // %35
  Value *scJDone = builder.CreateICmpEQ(scJNext, builder.getInt64(16));         // %36
  builder.CreateCondBr(scJDone, scBB37, scBB13);

  builder.SetInsertPoint(scBB37);
  Value *scCosPtr = builder.CreateInBoundsGEP(int64Type, scCosTable, scI); // %38
  builder.CreateStore(scXNew, scCosPtr); // cos_table[i] = x
  Value *scSinPtr = builder.CreateInBoundsGEP(int64Type, scSinTable, scI); // %39
  builder.CreateStore(scYNew, scSinPtr); // sin_table[i] = y
  Value *scINext = builder.CreateAdd(scI, builder.getInt64(1), "", true, true); // %40
  Value *scIDone = builder.CreateICmpEQ(scINext, builder.getInt64(256));        // %41
  builder.CreateCondBr(scIDone, scBB4, scBB5);

  scI->addIncoming(builder.getInt64(0), scEntry);
  scI->addIncoming(scINext, scBB37);
  scJ->addIncoming(builder.getInt64(0), scBB5);
  scJ->addIncoming(scJNext, scBB31);
  scZcPhi->addIncoming(scT64, scBB5);
  scZcPhi->addIncoming(scZcNew, scBB31);
  scYPhi->addIncoming(builder.getInt64(0), scBB5);
  scYPhi->addIncoming(scYNew, scBB31);
  scXPhi->addIncoming(builder.getInt64(K_INV), scBB5);
  scXPhi->addIncoming(scXNew, scBB31);
  scZcNew->addIncoming(scZcPos, scBB23);
  scZcNew->addIncoming(scZcNeg, scBB27);
  scXNew->addIncoming(scXnPos, scBB23);
  scXNew->addIncoming(scXnNeg, scBB27);
  scYNew->addIncoming(scYnPos, scBB23);
  scYNew->addIncoming(scYnNeg, scBB27);

  // define void @app() {
  BasicBlock *appEntry = BasicBlock::Create(context, "", appFunc);
  BasicBlock *appBB9 = BasicBlock::Create(context, "", appFunc);
  BasicBlock *appBB46 = BasicBlock::Create(context, "", appFunc);
  BasicBlock *appBB50 = BasicBlock::Create(context, "", appFunc);
  BasicBlock *appBB51 = BasicBlock::Create(context, "", appFunc);
  BasicBlock *appBB54 = BasicBlock::Create(context, "", appFunc);
  BasicBlock *appBB140 = BasicBlock::Create(context, "", appFunc);
  BasicBlock *appBB143 = BasicBlock::Create(context, "", appFunc);
  BasicBlock *appBB154 = BasicBlock::Create(context, "", appFunc);
  BasicBlock *appBB155 = BasicBlock::Create(context, "", appFunc);
  BasicBlock *appBB158 = BasicBlock::Create(context, "", appFunc);
  BasicBlock *appBB171 = BasicBlock::Create(context, "", appFunc);
  BasicBlock *appBB174 = BasicBlock::Create(context, "", appFunc);

  // Clamp helper: smax(v, 0) then umin(v, 255)
  auto clamp0_255 = [&](Value *v) -> Value * {
    Value *lo = builder.CreateBinaryIntrinsic(Intrinsic::smax, v,
                                              builder.getInt64(0));
    return builder.CreateBinaryIntrinsic(Intrinsic::umin, lo,
                                         builder.getInt64(255));
  };

  auto randCoordinate = [&]() -> Value * {
    Value *rnd = builder.CreateCall(simRandFunc);
    Value *mod = builder.CreateSRem(rnd, builder.getInt32(2001));
    Value *shl = builder.CreateShl(mod, builder.getInt32(16), "", false, true);
    Value *add = builder.CreateNSWAdd(shl, builder.getInt32(-65536000));
    Value *div = builder.CreateSDiv(add, builder.getInt32(1000));
    return builder.CreateSExt(div, int64Type);
  };

  builder.SetInsertPoint(appEntry);
  AllocaInst *sxAlloca = builder.CreateAlloca(ArrayType::get(int64Type, N_STARS));
  sxAlloca->setAlignment(Align(16));
  AllocaInst *syAlloca = builder.CreateAlloca(ArrayType::get(int64Type, N_STARS));
  syAlloca->setAlignment(Align(16));
  AllocaInst *szAlloca = builder.CreateAlloca(ArrayType::get(int64Type, N_STARS));
  szAlloca->setAlignment(Align(16));
  AllocaInst *crAlloca = builder.CreateAlloca(ArrayType::get(int64Type, N_STARS));
  crAlloca->setAlignment(Align(16));
  AllocaInst *cgAlloca = builder.CreateAlloca(ArrayType::get(int64Type, N_STARS));
  cgAlloca->setAlignment(Align(16));
  AllocaInst *cbAlloca = builder.CreateAlloca(ArrayType::get(int64Type, N_STARS));
  cbAlloca->setAlignment(Align(16));
  AllocaInst *cosAlloca = builder.CreateAlloca(ArrayType::get(int64Type, 256));
  cosAlloca->setAlignment(Align(16));
  AllocaInst *sinAlloca = builder.CreateAlloca(ArrayType::get(int64Type, 256));
  sinAlloca->setAlignment(Align(16));

  // setSinCosTables(sin_table, cos_table, atan_table)
  builder.CreateCall(setSinCosFunc, {sinAlloca, cosAlloca, atanTable});
  builder.CreateBr(appBB9);

  // %9: for (long i = 0; i < N_STARS; ++i) - stars init
  builder.SetInsertPoint(appBB9);
  PHINode *appI = builder.CreatePHI(int64Type, 2); // %10
  // sx[i] = (simRand() % 2001 - 1000) * 65536 / 1000
  Value *appSxInit = randCoordinate();
  Value *appSxPtrInit = builder.CreateInBoundsGEP(int64Type, sxAlloca, appI);
  builder.CreateStore(appSxInit, appSxPtrInit);
  // sy[i] = (simRand() % 2001 - 1000) * 65536 / 1000
  Value *appSyInit = randCoordinate();
  Value *appSyPtrInit = builder.CreateInBoundsGEP(int64Type, syAlloca, appI);
  builder.CreateStore(appSyInit, appSyPtrInit);
  // sz[i] = Z_MIN + (simRand() % 10000) * z_range / 10000
  Value *appSzRnd = builder.CreateCall(simRandFunc);
  Value *appSzMod = builder.CreateSRem(appSzRnd, builder.getInt32(10000));
  Value *appSzMul = builder.CreateNSWMul(appSzMod, builder.getInt32(Z_RANGE));
  Value *appSzDiv = builder.CreateSDiv(appSzMul, builder.getInt32(10000));
  Value *appSzAdd = builder.CreateNSWAdd(appSzDiv, builder.getInt32(Z_MIN));
  Value *appSzInit = builder.CreateSExt(appSzAdd, int64Type);
  Value *appSzPtrInit = builder.CreateInBoundsGEP(int64Type, szAlloca, appI);
  builder.CreateStore(appSzInit, appSzPtrInit);
  // cr[i] = colorShift()
  Value *appCrInit = builder.CreateCall(colorShiftFunc);
  Value *appCrPtrInit = builder.CreateInBoundsGEP(int64Type, crAlloca, appI);
  builder.CreateStore(appCrInit, appCrPtrInit);
  // cb[i] = -shift
  Value *appCbInit = builder.CreateNSWSub(builder.getInt64(0), appCrInit);
  Value *appCbPtrInit = builder.CreateInBoundsGEP(int64Type, cbAlloca, appI);
  builder.CreateStore(appCbInit, appCbPtrInit);
  // cg[i] = colorShift()
  Value *appCgInit = builder.CreateCall(colorShiftFunc);
  Value *appCgPtrInit = builder.CreateInBoundsGEP(int64Type, cgAlloca, appI);
  builder.CreateStore(appCgInit, appCgPtrInit);
  // ++i; if (i == N_STARS) ...
  Value *appINext = builder.CreateAdd(appI, builder.getInt64(1), "", true, true);
  Value *appIDone = builder.CreateICmpEQ(appINext, builder.getInt64(N_STARS));
  builder.CreateCondBr(appIDone, appBB46, appBB9);

  // %46: for (long step = 0; step < N_STEPS; ++step)
  builder.SetInsertPoint(appBB46);
  PHINode *appStep = builder.CreatePHI(int64Type, 2); // %47
  // if (simHasClick()) { simFlush(); return; }
  Value *appClick = builder.CreateCall(simHasClickFunc);
  Value *appNoClick = builder.CreateICmpEQ(appClick, builder.getInt32(0));
  builder.CreateCondBr(appNoClick, appBB54, appBB50);

  // %50: simFlush(); return;
  builder.SetInsertPoint(appBB50);
  builder.CreateCall(simFlushFunc);
  builder.CreateBr(appBB174);

  // %51: simFlush(); ++step; if (step == N_STEPS) -> exit
  builder.SetInsertPoint(appBB51);
  builder.CreateCall(simFlushFunc);
  Value *appStepNext = builder.CreateAdd(appStep, builder.getInt64(1), "", true, true);
  Value *appStepDone = builder.CreateICmpEQ(appStepNext, builder.getInt64(N_STEPS));
  builder.CreateCondBr(appStepDone, appBB174, appBB46);

  // %54: for (long i = 0; i < N_STARS; ++i) - one star motion
  builder.SetInsertPoint(appBB54);
  PHINode *appStar = builder.CreatePHI(int64Type, 2); // %55
  Value *appZPtr = builder.CreateInBoundsGEP(int64Type, szAlloca, appStar); // %56
  Value *appZ = builder.CreateLoad(int64Type, appZPtr);                     // %57 = z
  Value *appZp = builder.CreateNSWAdd(appZ, builder.getInt64(DZ * 6));      // %58 = zp
  Value *appZ255 = builder.CreateMul(appZ, builder.getInt64(255));          // %59
  // idx = clamp(((z - Z_MIN) * 255) / z_range)
  Value *appIdxRaw = builder.CreateAdd(appZ255, builder.getInt64(-Z_MIN * 255));
  Value *appIdxPre = builder.CreateSDiv(appIdxRaw, builder.getInt64(Z_RANGE));
  Value *appIdx = clamp0_255(appIdxPre);
  Value *appCosPtr = builder.CreateInBoundsGEP(int64Type, cosAlloca, appIdx);
  Value *appCosVal = builder.CreateLoad(int64Type, appCosPtr); // cos_table[idx]
  Value *appSinPtr = builder.CreateInBoundsGEP(int64Type, sinAlloca, appIdx);
  Value *appSinVal = builder.CreateLoad(int64Type, appSinPtr); // sin_table[idx]
  // idxp = clamp(((zp - Z_MIN) * 255) / z_range)
  Value *appIdxpRaw = builder.CreateAdd(appZ255, builder.getInt64(-(Z_MIN - DZ * 6) * 255));
  Value *appIdxpPre = builder.CreateSDiv(appIdxpRaw, builder.getInt64(Z_RANGE));
  Value *appIdxp = clamp0_255(appIdxpPre);
  Value *appCosPtrP = builder.CreateInBoundsGEP(int64Type, cosAlloca, appIdxp);
  Value *appCosValP = builder.CreateLoad(int64Type, appCosPtrP); // cos_table[idxp]
  Value *appSinPtrP = builder.CreateInBoundsGEP(int64Type, sinAlloca, appIdxp);
  Value *appSinValP = builder.CreateLoad(int64Type, appSinPtrP); // sin_table[idxp]
  // Rotate (multiply to the rotation matrix)
  Value *appSxPtr = builder.CreateInBoundsGEP(int64Type, sxAlloca, appStar);
  Value *appSx = builder.CreateLoad(int64Type, appSxPtr);
  Value *appSxCos = builder.CreateNSWMul(appSx, appCosVal);         // sx * cos
  Value *appSyPtr = builder.CreateInBoundsGEP(int64Type, syAlloca, appStar);
  Value *appSy = builder.CreateLoad(int64Type, appSyPtr);
  Value *appSySin = builder.CreateNSWMul(appSy, appSinVal);         // sy * sin
  Value *appRot1 = builder.CreateNSWSub(appSxCos, appSySin);
  Value *appRx = builder.CreateAShr(appRot1, builder.getInt64(16)); // rx
  Value *appSxSin = builder.CreateNSWMul(appSx, appSinVal);         // sx * sin
  Value *appSyCos = builder.CreateNSWMul(appSy, appCosVal);         // sy * cos
  Value *appRot2 = builder.CreateNSWAdd(appSyCos, appSxSin);
  Value *appRy = builder.CreateAShr(appRot2, builder.getInt64(16)); // ry
  Value *appSxCosP = builder.CreateNSWMul(appSx, appCosValP);       // sx * cos_p
  Value *appSySinP = builder.CreateNSWMul(appSy, appSinValP);       // sy * sin_p
  Value *appRot3 = builder.CreateNSWSub(appSxCosP, appSySinP);
  Value *appRxp = builder.CreateAShr(appRot3, builder.getInt64(16)); // rxp
  Value *appSxSinP = builder.CreateNSWMul(appSx, appSinValP);        // sx * sin_p
  Value *appSyCosP = builder.CreateNSWMul(appSy, appCosValP);        // sy * cos_p
  Value *appRot4 = builder.CreateNSWAdd(appSyCosP, appSxSinP);
  Value *appRyp = builder.CreateAShr(appRot4, builder.getInt64(16)); // ryp

  Value *appPx0 = builder.CreateNSWMul(appRx, builder.getInt64(SCALE));
  Value *appX0 = builder.CreateSDiv(appPx0, appZ);
  Value *appPy0 = builder.CreateNSWMul(appRy, builder.getInt64(SCALE));
  Value *appY0 = builder.CreateSDiv(appPy0, appZ);
  Value *appPx1 = builder.CreateNSWMul(appRxp, builder.getInt64(SCALE));
  Value *appX1 = builder.CreateSDiv(appPx1, appZp);
  Value *appX1c = builder.CreateNSWAdd(appX1, builder.getInt64(CX));
  Value *appPy1 = builder.CreateNSWMul(appRyp, builder.getInt64(SCALE));
  Value *appY1 = builder.CreateSDiv(appPy1, appZp);
  Value *appY1c = builder.CreateNSWAdd(appY1, builder.getInt64(CY));
  // brightness: b = clamp(((Z_MAX - z) * 255) / z_range)
  Value *appZInv = builder.CreateNSWSub(builder.getInt64(Z_MAX), appZ);
  Value *appB255 = builder.CreateNSWMul(appZInv, builder.getInt64(255));
  Value *appBPre = builder.CreateSDiv(appB255, builder.getInt64(Z_RANGE));
  Value *appB = clamp0_255(appBPre);
  // Set the violet color R = b, G = b/3, B = b (with random shifts)
  Value *appCrPtr = builder.CreateInBoundsGEP(int64Type, crAlloca, appStar);
  Value *appCrVal = builder.CreateLoad(int64Type, appCrPtr);
  Value *appR0 = builder.CreateNSWAdd(appCrVal, appB);
  Value *appB3 = builder.CreateUDiv(appB, builder.getInt64(3));
  Value *appCgPtr = builder.CreateInBoundsGEP(int64Type, cgAlloca, appStar);
  Value *appCgVal = builder.CreateLoad(int64Type, appCgPtr);
  Value *appG0 = builder.CreateNSWAdd(appCgVal, appB3);
  Value *appCbPtr = builder.CreateInBoundsGEP(int64Type, cbAlloca, appStar);
  Value *appCbVal = builder.CreateLoad(int64Type, appCbPtr);
  Value *appBl0 = builder.CreateNSWAdd(appCbVal, appB);
  Value *appR = clamp0_255(appR0);
  Value *appG = clamp0_255(appG0);
  Value *appBl = clamp0_255(appBl0);
  // argb = 0xFF000000 | (r << 16) | (g << 8) | bl
  Value *appR16 = builder.CreateShl(appR, builder.getInt64(16), "", true, true);
  Value *appG8 = builder.CreateShl(appG, builder.getInt64(8), "", true, true);
  Value *appRgb = builder.CreateOr(appG8, appR16);
  appRgb = builder.CreateOr(appRgb, appBl);
  Value *appArgb = builder.CreateOr(appRgb, builder.getInt64(0xFF000000));
  // Line from (x1, y1) to (x0, y0): dx = x0' - x1', dy = y0' - y1'
  Value *appDx = builder.CreateNSWSub(appX0, appX1);
  Value *appDy = builder.CreateNSWSub(appY0, appY1);
  Value *appAdx = builder.CreateBinaryIntrinsic(Intrinsic::abs, appDx, builder.getTrue());
  Value *appAdy = builder.CreateBinaryIntrinsic(Intrinsic::abs, appDy, builder.getTrue());
  Value *appLenMax = builder.CreateBinaryIntrinsic(Intrinsic::umax, appAdx, appAdy);
  Value *appLen = builder.CreateBinaryIntrinsic(Intrinsic::umax, appLenMax, builder.getInt64(1));
  builder.CreateBr(appBB143);

  builder.SetInsertPoint(appBB143);
  PHINode *appK = builder.CreatePHI(int64Type, 2); // %144
  Value *appKdx = builder.CreateNSWMul(appK, appDx);
  Value *appKdxL = builder.CreateSDiv(appKdx, appLen);
  Value *appX = builder.CreateNSWAdd(appX1c, appKdxL); // x = x1 + dx * k / len
  Value *appKdy = builder.CreateNSWMul(appK, appDy);
  Value *appKdyL = builder.CreateSDiv(appKdy, appLen);
  Value *appY = builder.CreateNSWAdd(appY1c, appKdyL); // y = y1 + dy * k / len
  // if (x < SIM_X_SIZE && y < SIM_Y_SIZE) putCross(x, y, argb);
  Value *appXOk = builder.CreateICmpULT(appX, builder.getInt64(SIM_X_SIZE));
  Value *appYOk = builder.CreateICmpULT(appY, builder.getInt64(SIM_Y_SIZE));
  Value *appInScreen = builder.CreateSelect(appXOk, appYOk, builder.getFalse());
  builder.CreateCondBr(appInScreen, appBB154, appBB155);

  builder.SetInsertPoint(appBB154);
  builder.CreateCall(putCrossFunc, {appX, appY, appArgb});
  builder.CreateBr(appBB155);

  builder.SetInsertPoint(appBB155);

  Value *appKNext = builder.CreateAdd(appK, builder.getInt64(1), "", true, false);
  Value *appKDone = builder.CreateICmpEQ(appK, appLen);
  builder.CreateCondBr(appKDone, appBB140, appBB143);

  // %140: sz[i] -= DZ; if (sz[i] < Z_MIN + DZ) respawn the star;
  builder.SetInsertPoint(appBB140);
  Value *appZNew = builder.CreateNSWAdd(appZ, builder.getInt64(-DZ));
  builder.CreateStore(appZNew, appZPtr);
  Value *appZLow = builder.CreateICmpSLT(appZ, builder.getInt64(Z_MIN + DZ));
  builder.CreateCondBr(appZLow, appBB158, appBB171);

  // %158: respawn: new sx, sy = (simRand() % 2001 - 1000) * 65536 / 1000, sz = Z_MAX
  builder.SetInsertPoint(appBB158);
  builder.CreateStore(randCoordinate(), appSxPtr);
  builder.CreateStore(randCoordinate(), appSyPtr);
  builder.CreateStore(builder.getInt64(Z_MAX), appZPtr);
  builder.CreateBr(appBB171);

  // %171: ++i; if (i == N_STARS) -> next step
  builder.SetInsertPoint(appBB171);
  Value *appStarNext = builder.CreateAdd(appStar, builder.getInt64(1), "", true, true);
  Value *appStarDone = builder.CreateICmpEQ(appStarNext, builder.getInt64(N_STARS));
  builder.CreateCondBr(appStarDone, appBB51, appBB54);

  builder.SetInsertPoint(appBB174);
  builder.CreateRetVoid();

  appI->addIncoming(builder.getInt64(0), appEntry);
  appI->addIncoming(appINext, appBB9);
  appStep->addIncoming(builder.getInt64(0), appBB9);
  appStep->addIncoming(appStepNext, appBB51);
  appStar->addIncoming(builder.getInt64(0), appBB46);
  appStar->addIncoming(appStarNext, appBB171);
  appK->addIncoming(builder.getInt64(0), appBB54);
  appK->addIncoming(appKNext, appBB155);

  // Dump LLVM IR
  module->print(outs(), nullptr);
  outs() << '\n';
  bool verif = verifyModule(*module, &outs());
  outs() << "[VERIFICATION] " << (verif ? "FAIL\n\n" : "OK\n\n");
  outs().flush();

  outs() << "[EE] Run\n";
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();

  ExecutionEngine *ee =
      EngineBuilder(std::unique_ptr<Module>(module)).create();
  if (!ee) {
    errs() << "[EE] Failed to create ExecutionEngine\n";
    return EXIT_FAILURE;
  }
  ee->InstallLazyFunctionCreator([](const std::string &fnName) -> void * {
    if (fnName == "simPutPixel") {
      return reinterpret_cast<void *>(simPutPixel);
    }
    if (fnName == "simFlush") {
      return reinterpret_cast<void *>(simFlush);
    }
    if (fnName == "simRand") {
      return reinterpret_cast<void *>(simRand);
    }
    if (fnName == "simHasClick") {
      return reinterpret_cast<void *>(simHasClick);
    }
    outs() << "[ExecutionEngine] Can't find function " << fnName
           << ". Catch the Segmentation fault:)\n";
    return nullptr;
  });
  ee->finalizeObject();

  simInit();

  ArrayRef<GenericValue> noargs;
  GenericValue v = ee->runFunction(appFunc, noargs);
  outs() << "[EE] Result: " << v.IntVal << '\n';

  simExit();
  return EXIT_SUCCESS;
}
