// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/slab.h>
#include <linux/io.h>
#include <linux/arm-smccc.h>
#include <linux/module.h>
#include <linux/gunyah.h>
#include <linux/uuid.h>

/* {c1d58fcd-a453-5fdb-9265-ce36673d5f14} */
static const uuid_t GUNYAH_UUID = UUID_INIT(0xc1d58fcd, 0xa453, 0x5fdb, 0x92,
					    0x65, 0xce, 0x36, 0x67, 0x3d, 0x5f,
					    0x14);

bool arch_is_gunyah_guest(void)
{
	struct arm_smccc_res res;
	uuid_t uuid;
	u32 *up;

	arm_smccc_1_1_invoke(ARM_SMCCC_VENDOR_HYP_CALL_UID_FUNC_ID, &res);
	{
		struct arm_smccc_res r;
		u32 id0 = ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_64,
					     ARM_SMCCC_OWNER_VENDOR_HYP, 0x0000);
		u32 id8 = ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_64,
					     ARM_SMCCC_OWNER_VENDOR_HYP, 0x8000);
		arm_smccc_1_1_hvc(id0, &r);
		pr_info("gunyah dbg: hvc fn0000 -> %lx %lx %lx %lx\n", r.a0, r.a1, r.a2, r.a3);
		arm_smccc_1_1_smc(id0, &r);
		pr_info("gunyah dbg: smc fn0000 -> %lx %lx %lx %lx\n", r.a0, r.a1, r.a2, r.a3);
		arm_smccc_1_1_smc(id8, &r);
		pr_info("gunyah dbg: smc fn8000 -> %lx %lx %lx %lx\n", r.a0, r.a1, r.a2, r.a3);
	}

	up = (u32 *)&uuid.b[0];
	up[0] = lower_32_bits(res.a0);
	up[1] = lower_32_bits(res.a1);
	up[2] = lower_32_bits(res.a2);
	up[3] = lower_32_bits(res.a3);

	if (res.a0 == 0x19bd54bd && res.a1 == 0x0b37571b &&
	    res.a2 == 0x946f609b && res.a3 == 0x54539de6)
		return true; /* DEBUG: QC_HYP build */
	return uuid_equal(&uuid, &GUNYAH_UUID);
}
EXPORT_SYMBOL_GPL(arch_is_gunyah_guest);

#define GUNYAH_HYPERCALL(fn)                                      \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_64, \
			   ARM_SMCCC_OWNER_VENDOR_HYP, fn)

/* clang-format off */
#define GUNYAH_HYPERCALL_HYP_IDENTIFY		GUNYAH_HYPERCALL(0x0000)
#define GUNYAH_HYPERCALL_BELL_SEND		GUNYAH_HYPERCALL(0x8012)
#define GUNYAH_HYPERCALL_BELL_SET_MASK		GUNYAH_HYPERCALL(0x8015)
#define GUNYAH_HYPERCALL_MSGQ_SEND		GUNYAH_HYPERCALL(0x801B)
#define GUNYAH_HYPERCALL_MSGQ_RECV		GUNYAH_HYPERCALL(0x801C)
#define GUNYAH_HYPERCALL_ADDRSPACE_MAP		GUNYAH_HYPERCALL(0x802B)
#define GUNYAH_HYPERCALL_ADDRSPACE_UNMAP	GUNYAH_HYPERCALL(0x802C)
#define GUNYAH_HYPERCALL_MEMEXTENT_DONATE	GUNYAH_HYPERCALL(0x8061)
#define GUNYAH_HYPERCALL_VCPU_RUN		GUNYAH_HYPERCALL(0x8065)
/* clang-format on */

