# mysong

Tocador de musicas para o TERMINAL, escrito em C++17, gratuito e local. Nao ha
nuvem, nao ha conta, nao ha telemetria: a musica esta no seu disco e o programa
mora na sua janella de terminal, com a estetica arrowline do RADICAL-OS.

Neste ponto do caminho o repositorio e casa recem-erguida: o binario abre,
escreve a sua marca, espera uma tecla e sahe limpo. Audio, catalogo e espectro
vem nas tarefas seguintes.

## O que exige instalado

Duas listas, e a fronteira entre ellas e firme. Sem o OBRIGATORIO o programa
nao compila, ou compila e nao toca. O OPCIONAL acrescenta feicao, e quem nao o
quiser nao perde nada do que ja funcciona. Quem so deseja compilar e ouvir para
na primeira lista.

Todas as versoes abaixo foram VERIFICADAS na maquina do autor (Ubuntu 24.04).
Nao sao minimos theoricos: sao os numeros que se sabe funccionarem.

### Obrigatorio: a FONTE com os glifos de seta

O `mysong` desenha a sua fita de estado com os glifos de seta `U+E0B0` e
`U+E0B2`, que moram na area de uso privado do Unicode e SO existem nas fontes
remendadas de Nerd Font. Isto e REQUISITO, e nao preferencia de gosto.

| Peca   | Referencia                | Observacao                            |
|--------|---------------------------|---------------------------------------|
| Fonte  | JetBrainsMono Nerd Font   | Qualquer Nerd Font serve; esta e a de referencia |

**Sem ella, o que se ve**: no logar de cada ponta afiada, um quadriculo vazio,
o chamado tofu (`□`). A fita continua correcta nas cores e nos rotulos, mas as
junccoes viram caixas, e a estetica arrowline morre. O programa nao adivinha a
presenca da fonte: o terminal nao expoe essa informacao de modo confiavel, e
inferi-la por largura de glifo e heuristica que engana. O requisito e declarado
aqui, e conferido pelo seu olho.

**Como se instala** (a de referencia, para um usuario so):

```sh
mkdir -p ~/.local/share/fonts
cd ~/.local/share/fonts
curl -fLO https://github.com/ryanoasis/nerd-fonts/releases/latest/download/JetBrainsMono.zip
unzip -o JetBrainsMono.zip && rm JetBrainsMono.zip
fc-cache -fv
```

Feito isso, aponte o seu terminal para a familia `JetBrainsMono Nerd Font`. No
Alacritty, e o campo `font.normal.family` do `alacritty.toml`.

### Obrigatorio: para compilar

| Peca              | Versao minima | Observacao                              |
|-------------------|---------------|------------------------------------------|
| Compilador C++17  | GCC 9 / Clang 10 | Verificado com GCC 13.3.0             |
| CMake             | 3.25          | Verificado com 3.28.3                    |
| Git               | qualquer      | O CMake busca as dependencias por ele    |
| Rede              | so na 1a vez  | FetchContent baixa FTXUI e doctest       |

A biblioteca de interface (FTXUI) e a de teste (doctest) NAO vem do gerenciador
de pacotes: o CMake as busca por FetchContent, cada uma presa a uma etiqueta
fixa. Logo, a primeira configuracao precisa de rede; as seguintes, nao.

## Como se compila

```sh
cmake -B build -S .
cmake --build build
```

## Como se roda

```sh
./build/mysong        # tecle q para sahir
```

## Como se roda a bateria de testes

```sh
ctest --test-dir build --output-on-failure
```

## Licenca

MIT. Veja o arquivo LICENSE.
