# SM-S906B / S906BXXSNGZD7 firmware record

This is the connected phone, not an S21 and not an Android 14 package.

## Live device (adb)

```text
model:            SM-S906B
device:           g0s
SoC:              s5e9925 (Exynos 2200)
sales / carrier:  EVR / EUX
AP/PDA:           S906BXXSNGZD7
CSC:              S906BOXMNGZD7
CP/baseband:      S906BXXSNGZD7
OS:               Android 16 (SDK 36)
One UI:           8.0 (80000)
security patch:   2026-05-05
display build:    BP2A.250605.031.A3.S906BXXSNGZD7
fingerprint:      samsung/g0sxeea/g0s:16/BP2A.250605.031.A3/S906BXXSNGZD7:user/release-keys
system fp:        samsung/g0sxxx/essi:16/BP2A.250605.031.A3/S906BXXSNGZD7:user/release-keys
vendor fp:        samsung/g0sxxx/g0s:12/SP1A.210812.016/S906BXXSNGZD7:user/release-keys
kernel release:   5.10.237-android12-9-31999025-abS906BXXSNGZD7
page size:        4096
SELinux:          Enforcing
sched_blocked_reason id: 104
bootloader:       locked, AVB green
Knox warranty:    already 1
```

`vendor` reporting Android 12 / SDK 31 is expected on this board. The
userspace OS is Android 16 / One UI 8. The kernel string `android12-9`
is the GKI branch (`android12-5.10`), not the OS version.

Root My Galaxy's public feed has no `SM-S906B` / `5.10.237` payload.
The closest published procedure is the no-BTF 5.10 A15 record, not the
6.1/6.6 S24/S25 profiles.

## Official package

FUS four-part (EUX and EVR):

```text
S906BXXSNGZD7/S906BOXMNGZD7/S906BXXSNGZD7/S906BXXSNGZD7
```

SamFw lists the same build as **B (Android 16) / One UI 8**, binary N,
security patch 2026-05-05, AP `..._meta_OS16.tar.md5`.

Downloaded with `download-firmware.sh` to
`firmware/S906BXXSNGZD7_EUX.zip` (9.3 GiB).

| Member | Size |
| --- | ---: |
| `BL_...tar.md5` | 9,144,433 |
| `AP_..._meta_OS16.tar.md5` | 11,272,745,083 |
| `CP_...tar.md5` | 41,431,150 |
| `CSC_OXM_S906BOXMNGZD7_...` | 320,358,504 |
| `HOME_CSC_OXM_...` | 320,327,789 |

FOTA `SYSTEM/build.prop` confirms:

```text
ro.build.version.release=16
ro.build.version.sdk=36
ro.build.version.oneui=80000
ro.build.PDA=S906BXXSNGZD7
ro.build.display.id=BP2A.250605.031.A3.S906BXXSNGZD7
ro.system.build.fingerprint=samsung/g0sxxx/essi:16/BP2A.250605.031.A3/S906BXXSNGZD7:user/release-keys
```

The kernel banner inside `boot.img` is identical to `uname -a` on the
handset, including `dpi@21DODA03` and `Thu May 7 18:59:57 KST 2026`.

## Extracted images

Android boot header version 4, kernel at `0x1000`, raw ARM64 `Image`
(not gzip). `text_offset=0`, `image_size=0x23e0000`, flags `0xa`.

| Object | Size | SHA-256 |
| --- | ---: | --- |
| `boot.img` | 67,108,864 | `0A08A2DD58F120F85587AD87DE43BC33B43B34139E3EF837480DB40A4933220E` |
| raw ARM64 `Image` | 34,583,040 | `933EC833946506A86F821B53E2C8CF7CCA6AC2111BBD42EE70A802164A05D215` |
| recovered `vmlinux.elf` | 40,222,525 | `B72E0226A279A38D9B18DAE67BA76CEAD16609B0B5B4DA8EF51A24EBC7D1DE29` |
| `sboot.bin` | 8,388,608 | `6F4B83A3A7C5DFC2DA649E95F4B0CAA1C98D164736D249A14AF97B06751B5E47` |

