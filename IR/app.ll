; ModuleID = '../src/app.c'
source_filename = "../src/app.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@PI_FIX = dso_local local_unnamed_addr constant i64 205887, align 8
@K_INV = dso_local local_unnamed_addr constant i64 39797, align 8
@Z_MIN = dso_local local_unnamed_addr constant i64 16000, align 8
@Z_MAX = dso_local local_unnamed_addr constant i64 100000, align 8
@z_range = dso_local local_unnamed_addr constant i64 84000, align 8
@DZ = dso_local local_unnamed_addr constant i64 700, align 8
@SWIRL = dso_local local_unnamed_addr constant i64 6500, align 8
@SCALE = dso_local local_unnamed_addr constant i64 844, align 8
@CX = dso_local local_unnamed_addr constant i64 768, align 8
@CY = dso_local local_unnamed_addr constant i64 384, align 8
@atan_table = dso_local constant [16 x i64] [i64 51472, i64 30386, i64 16055, i64 8149, i64 4091, i64 2047, i64 1024, i64 512, i64 256, i64 128, i64 64, i64 32, i64 16, i64 8, i64 4, i64 2], align 16

; Function Attrs: nounwind optsize sspstrong uwtable
define dso_local void @app() local_unnamed_addr #0 {
  %1 = alloca [5000 x i64], align 16
  %2 = alloca [5000 x i64], align 16
  %3 = alloca [5000 x i64], align 16
  %4 = alloca [5000 x i64], align 16
  %5 = alloca [5000 x i64], align 16
  %6 = alloca [5000 x i64], align 16
  %7 = alloca [256 x i64], align 16
  %8 = alloca [256 x i64], align 16
  call void @llvm.lifetime.start.p0(ptr nonnull %1) #6
  call void @llvm.lifetime.start.p0(ptr nonnull %2) #6
  call void @llvm.lifetime.start.p0(ptr nonnull %3) #6
  call void @llvm.lifetime.start.p0(ptr nonnull %4) #6
  call void @llvm.lifetime.start.p0(ptr nonnull %5) #6
  call void @llvm.lifetime.start.p0(ptr nonnull %6) #6
  call void @llvm.lifetime.start.p0(ptr nonnull %7) #6
  call void @llvm.lifetime.start.p0(ptr nonnull %8) #6
  call void @setSinCosTables(ptr noundef nonnull %8, ptr noundef nonnull %7, ptr noundef nonnull @atan_table) #7
  br label %9

9:                                                ; preds = %0, %9
  %10 = phi i64 [ 0, %0 ], [ %44, %9 ]
  %11 = tail call i32 (...) @simRand() #8
  %12 = srem i32 %11, 2001
  %13 = shl nsw i32 %12, 16
  %14 = add nsw i32 %13, -65536000
  %15 = sdiv i32 %14, 1000
  %16 = sext i32 %15 to i64
  %17 = getelementptr inbounds nuw i64, ptr %1, i64 %10
  store i64 %16, ptr %17, align 8, !tbaa !9
  %18 = tail call i32 (...) @simRand() #8
  %19 = srem i32 %18, 2001
  %20 = shl nsw i32 %19, 16
  %21 = add nsw i32 %20, -65536000
  %22 = sdiv i32 %21, 1000
  %23 = sext i32 %22 to i64
  %24 = getelementptr inbounds nuw i64, ptr %2, i64 %10
  store i64 %23, ptr %24, align 8, !tbaa !9
  %25 = tail call i32 (...) @simRand() #8
  %26 = srem i32 %25, 10000
  %27 = mul nsw i32 %26, 84000
  %28 = sdiv i32 %27, 10000
  %29 = add nsw i32 %28, 16000
  %30 = sext i32 %29 to i64
  %31 = getelementptr inbounds nuw i64, ptr %3, i64 %10
  store i64 %30, ptr %31, align 8, !tbaa !9
  %32 = tail call i32 (...) @simRand() #8
  %33 = srem i32 %32, 61
  %34 = add nsw i32 %33, -40
  %35 = sext i32 %34 to i64
  %36 = getelementptr inbounds nuw i64, ptr %4, i64 %10
  store i64 %35, ptr %36, align 8, !tbaa !9
  %37 = sub nsw i64 0, %35
  %38 = getelementptr inbounds nuw i64, ptr %6, i64 %10
  store i64 %37, ptr %38, align 8, !tbaa !9
  %39 = tail call i32 (...) @simRand() #8
  %40 = srem i32 %39, 61
  %41 = add nsw i32 %40, -40
  %42 = sext i32 %41 to i64
  %43 = getelementptr inbounds nuw i64, ptr %5, i64 %10
  store i64 %42, ptr %43, align 8, !tbaa !9
  %44 = add nuw nsw i64 %10, 1
  %45 = icmp eq i64 %44, 5000
  br i1 %45, label %46, label %9, !llvm.loop !11

