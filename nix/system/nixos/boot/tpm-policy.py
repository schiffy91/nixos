"""Own the admission and update boundary around Lanzaboote's PCR predictions."""
import contextlib
import fcntl
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import stat
import subprocess
import sys
import tempfile


def run(*args, capture=False):
    return subprocess.run(args, check=True, text=True,
                          stdout=subprocess.PIPE if capture else None).stdout


def protected(path, directory=False):
    """Reject writable parents and links for mutable policy authority."""
    path = Path(path)
    for parent in reversed((path, *path.parents)):
        info = parent.lstat()
        if info.st_uid != 0 or info.st_mode & 0o022 or stat.S_ISLNK(info.st_mode):
            raise RuntimeError(f"Unsafe TPM policy path: {parent}")
        if parent != path or directory:
            if not stat.S_ISDIR(info.st_mode):
                raise RuntimeError(f"Not a directory: {parent}")
        elif not stat.S_ISREG(info.st_mode):
            raise RuntimeError(f"Not a regular file: {parent}")
    return path


def eligible(system):
    try:
        system = Path(system).resolve(strict=True)
    except FileNotFoundError:
        return False
    if system.parent != Path('/nix/store'):
        return False
    marker = system / 'tpm-policy-version'
    return marker.is_file() and marker.read_text().strip() == '1'


def require_boot():
    if not eligible('/run/booted-system'):
        raise RuntimeError('Boot an eligible generation with the LUKS passphrase before enrolling a policy')
    variables = Path('/sys/firmware/efi/efivars')
    secure = variables / 'SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c'
    setup = variables / 'SetupMode-8be4df61-93ca-11d2-aa0d-00e098032b8c'
    if secure.read_bytes()[4:] != b'\x01' or setup.read_bytes()[4:] != b'\x00':
        raise RuntimeError('Managed TPM policy requires active Secure Boot outside Setup Mode')


def require_predictions(entries):
    values = {entry['pcr']: entry['values'] for entry in entries}
    if set(values) != {4, 7, 13} or not all(values.values()):
        raise RuntimeError('Incomplete TPM policy: SHA256 PCR4/7/13 are mandatory')
    if values[13] != ['00' * 32]:
        raise RuntimeError('TPM policy must reject executable companion extensions')


def read_policy(config):
    policy = json.loads(protected(config['policy']).read_text())
    if policy['pcrBank'] != 'sha256':
        raise RuntimeError('TPM policy must use SHA256')
    require_predictions(policy['pcrValues'])
    if not isinstance(policy.get('nvIndex'), int):
        raise RuntimeError('Missing TPM NV policy identity')
    return policy


def metadata(config):
    return json.loads(run('cryptsetup', 'luksDump', '--dump-json-metadata',
                          config['rootDevice'], capture=True))


def tpm_tokens(data):
    return {key: token for key, token in data['tokens'].items()
            if token.get('type') == 'systemd-tpm2'}


def managed_tokens(data, policy):
    return {key: token for key, token in tpm_tokens(data).items()
            if token.get('tpm2_pcrlock') and token.get('tpm2-pcrs') == [13]
            and token.get('tpm2-pcr-bank') == 'sha256'
            and token.get('tpm2_pcrlock_nv') == policy['nvHandle']
            and not token.get('tpm2-pin') and not token.get('tpm2_pubkey')
            and not token.get('tpm2_pubkey_pcrs')}


def require_password_slot(data):
    token_slots = {slot for token in data['tokens'].values()
                   for slot in token.get('keyslots', [])}
    if not set(data['keyslots']) - token_slots:
        raise RuntimeError('A separate passphrase/recovery keyslot must remain enrolled')


@contextlib.contextmanager
def component_snapshot(config):
    """Only eligible system closures may contribute generation alternatives."""
    source = protected(config['components'], directory=True)
    with tempfile.TemporaryDirectory(prefix='components-', dir=config['state']) as temporary:
        destination = Path(temporary)
        for child in source.iterdir():
            if child.name == '635-lanzaboote.pcrlock.d':
                continue
            # All mutable sources are root-owned and never followed via links.
            for item in [child, *child.rglob('*')] if child.is_dir() else [child]:
                protected(item, directory=item.is_dir())
            if child.is_dir():
                shutil.copytree(child, destination / child.name)
            else:
                shutil.copyfile(child, destination / child.name)
        generations = destination / '635-lanzaboote.pcrlock.d'
        generations.mkdir()
        count = 0
        for measurement in (source / generations.name).glob('*.pcrlock'):
            # Specialisations need their own eligibility check. Do not silently
            # admit an arbitrary suffix merely because its parent is eligible.
            match = re.fullmatch(r'(\d+)(?:-(.+))?\.pcrlock', measurement.name)
            if not match:
                raise RuntimeError(f'Unrecognized generation measurement: {measurement.name}')
            system = Path(f'/nix/var/nix/profiles/system-{match[1]}-link')
            if not eligible(system):
                continue
            if match[2]:
                system = system / 'specialisation' / match[2]
            if not system.exists() or not eligible(system):
                continue
            shutil.copyfile(protected(measurement), generations / measurement.name)
            count += 1
        if not count:
            raise RuntimeError('No eligible installed generation measurements')
        yield destination


