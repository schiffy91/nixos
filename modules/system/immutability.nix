{ inputs, config, lib, ... }: {
  boot.readOnlyNixStore = config.settings.immutability.enabled;
}
