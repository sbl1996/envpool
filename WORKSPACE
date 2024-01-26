workspace(name = "envpool2")

load("//envpool2:workspace0.bzl", workspace0 = "workspace")

workspace0()

load("//envpool2:workspace1.bzl", workspace1 = "workspace")

workspace1()

load("//envpool2:pip.bzl", pip_workspace = "workspace")

pip_workspace()

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "hedron_compile_commands",
    sha256 = "6326c883350d86fc111a9c74d8268714144f910907884f4ab4080c17247333a5",
    # Replace the commit hash (9335ff4470f3e9238e3aa81aff4b72c528e16c38) in both places (below) with the latest (https://github.com/hedronvision/bazel-compile-commands-extractor/commits/main), rather than using the stale one here.
    # Even better, set up Renovate and let it do the work for you (see "Suggestion: Updates" in the README).
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/199ca857b05a7a4dbb332e8d229158feb3f82638.tar.gz",
    strip_prefix = "bazel-compile-commands-extractor-199ca857b05a7a4dbb332e8d229158feb3f82638",
    # When you first run this tool, it'll recommend a sha256 hash to put here with a message like: "DEBUG: Rule 'hedron_compile_commands' indicated that a canonical reproducible form can be obtained by modifying arguments sha256 = ..."
)
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")
hedron_compile_commands_setup()
load("@hedron_compile_commands//:workspace_setup_transitive.bzl", "hedron_compile_commands_setup_transitive")
hedron_compile_commands_setup_transitive()
load("@hedron_compile_commands//:workspace_setup_transitive_transitive.bzl", "hedron_compile_commands_setup_transitive_transitive")
hedron_compile_commands_setup_transitive_transitive()
load("@hedron_compile_commands//:workspace_setup_transitive_transitive_transitive.bzl", "hedron_compile_commands_setup_transitive_transitive_transitive")
hedron_compile_commands_setup_transitive_transitive_transitive()
