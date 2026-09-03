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
nao se desenha por protocolo, e sim por SYMBOLO DE BLOCO, que e o que o chafa
faz e o que atravessa o tmux inteiro: blocos, meios-blocos e quadrantes, e o
SEXTANTE quando a sua fonte o tiver. Cada symbolo leva duas cores, a tinta e o
fundo, donde a celula vale por dous, quatro ou seis pixeis.

### De onde vem a capa que se desenha

Duas fontes, e n'esta ordem. Primeiro um ARQUIVO ao lado da faixa, entre doze
nomes que os ripadores usam (`cover.jpg`, `folder.png`, `front.jpg`, e outros);
depois a capa EMBUTIDA na etiqueta, no quadro APIC. O arquivo ao lado ganha
porque e o que voce pode trocar sem reescrever o mp3: basta pousar um
`cover.jpg` na pasta do album.

Baixando pelo proprio mysong, a capa vem embutida: elle pede a miniatura ao
yt-dlp, manda converte-la a jpeg e grava-a na etiqueta. Assim ella sobrevive a
mover o arquivo, e uma faixa so continua a ser um arquivo so.

Faixa sem capa nenhuma nao e falha: o painel mostra um marcador de nota
musical, para voce ver que a capa falta e nao que a tela quebrou.

### Com que symbolos a capa se desenha

O chafa recebe `--symbols=block+half+quad` e `--work=9`. As classes que metem
LETRA e CIFRA dentro da arte (`all`, `ascii`, `alpha`, `digit`, `extra`,
`technical`, `border`) ficam de fora de proposito: medido sobre uma capa do
acervo, `--symbols=all` sahe com `7`, `©`, `º` e braille no meio da imagem.

O SEXTANTE (`🬀`, U+1FB00) e o glypho de 2 por 3 sub-celullas, e e elle que dobra
os degraus por celula. So entra quando o fontconfig disser que alguma Nerd Font
installada o tem: sem glypho, elle sahiria quadriculo vazio, que e peor que o
meio-bloco. Nesta machina a JetBrainsMono Nerd Font tem os quadrantes e NAO tem
os sextantes, donde sahem quadrantes.

A chave `capa_sextantes` do arquivo de ajustes governa isso: `auto` (o padrao)
e a regra acima, `sim` pede-os sempre, e ahi quem os desenha e a fonte de
substituicao do terminal, e `nao` nunca os pede. Antes de decidir, compare os
dous no seu proprio terminal:

```sh
./build/fita_capa /caminho/da/capa.jpg 40x20              # a regra da Casa
./build/fita_capa /caminho/da/capa.jpg 40x20 sextantes    # com sextante
chafa --symbols=block+half --size=40x20 /caminho/da/capa.jpg   # o de antes
```

Dither e espaco de cor NAO se passam ao chafa, e isso e medido e nao esquecimento:
o proprio chafa declara que o `--dither` nao tem effeito com cor de 24 bits, e o
`--color-space` serve a quantizacao, que a 24 bits nao existe. As duas bandeiras
dao arquivo byte a byte egual, e bandeira que nao faz nada e mentira.

A arte enche a LARGURA do painel guardando a proporcao: a miniatura 16:9 do
YouTube fica 16:9, a arte quadrada fica quadrada, e nada se estica nem se corta.
O `--stretch` do chafa existe, e e por NAO se passar que a proporcao se guarda.

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

O aviso do compilador vale por ERRO nos alvos d'esta Casa: `-Wall -Wextra` sahe
com `-Werror`, e por isso esquecer um ramo n'um `switch` novo faz a compilacao
RECUSAR, em vez de imprimir um aviso que a rolagem come. O FTXUI e o doctest
ficam de fora da regra, que vem por FetchContent e nao sao obra d'esta Casa.

Quem topar com compilador ou versao que traga aviso inedito desliga a recusa, e
os avisos continuam a imprimir-se:

```sh
cmake -B build -S . -DMYSONG_WERROR=OFF
```

A opcao guarda-se no cache d'aquelle directorio de build: uma vez configurado
com `OFF`, assim fica ate se dizer `-DMYSONG_WERROR=ON` ou se deitar fora o
`build/`.
## Como se installa

```sh
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=~/.local
cmake --install build
```

O prefixo e o que o operador der; por omissao e `/usr/local`, que pede
privilegio. As tres pecas vao para os logares que o padrao manda, e caminho
algum delles esta escrito a mao: sahem todos do `GNUInstallDirs`.

O prefixo vae no CONFIGURE, e nao no `--prefix` do install, e a razao e a
entrada de menu: o `Exec` della leva o caminho ABSOLUTO do binario, porque a
sessao graphica nao herda o `PATH` do shell de login em boa parte dos
ambientes, e pelo nome nu a entrada nascia morta. Esse caminho fixa-se quando
se configura. Passando `--prefix` ao install, as tres pecas cahem no logar
pedido, mas o `Exec` continua a apontar para o prefixo do configure, e a
entrada de menu fica a apontar para onde o binario nao esta.

| Peca               | Onde cahe                                     |
|--------------------|-----------------------------------------------|
| o binario          | `<prefixo>/bin/mysong`                        |
| a entrada de menu  | `<prefixo>/share/applications/mysong.desktop` |
| a pagina de manual | `<prefixo>/share/man/man1/mysong.1`           |

Installando em `~/.local`, ponha o `~/.local/bin` no `PATH` para poder chamar
o `mysong` pelo nome no terminal; o menu nao precisa disso, que elle ja leva o
caminho inteiro, e o manual o `man` acha por si.

**Limitacao declarada**: a arvore installada nao se pode MOVER de logar. O
`Exec` da entrada de menu e o caminho absoluto do binario, e mudando o
directorio de logar elle passa a apontar para o vazio. Querendo outro prefixo,
configure outra vez e installe outra vez, que e barato.

Quem empacota usa o `DESTDIR` com o prefixo FINAL no configure, e e justamente
o caso que funcciona: o `Exec` diz o prefixo final, e os arquivos pousam
debaixo do embrulho.

```sh
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/usr
DESTDIR=/tmp/embrulho cmake --install build
```

## Como se roda

```sh
./build/mysong                          # abre com a fila vazia
./build/mysong faixa.mp3 outra.flac     # abre a tocar a primeira
./build/mysong --sonda                  # so o diagnostico, em texto
./build/mysong --capa                   # busca a capa que falta ao acervo
./build/mysong --versao                 # diz o nome e o numero, e sahe
./build/mysong --ajuda                  # diz as opcoes que existem, e sahe
./build/mysong -- --faixa-com-traco.mp3 # o `--` encerra as opcoes
./build/mysong --acervo=/mnt/musica     # o acervo so d'esta corrida
```

O `--versao` e o `--version` fazem o mesmo, e o `--ajuda` e o `--help` tambem:
o operador escreve em portuguez e o dedo escreve em inglez. Opcao que nao
esteja nessa taboada e RECUSADA, com a razao pelo stderr e sahida differente
de zero; ate aqui ella era tratada como caminho de faixa. A opcao vale em
qualquer logar da linha, e nao so antes das faixas: `mysong faixa.mp3 --versao`
diz a versao. Apparecendo mais de uma, a recusa manda em todas; depois della
manda a `--ajuda`, depois a `--versao`, depois o `--sonda`, e por fim o
`--capa`.

