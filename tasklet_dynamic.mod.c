#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0x3b55f6e5, "module_layout" },
	{ 0x6117b17, "device_destroy" },
	{ 0x37a0cba, "kfree" },
	{ 0x82072614, "tasklet_kill" },
	{ 0xb345619, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x1513fae6, "class_destroy" },
	{ 0x73c5263, "sysfs_remove_file_ns" },
	{ 0x9121d184, "kobject_put" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x9545af6d, "tasklet_init" },
	{ 0xe5b84659, "kmem_cache_alloc_trace" },
	{ 0x5c121357, "kmalloc_caches" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x19df047e, "sysfs_create_file_ns" },
	{ 0x8ef9e6d6, "kobject_create_and_add" },
	{ 0xf13a216c, "kernel_kobj" },
	{ 0x11324c7c, "device_create" },
	{ 0xad3d5f8a, "__class_create" },
	{ 0x8256103a, "cdev_add" },
	{ 0x5157cf63, "cdev_init" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x9ce8d77f, "vector_irq" },
	{ 0x3546cd5d, "irq_to_desc" },
	{ 0xfaef0ed, "__tasklet_schedule" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0xc5850110, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "67FDFC587DD67051E655B16");
