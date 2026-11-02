{ pkgs }:
pkgs.runCommand "proton-pdb-symbol-tests" {
  nativeBuildInputs = [ pkgs.llvmPackages.clang-unwrapped pkgs.llvmPackages.lld pkgs.pkgsCross.mingwW64.buildPackages.gcc ];
} ''
  mkdir -p "$out"
  cd "$out"
  clang --target=x86_64-pc-windows-msvc -g -gcodeview -ffunction-sections -O2 -c ${./Folded.c} -o Folded.obj
  clang --target=x86_64-pc-windows-msvc -ffunction-sections -O2 -c ${./Public.c} -o Public.obj
  lld-link /dll /noentry /debug /opt:icf /opt:ref /out:Folded.dll /pdb:Folded.pdb Folded.obj Public.obj
  x86_64-w64-mingw32-gcc -std=c11 -O2 ${./PdbSymbols.c} -o PdbSymbols.exe -ldbghelp
  rm Folded.obj Public.obj Folded.lib
''
