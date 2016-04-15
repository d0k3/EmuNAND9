WGEN := $(CURDIR)/simplegen.bat
UGEN := $(CURDIR)/simplegen.sh
ZIP  := $(CURDIR)/7za.exe a -xr!.*

export OUTPUT_D	:= $(CURDIR)/output
export ZIP3DSFX := $(CURDIR)/ZIP3DSFX
export PSTARTER := $(CURDIR)/extstarterpack


.PHONY: clean all

all:
	@[ -d $(OUTPUT_D) ] || mkdir -p $(OUTPUT_D)
	@make --no-print-directory -C $(ZIP3DSFX) -f $(ZIP3DSFX)/Makefile sfx_stub CONFIG=boot_installer
	@cp $(ZIP3DSFX)/output/ZIP3DSFX.3dsx $(OUTPUT_D)
	cp $(WGEN) $(OUTPUT_D)/drop_zip_here.bat
	cp $(UGEN) $(OUTPUT_D)/drop_zip_here.sh
	@$(ZIP) $(OUTPUT_D)/extstarterpack.zip $(PSTARTER)/*
	@cat $(OUTPUT_D)/ZIP3DSFX.3dsx $(OUTPUT_D)/extstarterpack.zip > $(OUTPUT_D)/starter.bin

clean:
	@echo clean ...
	@-make clean --no-print-directory -C ZIP3DSFX
	@rm -fr $(CURDIR)/output
