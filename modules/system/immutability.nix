{ config, lib, pkgs, ... }:
let
	device = if config.settings.disk.encryption.enable then config.settings.disk.by.mapper.root else config.settings.disk.by.partlabel.root;
	deviceDependency = if config.settings.disk.encryption.enable then "dev-mapper-${config.settings.disk.label.root}.device" else "dev-disk-by\\x2dpartlabel-${config.settings.disk.label.disk}\\x2d${config.settings.disk.label.main}\\x2d${config.settings.disk.label.root}.device";
	snapshotsSubvolumeName = config.settings.disk.subvolumes.snapshots.name;
	cleanName = config.settings.disk.immutability.persist.snapshots.cleanName;
	mode = config.settings.disk.immutability.mode;
	pathsToKeep = config.settings.disk.immutability.persist.paths;
	subvolumeNameMountPointPairs = config.settings.disk.subvolumes.nameMountPointPairs.resetOnBoot;

	pathsFile = pkgs.writeText "immutability-paths" (
		lib.concatMapStringsSep "\n" (path: path) pathsToKeep
	);

	immutabilityBin = pkgs.stdenv.mkDerivation {
		pname = "immutability";
		version = "1.0.0";
		src = ../../scripts/core/immutability.py;
		nativeBuildInputs = [ pkgs.python3 pkgs.python3Packages.cython ];
		dontUnpack = true;
		buildPhase = ''
			cp $src immutability.py
			${pkgs.python3Packages.cython}/bin/cython --embed -3 -o immutability.c immutability.py
			${pkgs.stdenv.cc}/bin/cc -O2 \
				$(${pkgs.python3}/bin/python3-config --cflags --embed) \
				-o immutability immutability.c \
				$(${pkgs.python3}/bin/python3-config --ldflags --embed)
		'';
		installPhase = ''
			mkdir -p $out/bin
			cp immutability $out/bin/
		'';
	};

in
lib.mkIf config.settings.disk.immutability.enable {
	fileSystems = lib.mkMerge (lib.lists.forEach (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes) (volume: { "${volume.mountPoint}".neededForBoot = lib.mkForce true; }));
	boot.nixStoreMountOpts = [ "ro" ];
	boot.tmp.useTmpfs = true;
	boot.initrd = {
		supportedFilesystems = [ "btrfs" ];
		systemd = {
			storePaths = [ "${pkgs.python3}" "${pathsFile}" ];
			extraBin = {
				btrfs = "${pkgs.btrfs-progs}/bin/btrfs";
				rsync = "${pkgs.rsync}/bin/rsync";
				immutability = "${immutabilityBin}/bin/immutability";
			};
			services.immutability = {
				description = "Factory resets BTRFS subvolumes via compiled Python";
				wantedBy = [ "initrd.target" ];
				requires = [ deviceDependency ];
				after = [ "systemd-cryptsetup@${config.settings.disk.partlabel.root}.service" deviceDependency ];
				before = [ "sysroot.mount" ];
				unitConfig.DefaultDependencies = "no";
				serviceConfig.Type = "oneshot";
				serviceConfig.Environment = "PYTHONHOME=${pkgs.python3}";
				script = ''
					immutability ${device} ${snapshotsSubvolumeName} ${cleanName} ${mode} ${pathsFile} ${subvolumeNameMountPointPairs}
				'';
			};
		};
	};
}