`vmlinux-to-elf` recovered 114,520 symbols at
`KIMAGE_TEXT_BASE = 0xffffffc008000000`.

No validated raw BTF blob (`CONFIG_DEBUG_INFO_BTF` is unset). IKCONFIG
is present. Layout work has to follow the A15 5.10 path (source +
disassembly), not the 6.1 BTF path.

Relevant IKCONFIG:

```text
CONFIG_SOC_S5E9925=y
CONFIG_ARM64_VA_BITS=39
CONFIG_PGTABLE_LEVELS=3
CONFIG_TRIM_UNUSED_KSYMS=y
CONFIG_MODVERSIONS=y
# CONFIG_MODULE_FORCE_LOAD is not set
CONFIG_SECURITY_SELINUX_DEVELOP=y
# CONFIG_SECURITY_SELINUX_DISABLE is not set
```

`vermagic`:

```text
5.10.237-android12-9-31999025-abS906BXXSNGZD7 SMP preempt mod_unload modversions aarch64
```

## Physical load (locked from this sboot)

`vendor_boot` v4, board `SRPUG08A023`, reports `kernel_addr=0x10008000`.
That is **not** the sboot jump target. Do not use it, and do not copy
A15 `0x40000000`.

The `Starting kernel...` path is at sboot file `0x144584`:

```text
0x144584  adrp x0, "Starting kernel..."
0x14458c  bl   printf
0x144590  adrp x8, 0x7e0000
0x144594  ldr  w8, [x8, #0xcec]     ; runtime slide slot
0x144598  mov  w9, #-0x80000000     ; 0x80000000
0x1445a4  add  x8, x8, x9
0x1445b0  blr  x8
```

The slot at `0x7e0cec` is zero in the BL image. The only store is at
`0x143a9c`, after an SMC:

```text
ubfiz w8, w8, #15, #6    ; slide = (smc_x2 & 0x3f) << 15
str   w8, [slot]         ; 0, 0x8000, ..., 0x1f8000
```

Image `text_offset` is 0, so:

```c
#define P0_PHYS_OFFSET       0x80000000ULL
#define P0_KERNEL_PHYS_LOAD  0x80000000ULL  /* plus runtime slide */
```

Runtime load = `0x80000000 + ((smc_x2 & 0x3f) << 15)`. That is 64
candidates in 32 KiB steps, not the 32 × 64 KiB table used on S24 FE.

## Recovered symbols

Offsets are `symbol_va - 0xffffffc008000000`.

| Use | Symbol / derivation | Offset |
| --- | --- | ---: |
| UMH callback | `call_usermodehelper_exec_work` | `0x000f85ac` |
| worker | `worker_thread` | `0x000ffc00` |
| trace caller | insn after blocking `bl schedule` | `0x000ffcc0` |
| `NOOP_LLSEEK_OFF` | `noop_llseek` | `0x0037ae34` |
| splice | `generic_file_splice_read` | `0x003c5f38` |
| configfs read | `configfs_read_file` | `0x0044b5c0` |
| configfs write | `configfs_write_bin_file` | `0x0044ba20` |
| ashmem ioctl | `ashmem_ioctl` | `0x00c6818c` |
| ashmem compat ioctl | `compat_ashmem_ioctl` | `0x00c68ae0` |
| ashmem mmap | `ashmem_mmap` | `0x00c68b38` |
| ashmem open | `ashmem_open` | `0x00c68d68` |
| ashmem release | `ashmem_release` | `0x00c68dec` |
| ashmem fdinfo | `ashmem_show_fdinfo` | `0x00c68f0c` |
| logger name | `"nfnetlink_log"` string | `0x018fe009` |
| pipe ops | `anon_pipe_buf_ops` | `0x019f4068` |
| ashmem fops | `ashmem_fops` | `0x01b7f150` |
| kmalloc table | `kmalloc_caches` | `0x01bc4b40` |
| ftrace start | `__start_ftrace_events` | `0x01e3fdc8` |
| blocked event | `__event_sched_blocked_reason` | `0x01e40080` |
| unbound wq | `system_unbound_wq` | `0x01e79e10` |
| logger registry | `loggers` | `0x01e812a8` |
| logger object | `nfulnl_logger` | `0x01e81380` |
| init task | `init_task` | `0x01e8dd00` |
| boot_id data slot | `random_table` boot_id `.data` | `0x0203e930` |
| ashmem misc fops | `ashmem_misc + 0x10` | `0x0207e060` |
| root tg | `root_task_group` | `0x02105080` |
| SELinux | `selinux_state` (enforcing at +0) | `0x02258d58` |
| boot id storage | `sysctl_bootid` | `0x022fd391` |

