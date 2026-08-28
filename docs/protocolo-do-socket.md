# O protocolo do socket de commando do `mysong`

> **Versão do protocolo: 1.** Este documento é CONTRACTO com quem escrever o outro
> lado. Foi escripto para que se implemente um cliente sem perguntar nada a ninguem,
> e todo exemplo aqui foi copiado verbatim de uma corrida de verdade.

## 1. Onde, e com que permissão

O servidor escuta num socket Unix de FLUXO (`AF_UNIX`, `SOCK_STREAM`) em:

```
$XDG_RUNTIME_DIR/mysong.sock
```

Nasce em modo `0600`, e o `$XDG_RUNTIME_DIR` do operador é `0700`. **Esta é a
protecção inteira: não ha senha, nem token, nem cifra.** Quem pode ler o
directorio é o operador, e mais ninguem.

Se `$XDG_RUNTIME_DIR` não estiver definido, o `mysong` **não abre socket algum** e
diz por que no `stderr`. Não ha recuo a `/tmp`: `/tmp` é escripta de todos, e socket
de commando ahi deixaria qualquer usuario da machina governar o tocador alheio, ou
criar o arquivo primeiro e passar a receber as ordens do operador.

Se já houver outra instância do `mysong` a servir naquelle caminho, a segunda
**recusa** e o diz. Ella não rouba o socket da primeira, e o arquivo da primeira
fica intacto. Se houver arquivo no caminho mas ninguem a escutar (resto de processo
morto), a instância nova reclama o caminho e segue.
## 2. Enquadramento

- **Uma mensagem por linha**, terminada por `\n` (`0x0A`). Não ha cabeçalho de
  tamanho, nem delimitador de outra especie.
- **UTF-8**, sempre, nas duas direcções.
- O socket é de **fluxo**, não de mensagem. Isto quer dizer duas cousas, e ambas são
  o caso normal e não a excepção: uma linha pode chegar partida em varias leituras, e
  varias linhas podem chegar numa leitura só. Um cliente correcto acumula até o `\n`
  e drena em laço; um servidor correcto faz o mesmo, e este faz.
- **Uma linha de pergunta, uma linha de resposta**, na ordem, com uma excepção só: a
  linha em branco (vazia, ou sómente brancos) é ignorada e **não produz resposta
  alguma**, nem uma linha vazia.
- Podem-se mandar varias mensagens sem esperar as respostas. Ellas voltam na ordem
  em que as perguntas chegaram.
- O cliente pode fechar a sua banda de escripta (meio fechamento) e seguir a ler; é o
  que o `nc` e qualquer canalisação fazem, e o servidor responde antes de fechar.
## 3. A forma da resposta

Toda resposta é um objecto JSON com a chave **`ok`** em primeiro logar.

**Acerto:**

```json
{"ok":true, ...os campos do verbo...}
```

**Erro:**

```json
{"ok":false,"erro":"<codigo>","razao":"<texto para olho humano>"}
```

O **`erro`** é para a machina: compare-o, ramifique por elle. A **`razao`** é para o
olho de quem depura, e pode mudar de redacção entre versões **sem** que a versão do
protocolo suba. Não ramifique pela `razao`.

### 3.1 A taboa dos codigos de erro

| `erro` | Que quer dizer | Onde olhar |
|---|---|---|
| `json_malformado` | A linha não é um objecto JSON do subconjunto que se aceita. | A linha que se mandou. |
| `verbo_ausente` | Falta a chave `verbo`, ou ella não é texto. | A mensagem. |
| `verbo_desconhecido` | O `verbo` não existe neste protocolo. | O nome que se digitou. |
| `nao_implementado` | O verbo EXISTE e está reservado; o seu subsystema ainda não chegou. Vem com a chave `issue`. | A issue que a resposta nomeia. |
| `argumento_invalido` | Um argumento falta, é do typo errado, ou está fóra de faixa. | A mensagem. |
| `recusado` | O nucleo disse não a uma ordem legitima (pausar o que está parado, `proxima` na borda da fila). | O ESTADO do tocador. |
| `linha_longa` | A linha passou de 64 KiB sem terminar em `\n`. A connexão fecha-se. | O cliente. |
| `lotado` | Ha 16 clientes ao mesmo tempo. A connexão fecha-se depois d'esta resposta. | Tente outra vez. |

A differença entre `recusado` e `argumento_invalido` importa, e é d'esta taboa que
ella se lê: `recusado` manda olhar o estado do tocador, `argumento_invalido` manda
olhar a mensagem que se escreveu.

