{ config, inputs, lib, pkgs, utils, ... }:
let
	device = if config.settings.disk.encryption.enable then config.settings.disk.by.mapper.root else config.settings.disk.by.partlabel.root;
	deviceDependency = if config.settings.disk.encryption.enable then "dev-mapper-${config.settings.disk.label.root}.device" else "dev-disk-by\\x2dpartlabel-${config.settings.disk.label.disk}\\x2d${config.settings.disk.label.main}\\x2d${config.settings.disk.label.root}.device";
	snapshotsSubvolumeName = config.settings.disk.subvolumes.snapshots.name;
	cleanName = config.settings.disk.immutability.persist.snapshots.cleanName;
	immutabilityMode = config.settings.disk.immutability.mode;
	immutabilityPersistRoot = config.settings.disk.immutability.persist.subvolumeRoot;
	immutabilityDryRun = config.settings.disk.immutability.dryRun;
	immutabilityEnabled = config.settings.disk.immutability.enable;
	nonPersistedGenerations = config.settings.disk.immutability.nonPersistedGenerations;
	restoreGeneration = config.settings.disk.immutability.restoreGeneration;
	restoreGenerationMinimum =
		if immutabilityMode == "restore-generation" then restoreGeneration
		else if immutabilityMode == "restore-previous" || immutabilityMode == "restore-a" then 1
		else if immutabilityMode == "restore-penultimate" || immutabilityMode == "restore-b" then 2
		else if immutabilityMode == "restore-c" then 3
		else 0;
	persistenceEnabled = config.settings.disk.persistence.enable;
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

	immutabilityPairArgs = lib.concatMapStringsSep " " (volume:
		lib.escapeShellArg "${volume.name}=${volume.mountPoint}"
	) resetVolumes;
	immutabilityOptions = lib.escapeShellArgs [ "--keep-generations" (toString nonPersistedGenerations) "--restore-generation" (toString restoreGeneration) ];
	immutabilityCommandArgs = lib.escapeShellArgs [ device snapshotsSubvolumeName cleanName immutabilityMode immutabilityPersistRoot immutabilitySpecFile ];
	immutabilityResetMountUnits = map (volume: "${utils.escapeSystemdPath volume.mountPoint}.mount") resetVolumes;
	immutabilitySpecFile = (pkgs.formats.toml {}).generate "immutability-spec.toml" {
		keep = lib.concatMap (volume:
			map (path: {
				inherit path;
				kind = "auto";
				mountPoint = volume.mountPoint;
				volume = volume.name;
			}) (sortPaths (pathsForVolume volume))
		) resetVolumes;
	};

	btrcpy = inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrcpy;
	immutabilityBin = pkgs.stdenv.mkDerivation {
		name = "immutability";
		src = ../..;
		nativeBuildInputs = [ btrcpy ];
		dontConfigure = true;
		buildPhase = ''
			btrcpy --strict-imports btrc/immutability/immutability.btrc -o immutability.c
			$CC -std=c11 -O2 immutability.c -lm -lpthread -o immutability
		'';
		installPhase = ''
			mkdir -p $out/bin
			cp immutability $out/bin/immutability
		'';
	};

in
lib.mkMerge [
(lib.mkIf (persistenceEnabled || immutabilityEnabled) {
	assertions = [
		{
			assertion = !immutabilityEnabled || restoreGenerationMinimum <= nonPersistedGenerations;
			message = "settings.disk.immutability.nonPersistedGenerations must keep enough generations for settings.disk.immutability.mode";
		}
	];
	environment.systemPackages = [ immutabilityBin ];
	environment.etc."immutability/spec.toml".source = immutabilitySpecFile;
	systemd.services."persistence-mounts" = {
		description = "Mount persistent BTRFS subvolumes into place";
		wantedBy = lib.optionals persistenceEnabled [ "local-fs.target" ];
		requires = [ deviceDependency ] ++ immutabilityResetMountUnits;
		after = [ deviceDependency ] ++ immutabilityResetMountUnits;
		before = [ "local-fs.target" ];
		unitConfig.DefaultDependencies = "no";
		serviceConfig = {
			Type = "oneshot";
			RemainAfterExit = true;
		};
		path = with pkgs; [ btrfs-progs coreutils util-linux ];
		script = ''
			${immutabilityBin}/bin/immutability ${lib.optionalString immutabilityDryRun "--dry-run "}mount ${lib.escapeShellArg device} ${lib.escapeShellArg immutabilityPersistRoot} ${immutabilitySpecFile}
		'';
	};
})
(lib.mkIf (immutabilityEnabled && config.settings.disk.immutability.enforce.onReboot) {
	fileSystems = lib.mkMerge (lib.lists.forEach (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes) (volume: { "${volume.mountPoint}".neededForBoot = lib.mkForce true; }));
	boot = {
		nixStoreMountOpts = [ "ro" ];
		initrd = {
			supportedFilesystems = [ "btrfs" ];
			systemd = {
				storePaths = [ "${immutabilityBin}" immutabilitySpecFile ];
				extraBin = {
					btrfs = "${pkgs.btrfs-progs}/bin/btrfs";
					cp = "${pkgs.coreutils}/bin/cp";
					mv = "${pkgs.coreutils}/bin/mv";
					findmnt = "${pkgs.util-linux}/bin/findmnt";
					umount = lib.mkDefault "${pkgs.util-linux}/bin/umount";
				};
				services."immutability" = {
					description = "Factory resets BTRFS subvolumes using persistent keep subvolumes";
					wantedBy = [ "initrd.target" ];
					requires = [ deviceDependency ];
					after = [ "systemd-cryptsetup@${config.settings.disk.partlabel.root}.service" deviceDependency ];
					before = [ "sysroot.mount" ];
					unitConfig.DefaultDependencies = "no";
					serviceConfig.Type = "oneshot";
					script = ''
						${immutabilityBin}/bin/immutability ${lib.optionalString immutabilityDryRun "--dry-run "}${immutabilityOptions} ${immutabilityCommandArgs} ${immutabilityPairArgs}
					'';
				};
			};
		};
	};
	})
]
