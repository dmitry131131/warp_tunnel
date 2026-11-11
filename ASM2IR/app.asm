; Warp ASM. Entry app. Register conventions are described in README.md.
; x0..x7 = star x/y/z, red/green/blue shifts, cos/sin tables.
; x8 = atan table, x9 = star index, x10 = frame index.

app:
    alloc x0, 5000
    alloc x1, 5000
    alloc x2, 5000
    alloc x3, 5000
    alloc x4, 5000
    alloc x5, 5000
    alloc x6, 256
    alloc x7, 256
    alloc x8, 16
    st x8, 0, 51472
    st x8, 1, 30386
    st x8, 2, 16055
    st x8, 3, 8149
    st x8, 4, 4091
    st x8, 5, 2047
    st x8, 6, 1024
    st x8, 7, 512
    st x8, 8, 256
    st x8, 9, 128
    st x8, 10, 64
    st x8, 11, 32
    st x8, 12, 16
    st x8, 13, 8
    st x8, 14, 4
    st x8, 15, 2
    mov x40, x7
    mov x41, x6
    mov x42, x8
    call setSinCosTables

    mov x9, 0
init_stars:
    rand x11
    srem.32 x11, x11, 2001
    shl.32 x11, x11, 16
    add.32 x11, x11, -65536000
    sdiv.32 x11, x11, 1000
    sext32 x11, x11
    st x0, x9, x11

    rand x11
    srem.32 x11, x11, 2001
    shl.32 x11, x11, 16
    add.32 x11, x11, -65536000
    sdiv.32 x11, x11, 1000
    sext32 x11, x11
    st x1, x9, x11

    rand x11
    srem.32 x11, x11, 10000
    mul.32 x11, x11, 84000
    sdiv.32 x11, x11, 10000
    add.32 x11, x11, 16000
    sext32 x11, x11
    st x2, x9, x11

    call colorShift
    st x3, x9, x40
    sub.64 x11, 0, x40
    st x5, x9, x11
    call colorShift
    st x4, x9, x40
    add.64 x9, x9, 1
    bcmp.slt.64 x9, 5000, init_stars

    mov x10, 0
frame:
    hasclick x11
    bcmp.eq.32 x11, 0, begin_frame
    flush
    ret

begin_frame:
    mov x9, 0
star:
    ; x11 = z, x12 = previous z, x14..x17 = cos/sin at both depths.
    ld x11, x2, x9
    add.64 x12, x11, 4200
    mul.64 x13, x11, 255
    add.64 x37, x13, -4080000
    sdiv.64 x37, x37, 84000
    clamp8 x37, x37
    ld x14, x6, x37
    ld x15, x7, x37
    add.64 x37, x13, -3009000
    sdiv.64 x37, x37, 84000
    clamp8 x37, x37
    ld x16, x6, x37
    ld x17, x7, x37

    ; x18/x19 = source coordinates, x20..x23 = rotated coordinates.
    ld x18, x0, x9
    ld x19, x1, x9
    mul.64 x20, x18, x14
    mul.64 x37, x19, x15
    sub.64 x20, x20, x37
    ashr.64 x20, x20, 16
    mul.64 x21, x18, x15
    mul.64 x37, x19, x14
    add.64 x21, x37, x21
    ashr.64 x21, x21, 16
    mul.64 x22, x18, x16
    mul.64 x37, x19, x17
    sub.64 x22, x22, x37
    ashr.64 x22, x22, 16
    mul.64 x23, x18, x17
    mul.64 x37, x19, x16
    add.64 x23, x37, x23
    ashr.64 x23, x23, 16

    ; x24/x25 = current offsets, x26/x27 = previous offsets.
    mov x24, x20
    mdiv.64 x24, 844, x11
    mov x25, x21
    mdiv.64 x25, 844, x11
    mov x26, x22
    mdiv.64 x26, 844, x12
    mov x27, x23
    mdiv.64 x27, 844, x12
    add.64 x28, x26, 768
    add.64 x29, x27, 384

    ; x30 = packed ARGB, x37/x38/x39 = red/green/blue.
    sub.64 x13, 100000, x11
    mdiv.64 x13, 255, 84000
    clamp8 x13, x13
    ld x37, x3, x9
    add.64 x37, x37, x13
    udiv.8 x38, x13, 3
    ld x39, x4, x9
    add.64 x38, x39, x38
    ld x39, x5, x9
    add.64 x39, x39, x13
    clamp8 x37, x37
    clamp8 x38, x38
    clamp8 x39, x39
    shl.64 x37, x37, 16
    shl.64 x38, x38, 8
    or.64 x30, x38, x37
    or.64 x30, x30, x39
    or.64 x30, x30, 0xFF000000

    ; x31/x32 = dx/dy, x33 = length, x34 = point index.
    sub.64 x31, x24, x26
    sub.64 x32, x25, x27
    abs x37, x31
    abs x38, x32
    umax x33, x37, x38
    umax x33, x33, 1
    mov x34, 0
