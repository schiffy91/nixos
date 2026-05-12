{ pkgs, ... }: {
  boot.kernelParams = [ "amd_iommu=on" "iommu=pt" ];
  hardware = {
    cpu.amd.updateMicrocode = true;
    firmware = [ pkgs.linux-firmware ];
  };
}
