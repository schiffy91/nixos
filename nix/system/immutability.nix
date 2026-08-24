{ config, inputs, lib, pkgs, utils, ... }:
let
	persistKey = path: lib.replaceStrings [ "%2F" ] [ "!" ] (lib.strings.escapeURL (lib.removePrefix "/" path));
	immutabilityVersion = "2026.08.22";
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

	bootPersistPaths = config.settings.disk.immutability.persist.neededForBoot;
	bootPersistCandidates = lib.concatMap pathsForVolume resetVolumes;
	bootPersistNested = path: lib.any (other: other != path && lib.hasPrefix (other + "/") path) bootPersistCandidates;
	bootPersistValid = path: lib.elem path bootPersistCandidates && !lib.hasSuffix "/" path && !bootPersistNested path;
	bootPersistFileSystems = lib.genAttrs bootPersistPaths (path: {
		inherit device;
		fsType = "btrfs";
		options = [ "subvol=${immutabilityPersistRoot}/dirs/${persistKey path}" "compress=zstd" "noatime" "nofail" "x-systemd.after=persistence-mounts.service" ];
		neededForBoot = true;  # initrd activation reads hashedPasswordFile from here
	});
	bootPersistInitrdMountUnits = map (path: "${utils.escapeSystemdPath "/sysroot${path}"}.mount") bootPersistPaths;

	immutabilityPairArgs = lib.concatMapStringsSep " " (volume:
		lib.escapeShellArg "${volume.name}=${volume.mountPoint}"
	) resetVolumes;
	immutabilityOptions = lib.escapeShellArgs [ "--keep-generations" (toString nonPersistedGenerations) "--restore-generation" (toString restoreGeneration) ];
	immutabilityCommandArgs = lib.escapeShellArgs [ device snapshotsSubvolumeName cleanName immutabilityMode immutabilityPersistRoot immutabilitySpecFile ];
	immutabilityResetMountUnits = map (volume: "${utils.escapeSystemdPath volume.mountPoint}.mount") resetVolumes;
	immutabilityVersionDir = "${snapshotsSubvolumeName}/.immutability";
	immutabilityVersionFile = "${immutabilityVersionDir}/version";
	immutabilityVersionPreflight = ''
		immutability_version_top="/run/immutability-version-top"
		immutability_version_dir="$immutability_version_top/${immutabilityVersionDir}"
		immutability_version_file="$immutability_version_top/${immutabilityVersionFile}"
		immutability_version_cleanup() {
			umount "$immutability_version_top" 2>/dev/null || true
			rmdir "$immutability_version_top" 2>/dev/null || true
		}
		trap immutability_version_cleanup EXIT
		mkdir -p "$immutability_version_top"
		mount -t btrfs -o subvolid=5 ${lib.escapeShellArg device} "$immutability_version_top"

		immutability_previous_version=""
		if [ -f "$immutability_version_file" ]; then
			IFS= read -r immutability_previous_version < "$immutability_version_file" || true
		fi
		case "$immutability_previous_version" in
			""|"0.1"|${lib.escapeShellArg immutabilityVersion})
				;;
			*)
				echo "Unsupported immutability version transition: $immutability_previous_version -> ${immutabilityVersion}" >&2
				exit 1
				;;
		esac
	'';
	immutabilityVersionCommit = ''
		mkdir -p "$immutability_version_dir"
		printf '%s\n' ${lib.escapeShellArg immutabilityVersion} > "$immutability_version_file"
		sync "$immutability_version_file" || sync
		immutability_version_cleanup
		trap - EXIT
	'';
	immutabilityVersionFinish = if immutabilityDryRun then ''
		immutability_version_cleanup
		trap - EXIT
	'' else immutabilityVersionCommit;
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
{
	config = lib.mkMerge [
		{
			_module.args.immutabilityPersistKey = persistKey;
		}
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
		(lib.mkIf persistenceEnabled {
			assertions = [
				{
					assertion = lib.all bootPersistValid bootPersistPaths;
					message = "settings.disk.immutability.persist.neededForBoot entries must be top-level persisted directories (no trailing slash) on a resetOnBoot volume";
				}
			];
			fileSystems = bootPersistFileSystems;
			boot.initrd.systemd.services.initrd-nixos-activation.after = bootPersistInitrdMountUnits;
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
								mkdir = "${pkgs.coreutils}/bin/mkdir";
								mv = "${pkgs.coreutils}/bin/mv";
								rmdir = "${pkgs.coreutils}/bin/rmdir";
								sync = "${pkgs.coreutils}/bin/sync";
								findmnt = "${pkgs.util-linux}/bin/findmnt";
								mount = lib.mkDefault "${pkgs.util-linux}/bin/mount";
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
									${immutabilityVersionPreflight}
									${immutabilityBin}/bin/immutability ${lib.optionalString immutabilityDryRun "--dry-run "}${immutabilityOptions} ${immutabilityCommandArgs} ${immutabilityPairArgs}
									${immutabilityVersionFinish}
								'';
							};
					};
				};
			};
			})
	];
}
