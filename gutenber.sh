#!/bin/bash

# Diretórios de origem e destino
SRC_DIR="epub"
DEST_DIR="../gutenberg"
MAX_SIZE=$((10 * 1024 * 1024)) # 10 MB em bytes
TOTAL_SIZE=0

# Cria o diretório de destino se não existir
mkdir -p "$DEST_DIR"

# Para cada subdiretório em SRC_DIR
for dir in "$SRC_DIR"/*/; do
    # Encontra o primeiro arquivo .txt na subpasta
    txt_file=$(find "$dir" -maxdepth 1 -type f -name "*.txt" | head -n 1)
    if [[ -n "$txt_file" ]]; then
        # Obtém o tamanho do arquivo
        file_size=$(stat -c %s "$txt_file")
        # Verifica se ao adicionar este arquivo ultrapassa o limite
        if (( TOTAL_SIZE + file_size > MAX_SIZE )); then
            break
        fi
        # Copia o arquivo para DEST_DIR
        cp "$txt_file" "$DEST_DIR/"
        # Atualiza o tamanho total
        TOTAL_SIZE=$((TOTAL_SIZE + file_size))
    fi
done

echo "Tamanho total copiado: $TOTAL_SIZE bytes"