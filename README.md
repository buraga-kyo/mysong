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
manda a `--ajuda`, depois a `--versao`, e por fim o `--sonda`.

A varredura do acervo corre em fio proprio ao abrir: a tela abre de pronto, com o
acervo da corrida anterior, e o `r` manda varrer outra vez.

### As teclas

| tecla | o que faz |
|---|---|
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
| `q` | sahe |

Duas buscas ha, e ellas nao sao a mesma: o `/` FILTRA o que esta a vista, sem
tocar a rede; o `s` PERGUNTA ao YouTube. Na secção NET, Enter encommenda a baixa
do achado eleito, e duas baixas correm ao mesmo tempo no maximo: as demais
esperam, e a linha do titulo diz quantas correm e quantas esperam.

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

A PRECEDENCIA, do mais forte para o mais fraco: o argumento da linha de
commando, a variavel de ambiente, este arquivo, e o padrao da Casa. O
`MYSONG_ACERVO` continua a valer, e continua a ganhar do arquivo; e vae CRU,
sem se aferir, que quem o poz no perfil do shell manda, e o acervo nao ha de
mudar debaixo dos pes de quem aponta para monte de rede que ainda nao montou.

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
|P.--. ||L.--. ||A.--. ||N.--. ||O.--. |.-.  |A.--. ||R