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

E as bibliotecas de systema, que o `pkg-config` acha e o CMake vai cobrando
tarefa a tarefa conforme a obra cresce (audio, etiquetas, catalogo, espectro):

| Biblioteca        | Versao aqui | Modulo pkg-config  | Pacote do apt        |
|-------------------|-------------|--------------------|----------------------|
| libmpv            | 2.2.0       | `mpv`              | `libmpv-dev`         |
| taglib            | 1.13.1      | `taglib`           | `libtag1-dev`        |
| sqlite3           | 3.45.1      | `sqlite3`          | `libsqlite3-dev`     |
| libpipewire       | 1.0.5       | `libpipewire-0.3`  | `libpipewire-0.3-dev`|
| fftw3             | 3.3.10      | `fftw3`            | `libfftw3-dev`       |
| dbus-1            | 1.14.10     | `dbus-1`           | `libdbus-1-dev`      |
| libcurl           | 8.5.0       | `libcurl`          | `libcurl4-openssl-dev`|
| fontconfig        | 2.15.0      | `fontconfig`       | `libfontconfig-dev`  |

Cada uma serve a uma feicao: `libmpv` toca, `taglib` le as etiquetas do
arquivo, `sqlite3` guarda o catalogo, `libpipewire` fala com o servidor de som,
`fftw3` transforma a onda em espectro, `dbus-1` publica o que toca para o
resto da area de trabalho, `libcurl` busca na rede, `fontconfig` acha a fonte.

Advertencia honesta: NESTE ponto do caminho o `CMakeLists.txt` ainda nao cobra
nenhuma d'ellas, porque o binario so abre janella. Estao listadas para que quem
prepara a maquina o faca uma vez, e nao oito vezes.

### Obrigatorio: os programas que o mysong chama

| Programa | Versao aqui | Para que                                        |
|----------|-------------|-------------------------------------------------|
| mpv      | 0.37.0      | Toca. E o motor de audio, chamado por libmpv    |

### Opcional: o que so acrescenta feicao

| Programa | Versao aqui | Feicao que traz, e o que se perde sem elle       |
|----------|-------------|--------------------------------------------------|
| chafa    | 1.19.0      | Desenha a capa do album. Sem elle, nao ha capa; o resto toca igual |
| yt-dlp   | 2026.08.19  | Busca audio do YouTube. Sem elle, so o disco local |

**Advertencia sobre o yt-dlp, e ella importa**: NAO o instale pelo apt. A
versao empacotada e velha demais e quebra contra o YouTube, que muda o seu
lado de fora sem avisar. O yt-dlp precisa de ser recente, e o caminho e:

```sh
uv tool install yt-dlp     # ou, se preferir: pipx install yt-dlp
```

### Tudo o que vem do apt, n'uma linha

```sh
sudo apt install build-essential cmake git libmpv-dev libtag1-dev \
  libsqlite3-dev libpipewire-0.3-dev libfftw3-dev libdbus-1-dev \
  libcurl4-openssl-dev libfontconfig-dev mpv chafa
```

Ficam de fora d'esta linha, de proposito: a fonte (que vem do release das Nerd
Fonts, secao acima), o yt-dlp (que do apt sahe velho), e o FTXUI com o doctest
(que o CMake busca por FetchContent).

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