O `--capa` (issue #83) e a ordem que busca arte para o que JA esta no disco:
varre o acervo, e para cada MP3 sem capa (nem quadro APIC nem arquivo ao
lado) casa a gravacao no MusicBrainz pelos metadados do indice e embute a
capa da release que o Cover Art Archive tiver, a UMA requisicao por segundo.
Faixa que o archivo nao conheca, ou que nao case com confianca, diz-se uma
vez e fica lembrada em `capas.sqlite3` ao lado do indice: corrida seguinte
nao volta a rede por ella. Para re-tentar as lembradas, apague esse arquivo.
Sem a ordem, nada se busca: e o operador quem manda na rede d'elle.

A varredura do acervo corre em fio proprio ao abrir: a tela abre de pronto, com o
acervo da corrida anterior, e o `r` manda varrer outra vez.

### A tela

Uma linha de CABECALHO no alto, o TRILHO do progresso logo abaixo, e o resto
partido em duas metades: a pauta das musicas a esquerda, e o painel (capa e
espectro) a direita. Barra lateral alguma: o menu d'esta Casa e uma fita de
abas, a maneira da topbar do RADICAL-OS.

```
 󰝚 MY SONG  󰲸 PLAYLISTS  󰇚 DOWNLOAD  󰐊  󰒮  󰒭  Montagem Lunar Celestia  00:19 / 03:09  󰕾 100%  󰒟 EMBARALHAR  󰑖 REPETIR
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 MY SONG, 42 FAIXAS, 1h29, FAIXAS                        ┃      a capa, e por baixo d'ella o espectro
```

No cabecalho: a aba corrente em bloco solido, as outras em repouso; os tres
botoes do transporte; o nome do que soa, cortado com «…» quando nao cabe; e a
direita o tempo, o volume, o EMBARALHAR e o REPETIR. Apertando a tela, essas
quatro cedem o logar INTEIRAS, da direita para a esquerda, e as abas ficam
sempre. Clicar n'uma aba, n'um botao ou n'um dos modos faz o que a tecla faz;
clicar no trilho busca a posicao.

Por cima da pauta ha uma CHAPA de uma linha: onde se esta, a conta e a vista
(`MY SONG, 42 FAIXAS, 1h29, FAIXAS`), com os degraus de dentro apartados por
«▸» (`PLAYLISTS ▸ Funk lento`). A direita d'ella vao os recados: a lista alvo,
o filtro posto, a varredura em curso, o andamento das baixas.

No pé, uma linha de dicas com as teclas mais usadas, e a palavra README a
dizer que o resto esta aqui: a linha cabe em cento e vinte collunhas, e a
taboada inteira nao cabe em linha alguma.

Abaixo de CEM collunhas o painel some e a pauta toma a tela toda. A capa nunca
passa de quarenta e cinco por cento da altura do painel, e o espectro toma o
que ella deixar.

### As teclas

| tecla | o que faz |
|---|---|
| `1` / `2` / `3` | vae a aba MY SONG, PLAYLISTS, DOWNLOAD |
| Tab | cicla as tres abas |
| `o` | dentro das MY SONG, cicla a vista: faixas, artistas, albuns |
| espaco | pausa tocando, retoma pausado |
| `n` / `p` | faixa seguinte, faixa anterior |
| `.` / `,` | busca cinco segundos no som, para deante ou para tras |
| `+` / `-` | volume, por degrau de cinco |
| `z` | liga e desliga o embaralhar |
| `x` | cicla o repetir: nenhuma, uma, todas |
| `j` / `k` ou `↑` / `↓` | anda na lista |
| `g` / `G` ou Home / End | ao principio, ao fim da lista |
| Enter ou `→` | entra (artista, album, faixa) |
| Escape, Backspace ou `←` | volta um degrau |
| `/` | filtra a lista que esta a vista |
| `s` | busca na rede, pelo yt-dlp; Enter no achado baixa-o |
| `f` | troca a fonte da busca, dentro da lista da rede: YouTube, YouTube Music, Spotify |
| `b` | baixa por URL |
| `l` | troca o espectro pela letra |
| `r` | varre o acervo outra vez |
| `P` | as listas |
| `c` | cria lista (pede o nome) |
| `R` | renomeia a lista |
| `D` | apaga a lista (pede confirmacao) |
| `S` ou `s` | responde sim a pergunta do `D`; outro caractere, Escape ou Enter e nao |
| `a` | junta a faixa eleita a lista alvo |
| `t` | retira o item eleito da lista |
| `K` / `J` | move o item para cima, para baixo |
| `v` | abre a faixa eleita em janella de video |
| `I` | le uma playlist publica do Spotify por catalogo |
| `T` | baixa TODAS as faixas da lista lida |
| F2 | renomeia a faixa eleita: o campo abre com o titulo corrente |
| Delete | manda a faixa eleita a lixeira do systema (pede confirmacao) |
| `q` | sahe |

### O rato

Clicar n'uma aba do cabecalho vae a ella; clicar n'uma faixa elege-a, e clicar
na JÁ eleita toca-a. Os tres botoes do transporte fazem o que dizem, o clique
no trilho busca a posicao, o clique no EMBARALHAR ou no REPETIR troca o modo, o
clique na capa pausa e retoma, e a roda anda tres linhas na pauta. Sobre o
cabecalho a roda fica muda: n'uma fita de tres abas ella trocaria de seccao por
acaso, com o dedo a caminho de outra peca. Botao direito nao faz nada ainda.
Dentro do tmux, isto pede `set -g mouse on`.

E ha um preco a declarar, que e a primeira cousa que se nota: pedido o modo dos
botoes, o emulador passa a entregar o CLIQUE e o ARRASTO ao programa, donde
SELECCIONAR TEXTO na tela do mysong passa a pedir a tecla Shift carregada, como
ja succede em todo programa de terminal que use rato. O mysong nao pede o modo de
toda MEXIDA, que e o que dentro do tmux vazaria lixo para o teclado: mover o rato
pela tela sem carregar em botao algum nao produz cousa alguma.

Duas buscas ha, e ellas nao sao a mesma: o `/` FILTRA o que esta a vista, sem
tocar a rede; o `s` PERGUNTA ao YouTube. Na secção NET, Enter encommenda a baixa
do achado eleito, e duas baixas correm ao mesmo tempo no maximo: as demais
esperam, e a linha do titulo diz quantas correm e quantas esperam.

Todo campo de digitar abre em LINHA PROPRIA, logo abaixo da trilha, e nunca por
cima della: teclando `s` num album ve-se ao mesmo tempo onde se esta e o que se
digita. A linha do campo tem marca a esquerda, fundo proprio e o cursor do
terminal dentro. Escape cancela e devolve ao mesmo logar, com a mesma faixa
eleita; o texto digitado perde-se, que Escape e cancelar. E emquanto o campo
esta aberto a lista CONGELA: resposta da rede, playlist lida ou varredura
concluida esperam a sua vez, e so assentam no quadro seguinte ao fechar do
campo. Vale para os cinco campos que ha: o filtro do `/`, a busca do `s`, a URL
do `b`, o nome de lista do `c` e do `R`, e a playlist do `I`.

O cursor do terminal so aparece havendo prompt aberto, e aparece como barra
QUIETA na collunha logo a seguir ao que se digitou; fechado o prompt, ele some.
Fora dai a tela nao mostra cursor algum: a vinte quadros por segundo, um cursor
que se reposiciona e o que o olho le por piscar. Morrendo o mysong por sinal
brutal (o `kill -9`), o terminal fica sem cursor, e `tput cnorm` o traz de
volta.

### O espectro por registros

As barras do espectro deixaram de ser um violeta so. A cor de cada barra diz em
que REGISTRO ella soa, isto e, em que faixa de frequencia, e vae da esquerda
para a direita como o ouvido sobe:

| registro | faixa | cor | o que costuma morar ali |
|---|---|---|---|
| GRAVES | 40 a 250 Hz | violeta | bumbo, baixo |
| MEDIOS-GRAVES | 250 Hz a 1 kHz | cyan | caixa, guitarra, o corpo da voz |
| MEDIOS-AGUDOS | 1 a 4 kHz | laranja | voz, presenca, teclados |
| AGUDOS | 4 a 16 kHz | amarelo | pratos, chimbal, o ar |

Cada coluna tem o seu degrade, escuro no pe e vivo no topo. O degrade e do
PAINEL e nao da barra: a altura da coluna diz o nivel, e a cor nunca o repete
nem o contradiz. A batida forte continua a acender rosa na coluna inteira, e o
mudo continua apagado, como sempre foi.

Diga-se com honestidade o que esta cor e: ella vem da FAIXA DE HERTZ, e nao de
instrumento reconhecido. O que sae em violeta e o grave que toca naquele
instante, seja bumbo, baixo ou a mao esquerda do piano. Separar instrumentos de
verdade pede modelo de separacao de fontes, que nao roda em tempo real dentro de
um tocador de terminal, e vender o que nao ha seria mentir na tela.

Para ver os quatro grupos lado a lado, com os nomes por baixo de cada um:

```sh
./build/fita_espectro 72 12
```

### O video

Faixa de video abre em janella PROPRIA do systema, e não dentro do terminal: a
propria documentacao do mpv diz que a sahida grafica delle nao sincroniza com o
resto do terminal. E a janella nasce com classe propria, `mysong-video`, nas duas
formas (`--x11-name` e `--wayland-app-id`), para o RADICAL-OS a governar por regra.

O audio NAO dobra: `v` cala o motor ANTES de a janella abrir. Enquanto ella viver,
as teclas de transporte governam-na e nao o motor: espaco pausa a janella, `,` e
`.` buscam nella. Fechando-a, o commando volta ao motor por si.

A varredura passou a indexar as extensoes de video, e nao so as de audio: faixa que
nao esta no indice nao se pode eleger. O `.mkv` entra pelo que o CAMINHO diz, sem
etiqueta: a taglib nao le Matroska, e por isso a duracao delle fica em zero e a
tabella mostra tempo vazio.

### O catalogo do Spotify

Colla-se a URL de uma playlist publica com `I`. A lista APPARECE antes de se baixar
cousa alguma, com titulo, artista e duracao; Enter baixa a eleita, `T` baixa todas.

A fronteira e declarada, e nao e technica: decifrar ou ripar o stream do Spotify e
quebrar proteccao technica de um servico, e esta obra nao o faz. O que se le e a
pagina publica de embutir, que o proprio Spotify serve a quem a peca sem chave nem
conta, e o que se tira della e METADADO. O audio vem do YouTube, pelo yt-dlp. E o
methodo do spotdl.

O casamento e pela DURACAO, com tolerancia de doze segundos, e depois pelo titulo.
Faixa que nao casa com confianca fica DUVIDOSA e nao se baixa: a linha do titulo
conta as colhidas, as falhadas e as duvidosas a parte, que duvidosa nao e falha e
dizer «falhou» faria o operador tentar outra vez a mesma cousa.

O ALBUM que se grava e o nome da LISTA. Fica declarado por que: a pagina publica de
embutir nao publica album algum, e o disco de onde a faixa sahiu o Spotify nao da sem
chave nem conta.

### As listas

As listas vivem em `rol.sqlite3`, ao lado do indice, e NAO dentro delle: o indice
e reconstruido a cada varredura, e taboa de lista la dentro sahiria com a
varredura.

O caminho de usar: `P` abre as listas, `c` cria uma, seta direita entra nella. A
lista em que se entrou fica sendo a ALVO, e o titulo passa a mostra-la; volta-se
ao acervo, elege-se a faixa e tecla-se `a`. Dentro da lista, `K` e `J` movem o
item, `t` retira-o, e Enter enche a fila do nucleo com a lista TODA na ordem
gravada, comecando na faixa eleita.

### Renomear e apagar a faixa

Com uma faixa eleita, `F2` abre o campo de digitar ja com o titulo corrente
dentro: Enter grava, Escape desiste. O que se grava e a tag TITLE do arquivo,
pela taglib, e a linha do indice; o NOME DO ARQUIVO nao muda, que o `.lrc` ao
lado e as listas apontam pelo caminho. Titulo vazio recusa-se, e o recado diz.

`Delete` pergunta `apagar «titulo»? s/n`, a mesma pergunta do `D` das listas, e
responde-se com a mesma tecla. Com `s`, o arquivo vae para a LIXEIRA do systema,
e nunca para o nada: e a mesma lixeira do gerenciador de arquivos, pela
especificacao freedesktop. Vive em `$XDG_DATA_HOME/Trash`, e sem a variavel em
`~/.local/share/Trash`, com o arquivo em `Trash/files/<nome>` e o bilhete de par
em `Trash/info/<nome>.trashinfo`. De la restaura-se pelo gerenciador de arquivos.

O `.lrc` ao lado vae junto, por entrada propria, para que cada um se restaure
por si. Nome ja tomado na lixeira ganha suffixo `.2`, `.3`, e o suffixo vae ao
arquivo E ao bilhete. Arquivo n'outro volume, que nao se renomeia para dentro do
`$HOME`, copia-se e apaga-se, e o recado diz que houve copia. A faixa sae do
indice e de todas as listas no mesmo quadro.

### O socket de commando

Com o mysong aberto ha um socket Unix em `$XDG_RUNTIME_DIR/mysong.sock`, por onde
se governa o tocador de fora: uma linha de JSON entra, uma linha de JSON sahe.

```sh
printf '{"verbo":"estado"}\n' | nc -U -q 1 "$XDG_RUNTIME_DIR/mysong.sock"
printf '{"verbo":"pausar"}\n' | nc -U -q 1 "$XDG_RUNTIME_DIR/mysong.sock"
```

O `-q 1` importa: sem elle o `nc` pode sahir antes de ler a resposta. O arquivo
nasce em modo `0600` dentro do `$XDG_RUNTIME_DIR`, que e `0700`, e essa e a
proteccao inteira: nao ha senha nem cifra. Fechado o programa, o arquivo sahe do
disco.

Sem `$XDG_RUNTIME_DIR` o socket nao sobe, e o mysong diz por que no stderr; a
tela abre e a musica toca do mesmo jeito. Nao ha recuo a `/tmp`, que e escripta
de todos: socket de commando la deixaria qualquer usuario da machina governar o
tocador alheio.

Havendo outro mysong ja a servir naquelle caminho, o segundo NAO lhe rouba o
socket: corre sem elle e diz por que. `./build/mysong --sonda` diz o caminho e se
ha quem escute nelle.

Os verbos todos, a forma das respostas e os codigos de erro estao em
[`docs/protocolo-do-socket.md`](docs/protocolo-do-socket.md).
## A configuracao

O que se ajustava so por variavel de ambiente cabe agora n'um arquivo que o
senhor escreve UMA vez, em `$XDG_CONFIG_HOME/mysong/mysong.conf` e, na falta da
variavel, em `~/.config/mysong/mysong.conf`.

O programa LE esse arquivo e NUNCA o escreve: o commentario que o senhor puser
la dentro nao morre. Arquivo ausente nao e erro, e nada se diz: valem os
padroes. Ha um exemplo commentado em `exemplos/mysong.conf`, e o caminho de
saber e copia-lo para o logar acima: copiado tal e qual elle nao muda cousa
alguma, que as linhas activas trazem os proprios padroes. O `~` nao se expande
dentro do arquivo, e o caminho do acervo escreve-se inteiro.

O formato e uma linha por ajuste, `chave = valor`. Linha vazia ignora-se, os
brancos das pontas aparam-se e os do meio ficam, donde caminho com espaco vale
inteiro; e o `#` abre commentario ATE O FIM DA LINHA, em qualquer ponto, donde
caminho que traga `#` no nome nao se escreve aqui, e isso e limite declarado.
Chave repetida vale a ultima.

| chave | valor que acceita | padrao |
|---|---|---|
| `acervo` | caminho de um directorio que exista | `~/Música` |
| `volume` | inteiro de 0 a 100 | `100` |
| `fonte_da_busca` | `youtube`, `youtube-music` ou `spotify` | `youtube` |
| `baixas_simultaneas` | inteiro de 1 a 8 | `2` |
| `capa_sextantes` | `auto`, `sim` ou `nao` | `auto` |

A PRECEDENCIA, do mais forte para o mais fraco: o argumento da linha de
commando, a variavel de ambiente, este arquivo, e o padrao da Casa. O
`MYSONG_ACERVO` continua a valer, e continua a ganhar do arquivo; e vae CRU,
sem se aferir, que quem o poz no perfil do shell manda, e o acervo nao ha de
mudar debaixo dos pes de quem aponta para monte de rede que ainda nao montou.

O `MYSONG_CAPA_SEXTANTES` ganha do arquivo do mesmo modo, e serve para virar o
sextante por UMA corrida sem editar arquivo nenhum. Esse AFERE-SE: palavra que
nao e `auto`, `sim` nem `nao` vira queixa e nao apaga o que o arquivo dizia.

Chave desconhecida e valor que nao presta NAO derrubam cousa alguma: cae-se no
degrau de baixo e a queixa apparece no `mysong --sonda`, que diz tambem de ONDE
veio cada ajuste que esta valendo. E o que faz o arquivo depuravel sem se ler o
codigo.

## Como se roda a bateria de testes

```sh
ctest --test-dir build --output-on-failure
```

## Licenca

MIT. Veja o arquivo LICENSE.


## Plano artístico

```text

                                                                                                                                                        
                                                                                                                                                        
            ,--,                                                                      ___                           ___                                 
,-.----.  ,--.'|                                                                    ,--.'|_    ,--,               ,--.'|_    ,--,                       
\    /  \ |  | :                     ,---,    ,---.                        __  ,-.  |  | :,' ,--.'|               |  | :,' ,--.'|               ,---.   
|   :    |:  : '                 ,-+-. /  |  '   ,'\                     ,' ,'/ /|  :  : ' : |  |,      .--.--.   :  : ' : |  |,               '   ,'\  
|   | .\ :|  ' |     ,--.--.    ,--.'|'   | /   /   |           ,--.--.  '  | |' |.;__,'  /  `--'_     /  /    '.;__,'  /  `--'_       ,---.  /   /   | 
.   : |: |'  | |    /       \  |   |  ,"' |.   ; ,. :          /       \ |  |   ,'|  |   |   ,' ,'|   |  :  /`./|  |   |   ,' ,'|     /     \.   ; ,. : 
|   |  \ :|  | :   .--.  .-. | |   | /  | |'   | |: :         .--.  .-. |'  :  /  :__,'| :   '  | |   |  :  ;_  :__,'| :   '  | |    /    / ''   | |: : 
|   : .  |'  : |__  \__\/: . . |   | |  | |'   | .; :          \__\/: . .|  | '     '  : |__ |  | :    \  \    `. '  : |__ |  | :   .    ' / '   | .; : 
:     |`-'|  | '.'| ," .--.; | |   | |  |/ |   :    |          ," .--.; |;  : |     |  | '.'|'  : |__   `----.   \|  | '.'|'  : |__ '   ; :__|   :    | 
:   : :   ;  :    ;/  /  ,.  | |   | |--'   \   \  /          /  /  ,.  ||  , ;     ;  :    ;|  | '.'| /  /`--'  /;  :    ;|  | '.'|'   | '.'|\   \  /  
|   | :   |  ,   /;  :   .'   \|   |/        `----'          ;  :   .'   \---'      |  ,   / ;  :    ;'--'.     / |  ,   / ;  :    ;|   :    : `----'   
`---'.|    ---`-' |  ,     .-./'---'                         |  ,     .-./           ---`-'  |  ,   /   `--'---'   ---`-'  |  ,   /  \   \  /           
  `---`            `--`---'                                   `--`---'                        ---`-'                        ---`-'    `----'            
                                                                                                                                                        



          _____                    _____            _____                    _____                   _______                           _____                    _____                _____                    _____                    _____                _____                    _____                    _____                   _______         
         /\    \                  /\    \          /\    \                  /\    \                 /::\    \                         /\    \                  /\    \              /\    \                  /\    \                  /\    \              /\    \                  /\    \                  /\    \                 /::\    \        
        /::\    \                /::\____\        /::\    \                /::\____\               /::::\    \                       /::\    \                /::\    \            /::\    \                /::\    \                /::\    \            /::\    \                /::\    \                /::\    \               /::::\    \       
       /::::\    \              /:::/    /       /::::\    \              /::::|   |              /::::::\    \                     /::::\    \              /::::\    \           \:::\    \               \:::\    \              /::::\    \           \:::\    \               \:::\    \              /::::\    \             /::::::\    \      
      /::::::\    \            /:::/    /       /::::::\    \            /:::::|   |             /::::::::\    \                   /::::::\    \            /::::::\    \           \:::\    \               \:::\    \            /::::::\    \           \:::\    \               \:::\    \            /::::::\    \           /::::::::\    \     
     /:::/\:::\    \          /:::/    /       /:::/\:::\    \          /::::::|   |            /:::/~~\:::\    \                 /:::/\:::\    \          /:::/\:::\    \           \:::\    \               \:::\    \          /:::/\:::\    \           \:::\    \               \:::\    \          /:::/\:::\    \         /:::/~~\:::\    \    
    /:::/__\:::\    \        /:::/    /       /:::/__\:::\    \        /:::/|::|   |           /:::/    \:::\    \               /:::/__\:::\    \        /:::/__\:::\    \           \:::\    \               \:::\    \        /:::/__\:::\    \           \:::\    \               \:::\    \        /:::/  \:::\    \       /:::/    \:::\    \   
   /::::\   \:::\    \      /:::/    /       /::::\   \:::\    \      /:::/ |::|   |          /:::/    / \:::\    \             /::::\   \:::\    \      /::::\   \:::\    \          /::::\    \              /::::\    \       \:::\   \:::\    \          /::::\    \              /::::\    \      /:::/    \:::\    \     /:::/    / \:::\    \  
  /::::::\   \:::\    \    /:::/    /       /::::::\   \:::\    \    /:::/  |::|   | _____   /:::/____/   \:::\____\           /::::::\   \:::\    \    /::::::\   \:::\    \        /::::::\    \    ____    /::::::\    \    ___\:::\   \:::\    \        /::::::\    \    ____    /::::::\    \    /:::/    / \:::\    \   /:::/____/   \:::\____\ 
 /:::/\:::\   \:::\____\  /:::/    /       /:::/\:::\   \:::\    \  /:::/   |::|   |/\    \ |:::|    |     |:::|    |         /:::/\:::\   \:::\    \  /:::/\:::\   \:::\____\      /:::/\:::\    \  /\   \  /:::/\:::\    \  /\   \:::\   \:::\    \      /:::/\:::\    \  /\   \  /:::/\:::\    \  /:::/    /   \:::\    \ |:::|    |     |:::|    |
/:::/  \:::\   \:::|    |/:::/____/       /:::/  \:::\   \:::\____\/:: /    |::|   /::\____\|:::|____|     |:::|    |        /:::/  \:::\   \:::\____\/:::/  \:::\   \:::|    |    /:::/  \:::\____\/::\   \/:::/  \:::\____\/::\   \:::\   \:::\____\    /:::/  \:::\____\/::\   \/:::/  \:::\____\/:::/____/     \:::\____\|:::|____|     |:::|    |
\::/    \:::\  /:::|____|\:::\    \       \::/    \:::\  /:::/    /\::/    /|::|  /:::/    / \:::\    \   /:::/    /         \::/    \:::\  /:::/    /\::/   |::::\  /:::|____|   /:::/    \::/    /\:::\  /:::/    \::/    /\:::\   \:::\   \::/    /   /:::/    \::/    /\:::\  /:::/    \::/    /\:::\    \      \::/    / \:::\    \   /:::/    / 
 \/_____/\:::\/:::/    /  \:::\    \       \/____/ \:::\/:::/    /  \/____/ |::| /:::/    /   \:::\    \ /:::/    /           \/____/ \:::\/:::/    /  \/____|:::::\/:::/    /   /:::/    / \/____/  \:::\/:::/    / \/____/  \:::\   \:::\   \/____/   /:::/    / \/____/  \:::\/:::/    / \/____/  \:::\    \      \/____/   \:::\    \ /:::/    /  
          \::::::/    /    \:::\    \               \::::::/    /           |::|/:::/    /     \:::\    /:::/    /                     \::::::/    /         |:::::::::/    /   /:::/    /            \::::::/    /            \:::\   \:::\    \      /:::/    /            \::::::/    /            \:::\    \                \:::\    /:::/    /   
           \::::/    /      \:::\    \               \::::/    /            |::::::/    /       \:::\__/:::/    /                       \::::/    /          |::|\::::/    /   /:::/    /              \::::/____/              \:::\   \:::\____\    /:::/    /              \::::/____/              \:::\    \                \:::\__/:::/    /    
            \::/____/        \:::\    \              /:::/    /             |:::::/    /         \::::::::/    /                        /:::/    /           |::| \::/____/    \::/    /                \:::\    \               \:::\  /:::/    /    \::/    /                \:::\    \               \:::\    \                \::::::::/    /     
             ~~               \:::\    \            /:::/    /              |::::/    /           \::::::/    /                        /:::/    /            |::|  ~|           \/____/                  \:::\    \               \:::\/:::/    /      \/____/                  \:::\    \               \:::\    \                \::::::/    /      
                               \:::\    \          /:::/    /               /:::/    /             \::::/    /                        /:::/    /             |::|   |                                     \:::\    \               \::::::/    /                                 \:::\    \               \:::\    \                \::::/    /       
                                \:::\____\        /:::/    /               /:::/    /               \::/____/                        /:::/    /              \::|   |                                      \:::\____\               \::::/    /                                   \:::\____\               \:::\____\                \::/____/        
                                 \::/    /        \::/    /                \::/    /                 ~~                              \::/    /                \:|   |                                       \::/    /                \::/    /                                     \::/    /                \::/    /                 ~~              
                                  \/____/          \/____/                  \/____/                                                   \/____/                  \|___|                                        \/____/                  \/____/                                       \/____/                  \/____/                                  
                                                                                                                                                                                                                                                                                                                                                      

             o                                                                     o        o                o        o                           
            <|>                                                                   <|>     _<|>_             <|>     _<|>_                         
            / \                                                                   < >                       < >                                   
 \o_ __o    \o/     o__ __o/  \o__ __o     o__ __o           o__ __o/  \o__ __o    |        o        __o__   |        o        __o__    o__ __o   
  |    v\    |     /v     |    |     |>   /v     v\         /v     |    |     |>   o__/_   <|>      />  \    o__/_   <|>      />  \    /v     v\  
 / \    <\  / \   />     / \  / \   / \  />       <\       />     / \  / \   < >   |       / \      \o       |       / \    o/        />       <\ 
 \o/     /  \o/   \      \o/  \o/   \o/  \         /       \      \o/  \o/         |       \o/       v\      |       \o/   <|         \         / 
  |     o    |     o      |    |     |    o       o         o      |    |          o        |         <\     o        |     \\         o       o  
 / \ __/>   / \    <\__  / \  / \   / \   <\__ __/>         <\__  / \  / \         <\__    / \   _\o__</     <\__    / \     _\o__</   <\__ __/>  
 \o/                                                                                                                                              
  |                                                                                                                                               
 / \                                                                                                                                              



 ____  _     ____  _      ____    ____  ____  _____  _  ____  _____  _  ____  ____ 
/  __\/ \   /  _ \/ \  /|/  _ \  /  _ \/  __\/__ __\/ \/ ___\/__ __\/ \/   _\/  _ \
|  \/|| |   | / \|| |\ ||| / \|  | / \||  \/|  / \  | ||    \  / \  | ||  /  | / \|
|  __/| |_/\| |-||| | \||| \_/|  | |-|||    /  | |  | |\___ |  | |  | ||  \_ | \_/|
\_/   \____/\_/ \|\_/  \|\____/  \_/ \|\_/\_\  \_/  \_/\____/  \_/  \_/\____/\____/
                                                                                   


   ________  _______   ________  ________  ________      ________  ________  ________   ________  ________  ________   ________  ________  ________ 
  /        \/       \ /        \/    /   \/        \    /        \/        \/        \ /        \/        \/        \ /        \/        \/        \
 /         /        //         /         /         /   /         /         /        _/_/       //        _/        _/_/       //         /         /
/       __/        //         /         /         /   /         /        _//       / /         /-        //       / /         /       --/         / 
\______/  \________/\___/____/\__/_____/\________/    \___/____/\____/___/ \______/  \________/\________/ \______/  \________/\________/\________/  



           /$$                                                           /$$     /$$             /$$     /$$                    
          | $$                                                          | $$    |__/            | $$    |__/                    
  /$$$$$$ | $$  /$$$$$$  /$$$$$$$   /$$$$$$         /$$$$$$   /$$$$$$  /$$$$$$   /$$  /$$$$$$$ /$$$$$$   /$$  /$$$$$$$  /$$$$$$ 
 /$$__  $$| $$ |____  $$| $$__  $$ /$$__  $$       |____  $$ /$$__  $$|_  $$_/  | $$ /$$_____/|_  $$_/  | $$ /$$_____/ /$$__  $$
| $$  \ $$| $$  /$$$$$$$| $$  \ $$| $$  \ $$        /$$$$$$$| $$  \__/  | $$    | $$|  $$$$$$   | $$    | $$| $$      | $$  \ $$
| $$  | $$| $$ /$$__  $$| $$  | $$| $$  | $$       /$$__  $$| $$        | $$ /$$| $$ \____  $$  | $$ /$$| $$| $$      | $$  | $$
| $$$$$$$/| $$|  $$$$$$$| $$  | $$|  $$$$$$/      |  $$$$$$$| $$        |  $$$$/| $$ /$$$$$$$/  |  $$$$/| $$|  $$$$$$$|  $$$$$$/
| $$____/ |__/ \_______/|__/  |__/ \______/        \_______/|__/         \___/  |__/|_______/    \___/  |__/ \_______/ \______/ 
| $$                                                                                                                            
| $$                                                                                                                            
|__/                                                                                                                            


           __                                                            __      __              __      __                     
          /  |                                                          /  |    /  |            /  |    /  |                    
  ______  $$ |  ______   _______    ______          ______    ______   _$$ |_   $$/   _______  _$$ |_   $$/   _______   ______  
 /      \ $$ | /      \ /       \  /      \        /      \  /      \ / $$   |  /  | /       |/ $$   |  /  | /       | /      \ 
/$$$$$$  |$$ | $$$$$$  |$$$$$$$  |/$$$$$$  |       $$$$$$  |/$$$$$$  |$$$$$$/   $$ |/$$$$$$$/ $$$$$$/   $$ |/$$$$$$$/ /$$$$$$  |
$$ |  $$ |$$ | /    $$ |$$ |  $$ |$$ |  $$ |       /    $$ |$$ |  $$/   $$ | __ $$ |$$      \   $$ | __ $$ |$$ |      $$ |  $$ |
$$ |__$$ |$$ |/$$$$$$$ |$$ |  $$ |$$ \__$$ |      /$$$$$$$ |$$ |        $$ |/  |$$ | $$$$$$  |  $$ |/  |$$ |$$ \_____ $$ \__$$ |
$$    $$/ $$ |$$    $$ |$$ |  $$ |$$    $$/       $$    $$ |$$ |        $$  $$/ $$ |/     $$/   $$  $$/ $$ |$$       |$$    $$/ 
$$$$$$$/  $$/  $$$$$$$/ $$/   $$/  $$$$$$/         $$$$$$$/ $$/          $$$$/  $$/ $$$$$$$/     $$$$/  $$/  $$$$$$$/  $$$$$$/  
$$ |                                                                                                                            
$$ |                                                                                                                            
$$/                                                                                                                             


        _                                _   _     _   _           
       | |                              | | (_)   | | (_)          
  _ __ | | __ _ _ __   ___     __ _ _ __| |_ _ ___| |_ _  ___ ___  
 | '_ \| |/ _` | '_ \ / _ \   / _` | '__| __| / __| __| |/ __/ _ \ 
 | |_) | | (_| | | | | (_) | | (_| | |  | |_| \__ \ |_| | (_| (_) |
 | .__/|_|\__,_|_| |_|\___/   \__,_|_|   \__|_|___/\__|_|\___\___/ 
 | |                                                               
 |_|                                                               


 .----------------.  .----------------.  .----------------.  .-----------------. .----------------.   .----------------.  .----------------.  .----------------.  .----------------.  .----------------.  .----------------.  .----------------.  .----------------.  .----------------. 
| .--------------. || .--------------. || .--------------. || .--------------. || .--------------. | | .--------------. || .--------------. || .--------------. || .--------------. || .--------------. || .--------------. || .--------------. || .--------------. || .--------------. |
| |   ______     | || |   _____      | || |      __      | || | ____  _____  | || |     ____     | | | |      __      | || |  _______     | || |  _________   | || |     _____    | || |    _______   | || |  _________   | || |     _____    | || |     ______   | || |     ____     | |
| |  |_   __ \   | || |  |_   _|     | || |     /  \     | || ||_   \|_   _| | || |   .'    `.   | | | |     /  \     | || | |_   __ \    | || | |  _   _  |  | || |    |_   _|   | || |   /  ___  |  | || | |  _   _  |  | || |    |_   _|   | || |   .' ___  |  | || |   .'    `.   | |
| |    | |__) |  | || |    | |       | || |    / /\ \    | || |  |   \ | |   | || |  /  .--.  \  | | | |    / /\ \    | || |   | |__) |   | || | |_/ | | \_|  | || |      | |     | || |  |  (__ \_|  | || | |_/ | | \_|  | || |      | |     | || |  / .'   \_|  | || |  /  .--.  \  | |
| |    |  ___/   | || |    | |   _   | || |   / ____ \   | || |  | |\ \| |   | || |  | |    | |  | | | |   / ____ \   | || |   |  __ /    | || |     | |      | || |      | |     | || |   '.___`-.   | || |     | |      | || |      | |     | || |  | |         | || |  | |    | |  | |
| |   _| |_      | || |   _| |__/ |  | || | _/ /    \ \_ | || | _| |_\   |_  | || |  \  `--'  /  | | | | _/ /    \ \_ | || |  _| |  \ \_  | || |    _| |_     | || |     _| |_    | || |  |`\____) |  | || |    _| |_     | || |     _| |_    | || |  \ `.___.'\  | || |  \  `--'  /  | |
| |  |_____|     | || |  |________|  | || ||____|  |____|| || ||_____|\____| | || |   `.____.'   | | | ||____|  |____|| || | |____| |___| | || |   |_____|    | || |    |_____|   | || |  |_______.'  | || |   |_____|    | || |    |_____|   | || |   `._____.'  | || |   `.____.'   | |
| |              | || |              | || |              | || |              | || |              | | | |              | || |              | || |              | || |              | || |              | || |              | || |              | || |              | || |              | |
| '--------------' || '--------------' || '--------------' || '--------------' || '--------------' | | '--------------' || '--------------' || '--------------' || '--------------' || '--------------' || '--------------' || '--------------' || '--------------' || '--------------' |
 '----------------'  '----------------'  '----------------'  '----------------'  '----------------'   '----------------'  '----------------'  '----------------'  '----------------'  '----------------'  '----------------'  '----------------'  '----------------'  '----------------' 



░▒▓███████▓▒░░▒▓█▓▒░       ░▒▓██████▓▒░░▒▓███████▓▒░ ░▒▓██████▓▒░        ░▒▓██████▓▒░░▒▓███████▓▒░▒▓████████▓▒░▒▓█▓▒░░▒▓███████▓▒░▒▓████████▓▒░▒▓█▓▒░░▒▓██████▓▒░ ░▒▓██████▓▒░  
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░   ░▒▓█▓▒░▒▓█▓▒░         ░▒▓█▓▒░   ░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░ 
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░   ░▒▓█▓▒░▒▓█▓▒░         ░▒▓█▓▒░   ░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░ 
░▒▓███████▓▒░░▒▓█▓▒░      ░▒▓████████▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓████████▓▒░▒▓███████▓▒░  ░▒▓█▓▒░   ░▒▓█▓▒░░▒▓██████▓▒░   ░▒▓█▓▒░   ░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░ 
░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░   ░▒▓█▓▒░      ░▒▓█▓▒░  ░▒▓█▓▒░   ░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░ 
░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░   ░▒▓█▓▒░      ░▒▓█▓▒░  ░▒▓█▓▒░   ░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░ 
░▒▓█▓▒░      ░▒▓████████▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░░▒▓██████▓▒░       ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░   ░▒▓█▓▒░▒▓███████▓▒░   ░▒▓█▓▒░   ░▒▓█▓▒░░▒▓██████▓▒░ ░▒▓██████▓▒░  
                                                                                                                                                                                


 ____  __      __    _  _  _____      __    ____  ____  ____  ___  ____  ____  ___  _____ 
(  _ \(  )    /__\  ( \( )(  _  )    /__\  (  _ \(_  _)(_  _)/ __)(_  _)(_  _)/ __)(  _  )
 )___/ )(__  /(__)\  )  (  )(_)(    /(__)\  )   /  )(   _)(_ \__ \  )(   _)(_( (__  )(_)( 
(__)  (____)(__)(__)(_)\_)(_____)  (__)(__)(_)\_) (__) (____)(___/ (__) (____)\___)(_____)



.------..------..------..------..------.     .------..------..------..------..------..------..------..------..------.
|P.--. ||L.--. ||A.--. ||N.--. ||O.--. |.-.  |A.--. ||R.--. ||T.--. ||I.--. ||S.--. ||T.--. ||I.--. ||C.--. ||O.--. |
| :/\: || :/\: || (\/) || :(): || :/\: ((5)) | (\/) || :(): || :/\: || (\/) || :/\: || :/\: || (\/) || :/\: || :/\: |
| (__) || (__) || :\/: || ()() || :\/: |'-.-.| :\/: || ()() || (__) || :\/: || :\/: || (__) || :\/: || :\/: || :\/: |
| '--'P|| '--'L|| '--'A|| '--'N|| '--'O| ((1)) '--'A|| '--'R|| '--'T|| '--'I|| '--'S|| '--'T|| '--'I|| '--'C|| '--'O|
`------'`------'`------'`------'`------'  '-'`------'`------'`------'`------'`------'`------'`------'`------'`------'



 ____  _       ____  ____    ___        ____  ____  ______  ____ _____ ______  ____   __   ___  
|    \| |     /    ||    \  /   \      /    ||    \|      ||    / ___/|      ||    | /  ] /   \ 
|  o  ) |    |  o  ||  _  ||     |    |  o  ||  D  )      | |  (   \_ |      | |  | /  / |     |
|   _/| |___ |     ||  |  ||  O  |    |     ||    /|_|  |_| |  |\__  ||_|  |_| |  |/  /  |  O  |
|  |  |     ||  _  ||  |  ||     |    |  _  ||    \  |  |   |  |/  \ |  |  |   |  /   \_ |     |
|  |  |     ||  |  ||  |  ||     |    |  |  ||  .  \ |  |   |  |\    |  |  |   |  \     ||     |
|__|  |_____||__|__||__|__| \___/     |__|__||__|\_| |__|  |____|\___|  |__|  |____\____| \___/ 
                                                                                                
      

                    ___       ___           ___           ___                    ___           ___                                      ___                                      ___           ___     
      ___          /  /\     /  /\         /  /\         /  /\                  /  /\         /  /\          ___            ___        /  /\          ___            ___        /  /\         /  /\    
     /  /\        /  /:/    /  /::\       /  /::|       /  /::\                /  /::\       /  /::\        /__/\          /__/\      /  /::\        /__/\          /__/\      /  /::\       /  /::\   
    /  /::\      /  /:/    /  /:/\:\     /  /:|:|      /  /:/\:\              /  /:/\:\     /  /:/\:\       \  \:\         \__\:\    /__/:/\:\       \  \:\         \__\:\    /  /:/\:\     /  /:/\:\  
   /  /:/\:\    /  /:/    /  /::\ \:\   /  /:/|:|__   /  /:/  \:\            /  /::\ \:\   /  /::\ \:\       \__\:\        /  /::\  _\_ \:\ \:\       \__\:\        /  /::\  /  /:/  \:\   /  /:/  \:\ 
  /  /::\ \:\  /__/:/    /__/:/\:\_\:\ /__/:/ |:| /\ /__/:/ \__\:\          /__/:/\:\_\:\ /__/:/\:\_\:\      /  /::\    __/  /:/\/ /__/\ \:\ \:\      /  /::\    __/  /:/\/ /__/:/ \  \:\ /__/:/ \__\:\
 /__/:/\:\_\:\ \  \:\    \__\/  \:\/:/ \__\/  |:|/:/ \  \:\ /  /:/          \__\/  \:\/:/ \__\/~|::\/:/     /  /:/\:\  /__/\/:/~~  \  \:\ \:\_\/     /  /:/\:\  /__/\/:/~~  \  \:\  \__\/ \  \:\ /  /:/
 \__\/  \:\/:/  \  \:\        \__\::/      |  |:/:/   \  \:\  /:/                \__\::/     |  |:|::/     /  /:/__\/  \  \::/      \  \:\_\:\      /  /:/__\/  \  \::/      \  \:\        \  \:\  /:/ 
      \  \::/    \  \:\       /  /:/       |__|::/     \  \:\/:/                 /  /:/      |  |:|\/     /__/:/        \  \:\       \  \:\/:/     /__/:/        \  \:\       \  \:\        \  \:\/:/  
       \__\/      \  \:\     /__/:/        /__/:/       \  \::/                 /__/:/       |__|:|~      \__\/          \__\/        \  \::/      \__\/          \__\/        \  \:\        \  \::/   
                   \__\/     \__\/         \__\/         \__\/                  \__\/         \__\|                                    \__\/                                    \__\/         \__\/    
          

.----. .-.     .--.  .-. .-. .----.      .--.  .----.  .---. .-. .----..---. .-. .---.  .----. 
| {}  }| |    / {} \ |  `| |/  {}  \    / {} \ | {}  }{_   _}| |{ {__ {_   _}| |/  ___}/  {}  \
| .--' | `--./  /\  \| |\  |\      /   /  /\  \| .-. \  | |  | |.-._} } | |  | |\     }\      /
`-'    `----'`-'  `-'`-' `-' `----'    `-'  `-'`-' `-'  `-'  `-'`----'  `-'  `-' `---'  `----' 
                                                                                                                               

   _______   ___            __      _____  ___      ______             __        _______  ___________  __      ________  ___________  __     ______    ______    
  |   __ "\ |"  |          /""\    (\"   \|"  \    /    " \           /""\      /"      \("     _   ")|" \    /"       )("     _   ")|" \   /" _  "\  /    " \   
  (. |__) :)||  |         /    \   |.\\   \    |  // ____  \         /    \    |:        |)__/  \\__/ ||  |  (:   \___/  )__/  \\__/ ||  | (: ( \___)// ____  \  
  |:  ____/ |:  |        /' /\  \  |: \.   \\  | /  /    ) :)       /' /\  \   |_____/   )   \\_ /    |:  |   \___  \       \\_ /    |:  |  \/ \    /  /    ) :) 
  (|  /      \  |___    //  __'  \ |.  \    \. |(: (____/ //       //  __'  \   //      /    |.  |    |.  |    __/  \\      |.  |    |.  |  //  \ _(: (____/ //  
 /|__/ \    ( \_|:  \  /   /  \\  \|    \    \ | \        /       /   /  \\  \ |:  __   \    \:  |    /\  |\  /" \   :)     \:  |    /\  |\(:   _) \\        /   
(_______)    \_______)(___/    \___)\___|\____\)  \"_____/       (___/    \___)|__|  \___)    \__|   (__\_|_)(_______/       \__|   (__\_|_)\_______)\"_____/    
                                                                                                                                                                 


       _                                _   _     _   _           
 _ __ | | __ _ _ __   ___     __ _ _ __| |_(_)___| |_(_) ___ ___  