def export_policy(config):
    """Repair missing export even when upstream skips an identical policy."""
    expected = read_policy(config)
    for esp in config['esps']:
        if not os.path.ismount(esp):
            raise RuntimeError(f'EFI system partition is not mounted: {esp}')
        directory = Path(esp) / 'loader/credentials'
        directory.mkdir(parents=True, exist_ok=True)
        protected(directory, directory=True)
        target = directory / 'pcrlock.nixosctl.cred'
        if target.exists():
            protected(target)
            try:
                exported = json.loads(run('systemd-creds', 'decrypt', '--allow-null',
                                          '--name=pcrlock.nixosctl', str(target), '-', capture=True))
                if exported == expected:
                    continue
            except (subprocess.CalledProcessError, ValueError):
                pass
        fd, name = tempfile.mkstemp(prefix='.pcrlock-', dir=directory)
        os.close(fd)
        try:
            run('systemd-creds', 'encrypt', '--with-key=null', '--name=pcrlock.nixosctl',
                config['policy'], name)
            with open(name, 'rb') as stream:
                os.fsync(stream.fileno())
            os.replace(name, target)
            verified = json.loads(run('systemd-creds', 'decrypt', '--allow-null',
                                      '--name=pcrlock.nixosctl', str(target), '-', capture=True))
            if verified != expected:
                raise RuntimeError(f'Boot policy export differs from persistent policy: {target}')
            directory_fd = os.open(directory, os.O_RDONLY | os.O_DIRECTORY)
            try:
                os.fsync(directory_fd)
            finally:
                os.close(directory_fd)
        finally:
            Path(name).unlink(missing_ok=True)


def enrollment_state(config):
    """Read prerequisites without changing credentials, policy, or NV state."""
    require_boot()
    if run(config['pcrlock'], 'is-supported', capture=True).strip() != 'yes':
        raise RuntimeError('TPM does not support managed PCR policies')
    policy = Path(config['policy'])
    protected(policy.parent, directory=True)
    previous = read_policy(config) if policy.exists() or policy.is_symlink() else None
    enrolled = metadata(config)
    bound = {key: token for key, token in tpm_tokens(enrolled).items()
             if token.get('tpm2_pcrlock')}
    if bound and previous is None:
        raise RuntimeError('Enrolled PCR policy is missing; refusing to create replacement NV authority')
    if bound and any(token.get('tpm2_pcrlock_nv') != previous['nvHandle']
                     for token in bound.values()):
        raise RuntimeError('Enrolled TPM token and policy NV identity differ; refusing replacement authority')
    return previous, enrolled, bound


def prediction_command(config, components):
    return [config['pcrlock'], '--strict=yes', '--pcr=4', '--pcr=7', '--pcr=13',
            f'--components={config["staticComponents"]}', f'--components={components}',
            f'--policy={config["policy"]}', '--location=770', '--entry-token=literal:nixosctl']


def verify_token(config, enrolled, policy):
    candidates = managed_tokens(enrolled, policy)
    if not candidates:
        raise RuntimeError('Existing PCR policy token does not meet the managed policy contract')
    token_id = next(iter(candidates))
    run('cryptsetup', 'open', '--test-passphrase', '--token-only', f'--token-id={token_id}',
        config['rootDevice'])


def preflight(config):
    previous, enrolled, bound = enrollment_state(config)
    require_password_slot(enrolled)
    with component_snapshot(config) as components:
        predictions = json.loads(run(*prediction_command(config, components),
                                     '--json=short', 'predict', capture=True))
        require_predictions(predictions['sha256'])
    if bound:
        verify_token(config, enrolled, previous)
    print('Managed TPM enrollment prerequisites verified; credentials unchanged')


