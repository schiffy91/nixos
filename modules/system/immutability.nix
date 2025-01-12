{ inputs, config, lib, ... }: {
  imports = [ inputs.impermanence.nixosModules.impermanence ];
  fileSystems = lib.listToAttrs (map (subvolume: { 
    name = "${subvolume.mountPoint}"; 
    value.neededForBoot = true; 
  }) config.settings.disk.subvolumes);
  environment.persistence = lib.listToAttrs (map (subvolume: {
    name = "${subvolume.mountPoint}";
    value = {
      enable = config.settings.disk.immutability.enable;
      directories = subvolume.persistDirectories;
      files = subvolume.persistFiles;
      hideMounts = true;
    };
  }) (builtins.filter (subvolume: subvolume.persistence) config.settings.disk.subvolumes));
  boot.initrd.postResumeCommands = lib.mkAfter ''
    mkdir /btrfs_tmp
    mount ${config.settings.disk.device} /btrfs_tmp
    if [[ -e /btrfs_tmp/root ]]; then
        mkdir -p /btrfs_tmp/old_roots
        timestamp=$(date --date="@$(stat -c %Y /btrfs_tmp/root)" "+%Y-%m-%-d_%H:%M:%S")
        mv /btrfs_tmp/root "/btrfs_tmp/old_roots/$timestamp"
    fi

    delete_subvolume_recursively() {
        IFS=$'\n'
        for i in $(btrfs subvolume list -o "$1" | cut -f 9- -d ' '); do
            delete_subvolume_recursively "/btrfs_tmp/$i"
        done
        btrfs subvolume delete "$1"
    }

    for i in $(find /btrfs_tmp/old_roots/ -maxdepth 1 -mtime +30); do
        delete_subvolume_recursively "$i"
    done

    btrfs subvolume create /btrfs_tmp/root
    umount /btrfs_tmp
  '';
}