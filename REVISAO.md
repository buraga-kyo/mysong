# Correções das issues em revisão

Base: c10b2109adea47ae6a90d19a520cbe038aeaabf5.
Responsável: buraga-kyo. Project: mysong, número 4.
Escopo: buraga-kyo/mysong#208 a buraga-kyo/mysong#218.

O lote corrige as implementações existentes e verifica cada critério das issues.
As issues voltaram para Em Progresso. A conclusão exige provas direcionadas,
revisão independente e receita manual para terminal, áudio, rede e tmux.
O aceite humano permanece pendente até confirmação do usuário.

## Constatações iniciais

- A carga de outra faixa não limpa a propriedade pause do mpv.
- O tocador deduz fim natural de qualquer transição para Parado.
- A fila recebe apenas faixas já tocadas em alguns caminhos da interface.
- A janela não consulta a visibilidade nem o painel ativo do tmux.
- A seleção de letra ignora a duração e aceita o primeiro resultado.
- O estaleiro guarda totais, mas não acompanha o progresso individual.

## Evidências

Compilação concluída: `cmake --build build -j 6`.
Testes direcionados: 62 casos e 568 asserções passaram, sem falhas.
Comando: `./build/testes/mysong_testes --source-file='*prova_fila.cpp,*prova_tocador.cpp,*prova_tocador_fios.cpp,*prova_api.cpp,*prova_unidades.cpp'`.
Nenhum teste humano foi declarado aprovado. Não houve push, PR ou merge.
O usuário pediu encerrar o trabalho iniciado sem começar novas correções.
