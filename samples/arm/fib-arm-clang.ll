; ModuleID = 'fib.c'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:32:64-v128:32:128-a0:0:32-n32-S32"
target triple = "armv4t-unknown-linux-gnu"

@.str = private unnamed_addr constant [12 x i8] c"fib(%u)=%u\0A\00", align 1
@.str1 = private unnamed_addr constant [16 x i8] c"fastfib(%u)=%u\0A\00", align 1
@.str2 = private unnamed_addr constant [19 x i8] c"fastfib_v2(%u)=%u\0A\00", align 1

define i32 @main() nounwind {
entry:
  %retval = alloca i32, align 4
  %n = alloca i32, align 4
  store i32 0, i32* %retval
  store i32 0, i32* %n, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32* %n, align 4
  %cmp = icmp ult i32 %0, 35
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %1 = load i32* %n, align 4
  %2 = load i32* %n, align 4
  %call = call i32 @fib(i32 %2)
  %call1 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([12 x i8]* @.str, i32 0, i32 0), i32 %1, i32 %call)
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %3 = load i32* %n, align 4
  %inc = add i32 %3, 1
  store i32 %inc, i32* %n, align 4
  br label %for.cond

for.end:                                          ; preds = %for.cond
  store i32 0, i32* %n, align 4
  br label %for.cond2

for.cond2:                                        ; preds = %for.inc7, %for.end
  %4 = load i32* %n, align 4
  %cmp3 = icmp ult i32 %4, 35
  br i1 %cmp3, label %for.body4, label %for.end9

for.body4:                                        ; preds = %for.cond2
  %5 = load i32* %n, align 4
  %6 = load i32* %n, align 4
  %call5 = call i32 @fastfib(i32 %6)
  %call6 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([16 x i8]* @.str1, i32 0, i32 0), i32 %5, i32 %call5)
  br label %for.inc7

for.inc7:                                         ; preds = %for.body4
  %7 = load i32* %n, align 4
  %inc8 = add i32 %7, 1
  store i32 %inc8, i32* %n, align 4
  br label %for.cond2

for.end9:                                         ; preds = %for.cond2
  store i32 0, i32* %n, align 4
  br label %for.cond10

for.cond10:                                       ; preds = %for.inc15, %for.end9
  %8 = load i32* %n, align 4
  %cmp11 = icmp ult i32 %8, 35
  br i1 %cmp11, label %for.body12, label %for.end17

for.body12:                                       ; preds = %for.cond10
  %9 = load i32* %n, align 4
  %10 = load i32* %n, align 4
  %call13 = call i32 @fastfib(i32 %10)
  %call14 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([19 x i8]* @.str2, i32 0, i32 0), i32 %9, i32 %call13)
  br label %for.inc15

for.inc15:                                        ; preds = %for.body12
  %11 = load i32* %n, align 4
  %inc16 = add i32 %11, 1
  store i32 %inc16, i32* %n, align 4
  br label %for.cond10

for.end17:                                        ; preds = %for.cond10
  ret i32 0
}

declare i32 @printf(i8*, ...)

define i32 @fib(i32 %n) nounwind {
entry:
  %n.addr = alloca i32, align 4
  store i32 %n, i32* %n.addr, align 4
  %0 = load i32* %n.addr, align 4
  %cmp = icmp ult i32 %0, 2
  br i1 %cmp, label %cond.true, label %cond.false

cond.true:                                        ; preds = %entry
  %1 = load i32* %n.addr, align 4
  br label %cond.end

cond.false:                                       ; preds = %entry
  %2 = load i32* %n.addr, align 4
  %sub = sub i32 %2, 1
  %call = call i32 @fib(i32 %sub)
  %3 = load i32* %n.addr, align 4
  %sub1 = sub i32 %3, 2
  %call2 = call i32 @fib(i32 %sub1)
  %add = add i32 %call, %call2
  br label %cond.end

cond.end:                                         ; preds = %cond.false, %cond.true
  %cond = phi i32 [ %1, %cond.true ], [ %add, %cond.false ]
  ret i32 %cond
}

