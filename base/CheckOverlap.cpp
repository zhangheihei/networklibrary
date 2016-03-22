#include "ClangSACheckers.h"
#include "InterCheckerAPI.h"
#include "clang/Basic/CharInfo.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/raw_ostream.h"


using namespace clang;
using namespace ento;

namespace{
class CheckOverlap:public Checker<eval::Call>{
	mutable std::unique_ptr<BugType> BT_Overlap,BT_Bounds,BT_Null;

	mutable const char *CurrentFunctionDescription;
public:
	//检查点
	bool evalCall(const CallExpr *CE, CheckerContext &C) const;
	//方法1
	void evalMemcpy(CheckerContext &C, const CallExpr *CE) const;
	//方法2
	void evalCopyCommon(CheckerContext &C, const CallExpr *CE,
				                      ProgramStateRef state,
				                      const Expr *Size,
				                      const Expr *Source,
				                      const Expr *Dest,
				                      bool Restricted = false,
				                      bool IsMempcpy = false) const;
	//方法3
	std::pair<ProgramStateRef , ProgramStateRef >
		  static assumeZero(CheckerContext &C, ProgramStateRef state, SVal V, QualType Ty);
	//方法4
	ProgramStateRef checkNonNull(CheckerContext &C, ProgramStateRef state, const Expr *S, SVal I) const;
	//方法5
	ProgramStateRef CheckLocation(CheckerContext &C,
				                   ProgramStateRef state,
			 	                   const Expr *S,
			 	                   SVal l,
			 					   const char *message = nullptr) const;


	//方法6
	 ProgramStateRef CheckBufferAccess(CheckerContext &C,
			 	                       ProgramStateRef state,
			 	                       const Expr *Size,
			 	                       const Expr *FirstBuf,
			 	                       const Expr *SecondBuf,
			 	                       const char *firstMessage = nullptr,
			 	                       const char *secondMessage = nullptr,
			 	                       bool WarnAboutSize = false) const;
	//方法7
	 ProgramStateRef CheckOverlap1(CheckerContext &C,
			 	                   ProgramStateRef state,
			 	                   const Expr *Size,
			 	                   const Expr *First,
			 	                   const Expr *Second) const;
	//方法8
	 void emitOverlapBug(CheckerContext &C,
			 	         ProgramStateRef state,
			 	         const Stmt *First,
			 	         const Stmt *Second) const;

};

} //end of namespacei

std::pair<ProgramStateRef, ProgramStateRef> CheckOverlap::assumeZero(CheckerContext &C, ProgramStateRef state, SVal V, QualType Ty)
{
	Optional<DefinedSVal> val = V.getAs<DefinedSVal>();
	
	if (!val)
		return std::pair<ProgramStateRef, ProgramStateRef>(state, state);

	SValBuilder &svalBuilder = C.getSValBuilder();
	DefinedOrUnknownSVal zero = svalBuilder.makeZeroVal(Ty);
	return state->assume(svalBuilder.evalEQ(state, *val, zero));

}


ProgramStateRef CheckOverlap::checkNonNull(CheckerContext &C, ProgramStateRef state, const Expr*S, SVal I) const
{
	//如果上一个检查点失败
	if (!state)
		return nullptr;
cout << "@@@@@@@@@@@@@@@@@@@@进去checkNonNull@@@@@@@@@@@@@@@@@@@@@@@@@" <<endl;

	ProgramStateRef stateNull, stateNonNull;
	std::tie(stateNull, stateNonNull) = assumeZero(C, state, I, S->getType());
	
	if (stateNull && !stateNonNull){
		ExplodedNode *N = C.addTransition(stateNull);
		if (!N)
			return nullptr;

		if (!BT_Null)
			BT_Null.reset(new BuiltinBug(this, "    ", "Null Pointer argument in call to byte string  function"));
		SmallString<80> buf;
		llvm::raw_svector_ostream os(buf);
		os << "Null pointer argument in call to" << "memory copy function";

		//为bug生成一个报告
		BuiltinBug *BT = static_cast<BuiltinBug*>(BT_Null.get());
		BugReport* report = new BugReport(*BT, os.str(), N);
		report->addRange(S->getSourceRange());
		bugreporter::trackNullOrUndefValue(N, S, *report);
		C.emitReport(report);
		return nullptr;

	}
	assert(stateNonNull);
	return stateNonNull;
}

