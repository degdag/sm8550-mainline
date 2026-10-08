// SPDX-License-Identifier: GPL-2.0-only
/*
 * EXPERIMENT. Fetch Qualcomm's "HypInfo" page the same way the stock ABL
 * (LinuxLoader.efi) does, and add the /hypervisor node from the DTBO it
 * carries to the live device tree, so the Gunyah drivers see the per-boot
 * resource-manager capability IDs.
 *
 * What the ABL does (read from its disassembly, so only the fields it reads
 * are known):
 *   - SCM call, service 0x06, command 0x09, no arguments (SMC id 0x02000609,
 *     SMC32 convention, SiP owner, yielding call). The result is the physical
 *     address of one 4 KiB page.
 *   - Page layout:
 *       +0x00  u32  magic 0xc06b0071
 *       +0x04  u32  version
 *       +0x14  u32  number of DTBOs (ABL supports at most 4)
 *       +0x18 + 16 * i : u64 physical address of DTBO i, u64 size of the area
 *   - The DTBO seen on this phone has five fragments. Only fragment@0 (the
 *     /hypervisor node) is applied here; the others target nodes mainline does
 *     not have (/soc/qcom,wdt@17410000, /soc/kryo-erp, /firmware/qcom_scm,
 *     &arch_timer) and would make the whole overlay fail.
 *
 * The vendor DTBO uses 3-cell GIC interrupt specifiers; they are widened to the
 * 4-cell form of mainline's GIC node before the overlay is applied.
 *
 * Needs CONFIG_OF_OVERLAY. The resource-manager node uses the vendor
 * compatible "qcom,resource-manager", so rsc_mgr.c needs that in its match
 * table.
 *
 * Reading a wrong address can fault the kernel: save your work before booting.
 */
#include <linux/arm-smccc.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/libfdt.h>
#include <linux/of.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <asm/byteorder.h>

#define HYPINFO_MAGIC		0xc06b0071
#define HYPINFO_MAX_DTBO	4
#define DTBO_MAX_SIZE		0x10000
#define HYPINFO_DUMP_DTBO	0	/* 1 = hex-dump each DTBO into dmesg */

struct dtbo_ref {
	u64 pa;
	u64 second;
};

/* Convert 3-cell GIC interrupt specifiers (type, num, flags) found in a vendor
 * overlay into the 4-cell form the mainline SM8550 GIC node uses (the 4th cell is
 * the PPI-partition phandle, 0 = none). Returns a new blob (caller frees) or NULL. */
static void *hypinfo_gic_irqs_to_4cell(const void *src, size_t size, size_t *outsize)
{
	size_t room = size + 4096;
	void *dst = kzalloc(room, GFP_KERNEL);
	int off, depth = 0, len;

	if (!dst)
		return NULL;
	if (fdt_open_into(src, dst, room)) {
		kfree(dst);
		return NULL;
	}
	for (off = 0; off >= 0; off = fdt_next_node(dst, off, &depth)) {
		const fdt32_t *p = fdt_getprop(dst, off, "interrupts", &len);
		unsigned int i, n;
		fdt32_t *buf;

		if (!p || len <= 0 || len % 12)
			continue;
		n = len / 12;
		buf = kmalloc_array(n, 16, GFP_KERNEL);
		if (!buf)
			continue;
		for (i = 0; i < n; i++) {
			buf[4 * i + 0] = p[3 * i + 0];
			buf[4 * i + 1] = p[3 * i + 1];
			buf[4 * i + 2] = p[3 * i + 2];
			buf[4 * i + 3] = cpu_to_fdt32(0);
		}
		/* p points into dst and is invalid after fdt_setprop; buf was copied first */
		if (fdt_setprop(dst, off, "interrupts", buf, n * 16))
			pr_warn("hypinfo: could not widen interrupts\n");
		kfree(buf);
	}
	fdt_pack(dst);
	*outsize = fdt_totalsize(dst);
	return dst;
}