define i32 @fastfib(i32 %n) nounwind {
entry:
  %n.addr = alloca i32, align 4
  %a = alloca [3 x i32], align 4
  %p = alloca i32*, align 4
  %i = alloca i32, align 4
  store i32 %n, i32* %n.addr, align 4
  %arraydecay = getelementptr inbounds [3 x i32]* %a, i32 0, i32 0
  store i32* %arraydecay, i32** %p, align 4
  store i32 0, i32* %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32* %i, align 4
  %1 = load i32* %n.addr, align 4
  %cmp = icmp ule i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i32* %i, align 4
  %cmp1 = icmp ult i32 %2, 2
  br i1 %cmp1, label %if.then, label %if.else

if.then:                                          ; preds = %for.body
  %3 = load i32* %i, align 4
  %4 = load i32** %p, align 4
  store i32 %3, i32* %4, align 4
  br label %if.end23

if.else:                                          ; preds = %for.body
  %5 = load i32** %p, align 4
  %arraydecay2 = getelementptr inbounds [3 x i32]* %a, i32 0, i32 0
  %cmp3 = icmp eq i32* %5, %arraydecay2
  br i1 %cmp3, label %if.then4, label %if.else8

if.then4:                                         ; preds = %if.else
  %arraydecay5 = getelementptr inbounds [3 x i32]* %a, i32 0, i32 0
  %add.ptr = getelementptr inbounds i32* %arraydecay5, i32 1
  %6 = load i32* %add.ptr, align 4
  %arraydecay6 = getelementptr inbounds [3 x i32]* %a, i32 0, i32 0
  %add.ptr7 = getelementptr inbounds i32* %arraydecay6, i32 2
  %7 = load i32* %add.ptr7, align 4
  %add = add i32 %6, %7
  %8 = load i32** %p, align 4
  store i32 %add, i32* %8, align 4
  br label %if.end22

if.else8:                                         ; preds = %if.else
  %9 = load i32** %p, align 4
  %arraydecay9 = getelementptr inbounds [3 x i32]* %a, i32 0, i32 0
  %add.ptr10 = getelementptr inbounds i32* %arraydecay9, i32 1
  %cmp11 = icmp eq i32* %9, %add.ptr10
  br i1 %cmp11, label %if.then12, label %if.else17

if.then12:                                        ; preds = %if.else8
  %arraydecay13 = getelementptr inbounds [3 x i32]* %a, i32 0, i32 0
  %10 = load i32* %arraydecay13, align 4
  %arraydecay14 = getelementptr inbounds [3 x i32]* %a, i32 0, i32 0
  %add.ptr15 = getelementptr inbounds i32* %arraydecay14, i32 2
  %11 = load i32* %add.ptr15, align 4
  %add16 = add i32 %10, %11
  %12 = load i32** %p, align 4
  store i32 %add16, i32* %12, align 4
  br label %if.end

if.else17:                                        ; preds = %if.else8
  %arraydecay18 = getelementptr inbounds [3 x i32]* %a, i32 0, i32 0
  %13 = load i32* %arraydecay18, align 4
  %arraydecay19 = getelementptr inbounds [3 x i32]* %a, i32 0, i32 0
  %add.ptr20 = getelementptr inbounds i32* %arraydecay19, i32 1
  %14 = load i32* %add.ptr20, align 4
  %add21 = add i32 %13, %14
  %15 = load i32** %p, align 4
  store i32 %add21, i32* %15, align 4
  br label %if.end

if.end:                                           ; preds = %if.else17, %if.then12
  br label %if.end22

if.end22:                                         ; preds = %if.end, %if.then4
  br label %if.end23

if.end23:                                         ; preds = %if.end22, %if.then
  %16 = load i32** %p, align 4
  %incdec.ptr = getelementptr inbounds i32* %16, i32 1
  store i32* %incdec.ptr, i32** %p, align 4
  %arraydecay24 = getelementptr inbounds [3 x i32]* %a, i32 0, i32 0
  %add.ptr25 = getelementptr inbounds i32* %arraydecay24, i32 2
  %cmp26 = icmp ugt i32* %incdec.ptr, %add.ptr25
  br i1 %cmp26, label %if.then27, label %if.end29

if.then27:                                        ; preds = %if.end23
  %arraydecay28 = getelementptr inbounds [3 x i32]* %a, i32 0, i32 0
  store i32* %arraydecay28, i32** %p, align 4
  br label %if.end29

if.end29:                                         ; preds = %if.then27, %if.end23
  br label %for.inc

for.inc:                                          ; preds = %if.end29
  %17 = load i32* %i, align 4
  %inc = add i32 %17, 1
  store i32 %inc, i32* %i, align 4
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %18 = load i32** %p, align 4
  %arraydecay30 = getelementptr inbounds [3 x i32]* %a, i32 0, i32 0
  %cmp31 = icmp eq i32* %18, %arraydecay30
  br i1 %cmp31, label %cond.true, label %cond.false

cond.true:                                        ; preds = %for.end
  %19 = load i32** %p, align 4
  %add.ptr32 = getelementptr inbounds i32* %19, i32 2
  %20 = load i32* %add.ptr32, align 4
  br label %cond.end

cond.false:                                       ; preds = %for.end
  %21 = load i32** %p, align 4
  %add.ptr33 = getelementptr inbounds i32* %21, i32 -1
  %22 = load i32* %add.ptr33, align 4
  br label %cond.end

cond.end:                                         ; preds = %cond.false, %cond.true
  %cond = phi i32 [ %20, %cond.true ], [ %22, %cond.false ]
  ret i32 %cond
}

define i32 @fastfib_v2(i32 %n) nounwind {
entry:
  %retval = alloca i32, align 4
  %n.addr = alloca i32, align 4
  %n0 = alloca i32, align 4
  %n1 = alloca i32, align 4
  %naux = alloca i32, align 4
  %i = alloca i32, align 4
  store i32 %n, i32* %n.addr, align 4
  store i32 0, i32* %n0, align 4
  store i32 1, i32* %n1, align 4
  %0 = load i32* %n.addr, align 4
  %cmp = icmp eq i32 %0, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  store i32 0, i32* %retval
  br label %return

if.end:                                           ; preds = %entry
  store i32 0, i32* %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %if.end
  %1 = load i32* %i, align 4
  %2 = load i32* %n.addr, align 4
  %sub = sub i32 %2, 1
  %cmp1 = icmp ult i32 %1, %sub
  br i1 %cmp1, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %3 = load i32* %n1, align 4
  store i32 %3, i32* %naux, align 4
  %4 = load i32* %n0, align 4
  %5 = load i32* %n1, align 4
  %add = add i32 %4, %5
  store i32 %add, i32* %n1, align 4
  %6 = load i32* %naux, align 4
  store i32 %6, i32* %n0, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %7 = load i32* %i, align 4
  %inc = add i32 %7, 1
  store i32 %inc, i32* %i, align 4
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %8 = load i32* %n1, align 4
  store i32 %8, i32* %retval
  br label %return

return:                                           ; preds = %for.end, %if.then
  %9 = load i32* %retval
  ret i32 %9
}