Checks already made on the raw Image:

- `nfulnl_logger+0x00` = `0xffffffc0098fe009` (`"nfnetlink_log"`)
- `nfulnl_logger+0x08` = `1`
- `nfulnl_logger+0x10` = `nfulnl_log_packet`
- `ashmem_misc+0x10` = `ashmem_fops`
- `random_table` entries are 0x40 bytes; `boot_id` data pointer is
  `sysctl_bootid`
- `worker_thread` idle path: `worker_enter_idle` then `bl schedule` at
  `0xffffffc0080ffcbc`; saved caller is the next insn `0xffffffc0080ffcc0`
- ftrace index `(__event_sched_blocked_reason - __start_ftrace_events)/8 = 87`.
  With 5.10 `__TRACE_LAST_TYPE = 17`, event id `17+87 = 104`, which
  matches `/sys/kernel/tracing/events/sched/sched_blocked_reason/id`
  on the phone

5.10 still uses the legacy `rt_mutex_waiter` (0x50) and
`configfs_read_file` / `configfs_write_bin_file` names. Those must not
be copied from a 6.1/6.6 profile.

## 5.10 layouts (this Image + g0s 5.10 headers)

Source reference: ExtremeXT `android_kernel_samsung_s5e9925` `lineage-23.2`
`kernel/locking/rtmutex_common.h` and `include/linux/sched.h`. Numeric
values below were **checked against this `S906BXXSNGZD7` Image**, not
copied from A15.

`CONFIG_DEBUG_RT_MUTEXES` is off. `rt_mutex_waiter` is the 0x50-byte
legacy layout. `task_blocks_on_rt_mutex` on this vmlinux:

```text
waiter.pi_tree_entry  0x18
waiter.task           0x30   stp with lock
waiter.lock           0x38
waiter.prio           0x40
waiter.deadline       0x48
sizeof                0x50
```

`task_struct` on this Image (`init_task` + `rt_mutex_setprio` /
`task_blocks_on_rt_mutex` / `sched_move_task`):

```text
task_struct.usage             0x40   init_task refcount 2
task_struct.prio              0x84   0x78 = 120
task_struct.normal_prio       0x8c
task_struct.sched_task_group  0x310  sched_move_task
task_struct.pi_lock           0x86c
task_struct.pi_waiters        0x880
task_struct.pi_top_task       0x890
task_struct.pi_blocked_on     0x898
```

Those eight task offsets happen to match A15, but they were re-derived
here. `file_operations` slots on `ashmem_fops` also match the 5.10
table (`ioctl 0x50`, `mmap 0x60`, `open 0x70`, `release 0x80`,
`show_fdinfo 0xe0`). `pwq.max_active` is `0x5c`, `nr_active` `0x58`.

`sizeof(struct page)` was not re-measured from BTF (none). Treat 0x40
as the 5.10 default until a `compound_head`/`slab_cache` accessor on
this Image is checked.

## KernelSU module

Built from KernelSU `v3.2.5` (`b0bc817b4e966aa6aa830834eaf6ef765d821d40`)
plus `KernelSU-v3.2.5-samsung-kdp-rkp-defex.patch`, against DDK
`ghcr.io/ylarod/ddk-min:android12-5.10`, with DDK `Module.symvers`
cleared and `UTS_RELEASE` replaced by the live string. Flags:

