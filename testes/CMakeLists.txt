# ══════════════════════════════════════════════════════════════════════════
#   TRACTADO DA BATERIA — testes/CMakeLists.txt
# ══════════════════════════════════════════════════════════════════════════
# DOMÍNIO ......... as fontes de prova deste directorio.
# CONTRA-DOMÍNIO .. o alvo mysong_testes, e um caso registrado no ctest para
#                   CADA caso do doctest, contado nominalmente.
# INVARIANTE ...... nenhuma prova existe sem estar registrada: o registro vem de
#                   doctest_discover_tests, que interroga o binario ja
#                   compilado, e nao de uma lista escripta a mao que envelhece.
# Q.E.D. .......... havendo registro por caso, `ctest` que nada encontre accusa
#                   contagem zero, e a falha silenciosa deixa de se esconder.
# ══════════════════════════════════════════════════════════════════════════
add_executable(mysong_testes fumo_marca.cpp prova_arrowline.cpp)
target_compile_options(mysong_testes PRIVATE ${MYSONG_AVISOS})
target_link_libraries(mysong_testes PRIVATE mysong_core doctest::doctest)

# A bateria da sonda (#22). Entra por target_sources, e não na lista do
# add_executable acima, para não collidir com as tarefas irmãs que mexem neste
# mesmo arquivo agora: linha nova não collide onde linha alterada collidiria.
target_sources(mysong_testes PRIVATE prova_sonda.cpp prova_tela_requisitos.cpp)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(mysong_testes)

# ─── Provas do núcleo de reprodução (issue #3) ──────────────────────────────
# Por target_sources, e não na lista do add_executable, para ACRESCENTAR sem
# tocar linha alguma que outra lavra esteja a editar.
target_sources(mysong_testes PRIVATE prova_fila.cpp prova_tocador.cpp)

# ─── Provas da superfície de commando (issue #4) ─────────────────────────────
# Por target_sources, e não na lista do add_executable, pela mesma regra das
# lavras acima: ACRESCENTAR linha, e não alterar linha que a tarefa irmã esteja a
# editar. Linha nova não collide onde linha alterada collidiria.
target_sources(mysong_testes PRIVATE prova_api.cpp prova_api_viva.cpp)

# ══════════════════════════════════════════════════════════════════════════
# ─── Prova do espectro (issue #5) ───────────────────────────────────────────
# Por target_sources, e não na lista do add_executable: linha nova não collide.
# Esta prova NÃO pede PipeWire nem fftw3 nos seus caminhos de inclusão: a
# mathematica se prova por espectro.hpp, que declara o plano da fftw adiante, e
# o contracto por analisador.hpp, que esconde o PipeWire atraz de um punho.
target_sources(mysong_testes PRIVATE prova_espectro.cpp)

# ─── Prova do DESENHO do espectro (issue #6) ─────────────────────────────────
# Por target_sources, e não na lista do add_executable: linha nova não collide.
# Chama-se prova_espectro_tui.cpp porque prova_espectro.cpp já é a prova do
# núcleo, e dous arquivos de egual nome não cabem no mesmo directorio.
# SEM PONTO E VÍRGULA em nome de caso algum lá dentro: o ponto e vírgula parte a
# lista do CMake, e zero casos corridos dá status de successo. Já succedeu.
target_sources(mysong_testes PRIVATE prova_espectro_tui.cpp)

#   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
# ══════════════════════════════════════════════════════════════════════════
