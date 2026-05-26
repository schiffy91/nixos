{ config, lib, pkgs, utils, ... }:
let
	device = if config.settings.disk.encryption.enable then config.settings.disk.by.mapper.root else config.settings.disk.by.partlabel.root;
	deviceDependency = if config.settings.disk.encryption.enable then "dev-mapper-${config.settings.disk.label.root}.device" else "dev-disk-by\\x2dpartlabel-${config.settings.disk.label.disk}\\x2d${config.settings.disk.label.main}\\x2d${config.settings.disk.label.root}.device";
	snapshotsSubvolumeName = config.settings.disk.subvolumes.snapshots.name;
	cleanName = config.settings.disk.immutability.persist.snapshots.cleanName;
	mode = config.settings.disk.immutability.mode;
	membraneMode = config.settings.disk.immutability.semipermeable_membrane.mode;
	membranePersistRoot = config.settings.disk.immutability.semipermeable_membrane.persist.subvolumeRoot;
	membraneDryRun = config.settings.disk.immutability.semipermeable_membrane.dryRun;
	membraneEnabled = config.settings.disk.immutability.enable && config.settings.disk.immutability.semipermeable_membrane.enable && config.settings.disk.immutability.implementation == "semipermeable_membrane";
	pathsToKeep = config.settings.disk.immutability.persist.paths;
	allVolumes = config.settings.disk.subvolumes.volumes;
	resetVolumes = lib.filter (volume: volume.resetOnBoot) allVolumes;
	pathDepth = path: builtins.length (lib.splitString "/" (lib.removePrefix "/" path));
	sortPaths = paths: lib.sort (a: b:
		let
			aDepth = pathDepth a;
			bDepth = pathDepth b;
		in if aDepth == bDepth then builtins.stringLength a < builtins.stringLength b else aDepth < bDepth
	) paths;

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
	membranePairArgs = lib.concatMapStringsSep " " (volume:
		lib.escapeShellArg "${volume.name}=${volume.mountPoint}"
	) resetVolumes;
	membraneCommandArgs = lib.escapeShellArgs [ device snapshotsSubvolumeName cleanName membraneMode membranePersistRoot membraneSpecFile ];
	membraneResetMountUnits = map (volume: "${utils.escapeSystemdPath volume.mountPoint}.mount") resetVolumes;
	membraneSpecFile = pkgs.writeText "semipermeable-membrane-spec" (
		lib.concatStringsSep "\n" (
			lib.concatMap (volume:
				map (path: "${volume.name}\t${volume.mountPoint}\t${path}\tauto") (sortPaths (pathsForVolume volume))
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
	semipermeableMembraneBin = pkgs.stdenv.mkDerivation {
		name = "semipermeable_membrane";
		src = ../../scripts/lib/semipermeable_membrane.rs;
		dontUnpack = true;
		nativeBuildInputs = [ pkgs.rustc ];
		buildPhase = "rustc --edition 2021 -O -o semipermeable_membrane $src";
		installPhase = "mkdir -p $out/bin && cp semipermeable_membrane $out/bin/";
	};

in
lib.mkMerge [
{
	assertions = [
		{
			assertion = !config.settings.disk.immutability.enable || config.settings.disk.immutability.implementation != "semipermeable_membrane" || config.settings.disk.immutability.semipermeable_membrane.enable;
			message = "settings.disk.immutability.implementation = \"semipermeable_membrane\" requires settings.disk.immutability.semipermeable_membrane.enable = true because it is still experimental.";
		}
	];
}
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
				umount = lib.mkDefault "${pkgs.util-linux}/bin/umount";
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
(lib.mkIf membraneEnabled {
	environment.systemPackages = [ semipermeableMembraneBin ];
	environment.etc."semipermeable_membrane/spec.tsv".source = membraneSpecFile;
	systemd.services."semipermeable-membrane-mounts" = {
		description = "Mount semipermeable membrane persistent BTRFS subvolumes into place";
		wantedBy = lib.optionals config.settings.disk.immutability.enforce.onReboot [ "local-fs.target" ];
		requires = [ deviceDependency ] ++ membraneResetMountUnits;
		after = [ deviceDependency ] ++ membraneResetMountUnits;
		before = [ "local-fs.target" ];
		unitConfig.DefaultDependencies = "no";
		serviceConfig = {
			Type = "oneshot";
			RemainAfterExit = true;
		};
		path = with pkgs; [ btrfs-progs coreutils gnused util-linux ];
		script = ''
			device=${lib.escapeShellArg device}
			tab="$(printf '\t')"
			top=/run/semipermeable-membrane-top
			selected=/run/semipermeable-membrane-mounted-targets
			mounted=0
			: > "$selected"
			covered_by_selected() {
				while IFS= read -r selectedTarget; do
					[ -n "$selectedTarget" ] || continue
					case "$1" in
						"$selectedTarget"/*) return 0 ;;
					esac
				done < "$selected"
				return 1
			}
			if ${lib.boolToString membraneDryRun}; then
				while IFS="$tab" read -r volume mountPoint target kind; do
					[ -n "$target" ] || continue
					if covered_by_selected "$target"; then
						echo "DRY skip covered nested persist mount $target"
						continue
					fi
					key=''${target#/}
					case "$key" in *'!'*) echo "semipermeable-membrane-mounts: unsupported ! in path $target" >&2; exit 1 ;; esac
					key=''${key//\//!}
					subvol=${membranePersistRoot}/dirs/$key
					echo "DRY if btrfs subvolume show $top/$subvol"
					echo "DRY mount -t btrfs -o subvolid=<id>,compress=zstd,noatime $device $target"
					printf '%s\n' "$target" >> "$selected"
				done < ${membraneSpecFile}
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
				if covered_by_selected "$target"; then
					continue
				fi
				key=''${target#/}
				case "$key" in *'!'*) echo "semipermeable-membrane-mounts: unsupported ! in path $target" >&2; exit 1 ;; esac
				key=''${key//\//!}
				subvol=${membranePersistRoot}/dirs/$key
				if btrfs subvolume show "$top/$subvol" >/dev/null 2>&1; then
					id="$(btrfs subvolume show "$top/$subvol" | sed -n 's/^[[:space:]]*Subvolume ID:[[:space:]]*//p' | head -n 1)"
					[ -n "$id" ] || exit 1
					mkdir -p "$target"
					if mountpoint -q "$target"; then
						current="$(findmnt -rn -o FSTYPE,OPTIONS --mountpoint "$target")"
						currentFs="''${current%% *}"
						currentOptions="''${current#* }"
						case "$currentFs,$currentOptions," in
							btrfs,*",subvolid=$id,"*) ;;
							*) echo "semipermeable-membrane-mounts: $target is mounted but not btrfs subvolid=$id" >&2; exit 1 ;;
						esac
					else
						mount -t btrfs -o "subvolid=$id,compress=zstd,noatime" "$device" "$target"
					fi
					printf '%s\n' "$target" >> "$selected"
				fi
			done < ${membraneSpecFile}
		'';
	};
})
(lib.mkIf (membraneEnabled && config.settings.disk.immutability.enforce.onReboot) {
	fileSystems = lib.mkMerge (lib.lists.forEach (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes) (volume: { "${volume.mountPoint}".neededForBoot = lib.mkForce true; }));
	boot.nixStoreMountOpts = [ "ro" ];
	boot.tmp.useTmpfs = true;
	boot.initrd = {
		supportedFilesystems = [ "btrfs" ];
		systemd = {
			storePaths = [ "${semipermeableMembraneBin}" membraneSpecFile ];
			extraBin = {
				btrfs = "${pkgs.btrfs-progs}/bin/btrfs";
				cp = "${pkgs.coreutils}/bin/cp";
				mv = "${pkgs.coreutils}/bin/mv";
				findmnt = "${pkgs.util-linux}/bin/findmnt";
				umount = lib.mkDefault "${pkgs.util-linux}/bin/umount";
			};
			services."semipermeable-membrane" = {
				description = "Factory resets BTRFS subvolumes using persistent keep subvolumes";
				wantedBy = [ "initrd.target" ];
				requires = [ deviceDependency ];
				after = [ "systemd-cryptsetup@${config.settings.disk.partlabel.root}.service" deviceDependency ];
				before = [ "sysroot.mount" ];
				unitConfig.DefaultDependencies = "no";
				serviceConfig.Type = "oneshot";
				script = ''
					${semipermeableMembraneBin}/bin/semipermeable_membrane ${lib.optionalString membraneDryRun "--dry-run "}${membraneCommandArgs} ${membranePairArgs}
				'';
			};
		};
	};
	})
]
