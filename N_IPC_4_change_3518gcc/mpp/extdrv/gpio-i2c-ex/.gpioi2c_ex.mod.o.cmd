cmd_/opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/mpp/extdrv/gpio-i2c-ex/gpioi2c_ex.mod.o := arm-hisiv100nptl-linux-gcc -Wp,-MD,/opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/mpp/extdrv/gpio-i2c-ex/.gpioi2c_ex.mod.o.d  -nostdinc -isystem /opt/hisi-linux-nptl/arm-hisiv100-linux/bin/../lib/gcc/arm-hisiv100-linux-uclibcgnueabi/4.4.1/include -I/opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include -Iarch/arm/include/generated -Iinclude  -include include/generated/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-hi3518/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -marm -fno-dwarf2-cfi-asm -fno-omit-frame-pointer -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -D__LINUX_ARM_ARCH__=5 -march=armv5te -mtune=arm9tdmi -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -fno-omit-frame-pointer -fno-optimize-sibling-calls -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DHI_XXXX  -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(gpioi2c_ex.mod)"  -D"KBUILD_MODNAME=KBUILD_STR(gpioi2c_ex)" -DMODULE  -c -o /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/mpp/extdrv/gpio-i2c-ex/gpioi2c_ex.mod.o /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/mpp/extdrv/gpio-i2c-ex/gpioi2c_ex.mod.c

source_/opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/mpp/extdrv/gpio-i2c-ex/gpioi2c_ex.mod.o := /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/mpp/extdrv/gpio-i2c-ex/gpioi2c_ex.mod.c