static void gunyah_dbg_info_area(void)
{
	register unsigned long x0 asm("x0") = 0;
	register unsigned long x1 asm("x1") = 0;
	register unsigned long x2 asm("x2") = 0;
	u8 *buf;
	int i;

	asm volatile("hvc #0x606a"
		     : "+r" (x0), "+r" (x1), "+r" (x2)
		     :
		     : "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11",
		       "x12", "x13", "x14", "x15", "x16", "x17", "memory");
	pr_info("gunyah dbg: info area err=%lx ipa=%lx size=%lx\n", x0, x1, x2);
	if (x0)
		return;

	buf = kzalloc(4096, GFP_KERNEL);
	if (!buf)
		return;
	for (i = 0; i < 32; i++) {
		register unsigned long a0 asm("x0") = i;
		register unsigned long a1 asm("x1") = (unsigned long)buf;
		register unsigned long a2 asm("x2") = 4096;
		register unsigned long a3 asm("x3") = 0;

		asm volatile("hvc #0x6077"
			     : "+r" (a0), "+r" (a1), "+r" (a2), "+r" (a3)
			     :
			     : "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11",
			       "x12", "x13", "x14", "x15", "x16", "x17", "memory");
		if (a0) {
			pr_info("gunyah dbg: entry %d err=%lx\n", i, a0);
			break;
		}
		pr_info("gunyah dbg: entry %d size=%lu type=%lx\n", i, a1, a2);
		print_hex_dump(KERN_INFO, "gunyah dbg:   ", DUMP_PREFIX_OFFSET,
			       16, 8, buf, min(a1, 96UL), false);
	}
	kfree(buf);
}

/**
 * gunyah_hypercall_hyp_identify() - Returns build information and feature flags
 *                               supported by Gunyah.
 * @hyp_identity: filled by the hypercall with the API info and feature flags.
 */
void gunyah_hypercall_hyp_identify(
	struct gunyah_hypercall_hyp_identify_resp *hyp_identity)
{
	register unsigned long x0 asm("x0") = 0;
	register unsigned long x1 asm("x1") = 0;
	register unsigned long x2 asm("x2") = 0;
	register unsigned long x3 asm("x3") = 0;

	/* This firmware only implements the native hvc #imm convention */
	asm volatile("hvc #0x6000"
		     : "+r" (x0), "+r" (x1), "+r" (x2), "+r" (x3)
		     :
		     : "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12",
		       "x13", "x14", "x15", "x16", "x17", "memory");

	hyp_identity->api_info = x0;
	hyp_identity->flags[0] = x1;
	hyp_identity->flags[1] = x2;
	hyp_identity->flags[2] = x3;
	gunyah_dbg_info_area();
}
EXPORT_SYMBOL_GPL(gunyah_hypercall_hyp_identify);

/*
 * This firmware (an older Gunyah) only implements the native hypercall ABI:
 * "hvc #0x6xxx" with the arguments in x0-x7 and the results in x0-x7. The SMCCC
 * wrapper the series uses (function ID in x0, arguments in x1..) returns -1
 * (unimplemented). The native register layout is the SMCCC one shifted down by
 * one register. Numbers and layouts are from gunyah_api.md.
 */
struct gh_native_regs {
	unsigned long r[8];
};

#define GH_NATIVE_HVC(name, imm)						\
static __always_inline void name(struct gh_native_regs *g)			\
{										\
	register unsigned long x0 asm("x0") = g->r[0];				\
	register unsigned long x1 asm("x1") = g->r[1];				\
	register unsigned long x2 asm("x2") = g->r[2];				\
	register unsigned long x3 asm("x3") = g->r[3];				\
	register unsigned long x4 asm("x4") = g->r[4];				\
	register unsigned long x5 asm("x5") = g->r[5];				\
	register unsigned long x6 asm("x6") = g->r[6];				\
	register unsigned long x7 asm("x7") = g->r[7];				\
										\
	asm volatile("hvc #" #imm						\
		     : "+r" (x0), "+r" (x1), "+r" (x2), "+r" (x3),		\
		       "+r" (x4), "+r" (x5), "+r" (x6), "+r" (x7)		\
		     :								\
		     : "x8", "x9", "x10", "x11", "x12", "x13", "x14",		\
		       "x15", "x16", "x17", "memory");				\
	g->r[0] = x0; g->r[1] = x1; g->r[2] = x2; g->r[3] = x3;			\
	g->r[4] = x4; g->r[5] = x5; g->r[6] = x6; g->r[7] = x7;			\
}