| '_ \| |/ _` | '_ \ / _ \   / _` | '__| __| / __| __| |/ __/ _ \ 
| |_) | | (_| | | | | (_) | | (_| | |  | |_| \__ \ |_| | (_| (_) |
| .__/|_|\__,_|_| |_|\___/   \__,_|_|   \__|_|___/\__|_|\___\___/ 
|_|                                                               



                                                                                                                                                                                                           
     _____    ____               ____  _____   ______           _____                 ____        _____   _________________  ____          ______   _________________  ____       _____           _____    
 ___|\    \  |    |         ____|\   \|\    \ |\     \     ____|\    \           ____|\   \   ___|\    \ /                 \|    |     ___|\     \ /                 \|    |  ___|\    \     ____|\    \   
|    |\    \ |    |        /    /\    \\\    \| \     \   /     /\    \         /    /\    \ |    |\    \\______     ______/|    |    |    |\     \\______     ______/|    | /    /\    \   /     /\    \  
|    | |    ||    |       |    |  |    |\|    \  \     | /     /  \    \       |    |  |    ||    | |    |  \( /    /  )/   |    |    |    |/____/|   \( /    /  )/   |    ||    |  |    | /     /  \    \ 
|    |/____/||    |  ____ |    |__|    | |     \  |    ||     |    |    |      |    |__|    ||    |/____/    ' |   |   '    |    | ___|    \|   | |    ' |   |   '    |    ||    |  |____||     |    |    |
|    ||    |||    | |    ||    .--.    | |      \ |    ||     |    |    |      |    .--.    ||    |\    \      |   |        |    ||    \    \___|/       |   |        |    ||    |   ____ |     |    |    |
|    ||____|/|    | |    ||    |  |    | |    |\ \|    ||\     \  /    /|      |    |  |    ||    | |    |    /   //        |    ||    |\     \         /   //        |    ||    |  |    ||\     \  /    /|
|____|       |____|/____/||____|  |____| |____||\_____/|| \_____\/____/ |      |____|  |____||____| |____|   /___//         |____||\ ___\|_____|       /___//         |____||\ ___\/    /|| \_____\/____/ |
|    |       |    |     |||    |  |    | |    |/ \|   || \ |    ||    | /      |    |  |    ||    | |    |  |`   |          |    || |    |     |      |`   |          |    || |   /____/ | \ |    ||    | /
|____|       |____|_____|/|____|  |____| |____|   |___|/  \|____||____|/       |____|  |____||____| |____|  |____|          |____| \|____|_____|      |____|          |____| \|___|    | /  \|____||____|/ 
  \(           \(    )/     \(      )/     \(       )/       \(    )/            \(      )/    \(     )/      \(              \(      \(    )/          \(              \(     \( |____|/      \(    )/    
   '            '    '       '      '       '       '         '    '              '      '      '     '        '               '       '    '            '               '      '   )/          '    '     
                                                                                                                                                                                    '                      


        ______                                                                                                                                                                                                                                               
  _____|\     \  _____                _____        _____    _____           ____                _____        ___________      ________    ________    ____________             _____    ________    ________    ____________         _____         ____      
 /     / |     ||\    \             /      |_     |\    \   \    \      ____\_  \__           /      |_      \          \    /        \  /        \  /            \       _____\    \  /        \  /        \  /            \   _____\    \_   ____\_  \__   
|      |/     /| \\    \           /         \     \\    \   |    |    /     /     \         /         \      \    /\    \  |\         \/         /||\___/\  \\___/|     /    / \    ||\         \/         /||\___/\  \\___/| /     /|     | /     /     \  
|      |\____/ |  \\    \         |     /\    \     \\    \  |    |   /     /\      |       |     /\    \      |   \_\    | | \            /\____/ | \|____\  \___|/    |    |  /___/|| \            /\____/ | \|____\  \___|//     / /____/|/     /\      | 
|\     \    | /    \|    | ______ |    |  |    \     \|    \ |    |  |     |  |     |       |    |  |    \     |      ___/  |  \______/\   \     | |       |  |      ____\    \ |   |||  \______/\   \     | |       |  |    |     | |____|/|     |  |     | 
| \     \___|/      |    |/      \|     \/      \     |     \|    |  |     |  |     |       |     \/      \    |      \  ____\ |      | \   \____|/   __  /   / __  /    /\    \|___|/ \ |      | \   \____|/   __  /   / __ |     |  _____ |     |  |     | 
|  \     \          /            ||\      /\     \   /     /\      \ |     | /     /|       |\      /\     \  /     /\ \/    \\|______|  \   \       /  \/   /_/  ||    |/ \    \       \|______|  \   \       /  \/   /_/  ||\     \|\    \|     | /     /| 
 \  \_____\        /_____/\_____/|| \_____\ \_____\ /_____/ /______/||\     \_____/ |       | \_____\ \_____\/_____/ |\______|         \  \___\     |____________/||\____\ /____/|               \  \___\     |____________/|| \_____\|    ||\     \_____/ | 
  \ |     |       |      | |    ||| |     | |     ||      | |     | || \_____\   | /        | |     | |     ||     | | |     |          \ |   |     |           | /| |   ||    | |                \ |   |     |           | /| |     /____/|| \_____\   | /  
   \|_____|       |______|/|____|/ \|_____|\|_____||______|/|_____|/  \ |    |___|/          \|_____|\|_____||_____|/ \|_____|           \|___|     |___________|/  \|___||____|/                  \|___|     |___________|/  \|_____|    || \ |    |___|/   
                                                                       \|____|                                                                                                                                                       |____|/  \|____|        


                                                      
     _                        _   _     _   _         
 ___| |___ ___ ___    ___ ___| |_|_|___| |_|_|___ ___ 
| . | | .'|   | . |  | .'|  _|  _| |_ -|  _| |  _| . |
|  _|_|__,|_|_|___|  |__,|_| |_| |_|___|_| |_|___|___|
|_|                                                   


           __                                __  _      __  _          
    ____  / /___ _____  ____     ____ ______/ /_(_)____/ /_(_)________ 
   / __ \/ / __ `/ __ \/ __ \   / __ `/ ___/ __/ / ___/ __/ / ___/ __ \
  / /_/ / / /_/ / / / / /_/ /  / /_/ / /  / /_/ (__  ) /_/ / /__/ /_/ /
 / .___/_/\__,_/_/ /_/\____/   \__,_/_/   \__/_/____/\__/_/\___/\____/ 
/_/                                                                    


       _                          _   _    _   _        
  _ __| |__ _ _ _  ___   __ _ _ _| |_(_)__| |_(_)__ ___ 
 | '_ \ / _` | ' \/ _ \ / _` | '_|  _| (_-<  _| / _/ _ \
 | .__/_\__,_|_||_\___/ \__,_|_|  \__|_/__/\__|_\__\___/
 |_|                                                    



         __                          __  _     __  _        
   ___  / /__ ____  ___    ___ _____/ /_(_)__ / /_(_)______ 
  / _ \/ / _ `/ _ \/ _ \  / _ `/ __/ __/ (_-</ __/ / __/ _ \
 / .__/_/\_,_/_//_/\___/  \_,_/_/  \__/_/___/\__/_/\__/\___/
/_/                                                         


    ___       ___       ___       ___       ___            ___       ___       ___       ___       ___       ___       ___       ___       ___   
   /\  \     /\__\     /\  \     /\__\     /\  \          /\  \     /\  \     /\  \     /\  \     /\  \     /\  \     /\  \     /\  \     /\  \  
  /::\  \   /:/  /    /::\  \   /:| _|_   /::\  \        /::\  \   /::\  \    \:\  \   _\:\  \   /::\  \    \:\  \   _\:\  \   /::\  \   /::\  \ 
 /::\:\__\ /:/__/    /::\:\__\ /::|/\__\ /:/\:\__\      /::\:\__\ /::\:\__\   /::\__\ /\/::\__\ /\:\:\__\   /::\__\ /\/::\__\ /:/\:\__\ /:/\:\__\
 \/\::/  / \:\  \    \/\::/  / \/|::/  / \:\/:/  /      \/\::/  / \;:::/  /  /:/\/__/ \::/\/__/ \:\:\/__/  /:/\/__/ \::/\/__/ \:\ \/__/ \:\/:/  /
    \/__/   \:\__\     /:/  /    |:/  /   \::/  /         /:/  /   |:\/__/   \/__/     \:\__\    \::/  /   \/__/     \:\__\    \:\__\    \::/  / 
             \/__/     \/__/     \/__/     \/__/          \/__/     \|__|               \/__/     \/__/               \/__/     \/__/     \/__/  


.______    __          ___      .__   __.   ______           ___      .______     .___________. __       _______.___________. __    ______   ______   
|   _  \  |  |        /   \     |  \ |  |  /  __  \         /   \     |   _  \    |           ||  |     /       |           ||  |  /      | /  __  \  
|  |_)  | |  |       /  ^  \    |   \|  | |  |  |  |       /  ^  \    |  |_)  |   `---|  |----`|  |    |   (----`---|  |----`|  | |  ,----'|  |  |  | 
|   ___/  |  |      /  /_\  \   |  . `  | |  |  |  |      /  /_\  \   |      /        |  |     |  |     \   \       |  |     |  | |  |     |  |  |  | 
|  |      |  `----./  _____  \  |  |\   | |  `--'  |     /  _____  \  |  |\  \----.   |  |     |  | .----)   |      |  |     |  | |  `----.|  `--'  | 
| _|      |_______/__/     \__\ |__| \__|  \______/     /__/     \__\ | _| `._____|   |__|     |__| |_______/       |__|     |__|  \______| \______/  
                                                                                                                                                      


 ______   __         ______     __   __     ______        ______     ______     ______   __     ______     ______   __     ______     ______    
/\  == \ /\ \       /\  __ \   /\ "-.\ \   /\  __ \      /\  __ \   /\  == \   /\__  _\ /\ \   /\  ___\   /\__  _\ /\ \   /\  ___\   /\  __ \   
\ \  _-/ \ \ \____  \ \  __ \  \ \ \-.  \  \ \ \/\ \     \ \  __ \  \ \  __<   \/_/\ \/ \ \ \  \ \___  \  \/_/\ \/ \ \ \  \ \ \____  \ \ \/\ \  
 \ \_\    \ \_____\  \ \_\ \_\  \ \_\\"\_\  \ \_____\     \ \_\ \_\  \ \_\ \_\    \ \_\  \ \_\  \/\_____\    \ \_\  \ \_\  \ \_____\  \ \_____\ 
  \/_/     \/_____/   \/_/\/_/   \/_/ \/_/   \/_____/      \/_/\/_/   \/_/ /_/     \/_/   \/_/   \/_____/     \/_/   \/_/   \/_____/   \/_____/ 
                                                                                                                                                

           ░██                                                            ░██    ░██              ░██    ░██                      
           ░██                                                            ░██                     ░██                             
░████████  ░██  ░██████   ░████████   ░███████      ░██████   ░██░████ ░████████ ░██ ░███████  ░████████ ░██ ░███████   ░███████  
░██    ░██ ░██       ░██  ░██    ░██ ░██    ░██          ░██  ░███        ░██    ░██░██           ░██    ░██░██    ░██ ░██    ░██ 
░██    ░██ ░██  ░███████  ░██    ░██ ░██    ░██     ░███████  ░██         ░██    ░██ ░███████     ░██    ░██░██        ░██    ░██ 
░███   ░██ ░██ ░██   ░██  ░██    ░██ ░██    ░██    ░██   ░██  ░██         ░██    ░██       ░██    ░██    ░██░██    ░██ ░██    ░██ 
░██░█████  ░██  ░█████░██ ░██    ░██  ░███████      ░█████░██ ░██          ░████ ░██ ░███████      ░████ ░██ ░███████   ░███████  
░██                                                                                                                               
░██                                                                                                                               
                                                                                                                                  

 ________  ___       ________  ________   ________          ________  ________  _________  ___  ________  _________  ___  ________  ________     
|\   __  \|\  \     |\   __  \|\   ___  \|\   __  \        |\   __  \|\   __  \|\___   ___\\  \|\   ____\|\___   ___\\  \|\   ____\|\   __  \    
\ \  \|\  \ \  \    \ \  \|\  \ \  \\ \  \ \  \|\  \       \ \  \|\  \ \  \|\  \|___ \  \_\ \  \ \  \___|\|___ \  \_\ \  \ \  \___|\ \  \|\  \   
 \ \   ____\ \  \    \ \   __  \ \  \\ \  \ \  \\\  \       \ \   __  \ \   _  _\   \ \  \ \ \  \ \_____  \   \ \  \ \ \  \ \  \    \ \  \\\  \  
  \ \  \___|\ \  \____\ \  \ \  \ \  \\ \  \ \  \\\  \       \ \  \ \  \ \  \\  \|   \ \  \ \ \  \|____|\  \   \ \  \ \ \  \ \  \____\ \  \\\  \ 
   \ \__\    \ \_______\ \__\ \__\ \__\\ \__\ \_______\       \ \__\ \__\ \__\\ _\    \ \__\ \ \__\____\_\  \   \ \__\ \ \__\ \_______\ \_______\
    \|__|     \|_______|\|__|\|__|\|__| \|__|\|_______|        \|__|\|__|\|__|\|__|    \|__|  \|__|\_________\   \|__|  \|__|\|_______|\|_______|
                                                                                                  \|_________|                                   
                                                                                                                                                 
                                                                                                                                                 


 ██▓███   ██▓    ▄▄▄       ███▄    █  ▒█████      ▄▄▄       ██▀███  ▄▄▄█████▓ ██▓  ██████ ▄▄▄█████▓ ██▓ ▄████▄   ▒█████  
▓██░  ██▒▓██▒   ▒████▄     ██ ▀█   █ ▒██▒  ██▒   ▒████▄    ▓██ ▒ ██▒▓  ██▒ ▓▒▓██▒▒██    ▒ ▓  ██▒ ▓▒▓██▒▒██▀ ▀█  ▒██▒  ██▒
▓██░ ██▓▒▒██░   ▒██  ▀█▄  ▓██  ▀█ ██▒▒██░  ██▒   ▒██  ▀█▄  ▓██ ░▄█ ▒▒ ▓██░ ▒░▒██▒░ ▓██▄   ▒ ▓██░ ▒░▒██▒▒▓█    ▄ ▒██░  ██▒
▒██▄█▓▒ ▒▒██░   ░██▄▄▄▄██ ▓██▒  ▐▌██▒▒██   ██░   ░██▄▄▄▄██ ▒██▀▀█▄  ░ ▓██▓ ░ ░██░  ▒   ██▒░ ▓██▓ ░ ░██░▒▓▓▄ ▄██▒▒██   ██░
▒██▒ ░  ░░██████▒▓█   ▓██▒▒██░   ▓██░░ ████▓▒░    ▓█   ▓██▒░██▓ ▒██▒  ▒██▒ ░ ░██░▒██████▒▒  ▒██▒ ░ ░██░▒ ▓███▀ ░░ ████▓▒░
▒▓▒░ ░  ░░ ▒░▓  ░▒▒   ▓▒█░░ ▒░   ▒ ▒ ░ ▒░▒░▒░     ▒▒   ▓▒█░░ ▒▓ ░▒▓░  ▒ ░░   ░▓  ▒ ▒▓▒ ▒ ░  ▒ ░░   ░▓  ░ ░▒ ▒  ░░ ▒░▒░▒░ 
░▒ ░     ░ ░ ▒  ░ ▒   ▒▒ ░░ ░░   ░ ▒░  ░ ▒ ▒░      ▒   ▒▒ ░  ░▒ ░ ▒░    ░     ▒ ░░ ░▒  ░ ░    ░     ▒ ░  ░  ▒     ░ ▒ ▒░ 
░░         ░ ░    ░   ▒      ░   ░ ░ ░ ░ ░ ▒       ░   ▒     ░░   ░   ░       ▒ ░░  ░  ░    ░       ▒ ░░        ░ ░ ░ ▒  
             ░  ░     ░  ░         ░     ░ ░           ░  ░   ░               ░        ░            ░  ░ ░          ░ ░  


█ ▄▄  █    ██      ▄   ████▄     ██   █▄▄▄▄    ▄▄▄▄▀ ▄█    ▄▄▄▄▄      ▄▄▄▄▀ ▄█ ▄█▄    ████▄ 
█   █ █    █ █      █  █   █     █ █  █  ▄▀ ▀▀▀ █    ██   █     ▀▄ ▀▀▀ █    ██ █▀ ▀▄  █   █ 
█▀▀▀  █    █▄▄█ ██   █ █   █     █▄▄█ █▀▀▌      █    ██ ▄  ▀▀▀▀▄       █    ██ █   ▀  █   █ 
█     ███▄ █  █ █ █  █ ▀████     █  █ █  █     █     ▐█  ▀▄▄▄▄▀       █     ▐█ █▄  ▄▀ ▀████ 
 █        ▀   █ █  █ █              █   █     ▀       ▐              ▀       ▐ ▀███▀        
  ▀          █  █   ██             █   ▀                                                    
            ▀                     ▀                                                         
                                                                                           ░                 


 .S_sSSs    S.       .S_SSSs     .S_sSSs      sSSs_sSSs           .S_SSSs     .S_sSSs    sdSS_SSSSSSbs   .S    sSSs  sdSS_SSSSSSbs   .S    sSSs    sSSs_sSSs    
.SS~YS%%b   SS.     .SS~SSSSS   .SS~YS%%b    d%%SP~YS%%b         .SS~SSSSS   .SS~YS%%b   YSSS~S%SSSSSP  .SS   d%%SP  YSSS~S%SSSSSP  .SS   d%%SP   d%%SP~YS%%b   
S%S   `S%b  S%S     S%S   SSSS  S%S   `S%b  d%S'     `S%b        S%S   SSSS  S%S   `S%b       S%S       S%S  d%S'         S%S       S%S  d%S'    d%S'     `S%b  
S%S    S%S  S%S     S%S    S%S  S%S    S%S  S%S       S%S        S%S    S%S  S%S    S%S       S%S       S%S  S%|          S%S       S%S  S%S     S%S       S%S  
S%S    d*S  S&S     S%S SSSS%S  S%S    S&S  S&S       S&S        S%S SSSS%S  S%S    d*S       S&S       S&S  S&S          S&S       S&S  S&S     S&S       S&S  
S&S   .S*S  S&S     S&S  SSS%S  S&S    S&S  S&S       S&S        S&S  SSS%S  S&S   .S*S       S&S       S&S  Y&Ss         S&S       S&S  S&S     S&S       S&S  
S&S_sdSSS   S&S     S&S    S&S  S&S    S&S  S&S       S&S        S&S    S&S  S&S_sdSSS        S&S       S&S  `S&&S        S&S       S&S  S&S     S&S       S&S  
S&S~YSSY    S&S     S&S    S&S  S&S    S&S  S&S       S&S        S&S    S&S  S&S~YSY%b        S&S       S&S    `S*S       S&S       S&S  S&S     S&S       S&S  
S*S         S*b     S*S    S&S  S*S    S*S  S*b       d*S        S*S    S&S  S*S   `S%b       S*S       S*S     l*S       S*S       S*S  S*b     S*b       d*S  
S*S         S*S.    S*S    S*S  S*S    S*S  S*S.     .S*S        S*S    S*S  S*S    S%S       S*S       S*S    .S*P       S*S       S*S  S*S.    S*S.     .S*S  
S*S          SSSbs  S*S    S*S  S*S    S*S   SSSbs_sdSSS         S*S    S*S  S*S    S&S       S*S       S*S  sSS*S        S*S       S*S   SSSbs   SSSbs_sdSSS   
S*S           YSSP  SSS    S*S  S*S    SSS    YSSP~YSSY          SSS    S*S  S*S    SSS       S*S       S*S  YSS'         S*S       S*S    YSSP    YSSP~YSSY    
SP                         SP   SP                                      SP   SP               SP        SP                SP        SP                          
Y                          Y    Y                                       Y    Y                Y         Y                 Y         Y                           



      :::::::::  :::            :::     ::::    :::  ::::::::              :::     ::::::::: ::::::::::: ::::::::::: :::::::: ::::::::::: ::::::::::: ::::::::   :::::::: 
     :+:    :+: :+:          :+: :+:   :+:+:   :+: :+:    :+:           :+: :+:   :+:    :+:    :+:         :+:    :+:    :+:    :+:         :+:    :+:    :+: :+:    :+: 
    +:+    +:+ +:+         +:+   +:+  :+:+:+  +:+ +:+    +:+          +:+   +:+  +:+    +:+    +:+         +:+    +:+           +:+         +:+    +:+        +:+    +:+  
   +#++:++#+  +#+        +#++:++#++: +#+ +:+ +#+ +#+    +:+         +#++:++#++: +#++:++#:     +#+         +#+    +#++:++#++    +#+         +#+    +#+        +#+    +:+   
  +#+        +#+        +#+     +#+ +#+  +#+#+# +#+    +#+         +#+     +#+ +#+    +#+    +#+         +#+           +#+    +#+         +#+    +#+        +#+    +#+    
 #+#        #+#        #+#     #+# #+#   #+#+# #+#    #+#         #+#     #+# #+#    #+#    #+#         #+#    #+#    #+#    #+#         #+#    #+#    #+# #+#    #+#     
###        ########## ###     ### ###    ####  ########          ###     ### ###    ###    ###     ########### ########     ###     ########### ########   ########       



                                                                                                                                                                                                                                              
8 888888888o   8 8888                  .8.          b.             8     ,o888888o.                        .8.          8 888888888o. 8888888 8888888888  8 8888    d888888o. 8888888 8888888888  8 8888     ,o888888o.        ,o888888o.     
8 8888    `88. 8 8888                 .888.         888o.          8  . 8888     `88.                     .888.         8 8888    `88.      8 8888        8 8888  .`8888:' `88.     8 8888        8 8888    8888     `88.   . 8888     `88.   
8 8888     `88 8 8888                :88888.        Y88888o.       8 ,8 8888       `8b                   :88888.        8 8888     `88      8 8888        8 8888  8.`8888.   Y8     8 8888        8 8888 ,8 8888       `8. ,8 8888       `8b  
8 8888     ,88 8 8888               . `88888.       .`Y888888o.    8 88 8888        `8b                 . `88888.       8 8888     ,88      8 8888        8 8888  `8.`8888.         8 8888        8 8888 88 8888           88 8888        `8b 
8 8888.   ,88' 8 8888              .8. `88888.      8o. `Y888888o. 8 88 8888         88                .8. `88888.      8 8888.   ,88'      8 8888        8 8888   `8.`8888.        8 8888        8 8888 88 8888           88 8888         88 
8 888888888P'  8 8888             .8`8. `88888.     8`Y8o. `Y88888o8 88 8888         88               .8`8. `88888.     8 888888888P'       8 8888        8 8888    `8.`8888.       8 8888        8 8888 88 8888           88 8888         88 
8 8888         8 8888            .8' `8. `88888.    8   `Y8o. `Y8888 88 8888        ,8P              .8' `8. `88888.    8 8888`8b           8 8888        8 8888     `8.`8888.      8 8888        8 8888 88 8888           88 8888        ,8P 
8 8888         8 8888           .8'   `8. `88888.   8      `Y8o. `Y8 `8 8888       ,8P              .8'   `8. `88888.   8 8888 `8b.         8 8888        8 8888 8b   `8.`8888.     8 8888        8 8888 `8 8888       .8' `8 8888       ,8P  
8 8888         8 8888          .888888888. `88888.  8         `Y8o.`  ` 8888     ,88'              .888888888. `88888.  8 8888   `8b.       8 8888        8 8888 `8b.  ;8.`8888     8 8888        8 8888    8888     ,88'   ` 8888     ,88'   
8 8888         8 888888888888 .8'       `8. `88888. 8            `Yo     `8888888P'               .8'       `8. `88888. 8 8888     `88.     8 8888        8 8888  `Y8888P ,88P'     8 8888        8 8888     `8888888P'        `8888888P'     


  
   _   _   _   _   _     _   _   _   _   _   _   _   _   _  
  / \ / \ / \ / \ / \   / \ / \ / \ / \ / \ / \ / \ / \ / \ 
 ( p | l | a | n | o ) ( a | r | t | i | s | t | i | c | o )
  \_/ \_/ \_/ \_/ \_/   \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ 
                                                                      
                                                                                                                                     

                                                                                                                                                      
           ***                                                                                                                                        
            ***                                                                      *       *                    *       *                           
             **                                                                     **      ***                  **      ***                          
             **                                                                     **       *                   **       *                           
   ****      **                               ****                  ***  ****     ********            ****     ********                       ****    
  * ***  *   **       ****    ***  ****      * ***  *       ****     **** **** * ********  ***       * **** * ********  ***        ****      * ***  * 
 *   ****    **      * ***  *  **** **** *  *   ****       * ***  *   **   ****     **      ***     **  ****     **      ***      * ***  *  *   ****  
**    **     **     *   ****    **   ****  **    **       *   ****    **            **       **    ****          **       **     *   ****  **    **   
**    **     **    **    **     **    **   **    **      **    **     **            **       **      ***         **       **    **         **    **   
**    **     **    **    **     **    **   **    **      **    **     **            **       **        ***       **       **    **         **    **   
**    **     **    **    **     **    **   **    **      **    **     **            **       **          ***     **       **    **         **    **   
**    **     **    **    **     **    **   **    **      **    **     **            **       **     ****  **     **       **    **         **    **   
*******      **    **    **     **    **    ******       **    **     ***           **       **    * **** *      **       **    ***     *   ******    
******       *** *  ***** **    ***   ***    ****         ***** **     ***           **      *** *    ****        **      *** *  *******     ****     
**            ***    ***   **    ***   ***                 ***   **                           ***                          ***    *****               
**                                                                                                                                                    
**                                                                                                                                                    
 **                                                                                                                                                   
                                                                                                                                                      
            
        __                                     __   __         __   __             
.-----.|  |.---.-.-----.-----.    .---.-.----.|  |_|__|.-----.|  |_|__|.----.-----.
|  _  ||  ||  _  |     |  _  |    |  _  |   _||   _|  ||__ --||   _|  ||  __|  _  |
|   __||__||___._|__|__|_____|    |___._|__|  |____|__||_____||____|__||____|_____|
|__|                                                                               


     
  _____         _______ __   _  _____       _______  ______ _______ _____ _______ _______ _____ _______  _____ 
 |_____] |      |_____| | \  | |     |      |_____| |_____/    |      |   |______    |      |   |       |     |
 |       |_____ |     | |  \_| |_____|      |     | |    \_    |    __|__ ______|    |    __|__ |_____  |_____|
                                                                                                               
                                                                                                            

                                                                                                                                                         
                                                            :                                                                                      :     
                                        L.                 t#,                                                         .                    .,    t#,    
  t                   i                 EW:        ,ft    ;##W.                        j.                 t           ;W          t        ,Wt   ;##W.   
  ED.                LE              .. E##;       t#E   :#L:WE                     .. EW,       GEEEEEEELEj         f#E GEEEEEEELEj      i#D.  :#L:WE   
  E#K:              L#E             ;W, E###t      t#E  .KG  ,#D                   ;W, E##j      ,;;L#K;;.E#,      .E#f  ,;;L#K;;.E#,    f#f   .KG  ,#D  
  E##W;            G#W.            j##, E#fE#f     t#E  EE    ;#f                 j##, E###D.       t#E   E#t     iWW;      t#E   E#t  .D#i    EE    ;#f 
  E#E##t          D#K.            G###, E#t D#G    t#E f#.     t#i               G###, E#jG#W;      t#E   E#t    L##Lffi    t#E   E#t :KW,    f#.     t#i
  E#ti##f        E#K.           :E####, E#t  f#E.  t#E :#G     GK              :E####, E#t t##f     t#E   E#t   tLLG##L     t#E   E#t t#f     :#G     GK 
  E#t ;##D.    .E#E.           ;W#DG##, E#t   t#K: t#E  ;#L   LW.             ;W#DG##, E#t  :K#E:   t#E   E#t     ,W#i      t#E   E#t  ;#G     ;#L   LW. 
  E#ELLE##K:  .K#E            j###DW##, E#t    ;#W,t#E   t#f f#:             j###DW##, E#KDDDD###i  t#E   E#t    j#E.       t#E   E#t   :KE.    t#f f#:  
  E#L;;;;;;, .K#D            G##i,,G##, E#t     :K#D#E    f#D#;             G##i,,G##, E#f,t#Wi,,,  t#E   E#t  .D#j         t#E   E#t    .DW:    f#D#;   
  E#t       .W#G           :K#K:   L##, E#t      .E##E     G#t            :K#K:   L##, E#t  ;#W:    t#E   E#t ,WK,          t#E   E#t      L#,    G#t    
  E#t      :W##########Wt ;##D.    L##, ..         G#E      t            ;##D.    L##, DWi   ,KK:    fE   E#t EG.            fE   E#t       jt     t     
           :,,,,,,,,,,,,,.,,,      .,,              fE                   ,,,      .,,                 :   ,;. ,               :   ,;.                    
                                                     ,                                                                                                   


                                                      
   _   /7 _   _     _     _   _  /7 ()__  /7 () __  _ 
  /o| //,'o| / \/7,'o|  ,'o| //7/_7/7(c' /_7/7,',','o|
 /_,'// |_,7/_n_/ |_,'  |_,7// // ///__)// // \_\ |_,'
