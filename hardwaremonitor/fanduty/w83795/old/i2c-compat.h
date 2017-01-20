#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16)
#error This driver is for kernel versions 2.6.16 and later
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 21)
/* Simplified version for compatibility */
struct i2c_board_info {
	char		type[I2C_NAME_SIZE];
	unsigned short	flags;
	unsigned short	addr;
};
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 25)
/* Some older kernels have a different, useless struct i2c_device_id */
#define i2c_device_id i2c_device_id_compat
struct i2c_device_id {
	char name[I2C_NAME_SIZE];
	kernel_ulong_t driver_data      /* Data private to the driver */
			__attribute__((aligned(sizeof(kernel_ulong_t))));
};
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 32)
static unsigned short empty_i2c[] =  { I2C_CLIENT_END };
static struct i2c_client_address_data addr_data = {
	.normal_i2c	= normal_i2c,
	.probe		= empty_i2c,
	.ignore		= empty_i2c,
};
#endif

/* Red Hat EL5 includes backports of these functions, so we can't redefine
 * our own. */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 24)
#if !(defined RHEL_MAJOR && RHEL_MAJOR == 5 && RHEL_MINOR >= 5)
static inline int strict_strtoul(const char *cp, unsigned int base,
				 unsigned long *res)
{
	*res = simple_strtoul(cp, NULL, base);
	return 0;
}

static inline int strict_strtol(const char *cp, unsigned int base, long *res)
{
	*res = simple_strtol(cp, NULL, base);
	return 0;
}
#endif
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 28)
#define DIV_ROUND_CLOSEST(x, divisor)(			\
{							\
	typeof(divisor) __divisor = divisor;		\
	(((x) + ((__divisor) / 2)) / (__divisor));	\
}							\
)
#endif