```text
CONFIG_KSU=m
CONFIG_KSU_SAMSUNG_KDP=y
CONFIG_KSU_SAMSUNG_RKP=y
CONFIG_KSU_SAMSUNG_DEFEX=y
CONFIG_KSU_SAMSUNG_NO_PATCH_TEXT=y
KBUILD_MODPOST_WARN=1
```

`NO_PATCH_TEXT` is on because this is Exynos (RKP/EL2), same reason as
the later Exynos 2400 artifacts.

| File | Size | SHA-256 |
| --- | ---: | --- |
| `kernelsu/android12-5.10_kernelsu-g0s-S906BXXSNGZD7-kdp.ko` | 352,944 | `b604accdef5073014fe39a04ea57a2779eaffe6e684139f71eb5a74eeaefb555` |

```text
vermagic: 5.10.237-android12-9-31999025-abS906BXXSNGZD7 SMP preempt mod_unload modversions aarch64
__versions size: 0
undefined imports: 198, all present in the recovered vmlinux
target CRC mismatches: 0
stop_machine: not imported
```

Samsung helpers resolved in this vmlinux:

```text
prepare_ro_creds
kdp_assign_pgd
kdp_usecount_dec_and_test
get_task_creds
set_task_creds
task_defex_enforce
sys_call_table
```

`ksud` was rebuilt for `aarch64-linux-android` (NDK r29, API 26) with
`android12-5.10_kernelsu.ko` embedded. Host copy:

```text
out/device/ksud-g0s-S906BXXSNGZD7-kdp
size: 4,882,120
SHA-256: 639cb2332cd89b62b47c6cbbbd81c1f5d4d1777a6e17cbfa43d56dbee195153d
```

It is on the phone at `/data/local/tmp/ksud`. `late-load` as the shell
user fails at `/data/adb/` because there is still no installer payload
for this firmware. KernelSU Manager `v3.2.5` (`32525`) is installed.

## Payload port (Root-My-Galaxy-Payloads)

`src/targets/g0s-S906BXXSNGZD7/` is complete: `target.h`,
`p0_fingerprint.h`, and `README.md`.

The P0 fingerprint table has **64 rows at `0x8000` steps** (probe
`0x1f8000`), generated from this Image:

```sh
tools/generate_p0_fingerprint.pl Image 0x1f8000 p0_fingerprint.h 0x8000 64
```

The generator grew optional `SLIDE_STEP`/`SLIDE_COUNT` arguments for this
(defaults keep the 32 × `0x10000` behavior). All 512 source qwords verified by
readback; the 64 rows are distinct with worst pairwise overlap 1/8 words.

Shared exploit code previously hard-coded the S24 slide validation
(`slide <= 0x1f0000`, `slide % 0x10000 == 0`) in five places; a `0x8000`-step
slide would have been rejected at `slide_commit_stext`. Those checks now use
overridable `SLIDE_P0_MAX_OFFSET`/`SLIDE_P0_ALIGN` (defaults unchanged), and
the g0s target sets `0x1f8000`/`0x8000`. `SLIDE_MAX_ATTEMPTS` is 64 and
`P0_ORACLE_PROBE_OFFSET` is `0x1f8000`.

Built with NDK r29 (`make TARGET=g0s-S906BXXSNGZD7 all release`); r0s and
essi-S721NKSSCDZF3 still build identically after the shared-code change.

| File | Size | SHA-256 |
| --- | ---: | --- |
| `artifacts/g0s-S906BXXSNGZD7/cve-2026-43499-app.so` | 104,128 | `b59afec4476ad8b8b4454e6a2c01e4397d696c9f1a28c92cb29b450d34fb0024` |

`support/targets-v3.json` gained payload `g0s-S906BXXSNGZD7`
(models `["SM-S906B"]`, kernelVersions `["5.10.237"]`).

## Still open

1. Confirm `struct page` member offsets on this Image (`0x40` layout is
   the 5.10 default used by the r0s sibling port).
2. Hardware execution.