static void hypinfo_apply_dtbo(int i, const struct dtbo_ref *d)
{
	u32 hdr[2], total;
	void *p, *fdt;
	int off, k, ret, ovcs_id = 0;
	char path[24];
	void *fixed;
	size_t fixed_sz;

	p = memremap(d->pa, sizeof(hdr), MEMREMAP_WB);
	if (!p) {
		pr_info("gunyah hypinfo: dtbo[%d] pa=%llx: memremap failed\n", i, d->pa);
		return;
	}
	memcpy(hdr, p, sizeof(hdr));
	memunmap(p);

	total = be32_to_cpu(hdr[1]);
	pr_info("gunyah hypinfo: dtbo[%d] pa=%llx fdt magic=%08x totalsize=%u\n",
		i, d->pa, be32_to_cpu(hdr[0]), total);
	if (be32_to_cpu(hdr[0]) != 0xd00dfeed || total < 64 || total > DTBO_MAX_SIZE)
		return;

	p = memremap(d->pa, total, MEMREMAP_WB);
	if (!p)
		return;
	if (HYPINFO_DUMP_DTBO)
		print_hex_dump(KERN_INFO, "gunyah hypdtbo: ", DUMP_PREFIX_OFFSET, 16, 1, p, total, false);
	fdt = kmemdup(p, total, GFP_KERNEL);
	memunmap(p);
	if (!fdt)
		return;

#ifdef CONFIG_OF_OVERLAY
	if (fdt_check_header(fdt)) {
		pr_info("gunyah hypinfo: dtbo[%d]: bad fdt header\n", i);
		goto out;
	}
	/* keep only fragment@0: the others target nodes mainline does not have */
	for (k = 1; k < 16; k++) {
		snprintf(path, sizeof(path), "/fragment@%d", k);
		off = fdt_path_offset(fdt, path);
		if (off >= 0)
			fdt_del_node(fdt, off);
	}
	off = fdt_path_offset(fdt, "/__fixups__");
	if (off >= 0)
		fdt_del_node(fdt, off);
	off = fdt_path_offset(fdt, "/__local_fixups__");
	if (off >= 0)
		fdt_del_node(fdt, off);

	/* the vendor GIC uses 3-cell interrupt specifiers, mainline SM8550's uses 4 */
	fixed = hypinfo_gic_irqs_to_4cell(fdt, fdt_totalsize(fdt), &fixed_sz);
	if (!fixed) {
		pr_info("gunyah hypinfo: dtbo[%d]: could not widen interrupts\n", i);
		goto out;
	}
	ret = of_overlay_fdt_apply(fixed, fixed_sz, &ovcs_id, NULL);
	kfree(fixed);
	pr_info("gunyah hypinfo: dtbo[%d] applied, ret=%d ovcs_id=%d\n", i, ret, ovcs_id);
out:
#else
	pr_info("gunyah hypinfo: CONFIG_OF_OVERLAY is off, not applying dtbo[%d]\n", i);
#endif
	kfree(fdt);
}

/* returns true if the page carries the HypInfo magic */
static bool hypinfo_parse(u64 pa)
{
	struct dtbo_ref d[HYPINFO_MAX_DTBO] = {};
	u32 w[6];
	u32 n = 0, i;
	void *p;

	p = memremap(pa, 0x1000, MEMREMAP_WB);
	if (!p) {
		pr_info("gunyah hypinfo: memremap of page %llx failed\n", pa);
		return false;
	}
	memcpy(w, p, sizeof(w));
	if (w[0] == HYPINFO_MAGIC) {
		n = min_t(u32, w[5], HYPINFO_MAX_DTBO);
		for (i = 0; i < n; i++)
			memcpy(&d[i], p + 0x18 + i * 16, sizeof(d[i]));
	}
	memunmap(p);

	pr_info("gunyah hypinfo: page pa=%llx magic=%08x (want %08x) version=%u count=%u\n",
		pa, w[0], HYPINFO_MAGIC, w[1], w[5]);
	for (i = 0; i < n; i++)
		hypinfo_apply_dtbo(i, &d[i]);
	return w[0] == HYPINFO_MAGIC;
}

static bool plausible_pa(u64 pa)
{
	return pa >= 0x80000000ULL && pa < 0x1000000000ULL && !(pa & 0xfff);
}

static int __init hypinfo_dbg_init(void)
{
	static const u32 ids[] = { 0x02000609, 0x42000609 };
	struct arm_smccc_res r;
	int i, c, tries;

	for (i = 0; i < ARRAY_SIZE(ids); i++) {
		u64 cand[3];

		tries = 0;
		do {
			arm_smccc_smc(ids[i], 0, 0, 0, 0, 0, 0, 0, &r);
		} while (r.a0 == 1 && ++tries < 100);	/* 1 = interrupted: call again */
		pr_info("gunyah hypinfo: smc %08x -> a0=%lx a1=%lx a2=%lx a3=%lx\n",
			ids[i], r.a0, r.a1, r.a2, r.a3);
		if (r.a0)
			continue;

		cand[0] = r.a1;
		cand[1] = r.a2;
		cand[2] = r.a3;
		for (c = 0; c < 3; c++) {
			if (plausible_pa(cand[c]) && hypinfo_parse(cand[c]))
				return 0;
		}
	}
	pr_info("gunyah hypinfo: no HypInfo page found\n");
	return 0;
}
late_initcall(hypinfo_dbg_init);
