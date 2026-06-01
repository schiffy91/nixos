{ pkgs, ... }: {
  boot.kernelParams = [ "iommu=pt" ];
  hardware = {
    cpu.amd.updateMicrocode = true;
    firmware = [ pkgs.linux-firmware ];
  };
}