## 4. Os verbos de leitura

### `versao`

Devolve o contracto, para que o cliente o possa exigir antes de confiar.

```
→ {"verbo":"versao"}
← {"ok":true,"obra":"mysong","protocolo":1}
```

| Campo | Typo | |
|---|---|---|
| `obra` | texto | Sempre `"mysong"`. |
| `protocolo` | inteiro | A versão d'este documento. |

### `estado`

O retracto inteiro, num instante só. **Os sete campos vêm sempre**, e é de proposito:
cliente que tenha de perguntar duas vezes para armar uma tela veria a segunda resposta
não casar com a primeira, porque entre as duas o mundo andou.

```
→ {"verbo":"estado"}
← {"ok":true,"estado":"Tocando","faixa":"/tmp/pa-s1-prova/01 - Canção 音楽.wav","posicao":1.275,"duracao":20.000,"volume":100,"indice":0,"tamanho":3}
```

| Campo | Typo | |
|---|---|---|
| `estado` | texto | `"Tocando"`, `"Pausado"` ou `"Parado"`. Tres, e não mais. |
| `faixa` | texto | O caminho da faixa corrente. **Vazio** quando a fila está vazia; não é erro. |
| `posicao` | duplo | Segundos desde o inicio da faixa, com tres casas. Zero quando nada toca. |
| `duracao` | duplo | Segundos de duração da faixa, com tres casas. |
| `volume` | inteiro | De 0 a 100. É o volume do MOTOR, e nunca o do systema. |
| `indice` | inteiro | O assento da faixa corrente na fila, contado de zero. |
| `tamanho` | inteiro | Quantas faixas ha na fila. |

### `fila`

As faixas na ordem, e o assento corrente. Listar **não toca faixa alguma**.

```
→ {"verbo":"fila"}
← {"ok":true,"faixas":["/tmp/pa-s1-prova/01 - Canção 音楽.wav","/tmp/pa-s1-prova/tom2.wav","/tmp/pa-s1-prova/tom3.wav"],"indice":0,"tamanho":3}
```

| Campo | Typo | |
|---|---|---|
| `faixas` | vector de textos | Os caminhos, na ordem em que se juntaram. Vector vazio se a fila está vazia. |
| `indice` | inteiro | O assento corrente, contado de zero. |
| `tamanho` | inteiro | Quantas faixas ha. |

> **Nota ao implementador**: esta é a UNICA resposta que traz um valor não escalar.
> Se estiver a escrever um parser á mão em vez de usar bibliotheca, é aqui que elle
> ha de saber ler um vector de cadeias.

## 5. Os verbos que mandam

### `juntar`

Junta uma faixa ao FIM da fila. Não a toca.

```
→ {"verbo":"juntar","caminho":"/musica/01 - Canção.flac"}
← {"ok":true,"tamanho":1}
```

| Argumento | Typo | |
|---|---|---|
| `caminho` | texto | Obrigatorio, e não pode ser vazio. Não se verifica se o arquivo existe: quem descobre é o `tocar`. |

Resposta: `tamanho` (inteiro), o novo tamanho da fila.

### `ir_para`

Muda o assento **e manda tocar**, á maneira de `proxima` e `anterior`. Mover sem tocar
deixaria o tocador a soar a faixa velha com o indice apontado á nova.

```
→ {"verbo":"ir_para","indice":2}
← {"ok":true}
```

| Argumento | Typo | |
|---|---|---|
| `indice` | inteiro | Contado de zero. Fóra da fila devolve `recusado`; negativo devolve `argumento_invalido`. |

### `tocar`, `pausar`, `retomar`, `proxima`, `anterior`, `parar`

Sem argumento algum. Respondem `{"ok":true}` quando o nucleo obedeceu, e
`{"ok":false,"erro":"recusado",...}` quando elle disse não.

```
→ {"verbo":"tocar"}
← {"ok":true}
→ {"verbo":"pausar"}
← {"ok":true}
→ {"verbo":"retomar"}
← {"ok":true}
→ {"verbo":"proxima"}
← {"ok":true}
→ {"verbo":"anterior"}
← {"ok":true}
→ {"verbo":"parar"}
← {"ok":true}
```

E a recusa, que tem a mesma forma para os seis:

```
→ {"verbo":"proxima"}
← {"ok":false,"erro":"recusado","razao":"o nucleo recusou a ordem \"proxima\" no estado corrente"}
```