46:                                               ; preds = %9, %51
  %47 = phi i64 [ %52, %51 ], [ 0, %9 ]
  %48 = tail call i32 (...) @simHasClick() #8
  %49 = icmp eq i32 %48, 0
  br i1 %49, label %54, label %50

50:                                               ; preds = %46
  tail call void (...) @simFlush() #8
  br label %174

51:                                               ; preds = %171
  tail call void (...) @simFlush() #8
  %52 = add nuw nsw i64 %47, 1
  %53 = icmp eq i64 %52, 10000
  br i1 %53, label %174, label %46, !llvm.loop !13

54:                                               ; preds = %46, %171
  %55 = phi i64 [ %172, %171 ], [ 0, %46 ]
  %56 = getelementptr inbounds nuw i64, ptr %3, i64 %55
  %57 = load i64, ptr %56, align 8, !tbaa !9
  %58 = add nsw i64 %57, 4200
  %59 = mul i64 %57, 255
  %60 = add i64 %59, -4080000
  %61 = sdiv i64 %60, 84000
  %62 = tail call i64 @llvm.smax.i64(i64 %61, i64 0)
  %63 = tail call i64 @llvm.umin.i64(i64 %62, i64 255)
  %64 = getelementptr inbounds nuw i64, ptr %7, i64 %63
  %65 = load i64, ptr %64, align 8, !tbaa !9
  %66 = getelementptr inbounds nuw i64, ptr %8, i64 %63
  %67 = load i64, ptr %66, align 8, !tbaa !9
  %68 = add i64 %59, -3009000
  %69 = sdiv i64 %68, 84000
  %70 = tail call i64 @llvm.smax.i64(i64 %69, i64 0)
  %71 = tail call i64 @llvm.umin.i64(i64 %70, i64 255)
  %72 = getelementptr inbounds nuw i64, ptr %7, i64 %71
  %73 = load i64, ptr %72, align 8, !tbaa !9
  %74 = getelementptr inbounds nuw i64, ptr %8, i64 %71
  %75 = load i64, ptr %74, align 8, !tbaa !9
  %76 = getelementptr inbounds nuw i64, ptr %1, i64 %55
  %77 = load i64, ptr %76, align 8, !tbaa !9
  %78 = mul nsw i64 %77, %65
  %79 = getelementptr inbounds nuw i64, ptr %2, i64 %55
  %80 = load i64, ptr %79, align 8, !tbaa !9
  %81 = mul nsw i64 %80, %67
  %82 = sub nsw i64 %78, %81
  %83 = ashr i64 %82, 16
  %84 = mul nsw i64 %77, %67
  %85 = mul nsw i64 %80, %65
  %86 = add nsw i64 %85, %84
  %87 = ashr i64 %86, 16
  %88 = mul nsw i64 %77, %73
  %89 = mul nsw i64 %80, %75
  %90 = sub nsw i64 %88, %89
  %91 = ashr i64 %90, 16
  %92 = mul nsw i64 %77, %75
  %93 = mul nsw i64 %80, %73
  %94 = add nsw i64 %93, %92
  %95 = ashr i64 %94, 16
  %96 = mul nsw i64 %83, 844
  %97 = sdiv i64 %96, %57
  %98 = mul nsw i64 %87, 844
  %99 = sdiv i64 %98, %57
  %100 = mul nsw i64 %91, 844
  %101 = sdiv i64 %100, %58
  %102 = add nsw i64 %101, 768
  %103 = mul nsw i64 %95, 844
  %104 = sdiv i64 %103, %58
  %105 = add nsw i64 %104, 384
  %106 = sub nsw i64 100000, %57
  %107 = mul nsw i64 %106, 255
  %108 = sdiv i64 %107, 84000
  %109 = tail call i64 @llvm.smax.i64(i64 %108, i64 0)
  %110 = tail call i64 @llvm.umin.i64(i64 %109, i64 255)
  %111 = getelementptr inbounds nuw i64, ptr %4, i64 %55
  %112 = load i64, ptr %111, align 8, !tbaa !9
  %113 = add nsw i64 %112, %110
  %114 = trunc nuw i64 %110 to i8
  %115 = udiv i8 %114, 3
  %116 = zext nneg i8 %115 to i64
  %117 = getelementptr inbounds nuw i64, ptr %5, i64 %55
  %118 = load i64, ptr %117, align 8, !tbaa !9
  %119 = add nsw i64 %118, %116
  %120 = getelementptr inbounds nuw i64, ptr %6, i64 %55
  %121 = load i64, ptr %120, align 8, !tbaa !9
  %122 = add nsw i64 %121, %110
  %123 = tail call i64 @llvm.smax.i64(i64 %113, i64 0)
  %124 = tail call i64 @llvm.umin.i64(i64 %123, i64 255)
  %125 = tail call i64 @llvm.smax.i64(i64 %119, i64 0)
  %126 = tail call i64 @llvm.umin.i64(i64 %125, i64 255)
  %127 = tail call i64 @llvm.smax.i64(i64 %122, i64 0)
  %128 = tail call i64 @llvm.umin.i64(i64 %127, i64 255)
  %129 = shl nuw nsw i64 %124, 16
  %130 = shl nuw nsw i64 %126, 8
  %131 = or disjoint i64 %130, %129
  %132 = or disjoint i64 %131, %128
  %133 = or disjoint i64 %132, 4278190080
  %134 = sub nsw i64 %97, %101
  %135 = sub nsw i64 %99, %104
  %136 = tail call i64 @llvm.abs.i64(i64 %134, i1 true)
  %137 = tail call i64 @llvm.abs.i64(i64 %135, i1 true)
  %138 = tail call i64 @llvm.umax.i64(i64 %136, i64 %137)
  %139 = tail call i64 @llvm.umax.i64(i64 %138, i64 1)
  br label %143

