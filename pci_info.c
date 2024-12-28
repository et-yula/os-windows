#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/pci.h>
#include <linux/string.h>
#include <linux/device.h>

#define PROC_NAME "pci_info"

static char buffer[1024];
static struct proc_dir_entry *entry;

ssize_t read_proc(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    struct pci_dev *pdev = NULL;
    int len = 0;

    if (*offset > 0) {
        return 0;
    }

    len += snprintf(buffer + len, sizeof(buffer) - len, "PCI Devices:\n");
    
    for_each_pci_dev(pdev) {
      len += snprintf(buffer + len, sizeof(buffer) - len,
                    "Vendor ID: %04x, Device ID: %04x, Vendor name: %s, Device name: %s\n",
                    pdev->vendor, pdev->device, pci_name(pdev), pci_name(pdev));
    }

    if (len >= sizeof(buffer)) {
        len = sizeof(buffer) - 1;
    }

    if (copy_to_user(buf, buffer, len)) {
        return -EFAULT;
    }

    *offset += len;
    return len;
}

static const struct proc_ops proc_fops = {
  .proc_read = read_proc
};

static int __init pci_info_init(void) {
    entry = proc_create(PROC_NAME, 0444, NULL, &proc_fops);
    if (!entry) {
        return -ENOMEM;
    }
    return 0;
}

static void __exit pci_info_exit(void) {
    proc_remove(entry);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("et-yula");
MODULE_DESCRIPTION("PCI info module");
module_init(pci_info_init);
module_exit(pci_info_exit);

