{ config, lib, pkgs, ... }:
let
	device = if config.settings.disk.encryption.enable then config.settings.disk.by.mapper.root else config.settings.disk.by.partlabel.root;
	deviceDependency = if config.settings.disk.encryption.enable then "dev-mapper-${config.settings.disk.label.root}.device" else "dev-disk-by\\x2dpartlabel-${config.settings.disk.label.disk}\\x2d${config.settings.disk.label.main}\\x2d${config.settings.disk.label.root}.device";
	snapshotsSubvolumeName = config.settings.disk.subvolumes.snapshots.name;
	cleanName = config.settings.disk.immutability.persist.snapshots.cleanName;
	mode = config.settings.disk.immutability.mode;
	pathsToKeep = config.settings.disk.immutability.persist.paths;
	allVolumes = config.settings.disk.subvolumes.volumes;
	resetVolumes = lib.filter (volume: volume.resetOnBoot) allVolumes;

	filterForVolume = volume: let
		mountPoint = volume.mountPoint;
		otherMounts = lib.filter (other: other.mountPoint != mountPoint && other.mountPoint != "/") allVolumes;
		relevantPaths = builtins.filter (path:
			if mountPoint == "/" then
				!(builtins.any (other: path == other.mountPoint || lib.hasPrefix (other.mountPoint + "/") path) otherMounts)
			else path == mountPoint || lib.hasPrefix (mountPoint + "/") path
		) pathsToKeep;
		toRelative = path:
			if mountPoint == "/" then lib.removePrefix "/" path
			else let stripped = lib.removePrefix (mountPoint + "/") path;
			in if path == mountPoint then "" else stripped;
		relativePaths = builtins.filter (path: path != "") (map toRelative relevantPaths);
		ancestorsOf = path: let
			parts = lib.splitString "/" path;
			parentParts = lib.init parts;
			indices = lib.range 0 (builtins.length parentParts - 1);
		in map (i: lib.concatStringsSep "/" (lib.take (i + 1) parentParts)) indices;
		allAncestors = lib.unique (lib.concatMap ancestorsOf relativePaths);
		filterLines = (map (ancestor: "+ /${ancestor}/") allAncestors)
			++ (lib.concatMap (path: [ "+ /${path}" "+ /${path}/" "+ /${path}/**" ]) relativePaths)
			++ [ "- *" ];
	in pkgs.writeText "immutability-filter-${volume.name}" (lib.concatStringsSep "\n" filterLines + "\n");

	pairArgs = lib.concatMapStringsSep " " (volume:
		"${volume.name}=${volume.mountPoint}:${filterForVolume volume}"
	) resetVolumes;

	immutabilityBin = pkgs.stdenv.mkDerivation {
		name = "immutability";
		src = ../../lib/immutability.rs;
		dontUnpack = true;
		nativeBuildInputs = [ pkgs.rustc ];
		buildPhase = "rustc --edition 2021 -O -o immutability $src";
		installPhase = "mkdir -p $out/bin && cp immutability $out/bin/";
	};

in
lib.mkIf config.settings.disk.immutability.enable {
	fileSystems = lib.mkMerge (lib.lists.forEach (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes) (volume: { "${volume.mountPoint}".neededForBoot = lib.mkForce true; }));
	boot.nixStoreMountOpts = [ "ro" ];
	boot.tmp.useTmpfs = true;
	boot.initrd = {
		supportedFilesystems = [ "btrfs" ];
		systemd = {
			storePaths = let
				filterFiles = map filterForVolume resetVolumes;
			in [ "${immutabilityBin}" ] ++ filterFiles;
			extraBin = {
				btrfs = "${pkgs.btrfs-progs}/bin/btrfs";
				cp = "${pkgs.coreutils}/bin/cp";
			};
			services.immutability = {
				description = "Factory resets BTRFS subvolumes";
				wantedBy = [ "initrd.target" ];
				requires = [ deviceDependency ];
				after = [ "systemd-cryptsetup@${config.settings.disk.partlabel.root}.service" deviceDependency ];
				before = [ "sysroot.mount" ];
				unitConfig.DefaultDependencies = "no";
				serviceConfig.Type = "oneshot";
				script = ''
					${immutabilityBin}/bin/immutability ${device} ${snapshotsSubvolumeName} ${cleanName} ${mode} ${pairArgs}
				'';
			};
		};
	};
}