ProgramStateRef CheckOverlap::CheckLocation(CheckerContext &C, 
											ProgramStateRef state, 
											const Expr *S, SVal I, 
											const char*warningMsg) const{
	//如果之前的check失败了，退出
	if(!state)
		return nullptr;
	//检查内存是否越界
	const MemRegion *R = I.getAsRegion();
	if (!R)
		return state;
	
	const ElementRegion* ER = dyn_cast<ElementRegion>(R);

	if(!ER)
		return state;
	assert(ER->getValueType() == C.getASTContext().CharTy && "CheckLocation should only be called with char* ElementRegions");
	
	//得到内存长度
	const SubRegion *superReg = cast<SubRegion>(ER->getSuperRegion());
	SValBuilder &svalBuilder = C.getSValBuilder();
	SVal Extent = svalBuilder.convertToArrayIndex(superReg->getExtent(svalBuilder));
	DefinedOrUnknownSVal Size = Extent.castAs<DefinedOrUnknownSVal>();

	//得到index
	DefinedOrUnknownSVal Idx = ER->getIndex().castAs<DefinedOrUnknownSVal>();

	ProgramStateRef StInBound = state->assumeInBound(Idx, Size, true);
	ProgramStateRef StOutBound = state->assumeInBound(Idx, Size, false);

	if (StOutBound && !StInBound){
		ExplodedNode *N = C.addTransition(state);
		if (!N)
			return nullptr;

	if (!BT_Bounds) {
		BT_Bounds.reset(new BuiltinBug(this, "Out-of-bound arry access", "Byte string function accesses out-of-bound array element"));
	}
	 BuiltinBug *BT = static_cast<BuiltinBug*>(BT_Bounds.get());

	 //为bug产生报告
	 BugReport *report = new BugReport(*BT, warningMsg, N);
	 report->addRange(S->getSourceRange());
	 C.emitReport(report);
	 return nullptr;
	}

	return StInBound;
}

ProgramStateRef CheckOverlap::CheckBufferAccess(CheckerContext &C,
												ProgramStateRef state,
												const Expr *Size,
												const Expr *FirstBuf,
												const Expr *SecondBuf,
												const char *firstMessage,
												const char *secondMessage,
												bool WarnAboutSize) const
{
	if (!state)
		return nullptr;
	cout << "@@@@@@@@@@@@@@@@@@@@进入CheckBufferAccess@@@@@@@@@@@@@@@@@@@"<< endl;

	SValBuilder &svalBuilder = C.getSValBuilder();
	ASTContext &Ctx = svalBuilder.getContext();
	const LocationContext*LCtx = C.getLocationContext();

	QualType sizeTy = Size->getType();
	QualType PtrTy = Ctx.getPointerType(Ctx.CharTy);

	//检查第一个内存区域是否为空
	SVal BufVal = state->getSVal(FirstBuf, LCtx);
	state = checkNonNull(C, state, FirstBuf, BufVal);
	if (!state)
		return nullptr;
	
	//得到第一块内存区域的长度
	 SVal LengthVal = state->getSVal(Size, LCtx);
	 Optional<NonLoc> Length = LengthVal.getAs<NonLoc>();
	  if (!Length)
		 return state;
	//构造一个size-1的偏移值
	 NonLoc One = svalBuilder.makeIntVal(1, sizeTy).castAs<NonLoc>();
	 NonLoc LastOffset = svalBuilder.evalBinOpNN(state, BO_Sub, *Length, One, sizeTy).castAs<NonLoc>();
	//检查第一块内存区域是否足够大
	 SVal BufStart = svalBuilder.evalCast(BufVal, PtrTy, FirstBuf->getType());
	 if (Optional<Loc> BufLoc = BufStart.getAs<Loc>()) {
			const Expr *warningExpr = (WarnAboutSize ? Size : FirstBuf);
		 	
			SVal BufEnd = svalBuilder.evalBinOpLN(state, BO_Add, *BufLoc, LastOffset, PtrTy);
		    state = CheckLocation(C, state, warningExpr, BufEnd, firstMessage);

			if (!state)
				return nullptr;
	 }

	 //检查第二块内存区域
	if (SecondBuf) {
			    BufVal = state->getSVal(SecondBuf, LCtx);
			    state = checkNonNull(C, state, SecondBuf, BufVal);
		    	if (!state)
				      return nullptr;

			BufStart = svalBuilder.evalCast(BufVal, PtrTy, SecondBuf->getType());
			if (Optional<Loc> BufLoc = BufStart.getAs<Loc>()) {
					const Expr *warningExpr = (WarnAboutSize ? Size : SecondBuf);
					SVal BufEnd = svalBuilder.evalBinOpLN(state, BO_Add, *BufLoc, LastOffset, PtrTy);
					state = CheckLocation(C, state, warningExpr, BufEnd, secondMessage);

				if (!state)
					return nullptr;
			}
		}
	return state;
}


