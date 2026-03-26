// |jit-test| --no-threads; --fast-warmup

function fnModShift1(x)  { return x % 1; }
function fnModShift2(x)  { return x % 3; }
function fnModShift3(x)  { return x % 7; }
function fnModShift4(x)  { return x % 15; }
function fnModShift5(x)  { return x % 31; }
function fnModShift6(x)  { return x % 63; }
function fnModShift7(x)  { return x % 127; }
function fnModShift8(x)  { return x % 255; }
function fnModShift9(x)  { return x % 511; }
function fnModShift10(x) { return x % 1023; }
function fnModShift11(x) { return x % 2047; }
function fnModShift12(x) { return x % 4095; }
function fnModShift13(x) { return x % 8191; }
function fnModShift14(x) { return x % 16383; }
function fnModShift15(x) { return x % 32767; }
function fnModShift16(x) { return x % 65535; }
function fnModShift17(x) { return x % 131071; }
function fnModShift18(x) { return x % 262143; }
function fnModShift19(x) { return x % 524287; }
function fnModShift20(x) { return x % 1048575; }
function fnModShift21(x) { return x % 2097151; }
function fnModShift22(x) { return x % 4194303; }
function fnModShift23(x) { return x % 8388607; }
function fnModShift24(x) { return x % 16777215; }
function fnModShift25(x) { return x % 33554431; }
function fnModShift26(x) { return x % 67108863; }
function fnModShift27(x) { return x % 134217727; }
function fnModShift28(x) { return x % 268435455; }
function fnModShift29(x) { return x % 536870911; }
function fnModShift30(x) { return x % 1073741823; }

function test(fn, d) {
  // Disable Ion in the test harness
  with ({}) {}

  let rand = 0x0D000721;

  for (let i = 0; i < 500; i++) {
    if (i < 450) {
      fn(1);
    } else {
      assertEq(fn(0), 0 % d);
      assertEq(fn(1), 1 % d);
      assertEq(fn(d - 1), (d - 1) % d);
      assertEq(fn(d), d % d);
      assertEq(fn(d + 1), (d + 1) % d);
      assertEq(fn(-1), -1 % d);
      assertEq(fn(-d), (-d) % d);
      assertEq(fn(0x7FFFFFFF), 0x7FFFFFFF % d);
      assertEq(fn(rand), rand % d);
      assertEq(fn(-rand), (-rand) % d);

      rand = ((rand * 0x41C64E6D + 0x3039) & 0xFFFFFFFF) | 0;
    }
  }
}

test(fnModShift1, 1);
test(fnModShift2, 3);
test(fnModShift3, 7);
test(fnModShift4, 15);
test(fnModShift5, 31);
test(fnModShift6, 63);
test(fnModShift7, 127);
test(fnModShift8, 255);
test(fnModShift9, 511);
test(fnModShift10, 1023);
test(fnModShift11, 2047);
test(fnModShift12, 4095);
test(fnModShift13, 8191);
test(fnModShift14, 16383);
test(fnModShift15, 32767);
test(fnModShift16, 65535);
test(fnModShift17, 131071);
test(fnModShift18, 262143);
test(fnModShift19, 524287);
test(fnModShift20, 1048575);
test(fnModShift21, 2097151);
test(fnModShift22, 4194303);
test(fnModShift23, 8388607);
test(fnModShift24, 16777215);
test(fnModShift25, 33554431);
test(fnModShift26, 67108863);
test(fnModShift27, 134217727);
test(fnModShift28, 268435455);
test(fnModShift29, 536870911);
test(fnModShift30, 1073741823);
