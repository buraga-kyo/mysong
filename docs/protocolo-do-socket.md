# O protocolo do socket de commando do `mysong`

> **Versão do protocolo: 2.** Este documento é CONTRACTO com quem escrever o outro
> lado. Foi escripto para que se implemente um cliente sem perguntar nada a ninguem,
> e todo exemplo aqui foi copiado verbatim de uma corrida de verdade.

**O que mudou da 1 para a 2**: dous verbos NOVOS, `embaralhar` e `repetir`, e dous
campos NOVOS no retracto do `estado`, `embaralhado` e `repetir`. Campo algum dos
velhos mudou de nome ou de typo, e verbo algum sahiu: cliente escripto contra a 1
segue a funccionar contra a 2 sem lhe tocar uma letra.

Uma cousa, porém, elle ha de saber, e é por ella que a versão sobe: com um dos
modos ligado, **a borda do `proxima` e do `anterior` deixa de ser o ultimo e o
primeiro assento**. Quem tenha chumbado essa borda no seu codigo leia a nota da
secção 5, ao pé da taboa dos verbos que mandam. Quem a não tenha chumbado nada
tem a fazer: os modos nascem desligados, e enquanto ninguem os ligar a fila é a
crua de sempre.

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
← {"ok":true,"obra":"mysong","protocolo":2}
```

| Campo | Typo | |
|---|---|---|
| `obra` | texto | Sempre `"mysong"`. |
| `protocolo` | inteiro | A versão d'este documento. Hoje 2. |

### `estado`

O retracto inteiro, num instante só. **Os nove campos vêm sempre**, e é de proposito:
cliente que tenha de perguntar duas vezes para armar uma tela veria a segunda resposta
não casar com a primeira, porque entre as duas o mundo andou.

```
→ {"verbo":"estado"}
← {"ok":true,"estado":"Tocando","faixa":"/tmp/pa-s1-prova/01 - Canção 音楽.wav","posicao":0.780,"duracao":20.000,"volume":100,"indice":0,"tamanho":3,"embaralhado":false,"repetir":"nenhuma"}
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
| `embaralhado` | booleano | `true` quando a fila anda por permutação. Ver o verbo `embaralhar`. |
| `repetir` | texto | `"nenhuma"`, `"uma"` ou `"todas"`. Ver o verbo `repetir`. |

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
| `proxima` | Anda para a frente **e toca**. | Na PONTA da passagem, e ver a nota abaixo: com o repetir em `nenhuma` e o embaralhar desligado, é o ultimo assento. Ahi **nada** desce ao motor e a faixa em curso segue. |
| `anterior` | Anda para tras **e toca**. | Na outra ponta, do mesmo modo. |
| `parar` | **Hoje PAUSA.** Ver a nota abaixo. | Quando não está a tocar. |

> **Onde é a ponta depende dos dous modos, desde a versão 2.** A linha do
> `proxima` acima descreve a fila crua, que é como ella nasce; ligados os modos,
> a borda muda, e muda de tres maneiras:
>
> - com `repetir` em `todas`, **não ha recusa**: a ultima leva á primeira, e a
>   primeira á ultima;
> - com `repetir` em `uma`, o `proxima` **não recusa nunca**, e devolve `ok`
>   sem andar: prende-se na faixa corrente, e ella recomeça. O `anterior`
>   **não** se prende, e anda como sempre;
> - com `embaralhar` ligado, a ponta é o fim da PERMUTAÇÃO, e não o ultimo
>   assento da fila: a recusa chega n'um `indice` qualquer, e não no maior.
>
> Cliente que precise de saber onde está a ponta ha de ler `embaralhado` e
> `repetir` do verbo `estado`. Cliente que os não leia continua a funccionar, e
> vê a fila crua enquanto ninguem ligar modo algum.

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

### `embaralhar`

Liga e desliga a permutação da fila. Ligar sorteia UMA ordem de toda a fila, com a
faixa corrente no principio d'ella, e anda-se por essa ordem: faixa alguma torna
antes de todas terem tocado. Esgotada, a permutação **não se re-sorteia**. Desligar
volta á ordem de chegada, e **a faixa corrente não troca**.

```
→ {"verbo":"embaralhar","ligado":true}
← {"ok":true,"embaralhado":true}
```

| Argumento | Typo | |
|---|---|---|
| `ligado` | booleano | Obrigatorio. Ausente, ou de outro typo, devolve `argumento_invalido`. |

Resposta: `embaralhado` (booleano), o valor que ficou.

### `repetir`

Assenta o modo de repetição. Tres valores, e sómente tres.

```
→ {"verbo":"repetir","modo":"todas"}
← {"ok":true,"repetir":"todas"}
→ {"verbo":"repetir","modo":"sempre"}
← {"ok":false,"erro":"argumento_invalido","razao":"o modo de repetir e \"nenhuma\", \"uma\" ou \"todas\""}
```

| `modo` | Que faz | Nome no MPRIS |
|---|---|---|
| `nenhuma` | A borda recusa, como sempre. | `None` |
| `uma` | `proxima` prende na faixa corrente, e ella recomeça. `anterior` **não** se prende. | `Track` |
| `todas` | A ultima leva á primeira, e a primeira á ultima. | `Playlist` |

Resposta: `repetir` (texto), o valor que ficou. Nome fóra dos tres devolve
`argumento_invalido`, e o modo fica como estava.

> **Os dous modos não sobrevivem ao fechar o programa**, e isso é decisão
> declarada e não esquecimento: arquivo de estado algum se escreve, e abrir o
> tocador outra vez dá os dous desligados.

## 6. Os verbos RESERVADOS

Estes tres nomes **existem** no protocolo e o seu subsystema **ainda não chegou**.
Respondem sempre `nao_implementado`, com a issue que os trará:

```
→ {"verbo":"espectro"}
← {"ok":false,"erro":"nao_implementado","razao":"o verbo \"espectro\" esta reservado e o seu subsystema ainda nao existe","issue":5}
```

| Verbo | Para que ha de servir | Issue |
|---|---|---|
| `espectro` | Ler as bandas do espectro. | 5 |
| `biblioteca` | Navegar a bibliotheca de músicas. | 8 |
| `baixar` | Disparar um download. | 11 |

Estão reservados de proposito, e não deixados fóra. Deixados fóra, dariam
`verbo_desconhecido`, que é a MESMA resposta de um erro de digitação, e ahi quem
escreve o cliente não saberia se errou o nome ou se a feição não chegou. Pode codar
contra estes nomes hoje: quando a feição chegar, o nome não muda.

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