ProgramStateRef CheckOverlap:: CheckOverlap1(CheckerContext &C,
												ProgramStateRef state,
												const Expr *Size,
												const Expr *First,
												const Expr *Second) const{
	//之前检查失败
	if (!state)
		return nullptr;
cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@进入CheckOverlap1@@@@@@@@@@@@@@@@@@@@@@"<<endl;
	 ProgramStateRef stateTrue, stateFalse;

	//获取内存值
	const LocationContext *LCtx = C.getLocationContext();
	SVal firstVal = state->getSVal(First, LCtx);
	SVal secondVal = state->getSVal(Second, LCtx);
	
	Optional<Loc> firstLoc = firstVal.getAs<Loc>();
	if (!firstLoc)
		return state;
cout << "@@@@@@@@@@@@@@@@@@@@@@@获取First@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;
	Optional<Loc> secondLoc = secondVal.getAs<Loc>();
	if (!secondLoc)
		return state;
cout << "@@@@@@@@@@@@@@@@@@@@@@@获取Second@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;
	//这两个内存区域是否相同
	SValBuilder &svalBuilder = C.getSValBuilder();
	std::tie(stateTrue, stateFalse) = state->assume(svalBuilder.evalEQ(state, *firstLoc, *secondLoc));

	if (stateTrue && stateFalse){
cout << "@@@@@@@@@@@@@@@@@@@@@@@进入First@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;
		emitOverlapBug(C, stateTrue,First, Second);
		return nullptr;
	}

cout << "@@@@@@@@@@@@@@@@@@@@@@@跳出相等@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;
	assert(stateFalse);
	state = stateFalse;

	//选出内存首地址先出现的内存区域
	QualType cmpTy = svalBuilder.getConditionType();
	SVal reverse = svalBuilder.evalBinOpLL(state,BO_GT,*firstLoc, *secondLoc, cmpTy);

	Optional<DefinedOrUnknownSVal> reverseTest = reverse.getAs<DefinedOrUnknownSVal>();

	if(!reverseTest)
		return state;

	std::tie(stateTrue, stateFalse) = state->assume(*reverseTest);
	if (stateTrue){
		if (stateFalse){
			//如果无法判断处哪个内存区域先出现
cout << "@@@@@@@@@@@@@@@@@@@@@@@无法判断谁先出现@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;
			return state;
		}else{
cout << "@@@@@@@@@@@@@@@@@@@@@@@大于@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;
			std::swap(firstLoc, secondLoc);
			std::swap(First, Second);
		}
	}

cout << "@@@@@@@@@@@@@@@@@@@@@@@小于@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;
	//获取长度
	SVal LengthVal = state->getSVal(Size, LCtx);
	Optional<NonLoc> Length = LengthVal.getAs<NonLoc>();
	if(!Length)
		return state;	
cout << "@@@@@@@@@@@@@@@@@@@@@@@得到长度@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;
	//
	ASTContext &Ctx = svalBuilder.getContext();
	QualType CharPtrTy = Ctx.getPointerType(Ctx.CharTy);
	SVal FirstStart = svalBuilder.evalCast(*firstLoc, CharPtrTy, First->getType());
	Optional<Loc> FirstStartLoc = FirstStart.getAs<Loc>();

	if(!FirstStartLoc)
		return state;
cout << "@@@@@@@@@@@@@@@@@@@@@@@获取FirstStartLoc@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;

	//计数第一块内存区域的尾地址
	SVal FirstEnd = svalBuilder.evalBinOpLN(state, BO_Add, *FirstStartLoc, *Length, CharPtrTy);
	Optional<Loc> FirstEndLoc = FirstEnd.getAs<Loc>();
	if (!FirstEndLoc)
		return state;
cout << "@@@@@@@@@@@@@@@@@@@@@@@获取FirstEndLoc@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;

	//判断第一块内存区域的末尾和第二块内存区域的开始是否重叠
	SVal Overlap = svalBuilder.evalBinOpLL(state, BO_GT, *FirstEndLoc, *secondLoc, cmpTy);

	Optional<DefinedOrUnknownSVal> OverlapTest = Overlap.getAs<DefinedOrUnknownSVal>();
	if(!OverlapTest)
		return state;

	std::tie(stateTrue,stateFalse) = state->assume(*OverlapTest);

	if (stateTrue&&!stateFalse){
cout << "@@@@@@@@@@@@@@@@@@@@@@@产生重叠@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;
		emitOverlapBug(C,stateTrue,First,Second);
		return nullptr;
	}
	//没有重叠
	assert(stateFalse);
	return stateFalse;

}


