# Plano de Refatoração: Espectro Visual e Composição do Rio

## 1. O Problema
Atualmente, o motor do espectro (em `src/tui/espectro.cpp`) e a sua sobreposição com a letra viva (em `src/tui/letra_viva.cpp`, conhecido como o "Rio") sofrem de acoplamento e potencial gargalo de performance no terminal. A cada quadro (potencialmente 60 vezes por segundo), o sistema reconstrói o `tapete_do_rio` cela a cela para mesclar a barra do espectro com o fundo das letras.

## 2. Objetivos da Refatoração
1. **Desacoplamento de Estado:** Separar a física da batida (o decaimento do pico, a meia-vida) da geometria da UI (FTXUI).
2. **Otimização do `tapete_do_rio`:** Substituir a alocação e varredura de vetor achatado (célula a célula) por uma composição direta de blocos verticais quando a letra não estiver por cima da coluna.
3. **Extração das Cores:** Levar a lógica do *Postulado do Poente Contido* (`tinta_do_registro` e limiares `PISO_DO_QUENTE`) para uma matriz constante ou classe de estilo estrita, limpando o loop de pintura.

## 3. Ações no Código
### Passo A: Núcleo do Espectro (`src/nucleo/espectro.*`)
- Centralizar o estado do pico e a física de meia-vida no núcleo, entregando à TUI apenas as alturas resolvidas, sem que a TUI precise iterar "o tempo que passou".

### Passo B: Otimização FTXUI (`src/tui/espectro.cpp` e `letra_viva.cpp`)
- Modificar o `elemento_do_rio`. Em vez de varrer cada célula do painel `[l * largura + c]`, calcular a interseção da área da letra com a área das colunas do espectro.
- Colunas que não cruzam com a caixa da letra atual (o bloco ativo) serão renderizadas puras usando o método antigo `elemento_do_espectro`, evitando a mescla célula a célula em 80% do painel.

### Passo C: Isolamento de Tokens (`src/tui/espectro.hpp`)
- Migrar os cálculos de `glow_hot`, `glow_core` e `v500` interpolados em `espectro.cpp` para mapeamentos pré-calculados em tempo de inicialização.

## 4. Testes e Validação
- Compilar com `-DMYSONG_WERROR=ON`.
- Rodar a TUI, medir a utilização de CPU com e sem a letra visível (`l`) e confirmar que a carga de varredura (CPU usage) sobre as linhas do terminal cai substancialmente durante animação travada.