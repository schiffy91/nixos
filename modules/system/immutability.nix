{ config, lib, pkgs, utils, ... }:
let
	device = if config.settings.disk.encryption.enable then config.settings.disk.by.mapper.root else config.settings.disk.by.partlabel.root;
	deviceDependency = if config.settings.disk.encryption.enable then "dev-mapper-${config.settings.disk.label.root}.device" else "dev-disk-by\\x2dpartlabel-${config.settings.disk.label.disk}\\x2d${config.settings.disk.label.main}\\x2d${config.settings.disk.label.root}.device";
	snapshotsSubvolumeName = config.settings.disk.subvolumes.snapshots.name;
	cleanName = config.settings.disk.immutability.persist.snapshots.cleanName;
	membraneMode = config.settings.disk.immutability.semipermeable_membrane.mode;
	membranePersistRoot = config.settings.disk.immutability.semipermeable_membrane.persist.subvolumeRoot;
	membraneDryRun = config.settings.disk.immutability.semipermeable_membrane.dryRun;
	membraneEnabled = config.settings.disk.immutability.enable;
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
		inherit (volume) mountPoint;
		otherMounts = lib.filter (other: other.mountPoint != mountPoint && other.mountPoint != "/") allVolumes;
	in builtins.filter (path:
			if mountPoint == "/" then
				!(builtins.any (other: path == other.mountPoint || lib.hasPrefix (other.mountPoint + "/") path) otherMounts)
			else path == mountPoint || lib.hasPrefix (mountPoint + "/") path
		) pathsToKeep;

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

	semipermeableMembraneBin = pkgs.stdenv.mkDerivation {
		name = "semipermeable_membrane";
		src = ../../generated/semipermeable_membrane.c;
		dontUnpack = true;
		buildPhase = ''
			$CC -std=c11 -O2 -o semipermeable_membrane $src -lm -lpthread
		'';
		installPhase = "mkdir -p $out/bin && cp semipermeable_membrane $out/bin/";
	};

in
lib.mkMerge [
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
		path = with pkgs; [ btrfs-progs coreutils util-linux ];
		script = ''
			${semipermeableMembraneBin}/bin/semipermeable_membrane ${lib.optionalString membraneDryRun "--dry-run "}mount ${lib.escapeShellArg device} ${lib.escapeShellArg membranePersistRoot} ${membraneSpecFile}
		'';
	};
})
(lib.mkIf (membraneEnabled && config.settings.disk.immutability.enforce.onReboot) {
	fileSystems = lib.mkMerge (lib.lists.forEach (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes) (volume: { "${volume.mountPoint}".neededForBoot = lib.mkForce true; }));
	boot = {
		nixStoreMountOpts = [ "ro" ];
		tmp.useTmpfs = true;
		initrd = {
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
	};
	})
]
