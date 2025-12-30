#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

emcc_cmd=${EMCC:-emcc}

compiler_jar=${CLOSURE_COMPILER_JAR:-}
if [[ -z "$compiler_jar" ]]; then
  if [[ -n "${EMSDK:-}" ]]; then
    candidate="$EMSDK/upstream/emscripten/node_modules/google-closure-compiler-java/compiler.jar"
    if [[ -f "$candidate" ]]; then
      compiler_jar=$candidate
    fi
  fi
fi
if [[ -z "$compiler_jar" ]]; then
  echo "CLOSURE_COMPILER_JAR not set and compiler.jar not found; set CLOSURE_COMPILER_JAR or EMSDK." >&2
  exit 1
fi

mkdir -p toweb assets

compiler_options2=(--compilation_level ADVANCED_OPTIMIZATIONS --isolation_mode IIFE --externs=custom-externs.js)
wasm_common=(--no-entry -Os -Wall -s WASM=1 -D_USING64BITS_ -s ASSERTIONS=0 -s NO_FILESYSTEM=1 --js-library lib.js --pre-js pre.js)

"$emcc_cmd" "${wasm_common[@]}" ulam.c isprime.c MontMultGraphic.c graphics.c copyStr.c -s "EXPORTED_FUNCTIONS=['_initUlam','_moveGraphic','_drawPartialGraphic','_nbrChanged','_getInformation','_getPixels']" -s TOTAL_MEMORY=67108864 -o ulam.wasm
"$emcc_cmd" "${wasm_common[@]}" gausspr.c isprime.c MontMultGraphic.c graphics.c -s "EXPORTED_FUNCTIONS=['_initGaussPr','_moveGraphic','_drawPartialGraphic','_nbrChanged','_getInformation','_getPixels']" -s TOTAL_MEMORY=67108864 -o gausspr.wasm

cat ulam.js common.js strings.js commonGraphics.js > ulamT.js
java -jar "$compiler_jar" "${compiler_options2[@]}" --js ulamT.js --js initGraphicNoAndroid.js --js_output_file ulamU.js
cp ulamU.js ulamV.js
cp ULAM.HTM toweb/
cp ULAM.HTM assets/ulam.html
perl replaceEmbeddedJS.pl 0000 toweb/ULAM.HTM ulamV.js ulam.wasm
cat ulam.js common.js commonAndroid.js stringsAndroid.js commonGraphics.js > ulamT.js
java -jar "$compiler_jar" "${compiler_options2[@]}" --js ulamT.js --js initGraphicAndroid.js --js androidextern.js --js_output_file ulamEA.js
perl replaceEmbeddedJSAnd.pl 0000 assets/ulam.html ulamEA.js privacidad_calc.html
cp EULAM.HTM toweb/
cp EULAM.HTM assets/eulam.html
perl replaceEmbeddedJS.pl 0000 toweb/EULAM.HTM ulamU.js ulam.wasm
java -jar "$compiler_jar" "${compiler_options2[@]}" --js ulamT.js --js initGraphicAndroid.js --js androidextern.js --js_output_file ulamSA.js
perl replaceEmbeddedJSAnd.pl 0000 assets/eulam.html ulamSA.js privacidad_calc.html

cat gausspr.js common.js strings.js commonGraphics.js > gaussprT.js
java -jar "$compiler_jar" "${compiler_options2[@]}" --js gaussprT.js --js initGraphicNoAndroid.js --js_output_file gaussprU.js
cp gaussprU.js gaussprV.js
cp GAUSSPR.HTM toweb/
cp GAUSSPR.HTM assets/gausspr.html
perl replaceEmbeddedJS.pl 0000 toweb/GAUSSPR.HTM gaussprV.js gausspr.wasm
cat gausspr.js common.js commonAndroid.js stringsAndroid.js commonGraphics.js > gaussprT.js
java -jar "$compiler_jar" "${compiler_options2[@]}" --js gaussprT.js --js initGraphicAndroid.js --js androidextern.js --js_output_file gaussprEA.js
perl replaceEmbeddedJSAnd.pl 0000 assets/gausspr.html gaussprEA.js privacidad_calc.html
cp PRGAUSS.HTM toweb/
cp PRGAUSS.HTM assets/prgauss.html
perl replaceEmbeddedJS.pl 0000 toweb/PRGAUSS.HTM gaussprU.js gausspr.wasm
java -jar "$compiler_jar" "${compiler_options2[@]}" --js gaussprT.js --js initGraphicAndroid.js --js androidextern.js --js_output_file gaussprSA.js
perl replaceEmbeddedJSAnd.pl 0000 assets/prgauss.html gaussprSA.js privacidad_calc.html

rm -f ulamT.js gaussprT.js
