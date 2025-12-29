#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

mkdir -p toweb
cp .htaccess_orig toweb/.htaccess
cp *00*js toweb/ 2>/dev/null || true
rm -f *00*js
cp *.webmanifest toweb/ 2>/dev/null || true
(
  cd toweb
  perl ../csp.pl
  perl ../csp2.pl
)
