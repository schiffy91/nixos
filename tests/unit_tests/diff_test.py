from bin.diff import (
    top_ancestor, collapse, at_depth, collapse_to_persist,
)

class TestTopAncestor:
    def test_ephemeral_path_returns_first_non_mount(self):
        keep = ["/etc/nixos", "/var/log"]
        mounts = {"/", "/home"}
        assert top_ancestor("/tmp/foo/bar", keep, mounts) == "/tmp"
    def test_persisted_path_collapses_to_keep_boundary(self):
        keep = ["/etc/nixos"]
        mounts = {"/"}
        assert top_ancestor("/etc/nixos/flake.nix", keep, mounts) == "/etc/nixos"
    def test_root_mount_skipped(self):
        keep = []
        mounts = {"/"}
        assert top_ancestor("/tmp", keep, mounts) == "/tmp"
    def test_nested_keep_path_skips_ancestor(self):
        keep = ["/var/lib/nixos"]
        mounts = {"/"}
        assert top_ancestor("/var/lib/nixos/state", keep, mounts) == "/var/lib/nixos"
    def test_unrelated_keep_path_does_not_affect(self):
        keep = ["/etc/nixos"]
        mounts = {"/"}
        assert top_ancestor("/tmp/cache/file", keep, mounts) == "/tmp"

class TestCollapse:
    def test_collapses_to_top_ancestors(self):
        ephemeral = {"/tmp/a/b", "/tmp/a/c", "/tmp/d"}
        keep = ["/etc/nixos"]
        mounts = {"/"}
        result = collapse(ephemeral, keep, mounts)
        assert result == ["/tmp"]
    def test_empty_ephemeral(self):
        assert collapse(set(), [], {"/"}) == []
    def test_separate_trees_preserved(self):
        ephemeral = {"/tmp/a", "/var/cache/b"}
        result = collapse(ephemeral, [], {"/"})
        assert result == ["/tmp", "/var"]

class TestAtDepth:
    def test_depth_zero_returns_bases(self):
        assert at_depth(["/tmp", "/var"], {"/tmp/a", "/var/b"}, 0) == ["/tmp", "/var"]
    def test_depth_one_expands(self):
        ephemeral = {"/tmp/a/x", "/tmp/b/y"}
        result = at_depth(["/tmp"], ephemeral, 1)
        assert result == ["/tmp/a", "/tmp/b"]
    def test_depth_none_returns_all(self):
        ephemeral = {"/tmp/a/b/c"}
        result = at_depth(["/tmp"], ephemeral, None)
        assert result == ["/tmp/a/b/c"]
    def test_empty_bases(self):
        assert at_depth([], {"/tmp/a"}, 1) == []

class TestCollapseToPersist:
    def test_maps_files_to_keep_paths(self):
        persisted = {"/etc/nixos/flake.nix", "/var/log/syslog"}
        keep = ["/etc/nixos", "/var/log"]
        result = collapse_to_persist(persisted, keep)
        assert result == ["/etc/nixos", "/var/log"]
    def test_exact_match(self):
        result = collapse_to_persist({"/etc/nixos"}, ["/etc/nixos"])
        assert result == ["/etc/nixos"]
    def test_no_match(self):
        result = collapse_to_persist({"/tmp/random"}, ["/etc/nixos"])
        assert result == []
    def test_empty_input(self):
        assert collapse_to_persist(set(), ["/etc/nixos"]) == []
