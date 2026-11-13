{ lib, identityAgent, forwardAgentHosts }:
let
  agent = if lib.hasInfix " " identityAgent then "\"${identityAgent}\"" else identityAgent; # ssh_config splits unquoted paths at spaces
in ''
  # Remote sessions must use the incoming agent instead of the remote
  # machine's desktop socket. Local sessions keep the 1Password agent.
  # sudo preserves SSH_AUTH_SOCK; root has no desktop agent of its own.
  Match localuser root
    IdentityAgent SSH_AUTH_SOCK
  Match exec "test -n \"$SSH_CONNECTION\" && test -S \"$SSH_AUTH_SOCK\""
    IdentityAgent SSH_AUTH_SOCK
  ${lib.optionalString (forwardAgentHosts != []) ''
  Host ${lib.concatStringsSep " " (lib.unique (forwardAgentHosts ++ map lib.toLower forwardAgentHosts))}
    ForwardAgent yes
  ''}
  Host *
    IdentityAgent ${agent}
    ForwardAgent no
''
