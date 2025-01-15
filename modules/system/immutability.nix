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
      extraUtilsCommands = ''
        for pkg in ${lib.escapeShellArgs (map toString initrdPkgs)}; do
          copy_bin_and_libs $pkg
        done
      '';
    systemd.services.immutability = {
      description = "Enforce immutability at boot-time by deleting all new paths that aren't managed by NixOS or marked to persist.";
      wantedBy = [ "initrd.target" ];
      before = [ "sysroot.mount" ];
      after = if config.settings.disk.encryption.enable then [ "systemd-cryptsetup@*.service" ] else [ ];
      path = initrdPkgs;
      unitConfig.DefaultDependencies = "no";
      serviceConfig = {
        Type = "oneshot";
        StandardOutput = "journal+console";
        StandardError = "journal+console";
      };
      script = ''
        # Ensure basic utils are available
        export PATH="${lib.makeBinPath initrdPkgs}:$PATH"
        
        # Write the script to a file in the initrd
        mkdir -p /usr/local/bin
        cat > /usr/local/bin/immutability.sh << 'EOL'
        ${(builtins.readFile immutabilityFilePath)}
        EOL
        chmod +x /usr/local/bin/immutability.sh
        
        # For each subvolume, mount and process
        for i in ${config.settings.disk.subvolumes.volumesNeededForBoot}; do 
          vol=$(echo "$i" | cut -d'=' -f1)  # Fix pattern matching
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
}