void CheckOverlap::emitOverlapBug(CheckerContext &C, ProgramStateRef state, const Stmt *First, const Stmt *Second) const
{
	ExplodedNode *N = C.addTransition(state);
	if(!N)
		return;

	if(!BT_Overlap)
		BT_Overlap.reset(new BugType(this," ", "Improper arguments"));

	//
	BugReport *report = new BugReport(*BT_Overlap,"Arguments must not be overlapping buffers", N);
	report->addRange(First->getSourceRange());
	report->addRange(Second->getSourceRange());	
	C.emitReport(report);
}

void CheckOverlap::evalMemcpy(CheckerContext &C, const CallExpr *CE) const
{
cout << "@@@@@@@@@@@@@@@@@@进入evalMemcpy@@@@@@@@@@@@@" << endl;
	const Expr *Dest = CE->getArg(0);
	ProgramStateRef state = C.getState();

	evalCopyCommon(C, CE, state, CE->getArg(3), Dest, CE->getArg(2), true);

}

void CheckOverlap::evalCopyCommon(CheckerContext &C,
								  const CallExpr *CE,
								  ProgramStateRef state,
								  const Expr *Size,
								  const Expr *Dest,
								  const Expr *Source,
								  bool Restricted,
								  bool IsMempcpy) const
{

	cout << "进入@@@@@@@@@@@@@@@evalCopyCommon@@@@@@@@@@@@@@@@@@" << endl;
	//判断size是否为0
	const LocationContext *LCtx = C.getLocationContext();
	SVal sizeVal = state->getSVal(Size, LCtx);
	QualType sizeTy = Size->getType();

	ProgramStateRef stateZeroSize, stateNonZeroSize;
	std::tie(stateZeroSize, stateNonZeroSize) = assumeZero(C, state, sizeVal, sizeTy);

	//得到dest的Value
	SVal destVal = state->getSVal(Dest, LCtx);

	//如果size 为0 不会产生任何重叠 
	if (stateZeroSize && !stateNonZeroSize){
		stateZeroSize = stateZeroSize->BindExpr(CE, LCtx, destVal);
		C.addTransition(stateZeroSize);
		return;
	}

	//如果size非零
	if (stateNonZeroSize){
		state = stateNonZeroSize;
cout<< "@@@@@@@@@@@@@@@@@@@@@@@@size不为零@@@@@@@@@@@@@@@@@@@"<<endl;		
		//检查dest是否为空
		state = checkNonNull(C, state, Dest, destVal);
		if (!state)
			return;

		//获取Src的Value
		SVal srcVal = state->getSVal(Source, LCtx);
		//检查source是否为空
		state = checkNonNull(C, state, Source, srcVal);
cout << "@@@@@@@@@@@@@@@@@Source是否为空@@@@@@@@@@@@@@@@" << endl;		
		if (!state)
		{	
			cout << "@@@@@@@@@@@@@@@@@@@@@Source为空@@@@@@@@@@@@@@@@@@@@@@"<<endl;
			return;
		}
		const char* const writeWarning = "Memory copy function overflows destination buffer";
		const char* const writeWarning1 = "Memeory copy function overflows source buffer";
		state = CheckBufferAccess(C, state, Size, Dest, Source, writeWarning, writeWarning1);

		if (Restricted)
			state = CheckOverlap1(C, state, Size, Dest, Source);

		if (!state)
			return;
		state = state->BindExpr(CE, LCtx, destVal);

		C.addTransition(state);
	}
}


bool CheckOverlap::evalCall(const CallExpr *CE, CheckerContext &C) const 
{
	cout << "@@@@@@@@@@@@@@@@@@进入evalCall@@@@@@@@@@@@@@@@@@@@@" << endl;
	const FunctionDecl *FDecl = C.getCalleeDecl(CE);
	CE->dump();
	if (!FDecl)
	{
		cout << "@@@@@@@@@@@@@@@@@@@@进入myIf@@@@@@@@@@@@@@@@@@" << endl;
		cout << "@@@@@@@@@@@@@@@@@@@@@@@@@!!!!!退出!!!!!@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << endl;
		return false;
	}
	if (FDecl->getNameAsString() == "abc")
	{
		if (CE->getNumArgs() < 4)
			return false;
		cout << "@@@@@@@@@@@@@@@@@@@进入。。。myIf1。。。@@@@@@@@@@@@@@@@@" << endl;
		evalMemcpy(C, CE);
		cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$退出$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << endl;
		return true;
	}else
	{
		return false;
	}
	
}


void  ento::registerCheckOverlap(CheckerManager &mgr) {
	mgr.registerChecker<CheckOverlap>();
	}
