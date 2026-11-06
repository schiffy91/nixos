{ pkgs, ... }: {
  # First 7950X CCD: eight physical cores and their SMT siblings share L3.
  settings.apps.gaming.cpuAffinity = "0-7,16-23";
  boot.kernelModules = [ "msr" ];  # fwupd msr plugin: SME + SMM-lock checks
  # No iommu=pt: ASUS ACPI lacks ExternalFacingPort on the TB4 root port, so tunneled
  # devices are never marked untrusted and passthrough would give them identity DMA.
  # Translated DMA-FQ domains are the only Thunderbolt DMA protection this board has.
  hardware = {
    cpu.amd.updateMicrocode = true;
    firmware = [ pkgs.linux-firmware ];
  };
}