140:                                              ; preds = %155
  %141 = add nsw i64 %57, -700
  store i64 %141, ptr %56, align 8, !tbaa !9
  %142 = icmp slt i64 %57, 16700
  br i1 %142, label %158, label %171

143:                                              ; preds = %54, %155
  %144 = phi i64 [ 0, %54 ], [ %156, %155 ]
  %145 = mul nsw i64 %144, %134
  %146 = sdiv i64 %145, %139
  %147 = add nsw i64 %102, %146
  %148 = mul nsw i64 %144, %135
  %149 = sdiv i64 %148, %139
  %150 = add nsw i64 %105, %149
  %151 = icmp ult i64 %147, 1536
  %152 = icmp ult i64 %150, 768
  %153 = select i1 %151, i1 %152, i1 false
  br i1 %153, label %154, label %155

154:                                              ; preds = %143
  tail call void @putCross(i64 noundef %147, i64 noundef %150, i64 noundef %133) #7
  br label %155

155:                                              ; preds = %154, %143
  %156 = add nuw i64 %144, 1
  %157 = icmp eq i64 %144, %139
  br i1 %157, label %140, label %143, !llvm.loop !14

158:                                              ; preds = %140
  %159 = tail call i32 (...) @simRand() #8
  %160 = srem i32 %159, 2001
  %161 = shl nsw i32 %160, 16
  %162 = add nsw i32 %161, -65536000
  %163 = sdiv i32 %162, 1000
  %164 = sext i32 %163 to i64
  store i64 %164, ptr %76, align 8, !tbaa !9
  %165 = tail call i32 (...) @simRand() #8
  %166 = srem i32 %165, 2001
  %167 = shl nsw i32 %166, 16
  %168 = add nsw i32 %167, -65536000
  %169 = sdiv i32 %168, 1000
  %170 = sext i32 %169 to i64
  store i64 %170, ptr %79, align 8, !tbaa !9
  store i64 100000, ptr %56, align 8, !tbaa !9
  br label %171