trail:
    mov x35, x34
    mdiv.64 x35, x31, x33
    add.64 x35, x28, x35
    mov x36, x34
    mdiv.64 x36, x32, x33
    add.64 x36, x29, x36
    brect x35, x36, draw_point
    jmp next_point

draw_point:
    mov x40, x35
    mov x41, x36
    mov x42, x30
    call putCross

next_point:
    bcmp.eq.64 x34, x33, move_star
    add.64 x34, x34, 1
    jmp trail

move_star:
    sub.64 x37, x11, 700
    st x2, x9, x37
    bcmp.slt.64 x11, 16700, respawn
    jmp next_star

respawn:
    rand x37
    srem.32 x37, x37, 2001
    shl.32 x37, x37, 16
    add.32 x37, x37, -65536000
    sdiv.32 x37, x37, 1000
    sext32 x37, x37
    st x0, x9, x37
    rand x37
    srem.32 x37, x37, 2001
    shl.32 x37, x37, 16
    add.32 x37, x37, -65536000
    sdiv.32 x37, x37, 1000
    sext32 x37, x37
    st x1, x9, x37
    st x2, x9, 100000

next_star:
    add.64 x9, x9, 1
    bcmp.slt.64 x9, 5000, star
    flush
    add.64 x10, x10, 1
    bcmp.slt.64 x10, 10000, frame
    ret

; Inputs x40 = sin base, x41 = cos base, x42 = atan base.
; Clobbers x43..x52.
setSinCosTables:
    mov x43, 0
table_entry:
    mul.32 x52, x43, 84000
    udiv.32 x52, x52, 255
    add.32 x52, x52, 16000
    udiv.32 x45, 425984000, x52
    mov x44, 0
    mov x46, 39797
    mov x47, 0
cordic:
    ashr.64 x49, x47, x44
    ashr.64 x48, x46, x44
    ld x50, x42, x44
    bcmp.sgt.64 x45, -1, cordic_positive
    add.64 x51, x49, x46
    sub.64 x47, x47, x48
    add.64 x45, x50, x45
    jmp cordic_next

cordic_positive:
    sub.64 x51, x46, x49
    add.64 x47, x48, x47
    sub.64 x45, x45, x50

cordic_next:
    mov x46, x51
    add.64 x44, x44, 1
    bcmp.slt.64 x44, 16, cordic
    st x41, x43, x46
    st x40, x43, x47
    add.64 x43, x43, 1
    bcmp.slt.64 x43, 256, table_entry
    ret

; Inputs x40 = x, x41 = y, x42 = ARGB. Clobbers x43.
putCross:
    putpixel x40, x41, x42
    bcmp.slt.64 x40, 1535, cross_right
    jmp cross_check_left
cross_right:
    add.32 x43, x40, 1
    putpixel x43, x41, x42
cross_check_left:
    bcmp.sgt.64 x40, 0, cross_left
    jmp cross_check_down
cross_left:
    add.32 x43, x40, -1
    putpixel x43, x41, x42
cross_check_down:
    bcmp.slt.64 x41, 767, cross_down
    jmp cross_check_up
cross_down:
    add.32 x43, x41, 1
    putpixel x40, x43, x42
cross_check_up:
    bcmp.sgt.64 x41, 0, cross_up
    ret
cross_up:
    add.32 x43, x41, -1
    putpixel x40, x43, x42
    ret

; Result x40. Clobbers x40.
colorShift:
    rand x40
    srem.32 x40, x40, 61
    add.32 x40, x40, -40
    sext32 x40, x40
    ret
