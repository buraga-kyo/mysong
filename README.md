# mysong

Tocador de musicas para o TERMINAL, escrito em C++17, gratuito e local. Nao ha
nuvem, nao ha conta, nao ha telemetria: a musica esta no seu disco e o programa
mora na sua janella de terminal, com a estetica arrowline do RADICAL-OS.

A obra cresce tarefa a tarefa, e cada uma acrescenta feicao ao mesmo binario.
Este arquivo nao narra em que ponto do caminho ella esta, porque tal narrativa
envelhece a cada tarefa que entra: diz o que nao caduca, isto e, o que se exige
instalado, como se compila, como se roda, e como se roda a bateria de provas.

## O que exige instalado

Duas listas, e a fronteira entre ellas e firme. Sem o OBRIGATORIO da-se um de
tres: o programa nao compila; compila e nao toca; ou RECUSA ABRIR. O OPCIONAL
acrescenta feicao, e quem nao o quiser nao perde nada do que ja funcciona. Quem
so deseja compilar e ouvir para na primeira lista.

O terceiro modo e o da fonte, e a recusa e ACTIVA, nao apenas politica: ao
arrancar, o `mysong` sonda os seus requisitos, e faltando a fonte pinta a tela
das faltas, diz o que falta, e sahe sem erguer o tocador.

Todas as versoes abaixo foram VERIFICADAS na maquina do autor (Ubuntu 24.04).
Nao sao minimos theoricos: sao os numeros que se sabe funccionarem.

### Obrigatorio: a FONTE com os glifos de seta

O `mysong` desenha a sua fita de estado com os glifos de seta `U+E0B0` e
`U+E0B2`, que moram na area de uso privado do Unicode e SO existem nas fontes
remendadas de Nerd Font. Isto e REQUISITO, e nao preferencia de gosto.

| Peca   | Referencia                | Observacao                            |
|--------|---------------------------|---------------------------------------|
| Fonte  | JetBrainsMono Nerd Font   | Qualquer Nerd Font serve; esta e a de referencia |

**Sem ella, o que degradaria** (e que o senhor nao chegara a ver, porque o
programa nao abre): no logar de cada ponta afiada, um quadriculo vazio, o
chamado tofu (`□`). A fita continuaria correcta nas cores e nos rotulos, mas as
junccoes viriam caixas, e a estetica arrowline morreria. E por isso que a fonte
entra como bloqueio, e nao como aviso: o que se perde e justamente o desenho.
O programa nao adivinha a presenca da fonte por largura de glifo, que seria
heuristica a enganar: sonda os requisitos ao arrancar, e o que aqui se declara
e o que ella cobra.

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

A taboada esta inteira de proposito, e nao restricta ao que o `CMakeLists.txt`
cobra em cada momento: ella existe para que quem prepara a maquina o faca UMA
vez, pela linha do apt mais abaixo, e nao volte aqui a cada tarefa que entra.

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

### Obrigatorio: o terminal, e o servidor de som

| Peca             | Exigencia      | Verificado aqui com                     |
|------------------|----------------|-----------------------------------------|
| Terminal         | truecolor      | Alacritty 0.13.2 dentro de tmux 3.4     |
| `COLORTERM`      | `truecolor`    | O programa emitte sempre `38;2;R;G;B`   |
| Servidor de som  | PipeWire       | PipeWire 1.0.5                          |

O truecolor e exigencia dura: a taboada de cores d'esta Casa e de vinte e
quatro bits, e o programa NAO degrada para as 256 cores do cubo. N'um terminal
pobre as cores sahem erradas, e isso e limitacao declarada, nao defeito.

Alacritty dentro de tmux funcciona, e e o arranjo em que a obra se confere.
Mas fica registado o que NAO passa n'esse arranjo: protocolo de imagem algum.
Nem o kitty graphics, nem o sixel: o tmux os engole. D'onde a capa do album
nao se desenha por protocolo, e sim por MEIO-BLOCO (o caractere `▀` com
tinta e fundo differentes, dous pixeis por celula), que e o que o chafa faz e
o que atravessa o tmux inteiro.

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
./build/mysong                          # abre com a fila vazia
./build/mysong faixa.mp3 outra.flac     # abre a tocar a primeira
./build/mysong --sonda                  # so o diagnostico, em texto
```

Nao ha varredura de acervo ainda (issue #34): por ora a fila entra pela linha de
commando.

### As teclas

| tecla | o que faz |
|---|---|
| espaco | pausa tocando, retoma pausado |
| `n` / `p` | faixa seguinte, faixa anterior |
| `.` / `,` | busca cinco segundos no som, para deante ou para tras |
| `+` / `-` | volume, por degrau de cinco |
| `j` / `k` ou `↑` / `↓` | anda na lista |
| Enter ou `→` | entra (artista, album, faixa) |
| Escape, Backspace ou `←` | volta um degrau |
| `/` | busca na lista |
| `b` | baixa por URL |
| `l` | troca o espectro pela letra |
| `r` | varre o acervo outra vez |
| `q` | sahe |

## Como se roda a bateria de testes

```sh
ctest --test-dir build --output-on-failure
```

## Licenca

MIT. Veja o arquivo LICENSE.