171:                                              ; preds = %158, %140
  %172 = add nuw nsw i64 %55, 1
  %173 = icmp eq i64 %172, 5000
  br i1 %173, label %51, label %54, !llvm.loop !15

174:                                              ; preds = %51, %50
  call void @llvm.lifetime.end.p0(ptr nonnull %8) #6
  call void @llvm.lifetime.end.p0(ptr nonnull %7) #6
  call void @llvm.lifetime.end.p0(ptr nonnull %6) #6
  call void @llvm.lifetime.end.p0(ptr nonnull %5) #6
  call void @llvm.lifetime.end.p0(ptr nonnull %4) #6
  call void @llvm.lifetime.end.p0(ptr nonnull %3) #6
  call void @llvm.lifetime.end.p0(ptr nonnull %2) #6
  call void @llvm.lifetime.end.p0(ptr nonnull %1) #6
  ret void
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(ptr captures(none)) #1

; Function Attrs: nofree norecurse nosync nounwind optsize sspstrong memory(argmem: readwrite) uwtable
define dso_local void @setSinCosTables(ptr noundef writeonly captures(none) %0, ptr noundef writeonly captures(none) %1, ptr noundef readonly captures(none) %2) local_unnamed_addr #2 {
  br label %5

4:                                                ; preds = %37
  ret void

5:                                                ; preds = %3, %37
  %6 = phi i64 [ 0, %3 ], [ %40, %37 ]
  %7 = trunc nuw nsw i64 %6 to i32
  %8 = mul nuw nsw i32 %7, 84000
  %9 = udiv i32 %8, 255
  %10 = add nuw nsw i32 %9, 16000
  %11 = udiv i32 425984000, %10
  %12 = zext nneg i32 %11 to i64
  br label %13

13:                                               ; preds = %5, %31
  %14 = phi i64 [ 0, %5 ], [ %35, %31 ]
  %15 = phi i64 [ %12, %5 ], [ %32, %31 ]
  %16 = phi i64 [ 0, %5 ], [ %34, %31 ]
  %17 = phi i64 [ 39797, %5 ], [ %33, %31 ]
  %18 = icmp sgt i64 %15, -1
  %19 = ashr i64 %16, %14
  %20 = ashr i64 %17, %14
  %21 = getelementptr inbounds nuw i64, ptr %2, i64 %14
  %22 = load i64, ptr %21, align 8, !tbaa !9
  br i1 %18, label %23, label %27

23:                                               ; preds = %13
  %24 = sub nsw i64 %17, %19
  %25 = add nsw i64 %20, %16
  %26 = sub nsw i64 %15, %22
  br label %31

27:                                               ; preds = %13
  %28 = add nsw i64 %19, %17
  %29 = sub nsw i64 %16, %20
  %30 = add nsw i64 %22, %15
  br label %31

31:                                               ; preds = %27, %23
  %32 = phi i64 [ %26, %23 ], [ %30, %27 ]
  %33 = phi i64 [ %24, %23 ], [ %28, %27 ]
  %34 = phi i64 [ %25, %23 ], [ %29, %27 ]
  %35 = add nuw nsw i64 %14, 1
  %36 = icmp eq i64 %35, 16
  br i1 %36, label %37, label %13, !llvm.loop !16

