# Gunyah on the OnePlus 11 (salami / SM8550): experiment notes

Status (2026-10-09): **dead end on the current OEM firmware.** This branch holds
debug-quality code and findings. It is not meant for upstream as it stands.

## Goal

Run Gunyah guests through KVM at EL1, using Karim Manaouil's RFC series
"Running Qualcomm's Gunyah Guests via KVM in EL1" (April 2025), on a mainline
7.2 kernel on a OnePlus 11. Test: Karim's modified QEMU with `-accel kvm`.

## What works (all seen in dmesg or program output)

- The series builds on 7.2 after two fixes (`of_node_to_fwnode` renamed to
  `of_fwnode_handle`; a `kvm_arch_vcpu_unlocked_ioctl` stub in
  `arch/arm64/kvm/gunyah.c`, because `arm.c` is not built when `CONFIG_GUNYAH`
  replaces `CONFIG_KVM_ARM`).
- Hypervisor identification through the native hypercall ABI:
  `Running under Gunyah hypervisor 51/v1`, `KVM for Gunyah is available!`,
  and `/dev/kvm` exists.
- The resource manager (RM) node is recovered from the hypervisor at runtime
  and `gunyah_rsc_mgr` binds to it.
- Message-queue traffic to the RM works: sends return OK and the RM answers.
- `KVM_CREATE_VM` succeeds, a VMID is allocated, and the DTB memory parcel is
  lent (`mem_handle=2`) in later runs.

## Firmware facts found along the way

### Hypercall ABI

- The SMCCC vendor-hyp wrapper the series uses (function ID in x0, e.g. 0x8000)
  returns -1 (unimplemented). This firmware only implements the native ABI:
  `hvc #0x6xxx`, arguments in x0-x7, results in x0-x7 (per gunyah_api.md).
- Native numbers used: identify 0x6000, bell_send 0x6012, bell_set_mask 0x6015,
  msgq_send 0x601B, msgq_recv 0x601C, addrspace_map 0x602B,
  addrspace_unmap 0x602C, memextent_donate **0x6033** (the series' 0x8061 has
  no native counterpart), vcpu_run 0x6065.
- Identify returned x0 = 0x5100000000008001 (top byte 0x51 = 'Q', API version 1).
- The information-area hypercalls (0x606a / 0x6077) are unimplemented.

### Where the RM capability IDs come from

- The RM capability IDs (unit address and `reg` of
  `qcom,resource-manager-rpc@...`) change on every boot. They are not in any
  vendor DTS: the RM generates the node as a DT overlay. The interrupts are
  fixed (SPI 928 and 929, edge rising).
- Stock ABL (LinuxLoader, extracted from abl.img) gets the overlay with one SCM
  SIP call: SMC id 0x02000609 (service 0x06, command 0x09), no arguments.
  Result word 1 is the physical address of a 4 KiB "HypInfo" page:
  - +0x00 u32 magic 0xC06B0071
  - +0x04 u32 version (1 on this phone)
  - +0x14 u32 number of DTBOs (ABL supports at most 4)
  - +0x18 + 16 * i: u64 physical address of DTBO i, then a second u64
    (meaning not determined)
  - the DTBO on this phone is 1536 bytes; fragment@0 holds `/hypervisor`.
- `qcom_scm`'s "is call available" query says 0 for this call even though it
  works, so it is unreliable for vendor-private calls.
- The vendor DT uses 3-cell GIC interrupt specifiers; mainline SM8550's GIC has
  `#interrupt-cells = <4>`. The overlay's `interrupts` must be widened
  (append a 0 phandle cell) or the second interrupt is not found.
- `gunyah_platform_probe_capability()` stored `platform_get_irq()` in an
  unsigned field, hiding the real error; fixed with a signed temporary.

## The RM conversation observed (message IDs from the public RM source)