GH_NATIVE_HVC(gh_hvc_bell_send,         0x6012)
GH_NATIVE_HVC(gh_hvc_bell_set_mask,     0x6015)
GH_NATIVE_HVC(gh_hvc_msgq_send,         0x601B)
GH_NATIVE_HVC(gh_hvc_msgq_recv,         0x601C)
GH_NATIVE_HVC(gh_hvc_addrspace_map,     0x602B)
GH_NATIVE_HVC(gh_hvc_addrspace_unmap,   0x602C)
GH_NATIVE_HVC(gh_hvc_memextent_donate,  0x6033)	/* NOT 0x6061: that is the SMCCC-era ID */
GH_NATIVE_HVC(gh_hvc_vcpu_run,          0x6065)

/**
 * gunyah_hypercall_bell_send() - Assert a gunyah doorbell
 * @capid: capability ID of the doorbell
 * @new_flags: bits to set on the doorbell
 * @old_flags: Filled with the bits set before the send call if return value is GUNYAH_ERROR_OK
 */
enum gunyah_error gunyah_hypercall_bell_send(u64 capid, u64 new_flags, u64 *old_flags)
{
	struct gh_native_regs g = { .r = { capid, new_flags, 0 } };

	gh_hvc_bell_send(&g);
	if (g.r[0] == GUNYAH_ERROR_OK && old_flags)
		*old_flags = g.r[1];

	return g.r[0];
}
EXPORT_SYMBOL_GPL(gunyah_hypercall_bell_send);

/**
 * gunyah_hypercall_bell_set_mask() - Set masks on a Gunyah doorbell
 * @capid: capability ID of the doorbell
 * @enable_mask: which bits trigger the receiver interrupt
 * @ack_mask: which bits are automatically acknowledged when the receiver
 *            interrupt is ack'd
 */
enum gunyah_error gunyah_hypercall_bell_set_mask(u64 capid, u64 enable_mask, u64 ack_mask)
{
	struct gh_native_regs g = { .r = { capid, enable_mask, ack_mask, 0 } };

	gh_hvc_bell_set_mask(&g);

	return g.r[0];
}
EXPORT_SYMBOL_GPL(gunyah_hypercall_bell_set_mask);

/**
 * gunyah_hypercall_msgq_send() - Send a buffer on a message queue
 * @capid: capability ID of the message queue to add message
 * @size: Size of @buff
 * @buff: Address of buffer to send
 * @tx_flags: See GUNYAH_HYPERCALL_MSGQ_TX_FLAGS_*
 * @ready: If the send was successful, ready is filled with true if more
 *         messages can be sent on the queue. If false, then the tx IRQ will
 *         be raised in future when send can succeed.
 */
enum gunyah_error gunyah_hypercall_msgq_send(u64 capid, size_t size, void *buff,
					     u64 tx_flags, bool *ready)
{
	struct gh_native_regs g = { .r = { capid, size, (uintptr_t)buff, tx_flags, 0 } };

	gh_hvc_msgq_send(&g);
	if (g.r[0] == GUNYAH_ERROR_OK)
		*ready = !!g.r[1];

	return g.r[0];
}
EXPORT_SYMBOL_GPL(gunyah_hypercall_msgq_send);

/**
 * gunyah_hypercall_msgq_recv() - Receive a buffer from a message queue
 * @capid: capability ID of the message queue to receive from
 * @buff: Address of buffer to copy received data into
 * @size: Size of @buff
 * @recv_size: If the receive was successful, recv_size is filled with the
 *             size of data received. Will be <= size.
 * @ready: If the receive was successful, ready is filled with true if more
 *         messages are ready to be received on the queue. If false, then the
 *         rx IRQ will be raised in future when recv can succeed.
 */
