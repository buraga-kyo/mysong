#!/usr/bin/env bash
set -euo pipefail
command -v ffmpeg >/dev/null || { echo 'Instale o ffmpeg antes de continuar.' >&2; exit 1; }
raiz="${XDG_DATA_HOME:-$HOME/.local/share}/mysong/separador"
if command -v uv >/dev/null; then
  uv venv --python python3 --allow-existing "$raiz"
  uv pip install --python "$raiz/bin/python" --index-url https://download.pytorch.org/whl/cpu 'torch==2.5.1+cpu' 'torchaudio==2.5.1+cpu'
  uv pip install --python "$raiz/bin/python" 'demucs==4.0.1' 'numpy<2' 'soundfile>=0.12'
else
  python3 -m venv "$raiz"
  "$raiz/bin/python" -m pip install --index-url https://download.pytorch.org/whl/cpu 'torch==2.5.1+cpu' 'torchaudio==2.5.1+cpu'
  "$raiz/bin/python" -m pip install 'demucs==4.0.1' 'numpy<2' 'soundfile>=0.12'
fi
printf 'Separador instalado. Abra o MySong e pressione i.\n'
