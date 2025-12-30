#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

if [[ $# -lt 1 ]]; then
  echo "usage: $0 <id> [end]" >&2
  exit 1
fi

build_id=$1
mkdir -p toweb assets string
rm -f toweb/*

if [[ "${2:-}" == "end" ]]; then
  ./a1.sh
  exit 0
fi

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

compiler_options=(--compilation_level ADVANCED_OPTIMIZATIONS --isolation_mode IIFE --externs=custom-externs.js --js androidextern.js --js commonNoAndroid.js)
compiler_options_and=(--compilation_level ADVANCED_OPTIMIZATIONS --isolation_mode IIFE --externs=custom-externs.js --js androidextern.js --js commonAndroid.js)
compile_flags=(-r -Os -Wall -finline-functions -DNDEBUG)
common_link_flags=(-Os --no-entry -s DYNAMIC_EXECUTION=0 -s SUPPORT_ERRNO=0 -s ASSERTIONS=0 -s NO_FILESYSTEM=1 --js-library lib.js --pre-js pre.js)
common_flags_extra=()
if [[ -n "${COMMON_FLAGS:-}" ]]; then
  read -r -a common_flags_extra <<< "${COMMON_FLAGS}"
fi
js_common=("${common_link_flags[@]}" -s WASM=0 -s SINGLE_FILE=1 -s TEXTDECODER=1 "-s INCOMING_MODULE_JS_API=['preRun','noInitialRun']" -s WASM_ASYNC_COMPILATION=0 "-s ENVIRONMENT=worker" --closure 1 --memory-init-file 0 obj.o)
wasm_common=("${common_link_flags[@]}" -s WASM=1 "${common_flags_extra[@]}" -D_USING64BITS_ obj.o)

rm -f *.wasm *00*js

fsquares_files=(expression.c parseexpr.c partition.c errors.c copyStr.c bigint.c division.c baseconv.c karatsuba.c ClassicalMult.c modmult.c MontgomeryMult.c sqroot.c output.c bignbr.c showtime.c inputstr.c batch.c gcdrings.c fft.c)
fsquares_options=(-s "EXPORTED_FUNCTIONS=['_doWork','_getInputStringPtr']" -s TOTAL_MEMORY=34275328)
fsquares_js=(--js interface.js --js config.js --js common.js --js buttons.js --js feedback.js --js wizard.js)

polfact_files=(expression.c parseexpr.c partition.c errors.c copyStr.c bigint.c linkedbignbr.c division.c baseconv.c karatsuba.c ClassicalMult.c modmult.c MontgomeryMult.c sqroot.c rootseq.c lineareq.c quadraticeq.c cubiceq.c quartics.c quintics.c quinticsData.c bigrational.c output.c polynomial.c polyexpr.c multpoly.c divpoly.c fftpoly.c intpolfact.c modpolfact.c polfact.c polfacte.c bignbr.c showtime.c inputstr.c fft.c)
polfact_options=(-s "EXPORTED_FUNCTIONS=['_doWork','_getInputStringPtr']" -s TOTAL_MEMORY=301334528)
polfact_js=(--js polyfact.js --js common.js --js feedback.js)

dilog_files=(expression.c parseexpr.c partition.c errors.c copyStr.c bigint.c division.c baseconv.c karatsuba.c ClassicalMult.c modmult.c MontgomeryMult.c sqroot.c factor.c ecm.c siqs.c siqsLA.c dilog.c bignbr.c showtime.c inputstr.c fft.c)
dilog_options=(-s "EXPORTED_FUNCTIONS=['_doWork','_getInputStringPtr']" -s TOTAL_MEMORY=301989888)
dilog_js=(--js dislog.js --js config.js --js common.js --js buttons.js --js feedback.js)

quadmod_files=(expression.c parseexpr.c partition.c errors.c copyStr.c bigint.c division.c baseconv.c karatsuba.c ClassicalMult.c modmult.c MontgomeryMult.c sqroot.c factor.c ecm.c siqs.c siqsLA.c quadmod.c quadmodLL.c bignbr.c showtime.c inputstr.c fft.c)
quadmod_options=(-s "EXPORTED_FUNCTIONS=['_doWork','_getInputStringPtr']" -s TOTAL_MEMORY=301989888)
quadmod_js=(--js quadrmod.js --js config.js --js common.js --js buttons.js --js feedback.js)

gaussian_files=(GaussExpr.c parseexpr.c partition.c errors.c copyStr.c bigint.c division.c baseconv.c karatsuba.c ClassicalMult.c modmult.c MontgomeryMult.c sqroot.c factor.c ecm.c siqs.c siqsLA.c gaussian.c output.c bignbr.c showtime.c inputstr.c gcdrings.c fft.c)
gaussian_options=(-s "EXPORTED_FUNCTIONS=['_doWork','_getInputStringPtr']" -s TOTAL_MEMORY=301989888)
gaussian_js=(--js gauss.js --js config.js --js common.js --js buttons.js --js feedback.js)

ecm_files=(batch.c fft.c expression.c parseexpr.c partition.c errors.c copyStr.c bigint.c division.c baseconv.c karatsuba.c ClassicalMult.c modmult.c MontgomeryMult.c sqroot.c factor.c ecm.c siqs.c siqsLA.c ecmfront.c sumSquares.c gcdrings.c bignbr.c showtime.c inputstr.c fromBlockly.c linkedbignbr.c)
ecm_options=(-s "EXPORTED_FUNCTIONS=['_doWork','_copyString','_getInputStringPtr','_getFactorsAsciiPtr']" -s TOTAL_MEMORY=282460160)
ecm_js=(--js blocklyextern.js --js buttons.js --js ecmfront.js --js config.js --js common.js --js feedback.js --js wizard.js)

quad_files=(batch.c fft.c expression.c parseexpr.c partition.c errors.c copyStr.c bigint.c division.c baseconv.c karatsuba.c ClassicalMult.c modmult.c MontgomeryMult.c sqroot.c factor.c ecm.c siqs.c siqsLA.c quad.c quadmodLL.c output.c bignbr.c showtime.c inputstr.c)
quad_options=(-s "EXPORTED_FUNCTIONS=['_doWork','_copyString','_getInputStringPtr']" -s TOTAL_MEMORY=263192576)
quad_js=(--js quadr.js --js config.js --js common.js --js buttons.js --js feedback.js)

compile_lang() {
  local lang=$1
  perl internationalize.pl "string_${lang}.txt" string/strings.h

  "$emcc_cmd" "${compile_flags[@]}" "${fsquares_files[@]}" fsquares.c tsquares.c -o obj.o
  "$emcc_cmd" "${js_common[@]}" "${fsquares_options[@]}" -o "fsquaresW${build_id}${lang}.js"
  "$emcc_cmd" "${wasm_common[@]}" "${fsquares_options[@]}" -o "fsquares_${lang}.wasm"

  "$emcc_cmd" "${compile_flags[@]}" "${fsquares_files[@]}" fcubes.c -o obj.o
  "$emcc_cmd" "${js_common[@]}" "${fsquares_options[@]}" -o "fcubesW${build_id}${lang}.js"
  "$emcc_cmd" "${wasm_common[@]}" "${fsquares_options[@]}" -o "fcubes_${lang}.wasm"

  "$emcc_cmd" "${compile_flags[@]}" "${fsquares_files[@]}" tsqcubes.c tsquares.c -o obj.o
  "$emcc_cmd" "${js_common[@]}" "${fsquares_options[@]}" -o "tsqcubesW${build_id}${lang}.js"
  "$emcc_cmd" "${wasm_common[@]}" "${fsquares_options[@]}" -o "tsqcubes_${lang}.wasm"

  "$emcc_cmd" "${compile_flags[@]}" "${fsquares_files[@]}" contfrac.c -o obj.o
  "$emcc_cmd" "${js_common[@]}" "${fsquares_options[@]}" -o "contfracW${build_id}${lang}.js"
  "$emcc_cmd" "${wasm_common[@]}" "${fsquares_options[@]}" -o "contfrac_${lang}.wasm"

  "$emcc_cmd" "${compile_flags[@]}" -DPOLYEXPR=1 "${polfact_files[@]}" -o obj.o
  "$emcc_cmd" "${js_common[@]}" "${polfact_options[@]}" -o "polfactW${build_id}${lang}.js"
  "$emcc_cmd" "${wasm_common[@]}" "${polfact_options[@]}" -o "polfact_${lang}.wasm"

  "$emcc_cmd" "${compile_flags[@]}" "${dilog_files[@]}" -o obj.o
  "$emcc_cmd" "${js_common[@]}" "${dilog_options[@]}" -o "dilogW${build_id}${lang}.js"
  "$emcc_cmd" "${wasm_common[@]}" "${dilog_options[@]}" -o "dilog_${lang}.wasm"

  "$emcc_cmd" "${compile_flags[@]}" "${quadmod_files[@]}" -o obj.o
  "$emcc_cmd" "${js_common[@]}" "${quadmod_options[@]}" -o "quadmodW${build_id}${lang}.js"
  "$emcc_cmd" "${wasm_common[@]}" "${quadmod_options[@]}" -o "quadmod_${lang}.wasm"

  "$emcc_cmd" "${compile_flags[@]}" -DFACTORIZATION_FUNCTIONS=1 -DFACTORIZATION_APP=1 "${quad_files[@]}" -o obj.o
  "$emcc_cmd" "${js_common[@]}" "${quad_options[@]}" -o "quadW${build_id}${lang}.js"
  "$emcc_cmd" "${wasm_common[@]}" "${quad_options[@]}" -o "quad_${lang}.wasm"

  "$emcc_cmd" "${compile_flags[@]}" "${gaussian_files[@]}" -o obj.o
  "$emcc_cmd" "${js_common[@]}" "${gaussian_options[@]}" -o "gaussianW${build_id}${lang}.js"
  "$emcc_cmd" "${wasm_common[@]}" "${gaussian_options[@]}" -o "gaussian_${lang}.wasm"

  "$emcc_cmd" "${compile_flags[@]}" -DFACTORIZATION_FUNCTIONS=1 -DFACTORIZATION_APP=1 -DUSING_BLOCKLY=1 -DENABLE_VERBOSE=1 "${ecm_files[@]}" -o obj.o
  "$emcc_cmd" "${js_common[@]}" "${ecm_options[@]}" -o "ecmW${build_id}${lang}.js"
  "$emcc_cmd" "${wasm_common[@]}" "${ecm_options[@]}" -o "ecm_${lang}.wasm"

  rm -f obj.o
}

compile_lang en
compile_lang es

java -jar "$compiler_jar" "${compiler_options[@]}" --js intfwebw.js --js commonwebw.js --js_output_file intWW.js

java -jar "$compiler_jar" "${compiler_options[@]}" --define=app=0 --js cache.js --js calccode.js "${fsquares_js[@]}" --js worker.js --js_output_file WebGlue.js
java -jar "$compiler_jar" "${compiler_options_and[@]}" --define=app=0 "${fsquares_js[@]}" --js workerAndroid.js --js_output_file AndroidGlue.js
generate_glue_code_subr() {
  local html_en=$1
  local asset_en=$2
  local html_es=$3
  local asset_es=$4
  local base=$5

  cp WebGlue.js WebGlueBak.js
  cp AndroidGlue.js AndroidGlueBak.js
  cp "$html_en" toweb/
  cp "$html_en" "assets/$asset_en"
  perl replaceEmbeddedJS.pl "$build_id" "toweb/$html_en" WebGlue.js "${base}_en.wasm" intWW.js
  perl replaceEmbeddedJSAnd.pl "$build_id" "assets/$asset_en" AndroidGlue.js calc_privacy.html
  cp "$html_es" toweb/
  cp "$html_es" "assets/$asset_es"
  perl replaceEmbeddedJS.pl "$build_id" "toweb/$html_es" WebGlueBak.js "${base}_es.wasm" intWW.js
  perl replaceEmbeddedJSAnd.pl "$build_id" "assets/$asset_es" AndroidGlueBak.js privacidad_calc.html
}

generate_glue_code_subr FSQUARES.HTM fsquares.html SUMCUAD.HTM sumcuad.html fsquares

java -jar "$compiler_jar" "${compiler_options[@]}" --define=app=2 --js cache.js --js calccode.js "${fsquares_js[@]}" --js worker.js --js_output_file WebGlue.js
java -jar "$compiler_jar" "${compiler_options_and[@]}" --define=app=2 "${fsquares_js[@]}" --js workerAndroid.js --js_output_file AndroidGlue.js
generate_glue_code_subr FCUBES.HTM fcubes.html SUMCUBOS.HTM sumcubos.html fcubes

java -jar "$compiler_jar" "${compiler_options[@]}" --define=app=4 --js cache.js --js calccode.js "${fsquares_js[@]}" --js worker.js --js_output_file WebGlue.js
java -jar "$compiler_jar" "${compiler_options_and[@]}" --define=app=4 "${fsquares_js[@]}" --js workerAndroid.js --js_output_file AndroidGlue.js
generate_glue_code_subr CONTFRAC.HTM contfrac.html FRACCONT.HTM fraccont.html contfrac

java -jar "$compiler_jar" "${compiler_options[@]}" --define=app=6 --js cache.js --js calccode.js "${fsquares_js[@]}" --js worker.js --js_output_file WebGlue.js
java -jar "$compiler_jar" "${compiler_options_and[@]}" --define=app=6 "${fsquares_js[@]}" --js workerAndroid.js --js_output_file AndroidGlue.js
generate_glue_code_subr TSQCUBES.HTM tsqcubes.html TCUADCUB.HTM tcuadcub.html tsqcubes

java -jar "$compiler_jar" "${compiler_options[@]}" --js ecmfwebw.js --js commonwebw.js --js_output_file intWW.js

java -jar "$compiler_jar" "${compiler_options[@]}" --define=android=0 --js cache.js --js calccode.js "${polfact_js[@]}" --js worker.js --js_output_file WebGlue.js
java -jar "$compiler_jar" "${compiler_options_and[@]}" --define=android=1 "${polfact_js[@]}" --js workerAndroid.js --js_output_file AndroidGlue.js
generate_glue_code_subr POLFACT.HTM polfact.html FACTPOL.HTM factpol.html polfact

java -jar "$compiler_jar" "${compiler_options[@]}" --js cache.js --js calccode.js "${dilog_js[@]}" --js worker.js --js_output_file WebGlue.js
java -jar "$compiler_jar" "${compiler_options_and[@]}" "${dilog_js[@]}" --js workerAndroid.js --js_output_file AndroidGlue.js
generate_glue_code_subr dilog.htm dilog.html logdi.htm logdi.html dilog

java -jar "$compiler_jar" "${compiler_options[@]}" --js cache.js --js calccode.js "${quadmod_js[@]}" --js worker.js --js_output_file WebGlue.js
java -jar "$compiler_jar" "${compiler_options_and[@]}" "${quadmod_js[@]}" --js workerAndroid.js --js_output_file AndroidGlue.js
generate_glue_code_subr QUADMOD.HTM quadmod.html CUADMOD.HTM cuadmod.html quadmod

java -jar "$compiler_jar" "${compiler_options[@]}" --js cache.js --js calccode.js "${gaussian_js[@]}" --js worker.js --js_output_file WebGlue.js
java -jar "$compiler_jar" "${compiler_options_and[@]}" "${gaussian_js[@]}" --js workerAndroid.js --js_output_file AndroidGlue.js
generate_glue_code_subr GAUSSIAN.HTM gaussian.html GAUSIANO.HTM gausiano.html gaussian

java -jar "$compiler_jar" "${compiler_options[@]}" --js cache.js --js calccode.js --js ecmNoAndroid.js "${ecm_js[@]}" --js worker.js --js_output_file WebGlue.js
java -jar "$compiler_jar" "${compiler_options_and[@]}" --js ecmAndroid.js "${ecm_js[@]}" --js workerAndroid.js --js_output_file AndroidGlue.js
generate_glue_code_subr ECM.HTM ecm.html ECMC.HTM ecmc.html ecm
cp calculatorSW.js toweb/calcSW.js

java -jar "$compiler_jar" "${compiler_options[@]}" --js cache.js --js calccode.js "${quad_js[@]}" --js worker.js --js_output_file WebGlue.js
java -jar "$compiler_jar" "${compiler_options_and[@]}" "${quad_js[@]}" --js workerAndroid.js --js_output_file AndroidGlue.js
generate_glue_code_subr QUAD.HTM quad.html CUAD.HTM cuad.html quad

java -jar "$compiler_jar" "${compiler_options[@]}" --js dist.js --js common.js --js_output_file distE.js
cp distE.js distS.js
cp DIST.HTM toweb/
perl replaceEmbeddedJS.pl "$build_id" toweb/DIST.HTM distS.js
cp DISTANCE.HTM toweb/
perl replaceEmbeddedJS.pl "$build_id" toweb/DISTANCE.HTM distE.js

./w.sh
rm -f *.wasm
./a1.sh