| Verbo | Que faz | Quando devolve `recusado` |
|---|---|---|
| `tocar` | Toca a faixa do assento corrente. | Fila vazia, ou o motor recusou o arquivo. |
| `pausar` | Pausa. | Quando não está a tocar. |
| `retomar` | Retoma. | Quando não está pausado. |
| `proxima` | Anda para a frente **e toca**. | No ultimo assento. Ahi **nada** desce ao motor e a faixa em curso segue. |
| `anterior` | Anda para tras **e toca**. | No primeiro assento, do mesmo modo. |
| `parar` | **Hoje PAUSA.** Ver a nota abaixo. | Quando não está a tocar. |

> **`parar` pausa, hoje, e digo-o em vez de o esconder.** O nucleo do `mysong` não tem
> parada distincta da pausa, e alargar-lhe a interface não cabia nesta tarefa. O NOME
> do verbo já é o certo: quando a parada existir, muda-se o servidor e **o cliente não
> muda uma letra**. Se precisar hoje de parada de verdade, use `pausar` e `ir_para 0`.

### `buscar`

```
→ {"verbo":"buscar","segundos":5}
← {"ok":true}
```

| Argumento | Typo | |
|---|---|---|
| `segundos` | duplo | Posição ABSOLUTA desde o inicio da faixa. Apara-se ás bordas da faixa: além do fim vae para junto do fim, e antes do inicio vae para zero. `recusado` com o tocador parado. |

### `volume`

```
→ {"verbo":"volume","porcento":150}
← {"ok":true,"volume":100}
```

| Argumento | Typo | |
|---|---|---|
| `porcento` | inteiro | De 0 a 100. Fóra d'ahi **apara-se**, e a resposta diz o valor aparado: quem manda 150 lê 100. É o volume do MOTOR, e nunca o do systema. |

## 6. Os verbos que a versão 1 reservava

Na versão 1 estes tres nomes existiam e respondiam `nao_implementado`, com a issue
que os traria; e este documento dizia que se podia codar contra elles porque,
chegando a feição, o nome não mudaria. As tres issues fecharam. A versão 2 cumpre o
promettido: **os nomes são os mesmos**, e quem escreveu cliente contra elles não
muda uma letra, sómente passa a receber resposta em logar de recusa.

### `espectro`

As bandas correntes, e a ESCALA em que ellas estão. A escala vae junto porque quem
lê de fóra não tem a tela para adivinhar como as bandas se espaçam nem como a
magnitude foi comprimida: bandas sem escala são vinte e quatro numeros que não se
sabem pintar. Sem argumento algum.

```
→ {"verbo":"espectro"}
← {"ok":true,"bandas":[0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.850,0.312,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000,0.000],"quantas":24,"escala":"logarithmica","hertz_minimo":40.000,"hertz_maximo":16000.000,"magnitude":"decibeis","piso_decibeis":-60.000}
```

Acima, um tom de 440 Hz a tocar: acende a banda 9, que é onde ella ha de acender,
porque 24 * ln(440/40) / ln(16000/40) = 9,6. Em escala linear, 440 Hz cahiria na
banda 0. A escala que a resposta declara é a que a resposta cumpre.

| Campo | Typo | |
|---|---|---|
| `bandas` | vector de numeros | `quantas` magnitudes em [0,1], da mais grave para a mais aguda. Com nada a tocar vêm todas em zero, e a resposta é `ok`: silencio não é erro. |
| `quantas` | inteiro | Quantas bandas ha. Vinte e quatro hoje; leia-o, e não o presuma. |
| `escala` | texto | `"logarithmica"`: as bordas espaçam-se no logarithmo da frequencia, que é como o ouvido as separa. Linear daria vinte bandas de agudo e nenhuma de baixo. |
| `hertz_minimo`, `hertz_maximo` | duplo | A faixa que se pinta: de 40 a 16000. |
| `magnitude` | texto | `"decibeis"`: a magnitude vem comprimida, com 0 no piso e 1 na escala cheia. |
| `piso_decibeis` | duplo | O piso: -60. |

### `biblioteca`

Navega o índice do acervo. O **`corte`** é obrigatorio, e é elle que diz o que se
quer: tres cortes, e mais nenhum. Deduzir o corte dos argumentos que viessem seria
mais curto de escrever e falharia em silencio, que é o que este contracto não faz:
quem digitasse `artistaa` receberia a lista dos artistas com `ok` verdadeiro e
nunca saberia que errou o nome.

