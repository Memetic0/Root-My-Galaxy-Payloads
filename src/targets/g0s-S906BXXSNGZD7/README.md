# g0s-S906BXXSNGZD7

```text
device: Samsung Galaxy S22+ (SM-S906B, g0s, Exynos 2200 / s5e9925)
firmware: S906BXXSNGZD7 / EUX (OXM CSC S906BOXMNGZD7)
display build: BP2A.250605.031.A3.S906BXXSNGZD7
fingerprint: samsung/g0sxeea/g0s:16/BP2A.250605.031.A3/S906BXXSNGZD7:user/release-keys
kernel: 5.10.237-android12-9-31999025-abS906BXXSNGZD7
raw kernel SHA-256: 933EC833946506A86F821B53E2C8CF7CCA6AC2111BBD42EE70A802164A05D215
```

`target.h` contains the exact symbol, layout, physical-load, trace, and KASLR
values recovered from that firmware. `p0_fingerprint.h` contains 64 target
kernel page fingerprints and is checked against all 512 source qwords during
the release verification.

This is a no-BTF Samsung 5.10 target (android12-9 branch): layouts follow the
legacy `rt_mutex_waiter` (0x50), `KMALLOC_CGROUP_TYPE`/`KMALLOC_CACHE_TYPES`
are `0`/`2`, and the `configfs_buffer` offsets were measured from the binary.

Unlike the S24 FE / r0s sboot, this bootloader computes the kernel slide as
`(smc_x2 & 0x3f) << 15` (see `docs/SM-S906B-S906BXXSNGZD7.md`): 64 candidates
in `0x8000` steps up to `0x1f8000`, not 32 x `0x10000`. `SLIDE_P0_ALIGN`,
`SLIDE_P0_MAX_OFFSET`, `SLIDE_P0_OFFSET_CANDIDATES`, `SLIDE_MAX_ATTEMPTS`, and
`P0_ORACLE_PROBE_OFFSET` all follow that geometry.

Generate the 64-row fingerprint table from the raw kernel Image with:

```sh
tools/generate_p0_fingerprint.pl Image 0x1f8000 p0_fingerprint.h 0x8000 64
```

Build with Android NDK r29:

```sh
make TARGET=g0s-S906BXXSNGZD7 ANDROID_NDK_HOME=/path/to/android-ndk
```

The full analysis, KernelSU build record, and residual-risk notes are in
[`docs/SM-S906B-S906BXXSNGZD7.md`](../../../docs/SM-S906B-S906BXXSNGZD7.md).