deps_/opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/mpp/extdrv/gpio-i2c-ex/gpioi2c_ex.mod.o := \
    $(wildcard include/config/module/unload.h) \
  include/linux/module.h \
    $(wildcard include/config/symbol/prefix.h) \
    $(wildcard include/config/sysfs.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/kallsyms.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/tracepoints.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/event/tracing.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
    $(wildcard include/config/constructors.h) \
    $(wildcard include/config/debug/set/module/ronx.h) \
  include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/types.h \
  include/asm-generic/int-ll64.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/linux/posix_types.h \
  include/linux/stddef.h \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/posix_types.h \
  include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
  include/linux/const.h \
  include/linux/stat.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/stat.h \
  include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
  include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/spinlock/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/ring/buffer.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/compaction.h) \
  /opt/hisi-linux-nptl/arm-hisiv100-linux/bin/../lib/gcc/arm-hisiv100-linux-uclibcgnueabi/4.4.1/include/stdarg.h \
  include/linux/linkage.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/linkage.h \
  include/linux/bitops.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/bitops.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/system.h \
    $(wildcard include/config/function/graph/tracer.h) \
    $(wildcard include/config/cpu/32v6k.h) \
    $(wildcard include/config/cpu/xsc3.h) \
    $(wildcard include/config/cpu/fa526.h) \
    $(wildcard include/config/arch/has/barriers.h) \
    $(wildcard include/config/arm/dma/mem/bufferable.h) \
    $(wildcard include/config/cpu/sa1100.h) \
    $(wildcard include/config/cpu/sa110.h) \
    $(wildcard include/config/cpu/v6.h) \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  include/linux/typecheck.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/irqflags.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/arm/thumb.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/hwcap.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/outercache.h \
    $(wildcard include/config/outer/cache/sync.h) \
    $(wildcard include/config/outer/cache.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/memory.h \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/page/offset.h) \
    $(wildcard include/config/thumb2/kernel.h) \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/dram/size.h) \
    $(wildcard include/config/dram/base.h) \
    $(wildcard include/config/have/tcm.h) \
    $(wildcard include/config/arm/patch/phys/virt.h) \
    $(wildcard include/config/arm/patch/phys/virt/16bit.h) \
  arch/arm/mach-hi3518/include/mach/memory.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/sizes.h \
  include/asm-generic/sizes.h \
  include/asm-generic/memory_model.h \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/sparsemem.h) \
  include/asm-generic/cmpxchg-local.h \
  include/asm-generic/cmpxchg.h \
  include/asm-generic/bitops/non-atomic.h \
  include/asm-generic/bitops/fls64.h \
  include/asm-generic/bitops/sched.h \
  include/asm-generic/bitops/hweight.h \
  include/asm-generic/bitops/arch_hweight.h \
  include/asm-generic/bitops/const_hweight.h \
  include/asm-generic/bitops/lock.h \
  include/asm-generic/bitops/le.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/linux/swab.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/swab.h \
  include/linux/byteorder/generic.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/linux/printk.h \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
  include/linux/init.h \
    $(wildcard include/config/hotplug.h) \
  include/linux/dynamic_debug.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/debug/bugverbose.h) \
  include/asm-generic/bug.h \
    $(wildcard include/config/generic/bug/relative/pointers.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/div64.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/cache.h \
    $(wildcard include/config/arm/l1/cache/shift.h) \
    $(wildcard include/config/aeabi.h) \
  include/linux/seqlock.h \
  include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/generic/lockbreak.h) \
    $(wildcard include/config/preempt.h) \
    $(wildcard include/config/debug/lock/alloc.h) \
  include/linux/preempt.h \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  include/linux/thread_info.h \
    $(wildcard include/config/compat.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/thread_info.h \
    $(wildcard include/config/arm/thumbee.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/fpstate.h \
    $(wildcard include/config/vfpv3.h) \
    $(wildcard include/config/iwmmxt.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/domain.h \
    $(wildcard include/config/io/36.h) \
    $(wildcard include/config/cpu/use/domains.h) \
  include/linux/stringify.h \
  include/linux/bottom_half.h \
  include/linux/spinlock_types.h \
  include/linux/spinlock_types_up.h \
  include/linux/lockdep.h \
    $(wildcard include/config/lockdep.h) \
    $(wildcard include/config/lock/stat.h) \
    $(wildcard include/config/prove/rcu.h) \
  include/linux/rwlock_types.h \
  include/linux/spinlock_up.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/processor.h \
    $(wildcard include/config/have/hw/breakpoint.h) \
    $(wildcard include/config/arm/errata/754327.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/hw_breakpoint.h \
  include/linux/rwlock.h \
  include/linux/spinlock_api_up.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/atomic.h \
    $(wildcard include/config/generic/atomic64.h) \
  include/asm-generic/atomic64.h \
  include/asm-generic/atomic-long.h \
  include/linux/math64.h \
  include/linux/kmod.h \
  include/linux/gfp.h \
    $(wildcard include/config/kmemcheck.h) \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/zone/dma32.h) \
  include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/arch/populates/node/map.h) \
    $(wildcard include/config/flat/node/mem/map.h) \
    $(wildcard include/config/cgroup/mem/res/ctlr.h) \
    $(wildcard include/config/no/bootmem.h) \
    $(wildcard include/config/have/memory/present.h) \
    $(wildcard include/config/have/memoryless/nodes.h) \
    $(wildcard include/config/need/node/memmap/size.h) \
    $(wildcard include/config/need/multiple/nodes.h) \
    $(wildcard include/config/have/arch/early/pfn/to/nid.h) \
    $(wildcard include/config/sparsemem/extreme.h) \
    $(wildcard include/config/have/arch/pfn/valid.h) \
    $(wildcard include/config/nodes/span/other/nodes.h) \
    $(wildcard include/config/holes/in/zone.h) \
    $(wildcard include/config/arch/has/holes/memorymodel.h) \
  include/linux/wait.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/current.h \
  include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
  include/linux/nodemask.h \
  include/linux/bitmap.h \
  include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/string.h \
  include/linux/pageblock-flags.h \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/hugetlb/page/size/variable.h) \
  include/generated/bounds.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/page.h \
    $(wildcard include/config/cpu/copy/v3.h) \
    $(wildcard include/config/cpu/copy/v4wt.h) \
    $(wildcard include/config/cpu/copy/v4wb.h) \
    $(wildcard include/config/cpu/copy/feroceon.h) \
    $(wildcard include/config/cpu/copy/fa.h) \
    $(wildcard include/config/cpu/xscale.h) \
    $(wildcard include/config/cpu/copy/v6.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/glue.h \
  include/asm-generic/getorder.h \
  include/linux/memory_hotplug.h \
    $(wildcard include/config/memory/hotremove.h) \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
  include/linux/notifier.h \
  include/linux/errno.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/errno.h \
  include/asm-generic/errno.h \
  include/asm-generic/errno-base.h \
  include/linux/mutex.h \
    $(wildcard include/config/debug/mutexes.h) \
    $(wildcard include/config/have/arch/mutex/cpu/relax.h) \
  include/linux/rwsem.h \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  include/linux/rwsem-spinlock.h \
  include/linux/srcu.h \
  include/linux/topology.h \
    $(wildcard include/config/sched/smt.h) \
    $(wildcard include/config/sched/mc.h) \
    $(wildcard include/config/sched/book.h) \
    $(wildcard include/config/use/percpu/numa/node/id.h) \
  include/linux/cpumask.h \
    $(wildcard include/config/cpumask/offstack.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/debug/per/cpu/maps.h) \
    $(wildcard include/config/disable/obsolete/cpumask/functions.h) \
  include/linux/smp.h \
    $(wildcard include/config/use/generic/smp/helpers.h) \
  include/linux/percpu.h \
    $(wildcard include/config/need/per/cpu/embed/first/chunk.h) \
    $(wildcard include/config/need/per/cpu/page/first/chunk.h) \
    $(wildcard include/config/have/setup/per/cpu/area.h) \
  include/linux/pfn.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/percpu.h \
  include/asm-generic/percpu.h \
  include/linux/percpu-defs.h \
    $(wildcard include/config/debug/force/weak/per/cpu.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/topology.h \
  include/asm-generic/topology.h \
  include/linux/mmdebug.h \
    $(wildcard include/config/debug/vm.h) \
    $(wildcard include/config/debug/virtual.h) \
  include/linux/workqueue.h \
    $(wildcard include/config/debug/objects/work.h) \
    $(wildcard include/config/freezer.h) \
  include/linux/timer.h \
    $(wildcard include/config/timer/stats.h) \
    $(wildcard include/config/debug/objects/timers.h) \
  include/linux/ktime.h \
    $(wildcard include/config/ktime/scalar.h) \
  include/linux/jiffies.h \
  include/linux/timex.h \
  include/linux/param.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/param.h \
    $(wildcard include/config/hz.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/timex.h \
  arch/arm/mach-hi3518/include/mach/timex.h \
  include/linux/debugobjects.h \
    $(wildcard include/config/debug/objects.h) \
    $(wildcard include/config/debug/objects/free.h) \
  include/linux/sysctl.h \
  include/linux/rcupdate.h \
    $(wildcard include/config/rcu/torture/test.h) \
    $(wildcard include/config/tree/rcu.h) \
    $(wildcard include/config/tree/preempt/rcu.h) \
    $(wildcard include/config/preempt/rcu.h) \
    $(wildcard include/config/no/hz.h) \
    $(wildcard include/config/tiny/rcu.h) \
    $(wildcard include/config/tiny/preempt/rcu.h) \
    $(wildcard include/config/debug/objects/rcu/head.h) \
    $(wildcard include/config/preempt/rt.h) \
  include/linux/completion.h \
  include/linux/rcutiny.h \
  include/linux/elf.h \
  include/linux/elf-em.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/elf.h \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/user.h \
  include/linux/kobject.h \
  include/linux/sysfs.h \
  include/linux/kobject_ns.h \
  include/linux/kref.h \
  include/linux/moduleparam.h \
    $(wildcard include/config/alpha.h) \
    $(wildcard include/config/ia64.h) \
    $(wildcard include/config/ppc64.h) \
  include/linux/tracepoint.h \
  include/linux/jump_label.h \
    $(wildcard include/config/jump/label.h) \
  /opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/osdrv/kernel/linux-3.0.y/arch/arm/include/asm/module.h \
    $(wildcard include/config/arm/unwind.h) \
  include/trace/events/module.h \
  include/trace/define_trace.h \
  include/linux/vermagic.h \
  include/generated/utsrelease.h \

/opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/mpp/extdrv/gpio-i2c-ex/gpioi2c_ex.mod.o: $(deps_/opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/mpp/extdrv/gpio-i2c-ex/gpioi2c_ex.mod.o)

$(deps_/opt/yiview/work/Hi3518C_V100R001C01SPC040/01.software/board/Hi3518_SDK_V1.0.4.0/mpp/extdrv/gpio-i2c-ex/gpioi2c_ex.mod.o):