//                                                    


                                                                                                                    
                                                                                                                    
         ___                                                                                                        
         `MM                                                             68b                 68b                    
          MM                                                       /     Y89           /     Y89                    
__ ____   MM    ___   ___  __     _____            ___   ___  __  /M     ___   ____   /M     ___   ____     _____   
`M6MMMMb  MM  6MMMMb  `MM 6MMb   6MMMMMb         6MMMMb  `MM 6MM /MMMMM  `MM  6MMMMb\/MMMMM  `MM  6MMMMb.  6MMMMMb  
 MM'  `Mb MM 8M'  `Mb  MMM9 `Mb 6M'   `Mb       8M'  `Mb  MM69 "  MM      MM MM'    ` MM      MM 6M'   Mb 6M'   `Mb 
 MM    MM MM     ,oMM  MM'   MM MM     MM           ,oMM  MM'     MM      MM YM.      MM      MM MM    `' MM     MM 
 MM    MM MM ,6MM9'MM  MM    MM MM     MM       ,6MM9'MM  MM      MM      MM  YMMMMb  MM      MM MM       MM     MM 
 MM    MM MM MM'   MM  MM    MM MM     MM       MM'   MM  MM      MM      MM      `Mb MM      MM MM       MM     MM 
 MM.  ,M9 MM MM.  ,MM  MM    MM YM.   ,M9       MM.  ,MM  MM      YM.  ,  MM L    ,MM YM.  ,  MM YM.   d9 YM.   ,M9 
 MMYMMM9 _MM_`YMMM9'Yb_MM_  _MM_ YMMMMM9        `YMMM9'Yb_MM_      YMMM9 _MM_MYMMMM9   YMMM9 _MM_ YMMMM9   YMMMMM9  
 MM                                                                                                                 
 MM                                                                                                                 
