{ config, lib, pkgs, ... }: 
let 
  initrdPkgs = with pkgs; [ btrfs-progs rsync coreutils bash util-linux ];
  initrdKernelModules = [ "btrfs"];
  immutabilityFilePath = "../../bin/immutability.sh";
in 
lib.mkIf config.settings.disk.immutability.enable {
  fileSystems = lib.mkMerge (map (volume: { "${volume.mountPoint}".neededForBoot = lib.mkForce true; }) (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes));
  boot.readOnlyNixStore = true;
  boot.initrd = {
    kernelModules = initrdKernelModules;
    availableKernelModules = initrdKernelModules;
    systemd.services = {
      setup-immutability = {
        description = "Setup tools for immutability service";
        wantedBy = [ "initrd.target" ];
        before = [ "immutability.service" ];
        unitConfig.DefaultDependencies = "no";
        serviceConfig = {
          Type = "oneshot";
          StandardOutput = "journal+console";
          StandardError = "journal+console";
          RemainAfterExit = "yes";
        };
        path = initrdPkgs;
        script = ''
          export PATH="${lib.makeBinPath initrdPkgs}:$PATH"
          mkdir -p /usr/local/bin
          cat > /usr/local/bin/immutability.sh << 'EOL'
          ${(builtins.readFile immutabilityFilePath)}
          EOL
          chmod +x /usr/local/bin/immutability.sh
        '';
      };
      immutability = {
        description = "Enforce immutability at boot-time";
        wantedBy = [ "initrd.target" ];
        after = [ "setup-immutability.service" ] ++ (if config.settings.disk.encryption.enable then [ "systemd-cryptsetup@*.service" ] else [ ]);
        before = [ "sysroot.mount" ];
        requires = [ "setup-immutability.service" ];
        unitConfig.DefaultDependencies = "no";
        serviceConfig = {
          Type = "oneshot";
          StandardOutput = "journal+console";
          StandardError = "journal+console";
        };
        path = initrdPkgs;
        script = ''
          export PATH="${lib.makeBinPath initrdPkgs}:$PATH"
          for i in ${config.settings.disk.subvolumes.volumesNeededForBoot}; do 
            vol=$(echo "$i" | cut -d'=' -f1)
            mount_point="/mnt/$vol"
            mkdir -p "$mount_point"
            
            if mount -t btrfs -o subvol="$vol" "${config.settings.disk.by.partlabel.root}" "$mount_point"; then
              /usr/local/bin/immutability.sh \
                "$mount_point" \
                "$mount_point/${config.settings.disk.immutability.persist.snapshots.name}" \
                '${lib.strings.concatStringsSep " " config.settings.disk.immutability.persist.paths}'
              umount "$mount_point"
            else
              echo "Failed to mount $vol"
              exit 1
            fi
          done
        '';
      };
    };
  };
}