| Message | ID | Result on this phone |
|---|---|---|
| VM_ALLOC_VMID | 0x56000001 | OK |
| MEM_LEND (DTB parcel) | 0x51000012 | denied (3) in one run, accepted in later runs |
| VM_CONFIG_IMAGE | 0x56000009 | accepted in one run; ARGUMENT_INVALID (6) with auth=2; auth=0 was reported as "unimplemented" (not captured in the logs kept here, needs re-checking) |
| VM_SET_DEMAND_PAGING | 0x56000033 | unimplemented (error 0xFFFFFFFF) |
| VM_DEALLOCATE | 0x56000002 | unimplemented |

QEMU's `KVM_RUN` failed with `EPERM` while the lend was denied, then with
`EOPNOTSUPP` once demand paging was the failing step.

## Why it stops here

The series' memory model needs `VM_SET_DEMAND_PAGING` and
`VM_SET_ADDRESS_LAYOUT`: in demand-paging mode the RM gives the host a memory
extent with donate rights, which is how pages reach the guest
(`memextent_donate`). In the public RM history both messages first appear in
October 2025. The phone's RM only has the older (2023-era) flow. Without them
the host has no way to donate guest memory, and starting a VM anyway risks a
NULL dereference in the fault path, so the debug patch here stops before
`VM_START`.

The EL2-exit route used by Qualcomm's own U-Boot work (an SCM call that leaves
Gunyah) was not usable: the SCM query for service 0x01 / command 0x21 said
"not available". Since that query proved unreliable, the call itself was never
executed.

Only an OEM firmware update that carries a newer Gunyah/RM would change this.

## Things tried that did not pan out

- UEFI side (Mu-Silicium): `BootShim` leaves x0 (ABL's DT pointer) untouched,
  `ModuleEntryPoint` overwrites it; a SEC patch to save it was designed but
  never built because the Linux-side SCM loader made it unnecessary. A
  read-only EFI scanner for the live DT never produced visible output on
  Mu-Silicium's console and was dropped.
- Making `VM_SET_DEMAND_PAGING` / `VM_SET_ADDRESS_LAYOUT` non-fatal (debug
  patch) to see how far the old RM flow goes: it reached `VM_CONFIG_IMAGE`
  and was rejected there.

## What is on this branch

- `arch/arm64/gunyah/gunyah_hypercall.c`: native `hvc` versions of all
  hypercalls, plus debug prints in identify.
- `arch/arm64/kvm/gunyah.c`: the unlocked-ioctl stub, optional demand paging
  (debug), resource dump and stop before VM_START, DTB/memslot layout print.
- `drivers/virt/gunyah/hypinfo_dbg.c` (new): SCM call, HypInfo parsing,
  fragment@0 overlay with 3-to-4-cell interrupt widening.
- `drivers/virt/gunyah/rsc_mgr.c` (and related): vendor compatible, irq error
  handling fix, RPC debug prints.
- `drivers/firmware/qcom/qcom_scm.c`: debug probes for the SCM availability
  queries.
- `arch/arm64/boot/dts/qcom/sm8550-oneplus-salami.dts`, `gunyah_qcom.c`,
  `Makefile`: small changes, see the diff.

## Clean-up needed before showing this to anyone

- The `DEBUG: QC_HYP build` UID shortcut in `arch_is_gunyah_guest()`.
- Debug prints in `rsc_mgr.c`, `gunyah_hypercall.c` and `gunyah.c`.
- The SCM probes in `qcom_scm.c`.
- The loader keeps only fragment@0 and deletes the others; the other
  fragments target nodes mainline does not have.

## If the firmware ever changes

Boot a build from this branch and check `dmesg | grep -i gunyah`. The first
RM requests are logged; `VM_SET_DEMAND_PAGING` (0x56000033) and
`VM_SET_ADDRESS_LAYOUT` (0x56000034) are the ones to watch for. Remove the
resource-dump stop in `gunyah_vm_start()` once they succeed.
