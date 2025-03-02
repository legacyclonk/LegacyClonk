{
  pkgs ? import <nixpkgs> {},
  lib ? pkgs.lib,

  content ? null, # path to content repository
  useSDLMainloop ? false,
  withDeveloperMode ? true,
}:

assert useSDLMainloop -> !withDeveloperMode;

pkgs.stdenv.mkDerivation {
  name = "legacyclonk";

  src = builtins.filterSource (path: type: ! builtins.elem (baseNameOf path) [
    ".git" # leave out .git as it changes often in ways that do not affect the build
    "default.nix" # default.nix might change, but the only thing that matters is what it evaluates to, and nix takes care of that
    "result" # build result is irrelevant
    "build"
  ]) ./..;

  enableParallelInstalling = false;

  nativeBuildInputs = with pkgs; [ cmake pkg-config ];

  dontStrip = true;

  buildInputs = with pkgs; [
    openssl
    curl
    fmt
    freetype
    iconv
    libjpeg
    glew
    libpng
    SDL2
    SDL2_mixer
    libnotify
    miniupnpc
    zlib
    (spdlog.overrideAttrs (final: prev: {
      cmakeFlags = prev.cmakeFlags ++ [
        (lib.cmakeBool "SPDLOG_FMT_EXTERNAL" false)
        (lib.cmakeBool "SPDLOG_USE_STD_FORMAT" true)
      ];
    }))
  ] ++ lib.optional withDeveloperMode gtk3
    ++ lib.optionals (!useSDLMainloop) [ xorg.libXpm xorg.libXxf86vm ];

  cmakeBuildType = "RelWithDebInfo";

  cmakeFlags = [
    (lib.cmakeBool "USE_SYSTEM_SPDLOG" true)
    (lib.cmakeBool "USE_SDL_MAINLOOP" useSDLMainloop)
    (lib.cmakeBool "WITH_DEVELOPER_MODE" withDeveloperMode)
  ];

  postBuild = ''
    for path in ../planet/*.c4g ${lib.optionalString (content != null) "${content}/*.c4?"}
    do
      group=$(basename "$path")
      echo "$group"
      cp -rt . "$path"
      chmod -R +w "$group"
      ./c4group "$group" -p
    done
  '';

  installPhase = ''
    runHook preInstall

    mkdir -p "$out"
    cp -t "$out" clonk c4group *.c4?

    runHook postInstall
  '';
}
