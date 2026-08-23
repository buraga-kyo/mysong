# mysong

Tocador de musicas para o TERMINAL, escrito em C++17, gratuito e local. Nao ha
nuvem, nao ha conta, nao ha telemetria: a musica esta no seu disco e o programa
mora na sua janella de terminal, com a estetica arrowline do RADICAL-OS.

Neste ponto do caminho o repositorio e casa recem-erguida: o binario abre,
escreve a sua marca, espera uma tecla e sahe limpo. Audio, catalogo e espectro
vem nas tarefas seguintes.

## O que exige instalado

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