enum gunyah_error gunyah_hypercall_msgq_recv(u64 capid, void *buff, size_t size,
					     size_t *recv_size, bool *ready)
{
	struct gh_native_regs g = { .r = { capid, (uintptr_t)buff, size, 0 } };

	gh_hvc_msgq_recv(&g);
	if (g.r[0] == GUNYAH_ERROR_OK) {
		*recv_size = g.r[1];
		*ready = !!g.r[2];
	}

	return g.r[0];
}
EXPORT_SYMBOL_GPL(gunyah_hypercall_msgq_recv);

/**
 * gunyah_hypercall_addrspace_map() - Add memory to an address space from a memory extent
 * @capid: Address space capability ID
 * @extent_capid: Memory extent capability ID
 * @vbase: location in address space
 * @extent_attrs: Attributes for the memory
 * @flags: Flags for address space mapping
 * @offset: Offset into memory extent (physical address of memory)
 * @size: Size of memory to map; must be page-aligned
 */
enum gunyah_error gunyah_hypercall_addrspace_map(u64 capid, u64 extent_capid, u64 vbase,
					u32 extent_attrs, u32 flags, u64 offset, u64 size)
{
	struct gh_native_regs g = { .r = { capid, extent_capid, vbase, extent_attrs,
					   flags, offset, size, 0 } };

	gh_hvc_addrspace_map(&g);

	return g.r[0];
}
EXPORT_SYMBOL_GPL(gunyah_hypercall_addrspace_map);

/**
 * gunyah_hypercall_addrspace_unmap() - Remove memory from an address space
 * @capid: Address space capability ID
 * @extent_capid: Memory extent capability ID
 * @vbase: location in address space
 * @flags: Flags for address space mapping
 * @offset: Offset into memory extent (physical address of memory)
 * @size: Size of memory to map; must be page-aligned
 */
enum gunyah_error gunyah_hypercall_addrspace_unmap(u64 capid, u64 extent_capid, u64 vbase,
					u32 flags, u64 offset, u64 size)
{
	struct gh_native_regs g = { .r = { capid, extent_capid, vbase, flags, offset, size, 0, 0 } };

	gh_hvc_addrspace_unmap(&g);

	return g.r[0];
}
EXPORT_SYMBOL_GPL(gunyah_hypercall_addrspace_unmap);

/**
 * gunyah_hypercall_memextent_donate() - Donate memory from one memory extent to another
 * @options: donate options
 * @from_capid: Memory extent capability ID to donate from
 * @to_capid: Memory extent capability ID to donate to
 * @offset: Offset into memory extent (physical address of memory)
 * @size: Size of memory to donate; must be page-aligned
 */
enum gunyah_error gunyah_hypercall_memextent_donate(u32 options, u64 from_capid, u64 to_capid,
					    u64 offset, u64 size)
{
	struct gh_native_regs g = { .r = { options, from_capid, to_capid, offset, size, 0 } };

	gh_hvc_memextent_donate(&g);

	return g.r[0];
}
EXPORT_SYMBOL_GPL(gunyah_hypercall_memextent_donate);

/**
 * gunyah_hypercall_vcpu_run() - Donate CPU time to a vcpu
 * @capid: capability ID of the vCPU to run
 * @resume_data: Array of 3 state-specific resume data
 * @resp: Filled reason why vCPU exited when return value is GUNYAH_ERROR_OK
 *
 * See also:
 * https://github.com/quic/gunyah-hypervisor/blob/develop/docs/api/gunyah_api.md#run-a-proxy-scheduled-vcpu-thread
 */
enum gunyah_error
gunyah_hypercall_vcpu_run(u64 capid, unsigned long *resume_data,
			  struct gunyah_hypercall_vcpu_run_resp *resp)
{
	struct gh_native_regs g = { .r = { capid, resume_data[0], resume_data[1],
					   resume_data[2], 0 } };

	gh_hvc_vcpu_run(&g);
	if (g.r[0] == GUNYAH_ERROR_OK) {
		resp->sized_state = g.r[1];
		resp->state_data[0] = g.r[2];
		resp->state_data[1] = g.r[3];
		resp->state_data[2] = g.r[4];
	}

	return g.r[0];
}
EXPORT_SYMBOL_GPL(gunyah_hypercall_vcpu_run);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Gunyah Hypervisor Hypercalls");
