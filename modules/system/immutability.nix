{ config, lib, pkgs, utils, ... }:
let
	device = if config.settings.disk.encryption.enable then config.settings.disk.by.mapper.root else config.settings.disk.by.partlabel.root;
	deviceDependency = if config.settings.disk.encryption.enable then "dev-mapper-${config.settings.disk.label.root}.device" else "dev-disk-by\\x2dpartlabel-${config.settings.disk.label.disk}\\x2d${config.settings.disk.label.main}\\x2d${config.settings.disk.label.root}.device";
	snapshotsSubvolumeName = config.settings.disk.subvolumes.snapshots.name;
	cleanName = config.settings.disk.immutability.persist.snapshots.cleanName;
	mode = config.settings.disk.immutability.mode;
	v2Mode = config.settings.disk.immutability.v2.mode;
	v2PersistRoot = config.settings.disk.immutability.v2.persist.subvolumeRoot;
	v2DryRun = config.settings.disk.immutability.v2.dryRun;
	pathsToKeep = config.settings.disk.immutability.persist.paths;
	allVolumes = config.settings.disk.subvolumes.volumes;
	resetVolumes = lib.filter (volume: volume.resetOnBoot) allVolumes;

	pathsForVolume = volume: let
		mountPoint = volume.mountPoint;
		otherMounts = lib.filter (other: other.mountPoint != mountPoint && other.mountPoint != "/") allVolumes;
	in builtins.filter (path:
			if mountPoint == "/" then
				!(builtins.any (other: path == other.mountPoint || lib.hasPrefix (other.mountPoint + "/") path) otherMounts)
			else path == mountPoint || lib.hasPrefix (mountPoint + "/") path
		) pathsToKeep;

	filterForVolume = volume: let
		mountPoint = volume.mountPoint;
		relevantPaths = pathsForVolume volume;
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
	v2PairArgs = lib.concatMapStringsSep " " (volume:
		"${volume.name}=${volume.mountPoint}"
	) resetVolumes;
	v2ResetMountUnits = map (volume: "${utils.escapeSystemdPath volume.mountPoint}.mount") resetVolumes;
	v2SpecFile = pkgs.writeText "immutabilityv2-spec" (
		lib.concatStringsSep "\n" (
			lib.concatMap (volume:
				map (path: "${volume.name}\t${volume.mountPoint}\t${path}\tauto") (pathsForVolume volume)
			) resetVolumes
		) + "\n"
	);

	immutabilityBin = pkgs.stdenv.mkDerivation {
		name = "immutability";
		src = ../../scripts/lib/immutability.rs;
		dontUnpack = true;
		nativeBuildInputs = [ pkgs.rustc ];
		buildPhase = "rustc --edition 2021 -O -o immutability $src";
		installPhase = "mkdir -p $out/bin && cp immutability $out/bin/";
	};
	immutabilityV2Bin = pkgs.stdenv.mkDerivation {
		name = "immutabilityv2";
		src = ../../scripts/lib/immutabilityv2.rs;
		dontUnpack = true;
		nativeBuildInputs = [ pkgs.rustc ];
		buildPhase = "rustc --edition 2021 -O -o immutabilityv2 $src";
		installPhase = "mkdir -p $out/bin && cp immutabilityv2 $out/bin/";
	};

in
lib.mkMerge [
(lib.mkIf (config.settings.disk.immutability.enable && config.settings.disk.immutability.implementation == "v1" && config.settings.disk.immutability.enforce.onReboot) {
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
})
(lib.mkIf (config.settings.disk.immutability.enable && config.settings.disk.immutability.v2.enable && config.settings.disk.immutability.implementation == "v2" && config.settings.disk.immutability.enforce.onReboot) {
	fileSystems = lib.mkMerge (lib.lists.forEach (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes) (volume: { "${volume.mountPoint}".neededForBoot = lib.mkForce true; }));
	boot.nixStoreMountOpts = [ "ro" ];
	boot.tmp.useTmpfs = true;
	boot.initrd = {
		supportedFilesystems = [ "btrfs" ];
		systemd = {
			storePaths = [ "${immutabilityV2Bin}" v2SpecFile ];
			extraBin = {
				btrfs = "${pkgs.btrfs-progs}/bin/btrfs";
				cp = "${pkgs.coreutils}/bin/cp";
			};
			services."immutability-v2" = {
				description = "Factory resets BTRFS subvolumes using persistent keep subvolumes";
				wantedBy = [ "initrd.target" ];
				requires = [ deviceDependency ];
				after = [ "systemd-cryptsetup@${config.settings.disk.partlabel.root}.service" deviceDependency ];
				before = [ "sysroot.mount" ];
				unitConfig.DefaultDependencies = "no";
				serviceConfig.Type = "oneshot";
				script = ''
					${immutabilityV2Bin}/bin/immutabilityv2 ${lib.optionalString v2DryRun "--dry-run "}${device} ${snapshotsSubvolumeName} ${cleanName} ${v2Mode} ${v2PersistRoot} ${v2SpecFile} ${v2PairArgs}
				'';
			};
		};
	};
	systemd.services."immutability-v2-mounts" = {
		description = "Mount immutability v2 persistent BTRFS subvolumes into place";
		wantedBy = [ "local-fs.target" ];
		requires = [ deviceDependency ] ++ v2ResetMountUnits;
		after = [ deviceDependency ] ++ v2ResetMountUnits;
		before = [ "local-fs.target" ];
		unitConfig.DefaultDependencies = "no";
		serviceConfig = {
			Type = "oneshot";
			RemainAfterExit = true;
		};
		path = with pkgs; [ btrfs-progs coreutils util-linux ];
		script = ''
			device=${lib.escapeShellArg device}
			tab="$(printf '\t')"
			top=/run/immutability-v2-top
			mounted=0
			if ${lib.boolToString v2DryRun}; then
				while IFS="$tab" read -r volume mountPoint target kind; do
					[ -n "$target" ] || continue
					subvol=${v2PersistRoot}/''${target#/}
					echo "DRY if btrfs subvolume show $top/$subvol"
					echo "DRY mount -t btrfs -o subvol=$subvol,compress=zstd,noatime $device $target"
				done < ${v2SpecFile}
				exit 0
			fi
			mkdir -p "$top"
			if ! mountpoint -q "$top"; then
				mount -t btrfs -o ro,subvolid=5 "$device" "$top"
				mounted=1
			fi
			trap 'if [ "$mounted" = 1 ]; then umount "$top"; fi' EXIT
			while IFS="$tab" read -r volume mountPoint target kind; do
				[ -n "$target" ] || continue
				subvol=${v2PersistRoot}/''${target#/}
				if btrfs subvolume show "$top/$subvol" >/dev/null 2>&1; then
					mkdir -p "$target"
					if ! mountpoint -q "$target"; then
						mount -t btrfs -o "subvol=$subvol,compress=zstd,noatime" "$device" "$target"
					fi
				fi
			done < ${v2SpecFile}
		'';
	};
})
]
