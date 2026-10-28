; ModuleID = 'app.c'
source_filename = "app.c"

@atan_table = constant [16 x i64] [i64 51472, i64 30386, i64 16055, i64 8149, i64 4091, i64 2047, i64 1024, i64 512, i64 256, i64 128, i64 64, i64 32, i64 16, i64 8, i64 4, i64 2], align 16

declare void @simPutPixel(i32, i32, i32)

declare void @simFlush()

declare i32 @simRand()

declare i32 @simHasClick()

define i64 @colorShift() {
  %1 = call i32 @simRand()
  %2 = srem i32 %1, 61
  %3 = add nsw i32 %2, -40
  %4 = sext i32 %3 to i64
  ret i64 %4
}

define void @putCross(i64 %0, i64 %1, i64 %2) {
  %4 = trunc i64 %0 to i32
  %5 = trunc i64 %1 to i32
  %6 = trunc i64 %2 to i32
  call void @simPutPixel(i32 %4, i32 %5, i32 %6)
  %7 = icmp slt i64 %0, 1535
  br i1 %7, label %8, label %11

8:                                                ; preds = %3
  %9 = add i32 %4, 1
  call void @simPutPixel(i32 %9, i32 %5, i32 %6)
  %10 = icmp sgt i64 %0, 0
  br i1 %10, label %11, label %13

11:                                               ; preds = %8, %3
  %12 = add i32 %4, -1
  call void @simPutPixel(i32 %12, i32 %5, i32 %6)
  br label %13

13:                                               ; preds = %11, %8
  %14 = icmp slt i64 %1, 767
  br i1 %14, label %15, label %18

15:                                               ; preds = %13
  %16 = add i32 %5, 1
  call void @simPutPixel(i32 %4, i32 %16, i32 %6)
  %17 = icmp sgt i64 %1, 0
  br i1 %17, label %18, label %20

18:                                               ; preds = %15, %13
  %19 = add i32 %5, -1
  call void @simPutPixel(i32 %4, i32 %19, i32 %6)
  br label %20

20:                                               ; preds = %18, %15
  ret void
}

define void @setSinCosTables(ptr %0, ptr %1, ptr %2) {
  br label %5

4:                                                ; preds = %37
  ret void

5:                                                ; preds = %37, %3
  %6 = phi i64 [ 0, %3 ], [ %40, %37 ]
  %7 = trunc i64 %6 to i32
  %8 = mul nuw nsw i32 %7, 84000
  %9 = udiv i32 %8, 255
  %10 = add nuw nsw i32 %9, 16000
  %11 = udiv i32 425984000, %10
  %12 = zext i32 %11 to i64
  br label %13

13:                                               ; preds = %31, %5
  %14 = phi i64 [ 0, %5 ], [ %35, %31 ]
  %15 = phi i64 [ %12, %5 ], [ %32, %31 ]
  %16 = phi i64 [ 0, %5 ], [ %34, %31 ]
  %17 = phi i64 [ 39797, %5 ], [ %33, %31 ]
  %18 = icmp sgt i64 %15, -1
  %19 = ashr i64 %16, %14
  %20 = ashr i64 %17, %14
  %21 = getelementptr inbounds i64, ptr %2, i64 %14
  %22 = load i64, ptr %21, align 4
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
  br i1 %36, label %37, label %13

37:                                               ; preds = %31
  %38 = getelementptr inbounds i64, ptr %1, i64 %6
  store i64 %33, ptr %38, align 4
  %39 = getelementptr inbounds i64, ptr %0, i64 %6
  store i64 %34, ptr %39, align 4
  %40 = add nuw nsw i64 %6, 1
  %41 = icmp eq i64 %40, 256
  br i1 %41, label %4, label %5
}

