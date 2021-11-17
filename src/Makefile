ifndef NAUT_CONFIG_RISCV_HOST
obj-y += \
	nautilus/ \
	arch/ \
	asm/ \
	dev/ \
        fs/  \
        gc/  \
	rt/ \
        net/ \
	acpi/ \
	test/  
endif

obj-$(NAUT_CONFIG_RISCV_HOST) += \
	nautilus/ \
	arch/ \
	dev/

obj-$(NAUT_CONFIG_CXX_SUPPORT) += cxx/
obj-$(NAUT_CONFIG_LEGION_RT) += legion_runtime/
obj-$(NAUT_CONFIG_LOAD_LUA) += lua_src/
obj-$(NAUT_CONFIG_RUST_SUPPORT) += rust/
obj-$(NAUT_CONFIG_ASPACES) += aspace/
