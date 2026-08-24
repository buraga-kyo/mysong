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