def update(config):
    previous, enrolled, bound = enrollment_state(config)
    for action in ['lock-secureboot-policy', 'lock-secureboot-authority']:
        run(config['pcrlock'], action)
    with component_snapshot(config) as components:
        command = prediction_command(config, components)
        # Predict first for clear failure before any NV mutation. make-policy
        # repeats strict validation; this check is not a substitute for it.
        predictions = json.loads(run(*command, '--json=short', 'predict', capture=True))
        require_predictions(predictions['sha256'])
        run(*command, 'make-policy')
    current = read_policy(config)
    if previous is not None and previous['nvHandle'] != current['nvHandle']:
        raise RuntimeError('TPM policy unexpectedly changed its NV authority')
    if bound:
        # An identical-prediction no-op does not consult the TPM. After a power
        # loss between NV and JSON writes, matching JSON is not proof that the
        # TPM still authorizes it. Verify before exporting or claiming success.
        verify_token(config, enrolled, current)
    export_policy(config)
    print('Managed TPM policy covers SHA256 PCR4/7/13; boot credential verified')


def install(config):
    links = sorted(str(path) for path in Path('/nix/var/nix/profiles').glob('system-*-link'))
    if not links:
        raise RuntimeError('No system generation profiles')
    for esp in config['esps']:
        command = shlex.split(config['installCommand'].replace('\\\n', ' '), comments=True)
        command += [f'--public-key={config["publicKey"]}', f'--private-key={config["privateKey"]}',
                    f'--pcrlock-directory={config["components"]}']
        if config['protectedSystem']:
            command += [f'--protected-system={config["protectedSystem"]}']
        run(*command, esp, *links)
    if not eligible('/run/booted-system'):
        if Path(config['policy']).exists():
            raise RuntimeError('Refusing to update an enrolled policy from an ineligible boot')
        print('Prepared signed generations; boot an eligible generation before TPM enrollment')
        return
    update(config)


def enroll(config):
    preflight(config)
    update(config)
    before = metadata(config)
    require_password_slot(before)
    # No --wipe-slot here: prove the new token can actually unseal before
    # removing any old token. systemd skips its own unseal check for pcrlock.
    run('systemd-cryptenroll', config['rootDevice'], f'--tpm2-device={config["device"]}',
        f'--tpm2-pcrlock={config["policy"]}', '--tpm2-with-pin=no', '--tpm2-public-key=',
        '--tpm2-pcrs=13:sha256=' + '00' * 32)
    after = metadata(config)
    candidates = managed_tokens(after, read_policy(config))
    if len(candidates) != 1:
        raise RuntimeError('Expected exactly one managed no-PIN TPM token; old slots retained')
    token_id, token = next(iter(candidates.items()))
    run('cryptsetup', 'open', '--test-passphrase', '--token-only', f'--token-id={token_id}',
        config['rootDevice'])
    old_slots = {slot for key, old in tpm_tokens(after).items() if key != token_id
                 for slot in old['keyslots']} - set(token['keyslots'])
    if old_slots:
        run('systemd-cryptenroll', config['rootDevice'], '--wipe-slot=' + ','.join(sorted(old_slots)))
    require_password_slot(metadata(config))
    print('Managed TPM token verified; passphrase slot retained')


def main():
    if os.geteuid() != 0:
        raise RuntimeError('TPM policy administration requires root')
    config = json.loads(Path(sys.argv[1]).read_text())
    # Keep policy creation, enrollment and verification on the same configured
    # device/bank, independent of an invoking root shell's inherited defaults.
    os.environ['SYSTEMD_TPM2_DEVICE'] = config['device']
    os.environ['SYSTEMD_TPM2_HASH_ALGORITHMS'] = 'sha256'
    action = sys.argv[2]
    state = Path(config['state'])
    state.mkdir(parents=True, exist_ok=True, mode=0o700)
    protected(state, directory=True)
    fd = os.open(state / 'lock', os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
    with os.fdopen(fd, 'r+') as lock:
        protected(state / 'lock')
        fcntl.flock(lock, fcntl.LOCK_EX)
        if action == 'install':
            install(config)
        elif action == 'update':
            update(config)
        elif action == 'enroll':
            enroll(config)
        elif action == 'preflight':
            preflight(config)
        elif action == 'status':
            require_boot()
            policy = read_policy(config)
            data = metadata(config)
            managed = managed_tokens(data, policy)
            print(json.dumps({'requiredPcrs': [4, 7, 13], 'pcrBank': policy['pcrBank'],
                              'pcrs': [entry['pcr'] for entry in policy['pcrValues']],
                              'nvIndex': policy['nvIndex'], 'eligibleBoot': True,
                              'managedTokens': list(managed),
                              'unmanagedTokens': sorted(set(tpm_tokens(data)) - set(managed))}))
        else:
            raise RuntimeError(f'Unknown TPM policy action: {action}')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f'TPM policy: {error}', file=sys.stderr)
        sys.exit(1)
