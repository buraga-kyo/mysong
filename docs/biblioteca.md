# Biblioteca no Linux

O padrão é a pasta musical indicada por `XDG_MUSIC_DIR` em `user-dirs.dirs`,
seguida de `mysong`. Sem indicação válida, o programa procura pastas musicais
usuais no diretório pessoal e usa `Música` como último recurso.

Os downloads de `b`, da busca e das playlists remotas usam a mesma raiz:
`mysong/Artistas/<artista>/Musicas/<número - título>.<extensão>`.
Arquivos de vídeo locais podem ficar em `Artistas/<artista>/Clipes`.
O download atual continua extraindo áudio das fontes suportadas.

Pressione `B` para editar a pasta da biblioteca. Informe um caminho absoluto,
pressione Enter e reinicie o MySong. Escape cancela. A alteração preserva os
outros ajustes e comentários de `mysong.conf`. Caminhos com `#` não são aceitos
pelo formato atual. `--acervo` e `MYSONG_ACERVO` têm prioridade; remova-os para
editar pela tela. A mudança não transfere arquivos da biblioteca anterior.

As playlists aparecem em `Playlists/<nome>` como links simbólicos absolutos.
O número no nome do link preserva a ordem e permite músicas repetidas.
Criar, renomear, reordenar, retirar ou apagar uma playlist atualiza esse espelho.
Apagar uma playlist não apaga suas músicas. A pasta contém a marca `.mysong`;
não coloque arquivos próprios nela. Arquivos alheios impedem sua substituição.
Listas antigas são exportadas na abertura; nomes com barras precisam ser corrigidos.

F2 altera o título, o nome do arquivo, o índice, os links e a fila de reprodução.
A extensão permanece. Destinos já existentes são recusados sem sobrescrita.
A varredura e a renomeação não escrevem o índice simultaneamente.
