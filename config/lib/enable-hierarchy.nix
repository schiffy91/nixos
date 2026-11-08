{ lib }:
let
  # Walk option declarations, not arbitrary settings payloads or derivations.
  walk = path: schema: values: parent:
    if lib.isOption schema then
      if schema.type.name == "submodule" then
        walk path (schema.type.getSubOptions []) values parent
      else if schema.type.name == "attrsOf" && schema.type.nestedTypes.elemType.name == "submodule" then
        lib.concatLists (lib.mapAttrsToList (name: value:
          walk (path ++ [ name ]) (schema.type.getSubOptions []) value parent
        ) values)
      else []
    else let
      hasEnable = schema ? enable && lib.isOption schema.enable && schema.enable.type.name == "bool";
      current = if hasEnable then { inherit path; enable = values.enable; } else parent;
    in lib.optional (hasEnable && parent != null) {
      assertion = !values.enable || parent.enable;
      message = "${lib.showOption (path ++ [ "enable" ])} requires ${lib.showOption (parent.path ++ [ "enable" ])}; disable the child or enable its parent.";
    } ++ lib.concatLists (lib.mapAttrsToList (name: child:
      walk (path ++ [ name ]) child values.${name} current
    ) (builtins.removeAttrs schema [ "enable" ]));
in walk