define void @app() {
  %1 = alloca [5000 x i64], align 16
  %2 = alloca [5000 x i64], align 16
  %3 = alloca [5000 x i64], align 16
  %4 = alloca [5000 x i64], align 16
  %5 = alloca [5000 x i64], align 16
  %6 = alloca [5000 x i64], align 16
  %7 = alloca [256 x i64], align 16
  %8 = alloca [256 x i64], align 16
  call void @setSinCosTables(ptr %8, ptr %7, ptr @atan_table)
  br label %9

9:                                                ; preds = %9, %0
  %10 = phi i64 [ 0, %0 ], [ %38, %9 ]
  %11 = call i32 @simRand()
  %12 = srem i32 %11, 2001
  %13 = shl nsw i32 %12, 16
  %14 = add nsw i32 %13, -65536000
  %15 = sdiv i32 %14, 1000
  %16 = sext i32 %15 to i64
  %17 = getelementptr inbounds i64, ptr %1, i64 %10
  store i64 %16, ptr %17, align 4
  %18 = call i32 @simRand()
  %19 = srem i32 %18, 2001
  %20 = shl nsw i32 %19, 16
  %21 = add nsw i32 %20, -65536000
  %22 = sdiv i32 %21, 1000
  %23 = sext i32 %22 to i64
  %24 = getelementptr inbounds i64, ptr %2, i64 %10
  store i64 %23, ptr %24, align 4
  %25 = call i32 @simRand()
  %26 = srem i32 %25, 10000
  %27 = mul nsw i32 %26, 84000
  %28 = sdiv i32 %27, 10000
  %29 = add nsw i32 %28, 16000
  %30 = sext i32 %29 to i64
  %31 = getelementptr inbounds i64, ptr %3, i64 %10
  store i64 %30, ptr %31, align 4
  %32 = call i64 @colorShift()
  %33 = getelementptr inbounds i64, ptr %4, i64 %10
  store i64 %32, ptr %33, align 4
  %34 = sub nsw i64 0, %32
  %35 = getelementptr inbounds i64, ptr %6, i64 %10
  store i64 %34, ptr %35, align 4
  %36 = call i64 @colorShift()
  %37 = getelementptr inbounds i64, ptr %5, i64 %10
  store i64 %36, ptr %37, align 4
  %38 = add nuw nsw i64 %10, 1
  %39 = icmp eq i64 %38, 5000
  br i1 %39, label %40, label %9

40:                                               ; preds = %45, %9
  %41 = phi i64 [ 0, %9 ], [ %46, %45 ]
  %42 = call i32 @simHasClick()
  %43 = icmp eq i32 %42, 0
  br i1 %43, label %48, label %44

44:                                               ; preds = %40
  call void @simFlush()
  br label %166

45:                                               ; preds = %163
  call void @simFlush()
  %46 = add nuw nsw i64 %41, 1
  %47 = icmp eq i64 %46, 10000
  br i1 %47, label %166, label %40

48:                                               ; preds = %163, %40
  %49 = phi i64 [ 0, %40 ], [ %164, %163 ]
  %50 = getelementptr inbounds i64, ptr %3, i64 %49
  %51 = load i64, ptr %50, align 4
  %52 = add nsw i64 %51, 4200
  %53 = mul i64 %51, 255
  %54 = add i64 %53, -4080000
  %55 = sdiv i64 %54, 84000
  %56 = call i64 @llvm.smax.i64(i64 %55, i64 0)
  %57 = call i64 @llvm.umin.i64(i64 %56, i64 255)
  %58 = getelementptr inbounds i64, ptr %7, i64 %57
  %59 = load i64, ptr %58, align 4
  %60 = getelementptr inbounds i64, ptr %8, i64 %57
  %61 = load i64, ptr %60, align 4
  %62 = add i64 %53, -3009000
  %63 = sdiv i64 %62, 84000
  %64 = call i64 @llvm.smax.i64(i64 %63, i64 0)
  %65 = call i64 @llvm.umin.i64(i64 %64, i64 255)
  %66 = getelementptr inbounds i64, ptr %7, i64 %65
  %67 = load i64, ptr %66, align 4
  %68 = getelementptr inbounds i64, ptr %8, i64 %65
  %69 = load i64, ptr %68, align 4
  %70 = getelementptr inbounds i64, ptr %1, i64 %49
  %71 = load i64, ptr %70, align 4
  %72 = mul nsw i64 %71, %59
  %73 = getelementptr inbounds i64, ptr %2, i64 %49
  %74 = load i64, ptr %73, align 4
  %75 = mul nsw i64 %74, %61
  %76 = sub nsw i64 %72, %75
  %77 = ashr i64 %76, 16
  %78 = mul nsw i64 %71, %61
  %79 = mul nsw i64 %74, %59
  %80 = add nsw i64 %79, %78
  %81 = ashr i64 %80, 16
  %82 = mul nsw i64 %71, %67
  %83 = mul nsw i64 %74, %69
  %84 = sub nsw i64 %82, %83
  %85 = ashr i64 %84, 16
  %86 = mul nsw i64 %71, %69
  %87 = mul nsw i64 %74, %67
  %88 = add nsw i64 %87, %86
  %89 = ashr i64 %88, 16
  %90 = mul nsw i64 %77, 844
  %91 = sdiv i64 %90, %51
  %92 = mul nsw i64 %81, 844
  %93 = sdiv i64 %92, %51
  %94 = mul nsw i64 %85, 844
  %95 = sdiv i64 %94, %52
  %96 = add nsw i64 %95, 768
  %97 = mul nsw i64 %89, 844
  %98 = sdiv i64 %97, %52
  %99 = add nsw i64 %98, 384
  %100 = sub nsw i64 100000, %51
  %101 = mul nsw i64 %100, 255
  %102 = sdiv i64 %101, 84000
  %103 = call i64 @llvm.smax.i64(i64 %102, i64 0)
  %104 = call i64 @llvm.umin.i64(i64 %103, i64 255)
  %105 = getelementptr inbounds i64, ptr %4, i64 %49
  %106 = load i64, ptr %105, align 4
  %107 = add nsw i64 %106, %104
  %108 = udiv i64 %104, 3
  %109 = getelementptr inbounds i64, ptr %5, i64 %49
  %110 = load i64, ptr %109, align 4
  %111 = add nsw i64 %110, %108
  %112 = getelementptr inbounds i64, ptr %6, i64 %49
  %113 = load i64, ptr %112, align 4
  %114 = add nsw i64 %113, %104
  %115 = call i64 @llvm.smax.i64(i64 %107, i64 0)
  %116 = call i64 @llvm.umin.i64(i64 %115, i64 255)
  %117 = call i64 @llvm.smax.i64(i64 %111, i64 0)
  %118 = call i64 @llvm.umin.i64(i64 %117, i64 255)
  %119 = call i64 @llvm.smax.i64(i64 %114, i64 0)
  %120 = call i64 @llvm.umin.i64(i64 %119, i64 255)
  %121 = shl nuw nsw i64 %116, 16
  %122 = shl nuw nsw i64 %118, 8
  %123 = or i64 %122, %121
  %124 = or i64 %123, %120
  %125 = or i64 %124, 4278190080
  %126 = sub nsw i64 %91, %95
  %127 = sub nsw i64 %93, %98
  %128 = call i64 @llvm.abs.i64(i64 %126, i1 true)
  %129 = call i64 @llvm.abs.i64(i64 %127, i1 true)
  %130 = call i64 @llvm.umax.i64(i64 %128, i64 %129)
  %131 = call i64 @llvm.umax.i64(i64 %130, i64 1)
  br label %135

132:                                              ; preds = %147
  %133 = add nsw i64 %51, -700
  store i64 %133, ptr %50, align 4
  %134 = icmp slt i64 %51, 16700
  br i1 %134, label %150, label %163

135:                                              ; preds = %147, %48
  %136 = phi i64 [ 0, %48 ], [ %148, %147 ]
  %137 = mul nsw i64 %136, %126
  %138 = sdiv i64 %137, %131
  %139 = add nsw i64 %96, %138
  %140 = mul nsw i64 %136, %127
  %141 = sdiv i64 %140, %131
  %142 = add nsw i64 %99, %141
  %143 = icmp ult i64 %139, 1536
  %144 = icmp ult i64 %142, 768
  %145 = select i1 %143, i1 %144, i1 false
  br i1 %145, label %146, label %147

146:                                              ; preds = %135
  call void @putCross(i64 %139, i64 %142, i64 %125)
  br label %147

147:                                              ; preds = %146, %135
  %148 = add nuw i64 %136, 1
  %149 = icmp eq i64 %136, %131
  br i1 %149, label %132, label %135

150:                                              ; preds = %132
  %151 = call i32 @simRand()
  %152 = srem i32 %151, 2001
  %153 = shl nsw i32 %152, 16
  %154 = add nsw i32 %153, -65536000
  %155 = sdiv i32 %154, 1000
  %156 = sext i32 %155 to i64
  store i64 %156, ptr %70, align 4
  %157 = call i32 @simRand()
  %158 = srem i32 %157, 2001
  %159 = shl nsw i32 %158, 16
  %160 = add nsw i32 %159, -65536000
  %161 = sdiv i32 %160, 1000
  %162 = sext i32 %161 to i64
  store i64 %162, ptr %73, align 4
  store i64 100000, ptr %50, align 4
  br label %163

163:                                              ; preds = %150, %132
  %164 = add nuw nsw i64 %49, 1
  %165 = icmp eq i64 %164, 5000
  br i1 %165, label %45, label %48

166:                                              ; preds = %45, %44
  ret void
}

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.smax.i64(i64, i64) #0

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.umin.i64(i64, i64) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.abs.i64(i64, i1 immarg) #1

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.umax.i64(i64, i64) #0

attributes #0 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

