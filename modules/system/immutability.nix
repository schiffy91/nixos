{ config, lib, pkgs, ... }: 
let 
  initrdPkgs = with pkgs; [ btrfs-progs rsync coreutils bash util-linux ];
  initrdKernelModules = [ "btrfs"];
  immutabilityFilePath = toString ../../bin/immutability.sh;
in 
lib.mkIf config.settings.disk.immutability.enable {
  fileSystems = lib.mkMerge (lib.lists.forEach (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes) (volume: { "${volume.mountPoint}".neededForBoot = lib.mkForce true; }));
  boot.readOnlyNixStore = true;
  boot.initrd = {
    kernelModules = initrdKernelModules;
    availableKernelModules = initrdKernelModules;
    systemd.services = {
      systemd.services.restore-root = {
        description = "Setup tools for immutability service";
        wantedBy = [ "initrd.target" ];
        after = (if config.settings.disk.encryption.enable then [ "systemd-cryptsetup@*.service" ] else [ ]);
        before = [ "sysroot.mount" ];
        unitConfig.DefaultDependencies = "no";
        serviceConfig.Type = "oneshot";
        script = ''
          mkdir -p /mnt

          # We first mount the btrfs root to /mnt
          # so we can manipulate btrfs subvolumes.
          mount -o subvol=/ /dev/mapper/crypted /mnt

          # While we're tempted to just delete /root and create
          # a new snapshot from /root-blank, /root is already
          # populated at this point with a number of subvolumes,
          # which makes `btrfs subvolume delete` fail.
          # So, we remove them first.
          btrfs subvolume list -o /mnt/root |
          cut -f9 -d' ' |
          while read subvolume; do
            echo "deleting /$subvolume subvolume..."
            btrfs subvolume delete "/mnt/$subvolume"
          done &&
          echo "deleting /root subvolume..." &&
          btrfs subvolume delete /mnt/root

          echo "restoring blank /root subvolume..."
          btrfs subvolume snapshot /mnt/root-blank /mnt/root

          # Once we're done rolling back to a blank snapshot,
          # we can unmount /mnt and continue on the boot process.
          umount /mnt
        '';
      };
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
        after = [ "setup-immutability.service" ] ++ 
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
          # Debug output
          echo "PATH=$PATH"
          echo "Checking mount command..."
          which mount || echo "mount not found"
          
          # Ensure mount is in path
          export PATH="${lib.makeBinPath (with pkgs; [ util-linux ])}:${lib.makeBinPath initrdPkgs}:$PATH"
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