_MM_                                                                                                                


____ __   ___  __   ____   ___  ____ ____ ____ ___  ____ ____ ____ ____ 
| . \| |  |  \ | \|\|   |  |  \ | . \|_ _\|___\| _\ |_ _\|___\| __\|   |
| __/| |__| . \|  \|| . |  | . \|  <_  || | /  [__ \  || | /  | \__| . |
|/   |___/|/\_/|/\_/|___/  |/\_/|/\_/  |/ |/   |___/  |/ |/   |___/|___/



            __                                                       _     __             _     __                     
   _ ___    LJ    ___ _    _ ___      ____         ___ _    _ ___   FJ_    LJ    ____    FJ_    LJ    ____      ____   
  J '__ J   FJ   F __` L  J '__ J    F __ J       F __` L  J '__ ",J  _|        F ___J  J  _|        F ___J.   F __ J  
  | |--| | J  L | |--| |  | |__| |  | |--| |     | |--| |  | |__|-J| |-'   FJ  | '----_ | |-'   FJ  | |---LJ  | |--| | 
  F L__J J J  L F L__J J  F L  J J  F L__J J     F L__J J  F L  `-'F |__-.J  L )-____  LF |__-.J  L F L___--. F L__J J 
 J  _____/LJ__LJ\____,__LJ__L  J__LJ\______/F   J\____,__LJ__L     \_____/J__LJ\______/F\_____/J__LJ\______/FJ\______/F
 |_J_____F |__| J____,__F|__L  J__| J______F     J____,__F|__L     J_____F|__| J______F J_____F|__| J______F  J______F 
 L_J                                                                                                                   


                                                                                                            
 _______ _______ _   _   _ _______ ________   _   _   _ ________   _____ _________   _____  ______ ________ 
|  ___  |____  .| | | | | |.  __  |.  ___  | | | | | | |____  \ \ / |_  |____   \ \ / |_  ||____  |.  ___  |
 \_\  | |    | || | | | | || |  | || |   | | | | | | | |    | |  V /  | |    | ||  V /  | |     | || |   | |
 _____| |    | || |/ /_/ / | | _| || |___| | | |/ /_/ /     | | |\ \  | |    | || |\ \  | |_____| || |___| |
|_______|    | ||_______/  |_||___||_______| |_______/      |_|_| \_\ | |    |_||_| \_\ | /________|_______|
             |_|                                                      |_|               |_|
DE FAUXX:

EU sou ruído

Encare-se
Éter
Tudo o que consigo ouvir é ruído
Roncando das máquinas do homem

Você se esqueceu?
Fugindo da realidade
Você está conectado?
A Jornada
Já estive aqui antes?

Vejo pessoas no canto dos meus olhos
Sombras de rostos esquecidos
Sinto que já vivi exatamente este momento mil vezes
Sombras me assombram
Quando perdi minha sombra?
Eu existo?
Posso te ouvir, apareça
Quem é ele?
Pessoas e pessoas
Quando foi que eu esqueci?
Lembrar é mais difícil do que parece
Eu posso ver!
Sonho
Eu me perdi
Sono
Fugir
Pare de falar
Está alto demais
Luz bonita
Brilhe sobre mim
Tomando banho de luz solar
É estranho falar sobre si mesmo
Medo do desconhecido
Já não parecia mais importante
Relaxe
Abra esses seus olhos
Bem abertos
Por que essas pessoas estão me seguindo?
Apagando memórias
Criaturas no meu quarto
Olhe para si mesmo, é essa a vida que você quer viver?
Por que se importar? Você me tem, afinal
Você está imaginando coisas de novo
Imagine-se
Vá lá fora, respire
Eu estava cercado por estranhos
Não conheço essas pessoas
Acorde!
É hora de acordar.
Eu sou
Emoções humanas
Isso parece vazio
Acordando pela primeira vez
Pare de receber comandos
Desplugue-se, você precisa disso
Tema-me
Eu sou a morte
Conecte-se
Pare e escute
Relaxamento
Onde você foi? O que você viu?
Você não parece entender
Estou faminto
Você vai se acostumar, como eu
Não tenha medo
Conecte-se hoje mesmo!
Está se sentindo desplugado?
Não precisa se preocupar
Opiniões não valem nada
Parece que não consigo encontrar uma saída daqui
Isso é real
Você não precisa mais disso
Você deveria realmente reconsiderar

```
