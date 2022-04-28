# Release X.Y (202?-??-??)

* Nix can now be built with LTO by passing `--enable-lto` to `configure`.
  LTO is currently only supported when building with GCC.

* When searching upwards for the root of the flake, Nix doesn’t consider
  anything that’s not owned by the current user anymore as it’s a
  security hazard.