37:                                               ; preds = %31
  %38 = getelementptr inbounds nuw i64, ptr %1, i64 %6
  store i64 %33, ptr %38, align 8, !tbaa !9
  %39 = getelementptr inbounds nuw i64, ptr %0, i64 %6
  store i64 %34, ptr %39, align 8, !tbaa !9
  %40 = add nuw nsw i64 %6, 1
  %41 = icmp eq i64 %40, 256
  br i1 %41, label %4, label %5, !llvm.loop !17
}

; Function Attrs: optsize
declare i32 @simRand(...) local_unnamed_addr #3

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(ptr captures(none)) #1

; Function Attrs: optsize
declare i32 @simHasClick(...) local_unnamed_addr #3

; Function Attrs: optsize
declare void @simFlush(...) local_unnamed_addr #3

; Function Attrs: nounwind optsize sspstrong uwtable
define dso_local void @putCross(i64 noundef %0, i64 noundef %1, i64 noundef %2) local_unnamed_addr #0 {
  %4 = trunc i64 %0 to i32
  %5 = trunc i64 %1 to i32
  %6 = trunc i64 %2 to i32
  tail call void @simPutPixel(i32 noundef %4, i32 noundef %5, i32 noundef %6) #8
  %7 = icmp slt i64 %0, 1535
  br i1 %7, label %8, label %11

8:                                                ; preds = %3
  %9 = add i32 %4, 1
  tail call void @simPutPixel(i32 noundef %9, i32 noundef %5, i32 noundef %6) #8
  %10 = icmp sgt i64 %0, 0
  br i1 %10, label %11, label %13

11:                                               ; preds = %3, %8
  %12 = add i32 %4, -1
  tail call void @simPutPixel(i32 noundef %12, i32 noundef %5, i32 noundef %6) #8
  br label %13

13:                                               ; preds = %11, %8
  %14 = icmp slt i64 %1, 767
  br i1 %14, label %15, label %18

15:                                               ; preds = %13
  %16 = add i32 %5, 1
  tail call void @simPutPixel(i32 noundef %4, i32 noundef %16, i32 noundef %6) #8
  %17 = icmp sgt i64 %1, 0
  br i1 %17, label %18, label %20

18:                                               ; preds = %13, %15
  %19 = add i32 %5, -1
  tail call void @simPutPixel(i32 noundef %4, i32 noundef %19, i32 noundef %6) #8
  br label %20

20:                                               ; preds = %18, %15
  ret void
}

; Function Attrs: nounwind optsize sspstrong uwtable
define dso_local range(i64 -100, 21) i64 @colorShift() local_unnamed_addr #0 {
  %1 = tail call i32 (...) @simRand() #8
  %2 = srem i32 %1, 61
  %3 = add nsw i32 %2, -40
  %4 = sext i32 %3 to i64
  ret i64 %4
}

; Function Attrs: optsize
declare void @simPutPixel(i32 noundef, i32 noundef, i32 noundef) local_unnamed_addr #3

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.smax.i64(i64, i64) #4

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.umin.i64(i64, i64) #4

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.abs.i64(i64, i1 immarg) #5

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.umax.i64(i64, i64) #4

attributes #0 = { nounwind optsize sspstrong uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { nofree norecurse nosync nounwind optsize sspstrong memory(argmem: readwrite) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { optsize "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }
attributes #5 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #6 = { nounwind }
attributes #7 = { optsize }
attributes #8 = { nounwind optsize }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}
!llvm.errno.tbaa = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 22.1.5"}
!5 = !{!6, !6, i64 0}
!6 = !{!"int", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
!9 = !{!10, !10, i64 0}
!10 = !{!"long", !7, i64 0}
!11 = distinct !{!11, !12}
!12 = !{!"llvm.loop.mustprogress"}
!13 = distinct !{!13, !12}
!14 = distinct !{!14, !12}
!15 = distinct !{!15, !12}
!16 = distinct !{!16, !12}
!17 = distinct !{!17, !12}