```
→ {"verbo":"biblioteca","corte":"artistas"}
← {"ok":true,"corte":"artistas","artistas":["Bach","Coltrane"],"tamanho":2}
→ {"verbo":"biblioteca","corte":"albuns","artista":"Bach"}
← {"ok":true,"corte":"albuns","artista":"Bach","albuns":["Cantatas","Suites"],"tamanho":2}
→ {"verbo":"biblioteca","corte":"faixas","artista":"Bach","album":"Cantatas"}
← {"ok":true,"corte":"faixas","artista":"Bach","album":"Cantatas","numeros":[1,2],"titulos":["Aria","Coro 音楽"],"caminhos":["/tmp/pa-s4-prova/acervo/Bach/Cantatas/01 - Aria.flac","/tmp/pa-s4-prova/acervo/Bach/Cantatas/02 - Coro 音楽.flac"],"duracoes":[210,187],"tamanho":2}
```

| Argumento | Typo | |
|---|---|---|
| `corte` | texto | Obrigatorio. `"artistas"`, `"albuns"` ou `"faixas"`. Outro valor devolve `argumento_invalido` nomeando os tres. |
| `artista` | texto | Obrigatorio em `albuns` e em `faixas`, e não pode ser vazio. Em `artistas` ignora-se. |
| `album` | texto | Obrigatorio em `faixas`, e não pode ser vazio. |

A resposta ecoa o `corte` e os argumentos que a recortaram, para que uma resposta
lida fóra de contexto se saiba explicar. O `tamanho` é quantos itens vieram.

Nas **faixas**, quatro vectores sahem PARALELOS, e não um vector de objectos: o
JSON d'este contracto é plano de um nivel, e objecto dentro de objecto nem elle
emitte nem elle lê de volta. Os quatro têm sempre o mesmo comprimento, e elle é o
`tamanho`: a linha `i` dos quatro é a mesma faixa.

| Campo das faixas | Typo | |
|---|---|---|
| `numeros` | vector de numeros | O numero da faixa no album; 0 é «sem numero». |
| `titulos` | vector de textos | O titulo. |
| `caminhos` | vector de textos | O caminho no disco. É este que se passa ao `juntar`. |
| `duracoes` | vector de numeros | Segundos; 0 é «não medida». |

Sahem em ordem de numero e, empatando, de titulo, que é a ordem que o índice dá.

**Vector vazio não é erro.** Artista que não existe devolve `albuns` vazio com
`ok` verdadeiro, e album que não existe devolve as quatro columnas vazias: no
índice não ha artista sem album, donde «não tem» e «não existe» são a mesma cousa
vista de fóra. E índice AUSENTE responde egual a índice vazio, que é o que a
bibliotheca d'esta Casa já faz com banco que não existe.

```
→ {"verbo":"biblioteca","corte":"albuns","artista":"Ninguem"}
← {"ok":true,"corte":"albuns","artista":"Ninguem","albuns":[],"tamanho":0}
```

## 7. As bordas

| O que o cliente faz | O que o servidor faz |
|---|---|
| Manda linha em branco, ou sómente brancos. | Nada. Resposta alguma, e nem linha vazia. Não é erro. |
| Manda linha maior que 64 KiB sem `\n`. | Responde `linha_longa` e **fecha** a connexão. |
| Manda duas ou mais mensagens de uma vez. | Responde a todas, uma linha cada, na ordem. |
| Manda uma mensagem partida em varias escriptas. | Espera pelo `\n` e depois responde. |
| Fecha a banda de escripta e segue a ler (meio fechamento). | Responde ao que já chegou, e depois fecha. É o que o `nc` faz. |
| Fecha de todo no meio de uma resposta. | Recolhe o descriptor e segue vivo. O cliente seguinte é servido normalmente. |
| Abre e nada manda. | Nada. Não bloqueia os outros, nem o som. |
| Abre sendo o 17º ao mesmo tempo. | Responde `lotado` e fecha. |

## 8. Como falar com elle da linha de commando

```sh
# Com o nc (OpenBSD netcat). O -q 1 importa: sem elle o nc pode sahir antes de ler.
printf '{"verbo":"estado"}\n' | nc -U -q 1 "$XDG_RUNTIME_DIR/mysong.sock"

# Com o socat, se o tiver installado.
echo '{"verbo":"estado"}' | socat - UNIX:"$XDG_RUNTIME_DIR/mysong.sock"
```

