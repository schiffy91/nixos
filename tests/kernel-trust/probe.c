#include <linux/module.h>

static int __init probe_init(void)
{
    pr_info("module trust fixture loaded\n");
    return 0;
}

static void __exit probe_exit(void) { }
module_init(probe_init);
module_exit(probe_exit);
MODULE_LICENSE("GPL");
